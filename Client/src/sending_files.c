#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "sending_files.h"
#include "logger.h"
#include "network_functions.h"
#include "user_interface.h"

const size_t FILE_PORT = 27011;
static const size_t ONE = 1;
static const size_t EXTRA_SPACE = 5;                 // for snprint
static const int FILE_MOD = 0644;                    // owner can read and write, other can only read
static const unsigned int BUFFERS_COUNT = 1;         // for uv_fs_write
static const int64_t CURRENT_FILE_PTR = -1;          // for uv_fs_write
static const int64_t OFFSET = 0;                     // for uv_fs_sendfile
static const size_t DEC = 10;			     // for strtoul
static const long MIN_COMMAND_SIZE = 10;

// global vars for user interface
static winsize_t* console_size;
static windows_t* windows;
static user_info_t* user_data;

srv_command_t srv_instructions[] = {
    { "/file_name"      	,       parse_file_name   	},
    { "/shipping_info"		,	complete_sending	},
    { "/recipient_accepted"	,	start_sending	  	}
};
size_t instructions_count = sizeof(srv_instructions) / sizeof(srv_command_t);

void open_new_connection( main_struct_t* main_struct, main_connection_t* main_connection, unsigned long transfer_id, client_type_t client_type ){
    assert( main_struct );
    assert( main_connection );

    console_size = main_struct->console_size;
    windows = main_struct->windows;
    user_data = main_struct->user_data;

    uv_loop_t* loop = main_connection->loop;
    client_t* client = main_connection->client;

    uv_tcp_t* client_socket = (uv_tcp_t*)calloc( ONE, sizeof(uv_tcp_t) );
    uv_tcp_init( loop, client_socket );                                                            // init descriptor, but not make socket
    uv_connect_t* connect = (uv_connect_t*)calloc( ONE, sizeof(uv_connect_t) );

    struct sockaddr_in client_addr = {};                                                            // describe socket: port, ip ...
    if( uv_ip4_addr( user_data->ip, FILE_PORT, &client_addr ) != 0 ){                               // converting string to binary struct
        log_fatal( "error converting IP address to struct" );
        free( connect );
        return ;
    }
    int connect_status = 0;
    connect->data = client;                                                                         // save client
    client->client_type = client_type;                                                              // SENDER or RECEIVER
    client->transfer_id = transfer_id;                                                              // save transfer_id

    connect_status = uv_tcp_connect( connect, client_socket, (const struct sockaddr*)&client_addr, joined_cb );
    if( connect_status < 0 ){
        log_fatal( "server connection error" );
        free( connect );
        return ;
    }
    log_info( "successfully connected to file chanel" );
}

void joined_cb( uv_connect_t* req, int status ){
    assert( req );

    if( status < 0 ){
        log_fatal( "receiver joined callback get negative status" );
        return ;
    }

    client_t* client = (client_t*)req->data;
    client->file_handle = (uv_tcp_t*)req->handle;
    client->file_handle->data = client;                                                                  // save client info in free field of the struct
    int read_server = uv_read_start( (uv_stream_t*)client->file_handle, alloc_сb, read_cb );             // req->handle - file descriptor wrapper
    if( read_server < 0 ){
        log_warning( "uv_read_start return negative value" );
        return ;
    }
    if( client->client_type == RECEIVER ){
        send_file_data( client, "/receive_connected %lu\n", client->transfer_id );
    }
    else if( client->client_type == SENDER ){
        send_file_data( client, "/sender_connected %lu\n", client->transfer_id );
    }
    free( req );
}

void alloc_сb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    client_err_t client_err = NO_ERR;
    size_t predicted_size = client->file_buf_len + suggested_size + 1;
    if( client->file_buf_cap > 0 ){
        if( ( client_err = realloc_file_buf( client, predicted_size ) ) != NO_ERR ){
            return ;
        }
    }
    else{
        client->file_buf = (char*)calloc( predicted_size, sizeof(char) );
        if( !client->file_buf ){
            log_fatal( "can not allocate memory for client buf" );
            return ;
        }
    }

    client->file_buf_cap = client->file_buf_cap >= predicted_size
                          ? client->file_buf_cap
                          : predicted_size;
    buf->base = client->file_buf + client->file_buf_len;                                    // ptr of free place
    buf->len = client->file_buf_cap - client->file_buf_len - 1;                             // maximum bytes for reading
}

