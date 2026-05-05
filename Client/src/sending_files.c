#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "sending_files.h"
#include "logger.h"
#include "network_functions.h"
#include "user_interface.h"

const size_t FILE_PORT = 27011;
static const size_t ONE = 1;
static const size_t TWO = 2;
static const size_t EXTRA_SPACE = 5;                 // for snprint
static const int FILE_MOD = 0644;                    // owner can read and write, other can only read
static const unsigned int BUFFERS_COUNT = 1;         // for uv_fs_write
static const int64_t CURRENT_FILE_PTR = -1;          // for uv_fs_writ
static const size_t DEC = 10;			     // for strtoul
static const long MIN_COMMAND_SIZE = 3;
static const size_t PART_SIZE = 8192;		     // the file will be sent in parts of 1024 bytes
static const size_t MAX_DIGITS = 100;		     // for snprintf

// global vars for user interface
static winsize_t* console_size;
static windows_t* windows;
static user_info_t* user_data;

file_command_t srv_instructions[] = {
    { "/ok"			,	get_srv_answer  	},
    { "/chunk"			,       download_file   	},
    { "/close"		        ,	finish_downloading 	},
    { "/sender_ID"		,	get_transfer 		}
};
size_t instructions_count = sizeof(srv_instructions) / sizeof(file_command_t);

void open_new_connection( main_struct_t* main_struct, main_connection_t* main_connection ){
    assert( main_struct );
    assert( main_connection );

    console_size = main_struct->console_size;
    windows = main_struct->windows;
    user_data = main_struct->user_data;

    uv_loop_t* loop = main_connection->loop;
    client_t* client = main_connection->client;

    uv_tcp_t* client_socket = (uv_tcp_t*)calloc( ONE, sizeof(uv_tcp_t) );
    uv_connect_t* connect = (uv_connect_t*)calloc( ONE, sizeof(uv_connect_t) );
    uv_tcp_init( loop, client_socket );                                                            // init descriptor, but not make socket
    struct sockaddr_in client_addr = {};                                                            // describe socket: port, ip ...
    if( uv_ip4_addr( user_data->ip, FILE_PORT, &client_addr ) != 0 ){                               // converting string to binary struct
        log_fatal( "error converting IP address to struct" );
        free( connect );
        return ;
    }
    client->file_handle = client_socket;
    client_socket->data = client;

    int connect_status = 0;
    connect->data = client;                                                                         // save client
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
	free( req );
        return ;
    }

    client_t* client = (client_t*)req->data;
    int read_server = uv_read_start( (uv_stream_t*)client->file_handle, alloc_сb, read_cb );             // req->handle - file descriptor wrapper
    if( read_server < 0 ){
        log_warning( "uv_read_start return negative value" );
	free( req );
        return ;
    }
    if( client->client_type == RECEIVER ){
        send_file_data( client->file_handle, "/retrieve %lu\n", client->transfer_id );
    }
    else if( client->client_type == SENDER ){
	send_file_information( client );
    }
    free( req );
}

