#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

#include "commands.h"
#include "logging.h"
#include "database.h"

const size_t ONE = 1;                                                               //for calloc
const size_t INIT_ROOMS_NUMBER = 10;                                                //for initialization array with rooms
const size_t INIT_CLIENT_NUMBER = 10;                                               //for initialization array with clients

/*The array with commands is designed so that the length of commands increases,
 *so the shortest length is the length of the first command( command_map[0] )
*/

const command_map_t command_map[] = {
    { "/week"       ,   cmd_week        },
    { "/join"       ,   cmd_join        },
    { "/list"       ,   cmd_list        },
    { "/stop"       ,   cmd_stop        },
    { "/leave"      ,   cmd_leave       },
    { "/today"      ,   cmd_today       },
    { "/yesterday"  ,   cmd_yesterday   },
    { "/history"    ,   cmd_history     }
};
const size_t command_map_capacity = sizeof(command_map) / sizeof(command_map_t);
const size_t MIN_COMMAND_SIZE = strlen(command_map[0].command_name) - 1;

// array of pointers to rooms and its size
room_t** rooms = NULL;
size_t room_capacity = 0;

error init_rooms(){

    rooms = (room_t**)calloc( INIT_ROOMS_NUMBER, sizeof(room_t*) );                         //At the beginning, all pointers to rooms are null.
    if( rooms == NULL ){
        log_panic( "error initializing rooms array" );
        return MEMORY_ALLOC_ERR;
    }
    room_capacity = INIT_ROOMS_NUMBER;
    return CORRECT;
}

error init_single_room( room_t** room ){
    assert( room );

    *room = (room_t*)calloc( ONE, sizeof(room_t) );
    (*room)->clients_array = (client_t**)calloc( INIT_CLIENT_NUMBER, sizeof(client_t*) );      //At the beginning, all pointers to clients are null.
    if( (*room)->clients_array == NULL ){
        log_panic( "error initializing clients in single room" );
        return MEMORY_ALLOC_ERR;
    }
    (*room)->capacity = INIT_CLIENT_NUMBER;
    (*room)->user_counter = 0;
    (*room)->room_name = NULL;
    (*room)->message_history = NULL;

    return CORRECT;
}

ssize_t rooms_allocation( ssize_t room_index ){

    if( room_index >= 0 ){
        return room_index;
    }

    room_index = room_capacity;                                                                            // pointer to the first free space
    room_capacity *= 2;
    rooms = (room_t**)realloc( rooms, room_capacity * sizeof(room_t*) );                                      // realloc does not init data
    if( rooms == NULL ){
        log_panic( "room reallocation error" );
        return -1;
    }

    size_t current_room = room_index;
    for( ; current_room < room_capacity; current_room++ ){
        rooms[current_room] = NULL;
    }
    return room_index;
}

ssize_t clients_allocation( room_t* room, ssize_t client_index ){
    assert( room );

    if( client_index >= 0 ){
        return client_index;
    }

    client_index = room->capacity;                                                                         // pointer to the first free space
    room->capacity *= 2;
    room->clients_array = (client_t**)realloc( room->clients_array, room->capacity* sizeof(client_t*) );
    if( room->clients_array == NULL ){
        log_panic( "clients array reallocate error" );
        return -1;
    }

    client_t** clients_beginning = room->clients_array;
    client_t** current_client = clients_beginning + client_index;
    size_t new_capacity = room->capacity;
    for( ; current_client < clients_beginning + new_capacity; current_client++ ){
        *current_client = NULL;
    }
    return client_index;
}

void destroy_rooms(){

    size_t room_index = 0;
    for( ; room_index < room_capacity; room_index++ ){
        destroy_single_room( rooms[room_index] );
        rooms[room_index] = NULL;
    }

    free( rooms );
}

void destroy_single_room( room_t* room ){
    assert( room );

    room->capacity = 0;
    room->user_counter = 0;
    room->room_name = NULL;
    free( room->clients_array );
    fclose( room->message_history );
    free( room );
}

void connection_cb( uv_stream_t* server, int status ){
    assert( server );

    client_t* client = (client_t*)calloc( ONE, sizeof(client_t) );
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
        uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
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
                         "/today - show the group's message history for the current day\n"
                         "/yesterday - show the group's message history for yesterday\n"
                         "/week - show the group's message history for the last week\n"
                         "/history - show all unread messages\n"
                         "/leave - leave the current room\n"
                         "/stop - log out of the messenger\n\n" );
}

