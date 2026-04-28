#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "file_commands.h"
#include "logging.h"

static const size_t ONE = 1;
static const size_t INIT_NUM = 5;
static const size_t DEC = 10;

command_map_t supported_commands[] = {
    { "/sender_connected"        ,       init_sender     	},
    { "/destroy_transfer"	 ,	 destroy_channel	},
    { "/receive_connected"       ,       init_recipient  	},
    { "/recipient_accepted"	 ,	 send_agreement  	},
    { "/recipient_not_accepted"	 ,	 get_refusal		}
};
size_t commands_count = sizeof(supported_commands) / sizeof(command_map_t);
static const long int MIN_COMMAND_SIZE = 17;

file_transfer_t** transfer_array = NULL;
size_t transfer_array_cap = 0;

error init_transfers(){

    if( transfer_array == NULL ){
        transfer_array = (file_transfer_t**)calloc( INIT_NUM, sizeof(file_transfer_t*) );
        if( transfer_array == NULL ){
            log_panic( "calloc return null ptr" );
            return MEMORY_ALLOC_ERR;
        }
        transfer_array_cap = INIT_NUM;
    }

    log_info( "init transfer array" );
    return CORRECT;
}

error add_transfer( unsigned long transfer_id, const char* file_name ){
    assert( file_name );

    init_transfers();

    ssize_t free_index = find_free_transfer();
    free_index = realloc_transfers( free_index );
    if( free_index == -1 ){
        log_error( "error adding transfer" );
        return ADD_TRANSFER_ERR;
    }

    init_one_transfer( transfer_array + free_index, transfer_id, file_name );
    return CORRECT;
}

ssize_t find_free_transfer(){

    size_t transfer_index = 0;
    for(; transfer_index < transfer_array_cap; transfer_index++ ){
        if( transfer_array[transfer_index] == NULL ){
            log_debug( "free index in transfer array: %lu", transfer_index );
            return transfer_index;
        }
    }

    log_warning( "there are no elements in transfer array" );
    return -1;
}

ssize_t realloc_transfers( ssize_t free_index ){

    if( free_index != -1 ){
        return free_index;
    }

    size_t old_capacity = transfer_array_cap;

    transfer_array_cap *= 2;
    file_transfer_t** check_realloc = (file_transfer_t**)realloc( transfer_array, sizeof(file_transfer_t*) * transfer_array_cap );
    if( check_realloc == NULL ){
        log_error( "realloc returned null ptr" );
        return -1;
    }

    size_t free_elem = old_capacity;
    transfer_array = check_realloc;
    for(; free_elem < transfer_array_cap; free_elem++ ){
        transfer_array[free_elem] = NULL;
    }
    return old_capacity;
}

error init_one_transfer( file_transfer_t** transfer, unsigned long transfer_id, const char* file_name ){
    assert( transfer );
    assert( file_name );

    *transfer = (file_transfer_t*)calloc( ONE, sizeof(file_transfer_t) );
    if( *transfer == NULL ){
        log_error( "calloc return null ptr" );
        return MEMORY_ALLOC_ERR;
    }

    (*transfer)->transfer_id = transfer_id;
    (*transfer)->file_name = file_name;
    (*transfer)->recipient_handles = (uv_tcp_t**)calloc( ONE, sizeof(uv_tcp_t*) );
    (*transfer)->recipients_capacity = ONE;
    (*transfer)->open_files = (uv_tcp_t**)calloc( ONE, sizeof(uv_tcp_t*) );
    (*transfer)->open_files_cap = ONE;
    (*transfer)->recipients_count = 0;							// no one has connected yet
    (*transfer)->accepted_number = 0;							// no one to agreed download fine
    (*transfer)->not_accepted_number = 0;						// no one to disagreed download file
    return CORRECT;
}

