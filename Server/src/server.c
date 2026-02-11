#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>                                 // lib for socket
#include <sys/socket.h>                                 // lib for consts for socket
#include <netinet/in.h>                                 // sockaddr_in
#include <unistd.h>                                     // close
#include <arpa/inet.h>                                  // htons


#include "paint.h"
#include "myStringFunction.h"

const size_t port = 27005;                              // number of port, for console
const int backLog = 5;                                  // connection queue
const size_t sizeOfBuffer = 1024;                       // size of buffer for read from descriptor
const size_t maxIndexInBuffer = sizeOfBuffer - 1;

typedef struct sockaddr_in socketStruct;
typedef struct sockaddr universalNetworkStruct;

int main(){

    socketStruct server_addr;                              // struct describe socket for protocol
    int server_fd = socket( AF_INET, SOCK_STREAM, 0 );     // AF_INET -IPv4

    // AF_INET - address IPv4, SOCK_STREAM - reliable communication, 0 - protocol
    if( server_fd == -1 ){
        colorPrintf( NOMODE, RED, "Error of get socket:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return 0;
    }

    server_addr.sin_family = AF_INET;             // console
    server_addr.sin_port = htons(port);           // number of port
    server_addr.sin_addr.s_addr = INADDR_ANY;     // binding the socket to all network interfaces


    if( bind( server_fd, (universalNetworkStruct*)&server_addr, sizeof(server_addr) ) < 0 ){
        colorPrintf( NOMODE, RED, "Binding error:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return 0;
    }

    // Start of listening
    if( listen( server_fd, backLog ) < 0 ){
        colorPrintf( NOMODE, RED, "Listen error:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return 0;
    }

    //Work with client
    int client_fd;
    socketStruct client_addr;
    socklen_t clientSize = sizeof( client_addr );

    colorPrintf( NOMODE, YELLOW, "Connection...\n" );

    // Connection
    client_fd = accept( server_fd, (universalNetworkStruct*)&client_addr, &clientSize );
    if( client_fd < 0 ){
        colorPrintf( NOMODE, RED, "Accept error:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
    }
    colorPrintf( NOMODE, GREEN, "Connection client: %s\n", inet_ntoa( client_addr.sin_addr ) );

    char* buffer = (char*)calloc( sizeOfBuffer, sizeof(char) );
    buffer[ maxIndexInBuffer ] = '\0';

    while( true ){
        ssize_t statusOfRead = read( client_fd, buffer, maxIndexInBuffer );
        if( statusOfRead == -1 ){
            colorPrintf( NOMODE, RED, "Error of read:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        }
        if( strcmp( "STOP", buffer ) == 0 ){
            colorPrintf( NOMODE, YELLOW, "Client left the chat\n" );
            break;
        }
        colorPrintf( NOMODE, PURPLE, "Message from client: %s\n", buffer );
        colorPrintf( NOMODE, YELLOW, "Write message: " );

        memset( buffer, 0, sizeOfBuffer );

        size_t countOfBytes = sizeOfBuffer;
        size_t messageLen = getlineWrapper( &buffer, &countOfBytes, stdin );
        write( client_fd, buffer, messageLen );
        if( strcmp( "STOP", buffer ) == 0 ){
            colorPrintf( NOMODE, YELLOW, "Server left the chat\n" );
            break;
        }

        memset( buffer, 0, maxIndexInBuffer );
    }

    colorPrintf( NOMODE, GREEN, "Server secussful was started on port %lu\n", port );
    close( server_fd );
    close( client_fd );
    free( buffer );
    return 0;
}