void client_init( client_t* client ){
    assert( client );

    client->capacity = 0;
    client->buf = NULL;
    client->last_seen_message = NULL;
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
    if( client->last_seen_message ){
        free( client->last_seen_message );
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

    client_t** clients_beginning = client_room->clients_array;
    size_t clients_capacity = client_room->capacity;
    client_t** current_client = NULL;
    for( ; current_client < clients_beginning + clients_capacity; current_client++ ){
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
    uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
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
        else if(  newline_char - buf_start >= MIN_COMMAND_SIZE ){
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
        log_warning( "client_room is null ptr" );
        return NULL_PTR;
    }

    size_t client_capacity = client_room->capacity;
    client_t** client_beginning = client_room->clients_array;
    client_t** current_client = client_beginning;
    for( ; current_client < client_beginning + client_capacity; current_client++ ){
        if( !(*current_client) || client == *current_client || (*current_client)->in_room == false ){          // sending a message to others client
            continue;
        }
        client_send( *current_client, "%s\n", string );
        if( (*current_client)->last_seen_message != NULL ){
            free( (*current_client)->last_seen_message );                                                      // delete the previous message
        }
        (*current_client)->last_seen_message = strdup( string );
    }
    save_message( string, client_room->message_history);
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

    ssize_t free_room_index = get_free_room();
    free_room_index = rooms_allocation( free_room_index );
    init_single_room( rooms + free_room_index );

    ssize_t free_client_index = get_free_client( rooms[free_room_index] );
    free_client_index = clients_allocation( rooms[free_room_index], free_client_index );

    client->in_room = true;
    rooms[free_room_index]->room_name = strdup( name_begin );
    rooms[free_room_index]->clients_array[free_client_index] = client;
    ++(rooms[free_room_index]->user_counter);
    rooms[free_room_index]->message_history = create_history( rooms[free_room_index]->room_name );
    log_debug( "make new room and new client" );
    return CORRECT;
}

bool room_search( client_t* client,  const char* room_name ){
    assert( client );
    assert( room_name );

    size_t room_index = 0;
    room_t* current_room = NULL;
    for( ; room_index < room_capacity; room_index++ ){
        current_room = rooms[room_index];
        if( current_room && strcmp( room_name, current_room->room_name ) == 0 ){
            ssize_t free_client_index = get_free_client( current_room );
            free_client_index = clients_allocation( current_room, free_client_index );
            client->in_room = true;
            current_room->clients_array[free_client_index] = client;
            ++(current_room->user_counter);
            return true;
        }
    }
    return false;
}

ssize_t get_free_room(){

    size_t room_index = 0;
    for( ; room_index < room_capacity; room_index++ ){
        if( rooms[room_index] == NULL ){
            return room_index;
        }
    }
    return -1;
}

ssize_t get_free_client( room_t* room ){
    assert( room );

    size_t client_index = 0;
    size_t clients_capacity = room->capacity;
    for( ; client_index < clients_capacity; client_index++ ){
        if( room->clients_array[client_index] == NULL ){
            return client_index;
        }
    }
    return -1;
}

error cmd_list( client_t* client, const char* string ){
    assert( client );
    assert( string );

    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        client_send( client, "\n\nyou can't find out information about the room because you're not in it.\n\n" );
        log_warning( "client_room is null ptr" );
        return NULL_PTR;
    }

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
        log_warning( "client_room is null ptr" );
        return NULL_PTR;
    }

    log_debug( "string in cmd_leave: %s", string );
    size_t client_capacity = client_room->capacity;
    client_t** beginning_client = client_room->clients_array;
    client_t** current_client = beginning_client;
    for( ; current_client < beginning_client + client_capacity; current_client++ ){
        if( *current_client && *current_client == client && (*current_client)->in_room ){
            client->in_room = false;
            *current_client = NULL;
            --(client_room->user_counter);
            return CORRECT;
        }

    }
    log_warning( "client was not found in any room" );
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

    room_t** current_room = rooms;
    room_t** beginning_room = rooms;
    size_t client_capacity = 0;
    client_t** current_client = NULL;
    client_t** beginning_client = NULL;

    for( ; current_room < beginning_room + room_capacity; current_room++ ){
        if( *current_room == NULL ){
            continue;
        }
        beginning_client = (*current_room)->clients_array;
        current_client = beginning_client;
        client_capacity = (*current_room)->capacity;
        for( ; current_client < beginning_client + client_capacity; current_client++ ){
            if( *current_client && (*current_client)->in_room &&
                client == *current_client ){
                return *current_room;
            }
        }
    }
    return NULL;
}

error cmd_today( client_t* client, const char* string ){
    assert( client );
    assert( string );

    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        log_warning( "client_room is null ptr" );
        return NULL_PTR;
    }

    char* message_history = get_today_messages( client_room->message_history );
    if( message_history == NULL ){
        log_error( "error getting message history" );
        return NULL_PTR;
    }

    client_send( client, "__________Today's message history______\n%s\n\n", message_history );
    free( message_history );
    return CORRECT;
}

error cmd_yesterday( client_t* client, const char* string ){
    assert( client );
    assert( string );

    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        log_warning( "client_room is null ptr" );
        return NULL_PTR;
    }

    char* message_history = get_yesterday_messages( client_room->message_history );
    if( message_history == NULL ){
        log_error( "error getting message history" );
        return NULL_PTR;
    }

    client_send( client, "__________History of yesterday's messages______\n%s\n\n", message_history );
    free( message_history );
    return CORRECT;
};

error cmd_week( client_t* client, const char* string ){
    assert( client );
    assert( string );

    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        log_warning( "client_room is null ptr" );
        return NULL_PTR;
    }

    char* message_history = get_week_messages( client_room->message_history );
    if( message_history == NULL ){
        log_error( "error getting message history" );
        return NULL_PTR;
    }

    client_send( client, "__________Message history for the week______\n%s\n\n", message_history );
    free( message_history );
    return CORRECT;
}

error cmd_history( client_t* client, const char* string ){
    assert( client );
    assert( string );

    room_t* client_room = get_room( client );
    if( client_room == NULL ){
        log_warning( "client_room is null ptr" );
        return NULL_PTR;
    }

    char* message_history = get_unread_messages( client_room->message_history, client->last_seen_message );
    if( message_history == NULL ){
        log_error( "error getting message history" );
        return NULL_PTR;
    }

    client_send( client, "__________All unread messages______\n%s\n\n", message_history );
    free( message_history );
    return CORRECT;
}

void client_send(client_t* client, const char* format, ... ){
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
    uv_write( req, (uv_stream_t*)(&client->handle), &buffer, ONE, write_cb );                           //writing data to descriptor
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
