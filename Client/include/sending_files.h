#ifndef SENDING_FILES_H
#define SENDING_FILES_H

#include <uv.h>

#include "network_functions.h"
#include "user_interface.h"

typedef struct file_command_s {
    char* command_name;
    size_t (*func)( client_t* client, char* command_line );
} file_command_t;

void open_new_connection( main_struct_t* main_struct, main_connection_t* main_connection );

void alloc_сb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

void joined_cb( uv_connect_t* req, int status );

void send_file_information( client_t* client );

client_err_t realloc_file_buf( client_t* client, size_t predicted_size );

void read_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf );

void parse_file_data( client_t* client, ssize_t nread, size_t (*on_cmd)( client_t* client, char* instruction )  );

size_t parse_command( client_t* client, char* instruction );

size_t get_transfer( client_t* client, char* command );

size_t get_srv_answer( client_t* client, char* command );

void send_chunk( client_t* client );

void open_cb( uv_fs_t* open_req );

void fs_read_cb( uv_fs_t* read_req );

void fs_write_cb( uv_write_t* write_req, int status );

client_err_t get_file_size( client_t* client );

void transmission_end( client_t* client );

void close_file_ch( client_t* client );

size_t download_file( client_t* client, char* command );

chunk_data_t read_chunk( client_t* client, char* command );

void file_open_cb( uv_fs_t* open_req );

void receiver_write_cb( uv_fs_t* write_req );

void receiver_close_cb( uv_fs_t* close_cb );

size_t finish_downloading( client_t* client, char* command );

void send_file_data( uv_tcp_t* handle, const char* format, ... );

void record_cb( uv_write_t* req, int status );

void disconnect_cb( uv_shutdown_t* req, int status );

void fs_close( uv_fs_t* close_req );

void closure_cb( uv_handle_t* handle );

#endif
