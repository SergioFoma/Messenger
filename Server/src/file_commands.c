#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>

#include "file_commands.h"
#include "logging.h"

static const size_t ONE = 1;
static const size_t TWO = 2;
static const size_t INIT_NUM = 5;
static const size_t DEC = 10;
static const size_t MAX_TIME_LEN = 20;		     // for time( NULL ) - max count of digits in number
static const size_t EXTRA_SPACE = 5;                 // for snprint
static const size_t AUTO_BASE = 0;		     // for strtoul
static const int FILE_MOD = 0644;
static const size_t BUFFERS_COUNT = 1;		     // for fs_write
static const size_t MAX_DIGITS = 100;		     // for snprintf
static const size_t PART_SIZE = 8192;		     
const char* CHUNK_PATH = "Transfers";

chunk_command_t supported_commands[] = {
    { "/ok"			 ,	 send_chunk		},
    { "/chunk"			 ,	 save_chunk 		},
    { "/store"        	 	 ,       init_sender     	},
    { "/retrieve"		 , 	 init_recipient	 	}
};
size_t commands_count = sizeof(supported_commands) / sizeof(chunk_command_t);
static const long int MIN_COMMAND_SIZE = 3;

void connect_file_channel( uv_stream_t* server, int status ){
    assert( server );

    client_t* client = (client_t*)calloc( ONE, sizeof(client_t) );
    if( client == NULL ){
        log_error( "can not allocate memory for client" );
        return ;
    }

    client_init( client );
    uv_tcp_init( server->loop, &client->file_handle );                                                     //descriptor initialization
    client->file_handle.data = client;
    int accept_status = uv_accept( server, (uv_stream_t*)&client->file_handle );                           //communication between the client socket and the server socket
    if( accept_status < 0 ){
        log_error( "can not accept" );
        uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
        if( uv_shutdown( shutdown_req, (uv_stream_t*)&client->file_handle, shutdown_channel ) < 0 ){       //closing connection
            log_panic( "shutdown return negative value" );
            free( shutdown_req );
            free( client );
        }
        return ;
    }

    log_info( "new client connected to file channel" );
    uv_read_start( (uv_stream_t*)&client->file_handle, file_alloc_cb, read_file_ch );                      //starting an asynchronous connection
}

void file_alloc_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    size_t predicted_size = client->file_len + suggested_size + 1;
    if( client->file_buf_cap > 0 ){
        if( realloc_file_buf( client, predicted_size ) == MEMORY_REALLOC_ERR ){
            return ;
        }
    }
    else{
        client->file_buf = (char*)calloc( predicted_size, sizeof(char) );
        if( !client->file_buf ){
            log_panic( "error allocating memory for the client buffer" );
            return ;
        }
    }

    client->file_buf_cap = client->file_buf_cap >= predicted_size
                       ? client->file_buf_cap
                       : predicted_size;
    buf->base = client->file_buf + client->file_len;
    buf->len = client->file_buf_cap - client->file_len - 1;
}

error realloc_file_buf( client_t* client, size_t predicted_size ){
    assert( client );

    char* helpful_buffer = NULL;
    if( client->file_buf_cap < predicted_size ){
        helpful_buffer = (char*)realloc( client->file_buf, predicted_size );
        if( !helpful_buffer ){
            log_panic( "realloc returned null ptr" );
            return MEMORY_REALLOC_ERR;
        }
        client->file_buf = helpful_buffer;
    }

    return CORRECT;
}

void read_file_ch( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    if( nread >= 0 ){
        log_info( "read got %zd", nread );
        parse_message( client, nread, parse_instruction );
        return;
    }

    log_warning( "client closed file channel" );
    uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
    if( !uv_is_closing( (uv_handle_t*)handle )  ){
	free( shutdown_req );
	uv_close( (uv_handle_t*)handle, finish_cb );
	return ;
    }
}

void parse_message( client_t* client, ssize_t nread, size_t (*on_cmd)( client_t* client, char* string ) ){
    assert( client );

    char* buf_start = client->file_buf;
    log_debug( "buf_start: %s", buf_start );
    char* newline_char = NULL;

    client->file_len += (size_t)nread;
    log_info( "string len: %lu", client->file_len );

    size_t buffer_offset = 0;
    while( client->file_len > 0 ){
	if( buf_start[0] != '/' ){
	    log_error( "error of parsing message. BUF_START[0] = %c", buf_start[0] );
	    break;
	}	
	buffer_offset = parse_instruction( client, buf_start );
	if( buffer_offset == 0 ){
		log_warning( "buffer_offset = 0" );
		break;
	}
	buf_start += buffer_offset;
	
	log_debug( "buffer_offset = %lu", buffer_offset );
	log_info( "bytes left in the buffer: %lu", client->file_len - ( buf_start - client->file_buf ) );
        if( buf_start < client->file_buf + client->file_len ){
            memmove( client->file_buf, buf_start, client->file_len - ( buf_start - client->file_buf ) );
        }
        client->file_len -= buf_start - client->file_buf;
	buf_start = client->file_buf;
        log_info( "string line after: %lu", client->file_len );
    }
}