void send_file_information( client_t* client ){
    assert( client );
    
    if( get_file_size( client ) != NORMAL_WORK ){
	log_error( "error of getting file size" );
	return ;
    }
    log_debug( "client file name = '%s', client file cap = %lu", client->file_name, client->file_capacity );
    send_file_data( client->file_handle, "/store %s %lu\n", client->file_name, client->file_capacity );
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

void parse_file_data( client_t* client, ssize_t nread, size_t (*on_cmd)( client_t* client, char* instruction ) ){
    assert( client  );

    log_debug( "IN PARS FILE DATA = %.*s", client->file_buf_len + (size_t)nread, client->file_buf );

    char* newline_char = NULL;
    char* buf_start = client->file_buf;
    client->file_buf_len += (size_t)nread;

    size_t buffer_offset = 0;
    while( client->file_buf_len > 0 ){
	if( buf_start[0] != '/' ){
	    log_error( "error of parsing message. BUF_START[0] = %c", buf_start[0] );
	    break;
	}	
	buffer_offset = parse_command( client, buf_start );
	if( buffer_offset == 0 ){
		log_warning( "buffer_offset = 0" );
		break;
	}
	buf_start += buffer_offset;
	
	log_debug( "buffer_offset = %lu", buffer_offset );
	log_info( "bytes left in the buffer: %lu", client->file_buf_len - ( buf_start - client->file_buf ) );
        if( buf_start < client->file_buf + client->file_buf_len ){
            memmove( client->file_buf, buf_start, client->file_buf_len - ( buf_start - client->file_buf ) );
        }
        client->file_buf_len -= buf_start - client->file_buf;
	buf_start = client->file_buf;
        log_info( "string line after: %lu", client->file_buf_len );
    }
}

size_t parse_command( client_t* client, char* instruction){
    assert( client );
    assert( instruction );

    if( instruction[0] == '\0' ){
        return 0;
    }

    file_command_t* command_begining = srv_instructions;
    file_command_t* current_command =  command_begining;

    char* srv_command = NULL;
    size_t buffer_offset = 0;
    for(; current_command < command_begining + instructions_count; current_command++ ){
        srv_command = (*current_command).command_name;
        if( strncmp( instruction, srv_command, strlen(srv_command) ) == 0 ){
            log_debug( "command '%s' was founded", srv_command );
            buffer_offset = (*current_command).func( client, instruction );
            return buffer_offset;
        }
    }

    log_error( "command '%s' was not founded", instruction );
    return 0;
}

size_t get_transfer( client_t* client, char* command ){
    assert( client );
    assert( command );

    char* whitespace = strchr( command, ' ' );
    char* newline = strchr( command, '\n' );
    client->transfer_id = strtoul( whitespace + 1, NULL, DEC );

    waiting_download_win( windows );

    send_chunk( client );
    
    return newline - command + 1;
}

void send_chunk( client_t* client ){
    assert( client );
    
    if( client->file_capacity == 0 ){
	log_info( "SEDNER: finish sending file data!" );
	close_file_ch( client );
	return ;
    }
    uv_fs_t* open_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    open_req->data = client;
    int status = uv_fs_open( client->file_handle->loop, open_req, client->sender_path, O_RDONLY, FILE_MOD, open_cb );
}

void open_cb( uv_fs_t* open_req ){
    assert( open_req );

    if( open_req->result < 0 ){
        log_error( "FROM SENDER: file open err = %s", uv_strerror( (int)open_req->result ) );
        uv_fs_req_cleanup( open_req );
        free( open_req );
        return ;
    }
    
    client_t* client = (client_t*)open_req->data;
    client->file_fd = open_req->result;
    uv_fs_req_cleanup( open_req );
    free( open_req );

   client->file_data = (char*)calloc( PART_SIZE, sizeof(char) );
   uv_fs_t* read_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
   read_req->data = client;
   uv_buf_t read_buf = uv_buf_init( client->file_data, PART_SIZE );
   uv_fs_read( client->file_handle->loop, read_req, client->file_fd, &read_buf, ONE, client->offset, fs_read_cb );
}

void fs_read_cb( uv_fs_t* read_req ){
    assert( read_req );
 
    client_t* client = (client_t*)read_req->data;
    if( read_req->result < 0 ){
	log_error( "uv_fs_read return negative value in callback" );
	free( read_req );
	free( client->file_data );
	return ;
    }
    uv_fs_req_cleanup( read_req );
    free( read_req );

    size_t snprintf_len = strlen( "/chunk" ) + MAX_DIGITS;
    char* snprintf_line = (char*)calloc( MAX_DIGITS, sizeof(char)  );
    char* binary_chunk = client->file_data;
    size_t size = PART_SIZE <= client->file_capacity
	                   ? PART_SIZE
			   : client->file_capacity;

    // /chunk <transfer_id> <offset> <size>
    size_t command_size = snprintf( snprintf_line, snprintf_len, "/chunk %lu %lu %lu ",
		                   client->transfer_id, client->offset, size );

    uv_buf_t bufs[2] = {};
    bufs[0] = uv_buf_init( snprintf_line, command_size );
    bufs[1] = uv_buf_init( binary_chunk, size );

    uv_write_t* req = calloc( ONE, sizeof(uv_write_t) );
    if( !req ){
	log_error( "calloc return null ptr" );
	return ;
    }
    client->command_line = snprintf_line;
    req->data = client;
    uv_write( req, (uv_stream_t*)client->file_handle, bufs, TWO, fs_write_cb );
    client->file_capacity -= size;
    client->offset += size;
    transmission_end( client );
}

client_err_t get_file_size( client_t* client ){
    assert( client );

    char* sender_path = client->sender_path;
    struct stat file_data = {};
    log_info( "sender path before stat: %s", sender_path );
    int status = stat( sender_path, &file_data );
    if( status == -1 ){
        log_fatal( "client_type = %d, stat returned negative value", client->client_type);
        return STAT_ERR;
    }
    log_debug( "fle size from stat = %lu", file_data.st_size );
    client->file_capacity = file_data.st_size;
    log_info( "file successfully opened" );

    return NORMAL_WORK;
}

void transmission_end( client_t* client ){
    assert( client );

    if( client->file_capacity == 0 ){
	// /recipients_ID <transfer id> <file name>
	send_file_data( client->handle, "/recipients_ID %lu %s\n", client->transfer_id, client->file_name );
	log_debug( "SENDER FINISH SENDING" );
	dispatch_notification( windows->der_file_win );
    }
}

void close_file_ch( client_t* client ){
    assert( client );

    uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
    if( uv_shutdown( shutdown_req, (uv_stream_t*)client->file_handle, disconnect_cb ) < 0 ){
	log_error( "SENDER: shutdown return negative value" );
	return ;
    }
    log_info( "sendfer close file channel" );
}

void fs_write_cb( uv_write_t* req, int status ){
    assert( req );

    if( status < 0 ){
	log_error( "error sending file" );
	free( req );
	free( req->data );
	return ;
    }

   client_t* client = (client_t*)req->data;
   if( client->command_line ){
	free( client->command_line );
	client->command_line = NULL;
   }
   if( client->file_data ){
	free( client->file_data );
	client->file_data = NULL;
   }
    
   free( req );
   uv_fs_t* close_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
   uv_fs_close( client->file_handle->loop, close_req, client->file_fd, fs_close );
}

void fs_close( uv_fs_t* close_req ){
    assert( close_req );

    if( close_req->result < 0 ){
	log_error( "file close error" );
    }

    uv_fs_req_cleanup( close_req );
    free( close_req );
}

size_t get_srv_answer( client_t* client, char* command ){
    assert( client );
    assert( command );

    char* first_wh = strchr( command, ' ' );
    char* second_wh = strchr( first_wh + 1, ' ' );
    char* newline = strchr( second_wh + 1, '\n' );
    *second_wh = '\0';

    unsigned long transfer_id = strtoul( first_wh + 1, NULL, DEC );
    size_t current_offset = strtoul( second_wh + 1, NULL, DEC );

    if( transfer_id != client->transfer_id ){
	log_fatal( "IN CLIENT: client id != id from server. Client = %lu, tranfer = %lu.", client->transfer_id, transfer_id );
	return 0;
    }
    if( current_offset != client->offset ){
	log_fatal( "IN CLIENT: client offset != offset from server. Client offset = %lu, offset = %lu", client->offset, current_offset );
	return 0;
    }

    send_chunk( client );

    return newline - command + 1;
}

size_t download_file( client_t* client, char* command ){
    assert( client );
    assert( command );

    chunk_data_t* chunk_data = (chunk_data_t*)calloc( ONE, sizeof(chunk_data_t) );
    if( !chunk_data ){
	log_error( "calloc return null ptr" );
	return 0;
    }
    *chunk_data = read_chunk( client, command );
    if( client->transfer_id != chunk_data->transfer_id ){
	client->transfer_id = chunk_data->transfer_id;
    }
    client->chunk_data = chunk_data;
    
    size_t full_path_cap = strlen( client->file_name ) + strlen( client->receiver_path ) + EXTRA_SPACE;
    char* full_path = (char*)calloc( full_path_cap, sizeof(char) );
    size_t full_path_len = snprintf( full_path, full_path_cap, "%s%s", client->receiver_path, client->file_name );

    uv_fs_t* req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    client->full_path = full_path;
    req->data = client;
    int status = uv_fs_open( client->file_handle->loop, req, full_path, O_WRONLY | O_CREAT, FILE_MOD , file_open_cb );
    if( status < 0 ){
        log_error( "file creation err" );
        return 0;
    } 

    size_t command_len = chunk_data->binary_start - command + chunk_data->size;
    return command_len;
}

chunk_data_t read_chunk( client_t* client, char* command ){
    assert( client );
    assert( command );

    char* first_wh = strchr( command, ' ' );
    char* second_wh = strchr( first_wh + 1, ' ' );
    char* third_wh = strchr( second_wh + 1, ' ' );
    char* fourth_wh = strchr( third_wh + 1, ' ' );

    *second_wh = '\0';
    *third_wh = '\0';
    *fourth_wh = '\0';

    unsigned long trasnfer_id = strtoul( first_wh + 1, NULL, DEC );
    size_t offset = (unsigned long)strtoul( second_wh + 1, NULL, DEC );
    size_t size = (unsigned long)strtoul( third_wh + 1, NULL, DEC );
    char* binary_chunk = fourth_wh + 1;
    chunk_data_t chunk_info = { binary_chunk, trasnfer_id, offset, size };

    return chunk_info;
}

void file_open_cb( uv_fs_t* open_req ){
    assert( open_req );

    client_t* client = (client_t*)open_req->data;

    if( open_req->result < 0 ){
        log_error( "FROM RECEIVER: file open err = %s, load_path = '%s'", uv_strerror( (int)open_req->result ), client->full_path );
        uv_fs_req_cleanup( open_req );
        free( open_req );
        return ;
    }
    if( client->full_path ){
	free( client->full_path);
	client->full_path = NULL;
    }
    client->file_fd = open_req->result;
    ++( client->active_writes );
    
    // Pattern: /chunk <transfer-id> <offset> <size> <binary-chunk>
    chunk_data_t chunk_data = *(client->chunk_data);
    uv_fs_t* write_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    write_req->data = client;
    uv_buf_t write_buf = uv_buf_init( chunk_data.binary_start, chunk_data.size );
    uv_fs_write( open_req->loop, write_req, client->file_fd, &write_buf, BUFFERS_COUNT, chunk_data.offset, receiver_write_cb );
    uv_fs_req_cleanup( open_req );
    free( open_req );
}

void receiver_write_cb( uv_fs_t* write_req ){
    assert( write_req );

    if( write_req->result < 0 ){
	log_error( "error of writing data in srv file" );
	uv_fs_req_cleanup( write_req );
	free( write_req );
	return ;
    }
    
    client_t* client = (client_t*)write_req->data;
    --( client->active_writes );
    if( client->stopped_file_ch && client->active_writes == 0 ){
	uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
	uv_shutdown( shutdown_req, (uv_stream_t*)client->file_handle, disconnect_cb );
	uv_fs_req_cleanup( write_req );
        free( write_req );
	return ;
    }
    
    chunk_data_t chunk_info = *(client->chunk_data);
    send_file_data( client->file_handle, "/ok %lu %lu\n", client->transfer_id, chunk_info.offset + chunk_info.size );
    log_info( "IN RECEVIER: successfully saved %zu", write_req->result );
    
    uv_fs_t* close_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    close_req->data = client;
    uv_fs_close( write_req->loop, close_req, client->file_fd, receiver_close_cb );
    uv_fs_req_cleanup( write_req );
    free( write_req );
}

void receiver_close_cb( uv_fs_t* close_req ){
    assert( close_req );
    
    client_t* client = (client_t*)close_req->data;
    if( close_req->result < 0 ){
	log_error( "srv file close error" );
    }
    if( client->chunk_data ){
	free( client->chunk_data );
	client->chunk_data = NULL;
    }
    
    uv_fs_req_cleanup( close_req );
    free( close_req );
}

size_t finish_downloading( client_t* client, char* command ){
    assert( client );
    assert( command );

    client->app_state = FILE_REQUEST;
    client->stopped_file_ch = true;

    download_complete( windows );

    uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
    if( client->active_writes == 0 ){
	if( uv_shutdown( shutdown_req, (uv_stream_t*)client->file_handle, disconnect_cb ) < 0 ){
	    free( shutdown_req );
	    log_error( "RECEIVER: shutdown return negative value" );
	}
    }

    log_info( "receiver close file channel" );
    char* newline = strchr( command, '\n' );
    return newline - command + 1;
}


void send_file_data( uv_tcp_t* handle, const char* format, ... ){
    assert( format );

    if( !handle || uv_is_closing( (uv_stream_t*)handle ) ){
	log_warning( "socket was closed, can't write message" );
	return ;
    }

    uv_buf_t buffer = {};
    FILE* stream = open_memstream( &buffer.base, &buffer.len );                                         //creating a stream for recording
    va_list args = {};
    va_start( args, format );
    vfprintf( stream, format, args );
    fclose( stream );
    va_end( args );

    uv_write_t* req = (uv_write_t*)calloc( ONE, sizeof(uv_write_t) );
    req->data = buffer.base;
    if( uv_write( req, (uv_stream_t*)handle, &buffer, ONE, record_cb ) < 0 ){             //writing data to descriptor
        log_fatal( "write return negative value" );
        free( buffer.base );
        free( req );
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
    if( client->full_path ){
	free( client->full_path );
	client->full_path = NULL;
    }

   client->file_fd = 0;
   client->transfer_id = 0;
   client->file_buf_len = 0;
   client->file_buf_cap = 0;
   client->write_bytes = 0;
   client->offset = 0;
   client->file_capacity = 0;
   client->client_type = UNDEFINED_TYPE;
   client->stopped_file_ch = false;

   free( handle );                      // handle == client->file_handle
   client->file_handle = NULL;
}