client_err_t realloc_file_buf( client_t* client, size_t predicted_size ){
    assert( client );

    char* realloc_buf = NULL;
    if( client->file_buf_cap < predicted_size ){
        realloc_buf = (char*)realloc( client->file_buf, predicted_size * sizeof(char) );
        if( !realloc_buf ){
            log_fatal( "realloc return NULL ptr" );
            return MEMORY_ERR;
        }
        client->file_buf = realloc_buf;
    }
    return NO_ERR;
}

void read_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    if( nread >= 0 ){
        log_info( "read got %zd", nread );
	client->file_buf[ client->file_buf_len + nread ] = '\0';
        parse_file_data( client, nread, parse_command );
        return;
    }

    log_warning( "client closed file channel" );
    uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
    if( uv_shutdown( shutdown_req, handle, disconnect_cb ) < 0 ){
        log_fatal( "shutdown return negative value" );
        free( shutdown_req );
    }
}

void parse_file_data( client_t* client, ssize_t nread, void (*on_cmd)( client_t* client, char* instruction ) ){
    assert( client  );
    
    log_debug( "IN PARS FILE DATA = %.*s", client->file_buf_len + (size_t)nread, client->file_buf );

    char* newline_char = NULL;
    char* buf_start = client->file_buf;
    client->file_buf_len += (size_t)nread;

    if( buf_start[0] != '/' ){
	save_file_data( client, nread );
	buf_start += (size_t)nread;
    }

    while( ( newline_char = strchr( buf_start, '\n' ) ) && buf_start < client->file_buf + client->file_buf_len  ){
	*newline_char = '\0';
	if( newline_char - buf_start >= MIN_COMMAND_SIZE ){
	    log_debug( "file instruction = '%s'", buf_start );
	    on_cmd( client, buf_start );
	}
	buf_start = newline_char + 1;
    }

    // TEST
    log_info( "bytes left in the buffer: %lu", client->file_buf_len - ( buf_start - client->file_buf ) );
    if( buf_start < client->file_buf + client->file_buf_len ){
        memmove( client->file_buf, buf_start, client->file_buf_len - ( buf_start - client->file_buf ) );
    }
    client->file_buf_len -= buf_start - client->file_buf;
    log_info( "string line after: %lu", client->file_buf_len ); 
}

void parse_command( client_t* client, char* instruction){
    assert( client );
    assert( instruction );

    if( instruction[0] == '\0' ){
        return ;
    }

    srv_command_t* command_begining = srv_instructions;
    srv_command_t* current_command =  command_begining;

    char* srv_command = NULL;
    for(; current_command < command_begining + instructions_count; current_command++ ){
        srv_command = (*current_command).command_name;
        if( strncmp( instruction, srv_command, strlen(srv_command) ) == 0 ){
            log_debug( "command '%s' was founded", srv_command );
            (*current_command).func( client, instruction );
            return ;
        }
    }

    log_error( "command '%s' was not founded", instruction );
}

void clear_file_buffer( client_t* client ){
    assert( client );

    memset( client->file_buf, '\0', client->file_buf_len );
    client->file_buf_len = 0;
}

void save_file_data( client_t* client, ssize_t nread ){
    assert( client );

    log_debug( "before write data size = %zd", nread );
    uv_fs_t* write_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );

    client->file_capacity = nread;	// file_cap = nread, because server uses uv_write, that guarantees that the file data is sent
    write_req->data = client;

    uv_buf_t write_buf = uv_buf_init( client->file_buf, (size_t)nread );
    uv_fs_write( client->handle->loop, write_req, client->file_fd, &write_buf, BUFFERS_COUNT, CURRENT_FILE_PTR, file_write_cb );
}

void file_write_cb( uv_fs_t* req ){
    assert( req );

    if( req->result < 0 ){
        log_error( "error writing data in file" );
        return ;
    }
    
    client_t* client = (client_t*)req->data;
    log_info( "successfully saved %zd bytes out of %lu", req->result, client->file_capacity );
    client->write_bytes += (size_t)req->result;
    check_file_writting( client, req );
}

void check_file_writting( client_t* client, uv_fs_t* req ){
    assert( client  );
    assert( req  );

    if( client->write_bytes >= client->file_capacity ){
	log_info( "all bytes (%lu) write saved successfully", client->write_bytes );
	uv_fs_req_cleanup( req );
	free( req );
	client->write_bytes = 0;		// cleaning for next recipients
	//clear_file_buffer( client );
	download_complete( windows );
	return ;
    }

    uv_fs_req_cleanup( req );
    uv_buf_t write_buf = uv_buf_init( client->file_buf + client->write_bytes, client->file_capacity - client->write_bytes );
    uv_fs_write( client->handle->loop, req, client->file_fd, &write_buf, BUFFERS_COUNT, CURRENT_FILE_PTR, file_write_cb  );
}

