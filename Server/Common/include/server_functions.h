#ifndef SERVER_FUNCTIONS_H
#define SERVER_FUNCTIONS_H

#include <stdbool.h>
#include <uv.h>

typedef struct chunk_info_s {
    char* binary_start;
    unsigned long transfer_id;
    size_t offset;
    size_t size;
} chunk_info_t;


typedef struct client_info_s {
    uv_tcp_t handle;                                            // handle for work with chat and messages
    uv_tcp_t file_handle;                                       // handle for file transfer
    char* buf;
    char* file_buf;                                             // buffer for work wint file data
    char* last_seen_message;
    char* srv_file_data;
    char* load_path;
    char* chunk_line;
    char* file_name;
    chunk_info_t* chunk_info;
    size_t transfer_id;						// recipient's or sender's transfer number
    size_t capacity;
    size_t len;
    size_t file_len;
    size_t file_buf_cap;
    size_t file_capacity;
    size_t offset;
    int file_fd;
    bool is_stopped;
    bool in_room;
    bool is_bot;						// for chat bot
} client_t;

void client_init( client_t* client );

unsigned long hash( const char* string );

#endif
