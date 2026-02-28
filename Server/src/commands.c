#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "commands.h"
#include "logging.h"

const size_t one = 1;                                                               // for calloc
const size_t max_command_size = 100;
const size_t init_rooms_number = 10;                                                // for initialization array with rooms
const size_t init_client_number = 10;                                               // for initialization array with clients

const command_map_t command_map[] = {
    { "/join"   ,   cmd_join    },
    { "/list"   ,   cmd_list    },
    { "/leave"  ,   cmd_leave   },
    { "/stop"   ,   cmd_stop    }
};
const size_t command_map_capacity = sizeof(command_map) / sizeof(command_map_t);

data_rooms_t* rooms_info = NULL;                                                    // global struct

error init_rooms(){

    rooms_info = (data_rooms_t*)calloc( one, sizeof(data_rooms_t) );
    if( rooms_info == NULL ){
        log_panic( "error initializing info about rooms" );
        return MEMORY_ALLOC_ERR;
    }
    rooms_info->rooms = (room_t*)calloc( init_rooms_number, sizeof(room_t) );
    if( rooms_info->rooms == NULL ){
        log_panic( "error initializing rooms" );
        return MEMORY_ALLOC_ERR;

    }
    rooms_info->capacity = init_rooms_number;
    rooms_info->free_room = 0;
    rooms_info->room_errors = CORRECT;

    error room_error = CORRECT;
    size_t room_index = 0;
    for( ; room_index < init_rooms_number; room_index++ ){

        if( ( room_error = init_single_room( &(rooms_info->rooms[room_index]) ) ) != CORRECT ){
            return room_error;
        }
    }
    return CORRECT;
}

error init_single_room( room_t* room ){
    assert( room );

    room->clients_array = (client_t**)calloc( init_client_number, sizeof(client_t*) );
    if( room->clients_array == NULL ){
        log_panic( "error initializing clients in single room" );
        return MEMORY_ALLOC_ERR;
    }
    room->capacity = init_client_number;
    room->free_space = 0;
    room->user_counter = 0;
    room->room_name = NULL;

    return CORRECT;
}

error rooms_allocation(){

    if( rooms_info->free_room < rooms_info->capacity - 1 ){
        return CORRECT;
    }

    rooms_info->capacity *= 2;
    rooms_info->rooms = (room_t*)realloc( rooms_info->rooms, rooms_info->capacity * sizeof(room_t) );
    if( rooms_info->rooms == NULL ){
        log_panic( "room reallocate error" );
        return MEMORY_ALLOC_ERR;
    }
    return CORRECT;
}

error clients_allocation( room_t* room ){
    assert( room );

    if( room->free_space < room->capacity - 1 ){
        return CORRECT;
    }

    room->capacity *= 2;
    room->clients_array = (client_t**)realloc( room->clients_array, room->capacity* sizeof(client_t*) );
    if( room->clients_array == NULL ){
        log_panic( "clients array reallocate error" );
        return MEMORY_ALLOC_ERR;
    }
    return CORRECT;
}

void destroy_rooms(){

    size_t room_index = 0;
    for( ; room_index < rooms_info->capacity; room_index++ ){
        destroy_single_room( &(rooms_info->rooms[room_index]) );
    }

    rooms_info->capacity = 0;
    rooms_info->free_room = 0;
    rooms_info->room_errors = CORRECT;
    free( rooms_info );
}

void destroy_single_room( room_t* room ){
    assert( room );

    room->capacity = 0;
    room->free_space = 0;
    room->user_counter = 0;
    room->room_name = NULL;
    free( room->clients_array );
    free( room );
}

void connection_cb( uv_stream_t* server, int status ){
    assert( server );

    client_t* client = (client_t*)calloc( one, sizeof(client_t) );
    if( client == NULL ){
        log_error( "can not allocate memory for client" );
        return ;
    }

    client_init( client );
    uv_tcp_init( server->loop, &client->handle );                                                        //descriptor initialization
    client->handle.data = client;
    int accept_status = uv_accept( server, (uv_stream_t*)&client->handle );                              //communication between the client socket and the server socket
    if( accept_status < 0 ){
        log_error( "can not accept" );
        uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( one, sizeof(uv_shutdown_t) );
        uv_shutdown( shutdown_req, (uv_stream_t*)&client->handle, shutdown_cb );                         //closing connection
        return ;
    }

    sending_instruction( client );
    uv_read_start( (uv_stream_t*)&client->handle, alloc_cb, read_cb );                                   //starting an asynchronous connection
}

