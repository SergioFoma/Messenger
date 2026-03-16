#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <stdbool.h>

#include "uv.h"

typedef struct client_info_s {
    uv_tcp_t* handle;
    char* srv_buf;                  // buffer for data from server
    size_t srv_buf_cap;
    size_t srv_buf_len;
    char* con_buf;                  // buffer for data from console
    size_t con_buf_cap;
    size_t con_buf_len;
    bool is_stopped;
} client_t;

void init_console_fd( uv_loop_t* loop );

void connect_cb( uv_connect_t* req, int status );

void client_init( client_t* client );

void alloc_srv_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

void alloc_con_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

void read_srv_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf );

void read_con_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf );

void shutdown_cb( uv_shutdown_t* req, int status );

void close_cb( uv_handle_t* handle );

void write_cb( uv_write_t* req, int status );

#endif
