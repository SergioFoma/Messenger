#ifndef SENDING_FILES_H
#define SENDING_FILES_H

#include <uv.h>

#include "network_functions.h"
#include "user_interface.h"

void open_new_connection( main_struct_t* main_struct, main_connection_t* main_connection, unsigned long transfer_id, client_type_t client_type );

void alloc_сb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

void joined_cb( uv_connect_t* req, int status );

client_err_t realloc_file_buf( client_t* client, size_t predicted_size );

void read_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf );

void parse_file_data( client_t* client, ssize_t nread, void (*on_cmd)( client_t* client, char* instruction )  );

void parse_command( client_t* client, char* instruction );

void save_file_data( client_t* client, ssize_t nread );

void clear_file_buffer( client_t* client );

void file_write_cb( uv_fs_t* req );

void check_file_writting( client_t* client, uv_fs_t* req );

void parse_file_name( client_t* client, char* instruction );

void start_sending( client_t* client, char* instruction  );

void create_file( client_t* client );

void fs_cb( uv_fs_t* req );

void open_file( client_t* client );

void complete_sending( client_t* client, char* instruction );

void sending_file( client_t* client );

void send_cb( uv_fs_t* req );

void check_data_sending( client_t* client, uv_fs_t* req );

void request_not_accepted( client_t* client );

void destroy_file_ch( client_t* cleint );

void send_file_data( client_t* client, const char* format, ... );

void disconnect_cb( uv_shutdown_t* req, int status );

void closure_cb( uv_handle_t* handle );

void record_cb( uv_write_t* req, int status );

#endif