void sending_instruction( client_t* client ){
    assert( client );

    client_send( client, "Welcome to our messenger. You can:\n"
                         "/join [room_name] - join a room with name [room_name]\n"
                         "/list - find out the name of the room you are in and the number of participants\n"
                         "/leave - leave the current room\n"
                         "/stop - log out of the messenger\n\n" );
}

void client_init( client_t* client ){
    assert( client );

    client->capacity = 0;
    client->buf = NULL;
    client->len = 0;
    client->is_stopped = false;
    client->in_room = false;
}

void shutdown_cb( uv_shutdown_t* shutdown_req, int status ){
    assert( shutdown_req );

    if( !uv_is_closing( (uv_handle_t*)shutdown_req->handle ) ){
        uv_close( (uv_handle_t*)shutdown_req->handle, close_cb );                                       //closing a socket
    }
    free( shutdown_req );
}

void close_cb( uv_handle_t* handle ){
    assert( handle );

    client_t* client = (client_t*)handle->data;
    if( client->buf ){
        free( client->buf );
    }
    removing_client( client );
}

void removing_client( client_t* client ){
    assert( client );

    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        free( client );                                                                             //if the client is not in the room, we simply end it
        return ;
    }

    size_t client_index = 0;
    client_t** current_client = NULL;
    for( ; client_index < client_room->free_space; client_index++ ){
        current_client = &(client_room->clients_array[client_index]);
        if( *current_client == client ){
            free( client );
            *current_client = NULL;                                                                 //freeing up space in the room
            return ;
        }
    }
    free( client );
}

void alloc_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    if( client->capacity > 0 ){
        if( client->capacity < suggested_size + client->len ){
            client->buf = (char*)realloc( client->buf, client->len + suggested_size );
        }
    }
    else{
        client->buf = (char*)calloc( suggested_size, sizeof(char) );
    }

    client->capacity = client->capacity >= client->len + suggested_size ? client->capacity : client->len + suggested_size;
    buf->base = client->buf + client->len;
    buf->len = suggested_size;
}

void read_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    if( nread >= 0 ){
        log_info( "read got %zd", nread );
        parse_buffer( client, nread, parse_command );
        return;
    }

    log_warning( "client closed connection" );
    uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( one, sizeof(uv_shutdown_t) );
    uv_shutdown( shutdown_req, handle, shutdown_cb );
}

void parse_buffer( client_t* client, ssize_t nread, void (*on_cmd)( client_t* client, const char* string ) ){
    assert( client );

    char* buf_start = client->buf;
    log_debug( "buf_start: %s", buf_start );
    char* newline_char = NULL;

    client->len += (size_t)nread;
    log_info( "string len: %lu", client->len );

    while( buf_start < client->buf + client->len && ( newline_char = strchr( buf_start, '\n' ) ) ){
        *newline_char = '\0';
        log_debug( "%c\n", buf_start[0] );                                                                               // '\n' --> '\0'
        if( buf_start[0] != '/' ){
            send_message( client, buf_start );
        }
        else if(  newline_char - buf_start >= get_min_size() ){
            log_debug( "string from client: %s", buf_start );
            on_cmd( client, buf_start );
        }
        buf_start = newline_char + 1;
    }

    log_info( "bytes left in the buffer: %lu", client->len - ( buf_start - client->buf ) );
    if( buf_start < client->buf + client->len ){
        memmove( client->buf, buf_start, client->len - ( buf_start - client->buf ) );
    }
    client->len -= buf_start - client->buf;
    log_info( "string line after: %lu", client->len );
}

size_t get_min_size(){

    size_t command_index = 0;
    size_t min_size = max_command_size;
    size_t current_size = 0;
    for( ; command_index < command_map_capacity; command_index++ ){
        current_size = strlen(command_map[command_index].command_name);
        if( current_size < min_size ){
            min_size = current_size;
        }
    }
    return min_size;
}

void parse_command( client_t* client, const char* string ){
    assert( client );
    assert( string );

    log_debug( "string in parse_command: %s", string );
    if( string[0] == '\0' ){
        return ;
    }

    size_t command_index = 0;
    error command_error = CORRECT;
    for( ; command_index < command_map_capacity; command_index++ ){
        const char* current_command = command_map[command_index].command_name;
        if( strncmp( string, current_command, strlen( current_command ) ) == 0 ){
            command_error = command_map[command_index].cmd( client, string );
            error_check( command_error,(void)0 );
            return ;
        }
    }
    client_send( client, "Unknown command: %s\n", string );
}