void parse_file_name( client_t* client, char* instruction ){
    assert( client );
    assert( instruction );

    log_debug( "in get_file_name: inst = '%s'", instruction );
    char* whitespace = strchr( instruction, ' ' );
    log_debug( "after in get_file_name: inst = '%s'", instruction );

    char* file_name = whitespace + 1;
    client->file_name = strdup( file_name );
    log_info( "reciver: file name = '%s'", client->file_name );

    create_get_file_win( console_size, windows );
    file_accept_request( windows->der_file_win, file_name );
    //clear_file_buffer( client );
}

void start_sending( client_t* client, char* instruction  ){
    assert( client  );
    assert( instruction  );

    close_file_windows( windows );
    create_get_file_win( console_size, windows );
    waiting_download_win( windows );
    open_file( client );
}

void open_file( client_t* client ){
    assert( client );

    if( client->file_fd > 0 ){
	sending_file( client );
	return ;
    }

    uv_fs_t* req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    req->data = client;
    char* sender_path = client->sender_path;
    size_t path_len = strlen( sender_path );
    if( path_len > 0 && sender_path[ path_len - 1 ] == '\n' ){
        sender_path[ path_len - 1 ] = '\0';
    }
    // client->handle, because handle and file_handle have common loop
    log_debug( "from %d: file path before open = '%s'", client->client_type, sender_path )
    int status = uv_fs_open( client->handle->loop, req, sender_path, O_RDONLY, FILE_MOD, fs_cb );
    if( status < 0 ){
        log_error( "file creation err = %s", uv_strerror( (int)req->result ) );
        return ;
    }

    log_info( "file successfully opened" );
}

void fs_cb( uv_fs_t* req ){
    assert( req );

    client_t* client = (client_t*)req->data;
    if( req->result < 0 ){
        log_error( "from %d: file open err = %s", client->client_type, uv_strerror( (int)req->result ) );
        uv_fs_req_cleanup( req );
        free( req );
        return ;
    }

    client->file_fd = req->result;
    uv_fs_req_cleanup( req );
    free( req );

    if( client->client_type == SENDER ){
	sending_file( client  );
    }
    else if( client->client_type == RECEIVER ){
	send_file_data( client, "/recipient_accepted\n" );
    }
}

void complete_sending( client_t* client, char* instruction ){
    assert( client );
    assert( instruction );

    log_debug( "info about sending file in room is received" );
    // pattern: /shipping_info accepted_count recipients_count = /shipping_info 8 10 ---> 80% accepted file submission

    char* first_whitespace = strchr( instruction, ' ' );
    char* second_whitespace = strchr( first_whitespace + 1, ' ' );
    *second_whitespace = '\0';

    unsigned long accepted_count = strtoul( first_whitespace + 1, NULL, DEC );
    unsigned long recipients_count = strtoul( second_whitespace + 1, NULL, DEC );


    clear_file_line( windows->der_file_win );
    dispatch_notification( windows->der_file_win, accepted_count, recipients_count );
    //clear_file_buffer( client  );
}


void sending_file( client_t* client ){
    assert( client );

    uv_os_fd_t socket_fd = {};                                                      // uv_tcp_t --> uv_os_fd_t
    uv_fileno( (const uv_handle_t*)client->file_handle, &socket_fd );

    struct stat file_data = {};
    log_info( "sender path before stat: %s", client->sender_path );
    int status = stat( client->sender_path, &file_data );
    if( status == -1 ){
        log_fatal( "client_type = %d, stat returned negative value", client->client_type);
        return ;
    }
    log_debug( "fle size from stat = %lu", file_data.st_size );
    client->file_capacity = file_data.st_size;

    uv_fs_t* req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    req->data = client;
    uv_fs_sendfile( client->handle->loop, req, socket_fd, client->file_fd, OFFSET, file_data.st_size, send_cb );
}

void send_cb( uv_fs_t* req ){
    assert( req );

    if( req->result < 0 ){
        log_error( "Error of sending file" );
        free( req );
        uv_fs_req_cleanup( req );
        return ;
    }
    
    client_t* client = (client_t*)req->data;
    log_info( "successfully sent %zd bytes out of %lu", req->result, client->file_capacity );
    client->sent_bytes += (size_t)req->result;
    check_data_sending( client, req );
}