size_t parse_instruction( client_t* client, char* string ){
    assert( client );
    assert( string );

    log_debug( "string in parse_instruction: %s", string );
    if( string[0] == '\0' ){
        return 0;
    }

    size_t command_index = 0;
    size_t command_len = 0;
    for( ; command_index < commands_count; command_index++ ){
        const char* current_command = supported_commands[command_index].command_name;
        if( strncmp( string, current_command, strlen( current_command ) ) == 0 ){
            command_len = supported_commands[command_index].cmd( client, string );
	    log_debug( "command len = %lu", command_len );
            return command_len;
        }
    }
    send_file_data( &client->file_handle, "Unknown command: %s\n", string );
    return 0;
}

size_t init_sender( client_t* client, char* string ){
    assert( client );
    assert( string );
    // Pattern: /store <file-name> <file-size> <tags>
    char* first_whitespace = strchr( string, ' ' );
    char* second_whitespace = strchr( first_whitespace + 1, ' ' );
    char* newline = strchr( second_whitespace + 1, '\n' );
    *second_whitespace = '\0';
    *newline = '\0';

    char* file_name = first_whitespace + 1;
    log_debug( "file name in server: '%s'", file_name );
    char* string_file_cap = second_whitespace + 1;
    time_t seconds = time( NULL );						// currect secodns since 1970
    size_t hash_size = strlen( file_name ) + MAX_TIME_LEN + EXTRA_SPACE;
    char* hash_line = (char*)calloc( hash_size, sizeof(char) );
    snprintf( hash_line, hash_size, "%s_%ld", file_name, seconds );

    unsigned long transfer_id = hash( hash_line );
    client->file_capacity = strtoul( string_file_cap, NULL, AUTO_BASE );
    client->transfer_id =transfer_id;
    client->file_name = strdup( file_name );

    send_file_data( &client->file_handle, "/sender_ID %lu\n", transfer_id ); 
    free( hash_line );
    
    return newline - string + 1;
}

size_t save_chunk( client_t* client, char* string ){
    assert( client );
    assert( string );
    
    chunk_info_t* chunk_info = (chunk_info_t*)calloc( ONE, sizeof(chunk_info_t) );
    if( !chunk_info ){
	log_error( "calloc return null ptr" );
	return 0;
    }
    *chunk_info = read_chunk_line( client, string );
    if( client->transfer_id != chunk_info->transfer_id ){
	client->transfer_id = chunk_info->transfer_id;
    }
    client->chunk_info = chunk_info;
    
    size_t capacity = strlen( CHUNK_PATH ) + MAX_DIGITS;
    char* load_path = (char*)calloc( capacity, sizeof(char) );
    char* extension = strchr( client->file_name, '.' ) + 1;
    size_t command_len = snprintf( load_path, capacity, "%s/%lu.%s", CHUNK_PATH, client->transfer_id, extension );
    client->load_path = load_path;

    uv_fs_t* req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    req->data = client;
    int status = uv_fs_open( client->file_handle.loop, req, load_path, O_WRONLY | O_CREAT, FILE_MOD , fs_open_cb );
    if( status < 0 ){
        log_error( "file creation err" );
        return 0;
    }

    size_t inst_len = ( chunk_info->binary_start - string ) + chunk_info->size;
   return inst_len;
}

void fs_open_cb( uv_fs_t* open_req ){
    assert( open_req );

    client_t* client = (client_t*)open_req->data;

    if( open_req->result < 0 ){
        log_error( "FROM SERVER: file open err = %s, load_path = '%s'", uv_strerror( (int)open_req->result ), client->load_path );
        uv_fs_req_cleanup( open_req );
        free( open_req );
        return ;
    }
    if( client->load_path ){
	free( client->load_path );
	client->load_path = NULL;
    }
    client->file_fd = open_req->result;
    uv_fs_req_cleanup( open_req );
    free( open_req );
 
    // Pattern: /chunk <transfer-id> <offset> <size> <binary-chunk>
    chunk_info_t chunk_info = *(client->chunk_info);
    uv_fs_t* write_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    write_req->data = client;
    uv_buf_t write_buf = uv_buf_init( chunk_info.binary_start, chunk_info.size );
    uv_fs_write( client->file_handle.loop, write_req, client->file_fd, &write_buf, BUFFERS_COUNT, chunk_info.offset, file_write_cb );
}

