#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#include <uv.h>

#include "logging.h"
#include "commands.h"
#include "database.h"

const size_t PORT = 27010;
const int BACKLOG = 128;

int main( int argc, char** argv ){
    delete_dir();                                                                          // delete the previous history folder

    char* ip = NULL;
    if( argc > 1 ){
        ip = argv[1];                                                                       // getting ip from command line
    }
    else{
        log_panic( "connection IP address is not specified: %p", ip );
        return 0;
    }

    uv_tcp_t server = {};                                                                   // struct with descriptor, socket and ...( handle )
    uv_loop_t* loop = uv_default_loop();                                                    // init main cycle
    struct sockaddr_in server_addr = {};                                                    // port and IP

    uv_tcp_init( loop, &server );
    uv_ip4_addr( ip, PORT, &server_addr );
    int bind_status = uv_tcp_bind( &server, (struct sockaddr*)(&server_addr), 0 );           // binding the socket to the ip address and port
    if( bind_status < 0 ){
        log_panic( "can not bind" );
        return 0;
    }

    int listen_status = uv_listen( (uv_stream_t*)(&server), BACKLOG, connection_cb );       // switching a TCP socket to listening mode
    if( listen_status < 0 ){
        log_panic( "can not listen" );
        return 0;
    }

    log_info( "loop starting..." );
    make_dir();
    error init_error = init_rooms();
    error_check( init_error, 0 );
    uv_run( loop, UV_RUN_DEFAULT );
    destroy_rooms();
    return 0;
}
