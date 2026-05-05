#ifndef FILE_COMMANDS_H
#define FILE_COMMANDS_H

#include <uv.h>

#include "logging.h"
#include "server_functions.h"

typedef struct chunk_command_s {
    const char* command_name;
    size_t (*cmd)( client_t* client, char* string );
} chunk_command_t;

void connect_file_channel( uv_stream_t* server, int status );

void file_alloc_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

error realloc_file_buf( client_t* client, size_t suggested_size );

void read_file_ch( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf );

void parse_message( client_t* client, ssize_t nread, size_t (*on_cmd)( client_t* client, char* string ) );

size_t parse_instruction( client_t* client, char* string );

size_t init_sender( client_t* client, char* string );

size_t save_chunk( client_t* client, char* string );

void fs_open_cb( uv_fs_t* open_req );

void file_write_cb( uv_fs_t* write_req );

void file_close_cb( uv_fs_t* close_req );

chunk_info_t read_chunk_line( client_t* client, char* string );

size_t init_recipient( client_t* client, char* string );

char* get_file_name( client_t* client );

void send_srv_chunk( client_t* client );

void srv_open_cb( uv_fs_t* open_req );

void srv_file_size( client_t* client );

void srv_read_cb( uv_fs_t* read_req );

void check_file_end( client_t* client );

void srv_write_cb( uv_write_t* write_req, int status );

void srv_file_close( uv_fs_t* close_req );

size_t send_chunk( client_t* client, char* string );

void shutdown_channel( uv_shutdown_t* shutdown_req, int status );

void send_file_data( uv_tcp_t* file_handle, const char* format, ... );

void record_cb( uv_write_t* write_req, int status );

void finish_cb( uv_handle_t* handle );

#endif