void file_write_cb( uv_fs_t* write_req ){
    assert( write_req );

    if( write_req->result < 0 ){
	log_error( "error of writing data in srv file" );
	uv_fs_req_cleanup( write_req );
	free( write_req );
	return ;
    }
    
    client_t* client = (client_t*)write_req->data;
    chunk_info_t chunk_info = *(client->chunk_info);
    send_file_data( &client->file_handle, "/ok %lu %lu\n", client->transfer_id, chunk_info.offset + chunk_info.size );
    log_info( "IN SERVER: successfully saved %zd bytes out of %lu", write_req->result, client->file_capacity );

    uv_fs_req_cleanup( write_req );
    free( write_req );
    uv_fs_t* close_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    uv_fs_close( client->file_handle.loop, close_req, client->file_fd, file_close_cb );
}

void file_close_cb( uv_fs_t* close_req ){
    assert( close_req );

    if( close_req->result < 0 ){
	log_error( "srv file close error" );
    }

    uv_fs_req_cleanup( close_req );
    free( close_req );
}

chunk_info_t read_chunk_line( client_t* client, char* string ){
    assert( client );
    assert( string );

    char* first_wh = strchr( string, ' ' );
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
    chunk_info_t chunk_info = { binary_chunk, trasnfer_id, offset, size };

    return chunk_info;
}

size_t init_recipient( client_t* client, char* string ){
    assert( client );
    assert( string );

    char* first_wh = strchr( string, ' ' );
    char* newline = strchr( first_wh + 1, '\n' );
    *newline = '\0';
    
    client->transfer_id = strtoul( first_wh + 1, NULL, DEC );
    char* file_name = get_file_name( client );
    size_t path_cap = strlen( CHUNK_PATH ) + strlen( file_name ) + EXTRA_SPACE;
    char* load_path = calloc( path_cap, sizeof(char) );
    size_t path_len = snprintf( load_path, path_cap, "%s/%s", CHUNK_PATH, file_name );
    client->load_path = load_path;

    srv_file_size( client );

    send_srv_chunk( client );

    size_t command_len = newline - string + 1;
    return command_len;
}

char* get_file_name( client_t* client ){
    assert( client );

    struct dirent* folder_part = NULL;
    DIR* folder = opendir( CHUNK_PATH );

    if( folder == NULL ){
	log_error( "error of opening chunk folder" );
	return NULL;
    }
    
    char* file_name = NULL;
    unsigned long transfer_id = 0;
    while( (folder_part = readdir( folder ) ) != NULL ){
	file_name = folder_part->d_name;
	log_debug( "transfer file = '%s'", file_name );
	transfer_id = strtoul( file_name, NULL, DEC );
	if( transfer_id == client->transfer_id ){
	    return file_name;
	}
    }

    log_error( "error finding transfer file" );
    return NULL;
}

void srv_file_size( client_t* client ){
    assert( client );

    char* load_path = client->load_path;
    struct stat file_data = {};
    log_info( "sender path before stat: %s", load_path );
    int status = stat( load_path, &file_data );
    if( status == -1 ){
        log_panic( "IN SERVER: stat returned negative value" );
        return ;
    }
    log_debug( "fle size from stat = %lu", file_data.st_size );
    client->file_capacity = file_data.st_size;
    log_info( "file successfully opened" );
}

void send_srv_chunk( client_t* client ){
    assert( client );
    
    if( client->file_capacity == 0 ){
	log_info( "SERVER: finish sending file data" );
	free( client->load_path );
	client->load_path = NULL;
	return ;
    }
    uv_fs_t* open_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    open_req->data = client;
    int status = uv_fs_open( client->file_handle.loop, open_req, client->load_path, O_RDONLY, FILE_MOD, srv_open_cb );
}

void srv_open_cb( uv_fs_t* open_req ){
    assert( open_req );

    if( open_req->result < 0 ){
	log_error( "FROM SERVER: file open err = %s", uv_strerror( (int )open_req->result ) );
	uv_fs_req_cleanup( open_req );
	free( open_req );
	return ;
    }
    
    client_t* client = (client_t*)open_req->data;
    client->file_fd = open_req->result;
    uv_fs_req_cleanup( open_req );
    free( open_req );

    client->srv_file_data = (char*)calloc( PART_SIZE, sizeof(char) );
    uv_fs_t* read_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
    read_req->data = client;
    uv_buf_t read_buf = uv_buf_init( client->srv_file_data, PART_SIZE );
    uv_fs_read( client->file_handle.loop, read_req, client->file_fd, &read_buf, ONE, client->offset, srv_read_cb );
}

