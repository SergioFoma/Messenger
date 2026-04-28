#ifndef CHAT_COMMANDS_H
#define CHAT_COMMANDS_H

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "logging.h"
#include "server_functions.h"

typedef struct description_room_s {
    char* room_name;
    client_t** clients_array;
    FILE* message_history;                  // file ptr for read history
    size_t capacity;                        // capacity of array
    size_t user_counter;                    // number of active users
    int write_fd;                           // file descriptor for writing messages
} room_t;

error init_rooms();

error init_single_room( room_t** room );

ssize_t rooms_allocation( ssize_t room_index );

ssize_t clients_allocation( room_t* room, ssize_t client_index );

void destroy_rooms();

void removing_client( client_t* client );

void destroy_single_room( room_t* room );

void connection_cb( uv_stream_t* server, int status );

void shutdown_cb( uv_shutdown_t* shutdown_req, int status );

void sending_instruction( client_t* client );

void chat_alloc_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

error memory_realloc( client_t* client, size_t suggested_size );

void read_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf );

void parse_buffer( client_t* client, ssize_t nread, void (*on_cmd)( client_t* client, const char* string ) );

void parse_command( client_t* client, const char* string );

error send_message( client_t* client, const char* string );

error cmd_join( client_t* client, const char* string );

bool room_search( client_t* client,  const char* room_name );

ssize_t get_free_room();

ssize_t get_free_client( room_t* room );

error cmd_list( client_t* client, const char* string );

error cmd_leave( client_t* client, const char* string );

error cmd_stop( client_t* client, const char* string );

error cmd_today( client_t* client, const char* string );

error cmd_yesterday( client_t* client, const char* string );

error cmd_week( client_t* client, const char* string );

error cmd_history( client_t* client, const char* string );

error cmd_file_name( client_t* client, const char* string );

error cmd_bot_reg( client_t* client, const char* string );

error cmd_bot_file( client_t* client, const char* string );

room_t* get_room( client_t* client );

void client_send( client_t* client_stream, const char* format, ... );

void write_cb( uv_write_t* write_req, int status );

void close_cb( uv_handle_t* handle );

#endif
