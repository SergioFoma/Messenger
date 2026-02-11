#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>             // lib for socket
#include <sys/socket.h>             // lib for consts for socket
#include <netinet/in.h>             // sockaddr_in
#include <unistd.h>                 // close
#include <arpa/inet.h>              // htons

#include "paint.h"
#include "myStringFunction.h"

const size_t port = 27005;
const size_t sizeOfBuffer = 1024;                       // size of buffer for read from descriptor
const size_t maxIndexInBuffer = sizeOfBuffer - 1;
const size_t standardShipping = 0;

typedef struct sockaddr_in socketStruct;
typedef struct sockaddr universalNetworkStruct;

int main(int argc, char** argv){

    char* ipAddress = NULL;
    if( argc > 1 ){
        ipAddress = argv[1];                      // 192.168.1.104
    }
    else{
        colorPrintf( NOMODE, RED, "You don't write a ip address\n" );
        return 0;
    }

    int client_fd = socket( AF_INET, SOCK_STREAM, 0 );
    if( client_fd == -1 ){
        colorPrintf( NOMODE, RED, "Client get error of create socket:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return 0;
    }

    socketStruct server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton( AF_INET, ipAddress, &server_addr.sin_addr );

    socklen_t serverSize = sizeof( server_addr );

    int statusOfConnect = connect( client_fd, (universalNetworkStruct*)&server_addr, serverSize );
    if( statusOfConnect < 0 ){
        colorPrintf( NOMODE, RED, "Error of connect client with server:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        close( client_fd );
        return 0;
    }

    char* buffer = (char*)calloc( sizeOfBuffer, sizeof( char ) );
    while( true ){
        colorPrintf( NOMODE, YELLOW, "Write message: " );

        size_t countOfBytes = sizeOfBuffer;
        ssize_t messageLen = getlineWrapper( &buffer, &countOfBytes, stdin );
        send( client_fd, buffer, messageLen, standardShipping);

        if( strcmp( "STOP", buffer ) == 0 ){
            colorPrintf( NOMODE, YELLOW, "Client finish the chat\n" );
            break;
        }

        memset( buffer, 0, sizeOfBuffer );
        ssize_t statusOfReading = recv( client_fd, buffer, sizeOfBuffer, 0 );
        if( strcmp( "STOP", buffer ) == 0 ){
            colorPrintf( NOMODE, YELLOW, "Server finish the chat\n" );
            break;
        }
        if( statusOfReading > 0 ){
            colorPrintf( NOMODE, PURPLE, "Message from server: %s\n", buffer );
        }
        memset( buffer, 0, sizeOfBuffer );
    }

    close( client_fd );
    free( buffer );
    return 0;
}