ssize_t realloc_recipients( file_transfer_t* transfer, ssize_t free_index ){
    assert( transfer );

    if( free_index != -1 ){
        return free_index;
    }

    size_t old_numbers = transfer->recipients_capacity;

    transfer->recipients_capacity *= 2;
    uv_tcp_t** realloc_ptr = (uv_tcp_t**)realloc( transfer->recipient_handles, transfer->recipients_capacity * sizeof(uv_tcp_t*) );
    if( realloc_ptr == NULL ){
        log_panic( "realloc return null ptr" );
        return -1;
    }

    size_t free_elem = old_numbers;
    size_t new_number = transfer->recipients_capacity;
    for(; free_elem < new_number; free_elem++ ){
        realloc_ptr[free_elem] = NULL;
    }
    transfer->recipient_handles = realloc_ptr;
    return old_numbers;
}

void destroy_transfers(){

    size_t transfer_index = 0;
    for(; transfer_index < transfer_array_cap; transfer_index++ ){
        if( transfer_array[transfer_index] ){
            free( transfer_array[transfer_index] );
        }
    }
    free( transfer_array );
}

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
    if( client->file_capacity > 0 ){
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

    client->file_capacity = client->file_capacity >= predicted_size
                       ? client->file_capacity
                       : predicted_size;
    buf->base = client->file_buf + client->file_len;
    buf->len = client->file_capacity - client->file_len - 1;
}