error send_message( client_t* client, const char* string ){
    assert( client );
    assert( string );

    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        client_send( client, "\n\nto send messages to other users, go to the room\n\n" );
        log_debug( "client_room is null ptr" );
        return NULL_PTR;
    }

    size_t client_index = 0;
    for( ; client_index < client_room->free_space; client_index++ ){
        client_t* current_client = client_room->clients_array[client_index];
        if( !current_client || client == current_client || current_client->in_room == false ){               // sending a message to others client
            continue;
        }
        client_send( client_room->clients_array[client_index], "%s\n", string );
    }
    return CORRECT;
}

error cmd_join( client_t* client, const char* string ){
    assert( client );
    assert( string );


    const char* name_begin = strchr( string, ' ' ) + 1;
    if( room_search( client, name_begin ) ){
        log_debug( "add client in old room" );
        return CORRECT;
    }

    rooms_allocation();
    room_t* free_room = &(rooms_info->rooms[rooms_info->free_room]);
    clients_allocation( free_room );

    free_room->room_name = strdup( name_begin );
    client->in_room = true;
    free_room->clients_array[free_room->free_space] = client;
    ++(rooms_info->free_room);
    ++(free_room->free_space);
    ++(free_room->user_counter);
    log_debug( "make new room and new client" );
    return CORRECT;
}

bool room_search( client_t* client,  const char* room_name ){
    assert( client );
    assert( room_name );

    size_t room_index = 0;
    for( ; room_index < rooms_info->free_room; room_index++ ){
        room_t* current_room = &(rooms_info->rooms[room_index]);
        if( strcmp( room_name, current_room->room_name ) == 0 ){
            clients_allocation( current_room );
            client->in_room = true;
            current_room->clients_array[current_room->free_space] = client;
            ++(current_room->free_space);
            ++(current_room->user_counter);
            return true;
        }
    }
    return false;
}

error cmd_list( client_t* client, const char* string ){
    assert( client );
    assert( string );

    log_debug( "Now we in function: list" );
    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        client_send( client, "\n\nyou can't find out information about the room because you're not in it.\n\n" );
        log_debug( "client_room is null ptr" );
        return NULL_PTR;
    }

    log_debug( "in /list: %s", string );
    client_send( client, "\n"
                         "you are in the room: %s\n"
                         "number of clients in room: %lu\n\n",
                         client_room->room_name, client_room->user_counter );
    return CORRECT;
}

error cmd_leave( client_t* client, const char* string ){
    assert( client );
    assert( string );

    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        log_debug( "client_room is null ptr" );
        return NULL_PTR;
    }

    log_debug( "string in cmd_leave: %s", string );
    client_t* current_client = NULL;
    size_t client_index = 0;
    for( ; client_index < client_room->free_space; client_index++ ){
        current_client = client_room->clients_array[client_index];
        if( current_client == client && current_client->in_room ){
            client_room->clients_array[client_index]->in_room = false;
            client_room->clients_array[client_index] = NULL;
            --(client_room->user_counter);
            return CORRECT;
        }

    }
    log_debug( "client was not found in any room" );
    return SEARCH_ERROR;
}

error cmd_stop( client_t* client, const char* string ){
    assert( client );
    assert( string );

    client->is_stopped = true;
    cmd_leave( client, string );
    client_send( client, "The chat has been completed successfully\n" );
    return CORRECT;
}

room_t* get_room( client_t* client ){
    assert( client );

    size_t room_index = 0;
    size_t client_index = 0;
    room_t* current_room = NULL;
    client_t* current_client = NULL;

    for( room_index = 0; room_index < rooms_info->free_room; room_index++ ){
        current_room = &(rooms_info->rooms[room_index]);
        for( client_index = 0; client_index < current_room->free_space; client_index++ ){
            current_client = current_room->clients_array[client_index];
            if( current_client && current_client->in_room &&
                client == current_client ){
                return current_room;
            }
        }
    }
    return NULL;
}

void client_send(client_t* client, const char* format, ... ){
    assert( client );

    uv_buf_t buffer = {};
    FILE* stream = open_memstream( &buffer.base, &buffer.len );                                         //creating a stream for recording
    va_list args = {};
    va_start( args, format );
    vfprintf( stream, format, args );
    fclose( stream );
    va_end( args );

    uv_write_t* req = (uv_write_t*)calloc( one, sizeof(uv_write_t) );
    req->data = buffer.base;
    uv_write( req, (uv_stream_t*)(&client->handle), &buffer, one, write_cb );                           //writing data to descriptor
}

void write_cb( uv_write_t* write_req, int status ){
    assert( write_req );
    if( status < 0 ){
        log_error( "can not write message" );
        return ;
    }

    free( write_req->data );
    client_t* client = (client_t*)write_req->handle;
    if( client->is_stopped ){
        uv_close( (uv_handle_t*)client, close_cb );
    }
    free( write_req );
}