void check_data_sending( client_t* client, uv_fs_t* req ){
    assert( client );

    if( client->sent_bytes >= client->file_capacity ){
        log_info( "all bytes (%lu) sent successfully", client->sent_bytes );
        uv_fs_req_cleanup( req );
        free( req );
	client->sent_bytes = 0;
	//clear_file_buffer( client );
        return ;
    }

    uv_os_fd_t socket_fd = {};
    uv_fileno( (const uv_handle_t*)client->file_handle, &socket_fd );

    uv_fs_req_cleanup( req );
    uv_fs_sendfile( client->handle->loop, req, socket_fd, client->file_fd,
                    client->sent_bytes, client->file_capacity - client->sent_bytes, send_cb );
}

void create_file( client_t* client ){
    assert( client );

    clear_file_line( windows->der_file_win );
    waiting_download_win( windows );

    size_t total_capacity = strlen(client->scr_buf) + strlen(client->file_name) + EXTRA_SPACE;
    char* path_and_name = (char*)calloc( total_capacity, sizeof(char) );
    log_debug( "receiver: file_name = '%s'", client->file_name );
    log_debug( "receive: file_path = '%s'", client->scr_buf );
    int count = snprintf( path_and_name, total_capacity, "%s%s", client->scr_buf, client->file_name );
    log_debug( "path and name: %s", path_and_name );

    uv_fs_t* req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    req->data = client;
    int status = uv_fs_open( client->file_handle->loop, req, path_and_name, O_WRONLY | O_CREAT | O_TRUNC, FILE_MOD , fs_cb );
    if( status < 0 ){
        log_error( "file creation err" );
        free( path_and_name );
        return ;
    }
    client->receiver_path = path_and_name;
}

void request_not_accepted( client_t* client ){
    assert( client );

    log_info( "refusal to download the file" );

    client->stopped_file_ch = true;
    send_file_data( client, "/recipient_not_accepted\n" );
}

void destroy_file_ch( client_t* client ){
    assert( client );

    send_file_data( client, "/destroy_transfer %lu\n", client->transfer_id );
    log_info( "start destroy transfer" );
}

void send_file_data( client_t* client, const char* format, ... ){
    assert( client );
    assert( format );

    uv_buf_t buffer = {};
    FILE* stream = open_memstream( &buffer.base, &buffer.len );                                         //creating a stream for recording
    va_list args = {};
    va_start( args, format );
    vfprintf( stream, format, args );
    fclose( stream );
    va_end( args );

    uv_write_t* req = (uv_write_t*)calloc( ONE, sizeof(uv_write_t) );
    req->data = buffer.base;
    if( uv_write( req, (uv_stream_t*)(client->file_handle), &buffer, ONE, record_cb ) < 0 ){             //writing data to descriptor
        log_fatal( "write return negative value" );
        free( buffer.base );
        free( req );
	//clear_file_buffer( client );
    }
}

void disconnect_cb( uv_shutdown_t* req, int status ){
    assert( req );

    if( status < 0 ){
        free( req );
        log_error( "shutdown callback get negative status" );
    }
    if( !uv_is_closing( (uv_handle_t*)req->handle ) ){
        uv_close( (uv_handle_t*)req->handle, closure_cb );
    }
    free( req );
}

void record_cb( uv_write_t* req, int status ){
    assert( req );

    if( status < 0 ){
        log_error( "can not write message for server" );
        free( req->data );
        free( req );
        return ;
    }

    client_t* client = (client_t*)req->handle->data;
    if( client->stopped_file_ch ){
        uv_close( (uv_handle_t*)client->file_handle, closure_cb );
    }

    free( req->data );
    free( req );
}

void closure_cb( uv_handle_t* handle ){
    assert( handle );

    client_t* client = (client_t*)handle->data;

    if( client->file_buf ){
        free( client->file_buf );
        client->file_buf = NULL;
    }
    if( client->receiver_path ){
        free( client->receiver_path );
        client->receiver_path = NULL;
    }
    if( client->sender_path ){
	free( client->sender_path );
	client->sender_path = NULL;
    }
    if( client->file_name  ){
	free( client->file_name  );
	client->file_name = NULL;
    }

   client->file_fd = 0;
   client->sender_path = NULL;
   client->transfer_id = 0;
   client->file_buf_len = 0;
   client->file_buf_cap = 0;
   client->write_bytes = 0;
   client->sent_bytes = 0;
   client->file_capacity = 0;
   client->client_type = UNDEFINED_TYPE;
   client->stopped_file_ch = false;   

   free( handle );                      // handle == client->file_handle
   client->file_handle = NULL;
}
