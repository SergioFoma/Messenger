#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <assert.h>
#include <uv.h>

#include "logger.h"
#include "callbacks.h"
#include "user_interface.h"

const size_t PORT = 27010;
static const size_t ONE = 1;

bool is_valid_ip( const char* ip );

int main( int argc, char** argv ){

    if( argc < 2 ){
        log_fatal( "the file for saving logs is not specified" );
        return 0;
    }

    open_log_file( argv[1] );                                                                       // open file for logging (logger.h)
    user_info_t* user_data = start_registration();                                                  // user registration
    

    uv_loop_t* loop = uv_default_loop();                                                            // init main cycle
    uv_tcp_t client_socket = {};
    uv_tcp_init( loop, &client_socket );                                                            // init descriptor, but not make socket
    uv_connect_t* connect = (uv_connect_t*)calloc( ONE, sizeof(uv_connect_t) );
    if( is_valid_ip( user_data->ip ) == false ){
	free( connect );
	close_messenger( user_data );
	return 0;
    }

    struct sockaddr_in client_addr = {};                                                            // describe socket: port, ip ...
    if( uv_ip4_addr( user_data->ip, PORT, &client_addr ) != 0 ){                                    // converting string to binary struct
        log_fatal( "error converting IP address to struct" );
        close_messenger( user_data );
        free( connect );
        return 0;
    }
    if( uv_tcp_connect( connect, &client_socket, (const struct sockaddr*)&client_addr, connect_cb ) < 0 ){
        log_fatal( "server connection error" );
        free( connect );
        return 0;
    }

    log_info( "loop start ... " );
    uv_run( loop, UV_RUN_DEFAULT );

    free( connect );
    return 0;
}

bool is_valid_ip( const char* ip ){
    assert( ip );
    
    struct in_addr addr = {};
    bool is_valid = inet_pton( AF_INET, ip, &addr );
    if( !is_valid ){
	log_fatal( "incorrect ip address" );
    }

    return is_valid;
}
