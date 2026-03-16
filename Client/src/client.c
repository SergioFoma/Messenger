#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <uv.h>

#include "logger.h"
#include "callbacks.h"

const size_t PORT = 27010;
static const size_t ONE = 1;

int main( int argc, char** argv ){

    if( argc > 1 ){
        open_log_file( argv[1] );                                                                   // open file for logging (logger.h)
    }

    char* ip = NULL;
    if( argc > 2 ){
        ip = argv[2];
    }
    else{
        log_fatal( "connection error. IP address was not founded" );
        return 0;
    }

    uv_loop_t* loop = uv_default_loop();                                                            // init main cycle
    init_console_fd( loop );                                                                        // init console stream
    uv_tcp_t client_socket = {};
    uv_tcp_init( loop, &client_socket );                                                            // init descriptor, but not make socket

    uv_connect_t* connect = (uv_connect_t*)calloc( ONE, sizeof(uv_connect_t) );

    struct sockaddr_in client_addr = {};                                                            // describe socket: port, ip ...
    uv_ip4_addr( ip, PORT, &client_addr );                                                          // converting string to binary struct
    uv_tcp_connect( connect, &client_socket, (const struct sockaddr*)&client_addr, connect_cb );

    log_info( "loop start ... " );
    uv_run( loop, UV_RUN_DEFAULT );

    free( connect );
    return 0;
}

