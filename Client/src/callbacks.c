#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <uv.h>

#include "callbacks.h"
#include "logger.h"

static const size_t ONE = 1;
const char* STOP_COMMAND = "/stop";
size_t STOP_COMMAND_SIZE = sizeof("/stop") - 1;
uv_tty_t tty_stdout = {};

void init_console_fd( uv_loop_t* loop ){
    assert( loop );

    uv_tty_init( loop, &tty_stdout, 1, 0 );
}

// req - request
void connect_cb(uv_connect_t* req, int status ){
    assert( req );

    if( status < 0 ){
        log_fatal( "connect callback get negative status" );
        return ;
    }

    client_t* client = (client_t*)calloc( ONE, sizeof(client_t) );
    if( client == NULL ){
        log_fatal( "calloc return null ptr\n" );
        return ;
    }
    client_init( client );
    client->handle = (uv_tcp_t*)req->handle;
    client->handle->data = client;                                                                  // save client info in free field of the struct
    int read_server = uv_read_start( (uv_stream_t*)client->handle, alloc_srv_cb, read_srv_cb );     // req->handle - file descriptor wrapper
    tty_stdout.data = client;
    int read_console = uv_read_start( (uv_stream_t*)&tty_stdout, alloc_con_cb, read_con_cb );
    log_info( "read start" );
    if( read_server < 0 ){
        log_warning( "uv_read return negative value" );
        free(client);
    }
}

void client_init( client_t* client ){
    assert( client );

    client->is_stopped = false;
    // init server buf
    client->srv_buf = NULL;
    client->srv_buf_len = 0;
    client->srv_buf_cap = 0;
    // init console buf
    client->con_buf = NULL;
    client->con_buf_len = 0;
    client->con_buf_cap = 0;
}

void alloc_srv_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    if( client->srv_buf_cap > 0 ){
        if( client->srv_buf_cap < suggested_size + client->srv_buf_len ){
            client->srv_buf = (char*)realloc( client->srv_buf, (suggested_size + client->srv_buf_len) * sizeof(char) );
        }
    }
    else{
        client->srv_buf = (char*)calloc( suggested_size, sizeof(char) );
    }

    client->srv_buf_cap = client->srv_buf_cap >= client->srv_buf_len + suggested_size
                          ? client->srv_buf_cap
                          : client->srv_buf_len + suggested_size;
    buf->base = client->srv_buf +client->srv_buf_len;               // ptr of free place
    buf->len = suggested_size;                                      // maximum bytes for reading
}

void alloc_con_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    if( client->con_buf_cap > 0 ){
        if( client->con_buf_cap < suggested_size + client->con_buf_len ){
            client->con_buf = (char*)realloc( client->con_buf, (suggested_size + client->con_buf_len) * sizeof(char) );
        }
    }
    else{
        client->con_buf = (char*)calloc( suggested_size, sizeof(char) );
    }

    client->con_buf_cap = client->con_buf_cap >= client->con_buf_len + suggested_size
                          ? client->con_buf_cap
                          : client->con_buf_len + suggested_size;
    buf->base = client->con_buf +client->con_buf_len;               // ptr of free place
    buf->len = suggested_size;                                      // maximum bytes for reading
}

void read_srv_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;                             // restoring the client's structure
    if( nread < 0 ){
        log_warning( "server closed connection" );
        uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
        uv_shutdown( shutdown_req, handle, shutdown_cb );
        return;
    }

    client->srv_buf_len += (size_t)nread;
    uv_write_t* req = (uv_write_t*)calloc( ONE, sizeof(uv_write_t) );
    req->data = client;
    uv_buf_t message = uv_buf_init( buf->base, (size_t)nread );             // we indicate how many bytes need to be written
    uv_write( req, (uv_stream_t*)&tty_stdout, &message, ONE, write_cb );
}

void read_con_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    if( nread < 0 || strncmp( buf->base, STOP_COMMAND, STOP_COMMAND_SIZE ) == 0 ){
        log_warning( "user doesn't enter data from keyboard or finish program" );
        uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
        uv_shutdown( shutdown_req, handle, shutdown_cb );
        return;
    }

    client->con_buf_len += (size_t)nread;
    uv_write_t* req = (uv_write_t*)calloc( ONE, sizeof(uv_write_t) );
    req->data = client;
    uv_buf_t message = uv_buf_init( buf->base, (size_t)nread );
    uv_write( req, (uv_stream_t*)client->handle, &message, ONE, write_cb );
}

void shutdown_cb( uv_shutdown_t* req, int status ){
    assert( req );

    if( status < 0 ){
        log_error( "shutdown callback get negative status" );
    }
    if( !uv_is_closing( (uv_handle_t*)req->handle ) ){
        uv_close( (uv_handle_t*)req->handle, close_cb );
    }
    free( req );
}

void close_cb( uv_handle_t* handle ){
    assert( handle );

    client_t* client = (client_t*)handle->data;                  // restoring the client's struct
    if( client->srv_buf ){
        free( client->srv_buf );
    }
    if( client->con_buf ){
        free( client->con_buf );
    }
    free( client );
    uv_stop( handle->loop );
}

void write_cb( uv_write_t* req, int status ){
    assert( req );

    if( status < 0 ){
        log_error( "can not write message for server" );
        return ;
    }

    client_t* client = (client_t*)req->data;
    if( client->is_stopped ){
        uv_close( (uv_handle_t*)req->handle, close_cb );
    }
    free( req );
}
