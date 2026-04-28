#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include <uv.h>

#include "logging.h"

typedef struct client_info_s {
    uv_tcp_t handle;                                            // handle for work with chat and messages
    uv_tcp_t file_handle;                                       // handle for file transfer
    char* buf;
    char* file_buf;                                             // buffer for work wint file data
    char* last_seen_message;
    size_t capacity;
    size_t len;
    size_t file_len;
    size_t file_capacity;
    bool is_stopped;
    bool in_room;
    bool is_bot;						// for chat bot
} client_t;

typedef struct command_map_s{
    const char* command_name;
    error (*cmd)(client_t* client, const char* string );
} command_map_t;


void client_init( client_t* client );

#endif