error realloc_file_buf( client_t* client, size_t predicted_size ){
    assert( client );

    char* helpful_buffer = NULL;
    if( client->file_capacity < predicted_size ){
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
    if( uv_shutdown( shutdown_req, handle, shutdown_channel ) < 0 ){
        log_panic( "shutdown return negative value" );
        free( shutdown_req );
    }
}

void parse_message( client_t* client, ssize_t nread, void (*on_cmd)( client_t* client, const char* string ) ){
    assert( client );

    char* buf_start = client->file_buf;
    log_debug( "buf_start: %s", buf_start );
    char* newline_char = NULL;

    client->file_len += (size_t)nread;
    log_info( "string len: %lu", client->file_len );

    if( buf_start[0] != '/' ){
	send_file_part( client, buf_start  );
	buf_start += nread;
    }

    while( ( newline_char = strchr( buf_start, '\n' ) )  && buf_start < client->file_buf + client->file_len ){
        log_debug( "%c\n", buf_start[0] );                                                                               // '\n' --> '\0'
        *newline_char = '\0';
	if(  newline_char - buf_start >= MIN_COMMAND_SIZE ){
            log_debug( "string from client: %s", buf_start );
            on_cmd( client, buf_start );
        }
        buf_start = newline_char + 1;
    }

    log_info( "bytes left in the buffer: %lu", client->file_len - ( buf_start - client->file_buf ) );
    if( buf_start < client->file_buf + client->file_len ){
        memmove( client->file_buf, buf_start, client->file_len - ( buf_start - client->file_buf ) );
    }
    client->file_len -= buf_start - client->file_buf;
    log_info( "string line after: %lu", client->file_len );
}

void parse_instruction( client_t* client, const char* string ){
    assert( client );
    assert( string );

    log_debug( "string in parse_instruction: %s", string );
    if( string[0] == '\0' ){
        return ;
    }

    size_t command_index = 0;
    error command_error = CORRECT;
    for( ; command_index < commands_count; command_index++ ){
        const char* current_command = supported_commands[command_index].command_name;
        if( strncmp( string, current_command, strlen( current_command ) ) == 0 ){
            command_error = supported_commands[command_index].cmd( client, string );
            error_check( command_error,(void)0 );
            return ;
        }
    }
    send_file_data( &client->file_handle, "Unknown command: %s\n", string );
}

error send_file_part( client_t* client, const char* string ){
    assert( client );
    assert( string );

    file_transfer_t* current_transfer = find_transfer( &client->file_handle );

    uv_tcp_t** open_files_begining = current_transfer->open_files;
    uv_tcp_t** current_open_file_ptr = open_files_begining;
    uv_tcp_t* current_open_file = NULL;
    size_t open_files_cap = current_transfer->open_files_cap;

    log_info( "current files capacity = %lu", open_files_cap );
    for(; current_open_file_ptr < open_files_begining + open_files_cap; current_open_file_ptr++ ){
	current_open_file = *current_open_file_ptr;
        if( current_open_file ){
            send_file_data( current_open_file, "%s", string );
	    *current_open_file_ptr = NULL;
        }
    }

    return CORRECT;
}

error init_recipient( client_t* client, const char* string ){
    assert( client );
    assert( string );

    const char* whitespace = strchr( string, ' ' );
    unsigned long transfer_id = strtoul( whitespace + 1, NULL, DEC );

    file_transfer_t* current_transfer = NULL;
    add_recipient( &client->file_handle, transfer_id, &current_transfer );

    send_file_data( &client->file_handle, "/file_name %s\n", current_transfer->file_name );
    return CORRECT;
}

error add_recipient( uv_tcp_t* recipient_handle, unsigned long transfer_id, file_transfer_t** file_transfer ){
    assert( recipient_handle );
    assert( file_transfer );

    file_transfer_t* current_transfer = find_id( transfer_id );
    if( !current_transfer ){
        log_error( "such a transfer does not exist" );
        return FIND_TRANSFER_ERR;
    }

    ssize_t free_index = find_free_elem( current_transfer );
    free_index = realloc_recipients( current_transfer, free_index );
    if( free_index == -1 ){
        log_error( "can not realloc recipients" );
        return MEMORY_REALLOC_ERR;
    }

    current_transfer->recipient_handles[free_index] = recipient_handle;
    ++( current_transfer->recipients_count );
    *file_transfer = current_transfer;
    return CORRECT;
}

error init_sender( client_t* client, const char* string ){
    assert( client );
    assert( string );

    const char* whitespace = strchr( string, ' ' );
    unsigned long transfer_id = strtoul( whitespace + 1, NULL, DEC );

    add_sender( &client->file_handle, transfer_id );
    return CORRECT;
}

error add_sender( uv_tcp_t* sender_handle, unsigned long transfer_id ){
    assert( sender_handle );

    file_transfer_t* current_transfer = find_id( transfer_id );
    if( !current_transfer ){
        log_error( "such a transfer does not exist" );
        return FIND_TRANSFER_ERR;
    }

    current_transfer->sender_handle = sender_handle;
    return CORRECT;
}

error send_agreement( client_t* client, const char* string  ){
    assert( client  );
    assert( string  );

    file_transfer_t* current_transfer = find_transfer( &client->file_handle );
    if( !current_transfer ){
	log_error( "such a transfer does not exist" );
	return FIND_TRANSFER_ERR;
    }
    
    error state = add_open_file( client, current_transfer );
    if( state != CORRECT ){
	log_error( "error adding client with open file" );
	return state;
    }
    
    send_file_data( current_transfer->sender_handle, "/recipient_accepted\n", string  );
    ++( current_transfer->accepted_number );
    check_download_responses( current_transfer );
    log_debug( "client accepted the request to send data" );
    return CORRECT;
}

error add_open_file( client_t* client, file_transfer_t* file_transfer ){
    assert( client );
    assert( file_transfer );

    ssize_t free_index = find_free_place( file_transfer );
    free_index = check_enough_memory( file_transfer, free_index );
    if( free_index == -1 ){
	log_error( "error add open file" );
	return ADD_OPEN_FILE_ERR;
    }

    file_transfer->open_files[free_index] = &client->file_handle;
    log_info( "added a client with an open file to the position %zd", free_index );
    return CORRECT;
}

ssize_t find_free_place( file_transfer_t* file_transfer ){
    assert( file_transfer );

    uv_tcp_t** open_files = file_transfer->open_files;
    size_t current_file_ind = 0;
    size_t open_file_cap = file_transfer->open_files_cap;

    for(; current_file_ind < open_file_cap; current_file_ind++ ){
	if( open_files[current_file_ind] == NULL ){
	    return current_file_ind;
	}
    }
    
    return -1;
}

ssize_t check_enough_memory( file_transfer_t* file_transfer, ssize_t free_index ){
    assert( file_transfer );
    
    if( free_index > -1 ){
	return free_index;
    }

    size_t old_cap = file_transfer->open_files_cap;
    file_transfer->open_files_cap *= 2;
    size_t new_cap = file_transfer->open_files_cap;
    uv_tcp_t** realloc_ptr= (uv_tcp_t**)realloc( file_transfer->open_files, new_cap * sizeof(uv_tcp_t*) );
    if( !realloc_ptr ){
	log_panic( "realloc return NULL PTR" );
	return -1;
    }
    file_transfer->open_files = realloc_ptr;

    size_t file_index = old_cap;
    uv_tcp_t** open_files = file_transfer->open_files;
    for(; file_index < new_cap; file_index++ ){
	open_files[file_index] = NULL;
    }

    return old_cap;
}

error get_refusal( client_t* client, const char* string ){
    assert( client );
    assert( string );
    
    file_transfer_t* current_transfer = find_transfer( &client->file_handle );
    if( !current_transfer ){
	log_error( "such a transfer does not eist" );
	return FIND_TRANSFER_ERR;
    }
    
    ++( current_transfer->not_accepted_number );
    check_download_responses( current_transfer );
    log_debug( "cient not accepted the request to send data" );
    return CORRECT;
}

error check_download_responses( file_transfer_t* current_transfer ){
    assert( current_transfer );
    
    size_t accepted_count = current_transfer->accepted_number;
    size_t not_accepted_count = current_transfer->not_accepted_number;
    size_t recipients_count = current_transfer->recipients_count;
    log_debug( "accept = %lu, not accepted = %lu, recipients = %lu", accepted_count, not_accepted_count, recipients_count );
    if( accepted_count + not_accepted_count == recipients_count ){
	log_debug( "start shipping" );
	send_file_data( current_transfer->sender_handle, "/shipping_info %lu %lu\n", accepted_count, recipients_count );
	log_debug( "after send shipping");
	return CORRECT;
    }

    return NOT_ENOUGH_ANSWERS;
}

void shutdown_channel( uv_shutdown_t* shutdown_req, int status ){
    assert( shutdown_req );

    if( !uv_is_closing( (uv_handle_t*)shutdown_req->handle ) ){
        uv_close( (uv_handle_t*)shutdown_req->handle, finish_cb );                                       //closing a socket
    }
    free( shutdown_req );
}

error destroy_channel( client_t* client, const char* instruction ){
    assert( client );
    assert( instruction );

    file_transfer_t* file_transfer = find_transfer( &client->file_handle );
    destroy_transfer( &file_transfer );

    return CORRECT;
}

void destroy_transfer( file_transfer_t** file_transfer_ptr ){
    
    file_transfer_t* file_transfer = *file_transfer_ptr;
    if( file_transfer == NULL ){
	log_warning( "in destroy transfer: file transfer NULL PTR" );
	return ;
    }
    
    close_sockets( file_transfer );

    file_transfer->recipients_count = 0;
    file_transfer->not_accepted_number = 0;
    file_transfer->accepted_number = 0;
    file_transfer->file_name = 0;
    file_transfer->transfer_id = 0;
    file_transfer->recipients_capacity = 0;
    free( file_transfer->recipient_handles );
    file_transfer->open_files_cap = 0;
    free( file_transfer->open_files );
    *file_transfer_ptr = NULL;
}

void close_sockets( file_transfer_t* file_transfer ){
    assert( file_transfer );

    uv_tcp_t** recipients_begining = file_transfer->recipient_handles;
    uv_tcp_t** current_recipient = recipients_begining;
    size_t recipeints_cap = file_transfer->recipients_capacity;
    for(; current_recipient < recipients_begining + recipeints_cap; current_recipient++ ){
	if( *current_recipient && !uv_is_closing( (uv_handle_t*)(*current_recipient ) ) ){
	    uv_close( (uv_handle_t*)(*current_recipient ), finish_cb );
	}
    }

    uv_tcp_t* sender_handle = file_transfer->sender_handle;
    if( sender_handle && !uv_is_closing( (uv_handle_t* )sender_handle ) ){
	uv_close( (uv_handle_t* )sender_handle, finish_cb );
    }
}

void delete_client( client_t* client ){
    assert( client );

    uv_tcp_t* file_handle = &client->file_handle;
    file_transfer_t* current_transfer = find_transfer( file_handle );
    if( !current_transfer ){
        free( client );
        log_error( "such a transfer does not exist" );
        return ;
    }
    if( file_handle == current_transfer->sender_handle ){
        current_transfer->sender_handle = NULL;
        log_info( "sender was deleted" );
        free( client );
        return ;
    }

    uv_tcp_t** recipients_begining = current_transfer->recipient_handles;
    uv_tcp_t** current_recipient = recipients_begining;
    size_t recipients_capacity = current_transfer->recipients_capacity;

    for(; current_recipient < recipients_begining + recipients_capacity; current_recipient++ ){
        if( *current_recipient == file_handle ){
            *current_recipient = NULL;
	    --( current_transfer->recipients_count );
            log_info( "recipient was deleted" );
            free( client );
            return ;
        }
    }
}

file_transfer_t* find_transfer( uv_tcp_t* client_handle ){
    assert( client_handle );

    file_transfer_t** transfer_begining = transfer_array;
    file_transfer_t** current_transfer_ptr = transfer_begining;
    file_transfer_t* current_transfer = NULL;
    file_transfer_t* returned_value = NULL;

    for(; current_transfer_ptr < transfer_begining + transfer_array_cap; current_transfer_ptr++ ){
        current_transfer = *current_transfer_ptr;
        if( current_transfer == NULL ){
            continue;
        }
        if(  client_handle == current_transfer->sender_handle ){
	    log_debug( "sender is founded" );
            return current_transfer;
        }
        if( returned_value = find_recipient( current_transfer, client_handle) ){
	    log_debug( "recipient is founded" );
            return returned_value;
        }
    }
    return NULL;
}

file_transfer_t* find_recipient( file_transfer_t* current_transfer, uv_tcp_t* client_handle ){
    assert( current_transfer );
    assert( client_handle );

    if( client_handle == current_transfer->sender_handle ){
        return current_transfer;
    }

    uv_tcp_t** recipients_begining = current_transfer->recipient_handles;
    uv_tcp_t** current_recipient = recipients_begining;
    size_t recipients_capacity = current_transfer->recipients_capacity;

    for(; current_recipient < recipients_begining + recipients_capacity; current_recipient++ ){
        if( *current_recipient == client_handle ){
            return current_transfer;
        }
    }
    return NULL;
}

file_transfer_t* find_id( unsigned long transfer_id ){

    file_transfer_t** transfer_begining = transfer_array;
    file_transfer_t** current_transfer = transfer_begining;

    for(; current_transfer < transfer_begining + transfer_array_cap; current_transfer++ ){
        if( *current_transfer && (*current_transfer)->transfer_id == transfer_id ){
            return *current_transfer;
        }
    }
    return NULL;
}

ssize_t find_free_elem( file_transfer_t* current_transfer ){
    assert( current_transfer );

    uv_tcp_t** recipients = current_transfer->recipient_handles;
    size_t recipients_count = current_transfer->recipients_capacity;
    size_t recip_index = 0;

    for(; recip_index < recipients_count; recip_index++ ){
        if( recipients[recip_index] == NULL ){
            return recip_index;
        }
    }
    return -1;
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
    log_debug( "succesfully send = '%s'", buffer.base );
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

    delete_client( client );
}
