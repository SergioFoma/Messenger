#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>

#include <uv.h>

#include "logging.h"
#include "chat_commands.h"
#include "file_commands.h"
#include "database.h"

const size_t PORT = 27010;
const size_t FTP = 27011;                                                                   // for file sharing
const int BACKLOG = 128;

error listen_chat_channel( char* ip, uv_loop_t* loop, uv_tcp_t* chat_server, struct sockaddr_in* server_addr );
error listen_file_channel( char* ip, uv_loop_t* loop, uv_tcp_t* file_server, struct sockaddr_in* server_addr );

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

    uv_loop_t* loop = uv_default_loop();                                                    // init main cycle
    uv_tcp_t chat_server = {};
    struct sockaddr_in chat_server_addr = {};

    uv_tcp_t file_server = {};
    struct sockaddr_in file_server_addr = {};

    listen_chat_channel( ip, loop, &chat_server, &chat_server_addr );
    listen_file_channel( ip, loop, &file_server, &file_server_addr );

    log_info( "loop starting..." );
    make_dir();
    error init_error = init_rooms();
    error_check( init_error, 0 );
    uv_run( loop, UV_RUN_DEFAULT );
    destroy_rooms();
    destroy_transfers();
    return 0;
}

error listen_chat_channel( char* ip, uv_loop_t* loop, uv_tcp_t* chat_server, struct sockaddr_in* server_addr ){
    assert( ip );
    assert( loop );

    uv_tcp_init( loop, chat_server );
    uv_ip4_addr( ip, PORT, server_addr );
    int bind_status = uv_tcp_bind( chat_server, (struct sockaddr*)server_addr, 0 );            // binding the socket to the ip address and port
    if( bind_status < 0 ){
        log_panic( "can not bind" );
        return BIND_ERR;
    }

    int listen_status = uv_listen( (uv_stream_t*)chat_server, BACKLOG, connection_cb );       // switching a TCP socket to listening mode
    if( listen_status < 0 ){
        log_panic( "can not listen" );
        return LISTEN_ERR;
    }
    return CORRECT;
}

error listen_file_channel( char* ip, uv_loop_t* loop, uv_tcp_t* file_server, struct sockaddr_in* server_addr ){
    assert( ip );
    assert( loop );

    uv_tcp_init( loop, file_server );
    uv_ip4_addr( ip, FTP, server_addr );                                                          // user new port for file sharing
    int bind_status = uv_tcp_bind( file_server, (struct sockaddr*)server_addr, 0 );
    if( bind_status < 0 ){
        log_panic( "can not bind" );
        return BIND_ERR;
    }

    int listen_status = uv_listen( (uv_stream_t*)file_server, BACKLOG, connect_file_channel );     // switching a TCP socket to listening mode
    if( listen_status < 0 ){
        log_panic( "can not listen" );
        return LISTEN_ERR;
    }
    return CORRECT;
}