void srv_read_cb( uv_fs_t* read_req ){
    assert( read_req );

    client_t* client = (client_t*)read_req->data;
    if( read_req->result < 0 ){
	log_error( "uv_fs_read return negative value in callback" );
	free( read_req );
	free( client->srv_file_data );
	return ;
    }
    free( read_req );

    size_t snprintf_len = strlen( "/chunk" ) + MAX_DIGITS;
    char* snprintf_line = (char*)calloc( MAX_DIGITS, sizeof(char)  );
    char* binary_chunk = client->srv_file_data;
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
    client->chunk_line = snprintf_line;
    req->data = client;
    uv_write( req, &client->file_handle, bufs, TWO, srv_write_cb );
    client->file_capacity -= size;
    client->offset += size;
    check_file_end( client );
} 

void check_file_end( client_t* client ){
    assert( client );

    if( client->file_capacity == 0 ){
	send_file_data( &client->file_handle, "/close %lu\n", client->transfer_id );
    }
}

void srv_write_cb( uv_write_t* write_req, int status ){
   assert( write_req );

    if( status < 0 ){
	log_error( "error sending file" );
	free( write_req );
	free( write_req->data );
	return ;
    }

   client_t* client = (client_t*)write_req->data;
   if( client->chunk_line ){
	free( client->chunk_line );
	client->chunk_line = NULL;
   }
   if( client->srv_file_data ){
	free( client->srv_file_data );
	client->srv_file_data = NULL;
   }
    
   free( write_req );
   uv_fs_t* close_req = (uv_fs_t*)calloc( ONE, sizeof(uv_fs_t) );
   uv_fs_close( client->file_handle.loop, close_req, client->file_fd, srv_file_close );
}

void srv_file_close( uv_fs_t* close_req ){
    assert( close_req );
   
    if( close_req->result < 0 ){
	log_error( "nwegative result in close callbacl" );
    }

    uv_fs_req_cleanup( close_req );
    free( close_req );
}

size_t send_chunk( client_t* client, char* string ){
   assert( client );
   assert( string );

    char* first_wh = strchr( string, ' ' );
    char* second_wh = strchr( first_wh + 1, ' ' );
    char* newline = strchr( second_wh + 1, '\n' );
    *second_wh = '\0';
    *newline = '\0';

    unsigned long transfer_id = strtoul( first_wh + 1, NULL, DEC );
    size_t current_offset = strtoul( second_wh + 1, NULL, DEC );

    if( transfer_id != client->transfer_id ){
	log_panic( "IN SERVER: server id != id from client. Server = %lu, tranfer = %lu.", client->transfer_id, transfer_id );
	return 0;
    }
    if( current_offset != client->offset ){
	log_panic( "IN SERVER: server offset != offset from client. Server offset = %lu, offset = %lu", client->offset, current_offset );
	return 0;
    }

    send_srv_chunk( client );
    return newline - string + 1;
}

void shutdown_channel( uv_shutdown_t* shutdown_req, int status ){
    assert( shutdown_req );

    if( !uv_is_closing( (uv_handle_t*)shutdown_req->handle ) ){
        uv_close( (uv_handle_t*)shutdown_req->handle, finish_cb );                                       //closing a socket
    }
    free( shutdown_req );
}


void send_file_data( uv_tcp_t* file_handle, const char* format, ... ){
    assert( file_handle );
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
    if( uv_write( req, (uv_stream_t*)file_handle, &buffer, ONE, record_cb ) < 0 ){                                  //writing data to descriptor
        log_panic( "write return negative value" );
        free( buffer.base );
        free( req );
    }
    log_debug( "succesfully send = '%lu'", buffer.len );
}

void record_cb( uv_write_t* write_req, int status ){
    assert( write_req );

    if( status < 0 ){
        log_error( "can not write message" );
        free( write_req->data );
        free( write_req );
        return ;
    }
    
    char* mess = (char*)write_req->data;
    log_debug( "mess in record cb = '%s'", mess );

    client_t* client = (client_t*)write_req->handle->data;
    if( client->is_stopped ){
        uv_close( (uv_handle_t*)client, finish_cb );
    }
    free( write_req->data );
    free( write_req );
}

void finish_cb( uv_handle_t* handle ){
    assert( handle );

    client_t* client = (client_t*)handle->data;
    
    if( client->file_buf ){
        free( client->file_buf );
	client->file_buf = NULL;
    }
    if( client->file_name ){
	free( client->file_name );
	client->file_name = NULL;
    }
    if( client->load_path ){
	free( client->load_path );
	client->load_path = NULL;
    }

    free( client );
}
