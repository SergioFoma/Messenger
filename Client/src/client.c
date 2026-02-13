#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/socket.h>             // lib for socket
#include <sys/socket.h>             // lib for consts for socket
#include <netinet/in.h>             // sockaddr_in
#include <unistd.h>                 // close
#include <arpa/inet.h>              // htons

#include "paint.h"
#include "myStringFunction.h"
#include "client.h"

const size_t port = 27007;
const size_t sizeOfBuffer = 1024;                       // size of buffer for read from descriptor
const size_t maxIndexInBuffer = sizeOfBuffer - 1;
const size_t standardShipping = 0;
const size_t stdinFileno = 0;

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

    socketStruct server_addr = initServerStruct( ipAddress );
    socklen_t serverSize = sizeof( server_addr );

    int statusOfConnect = connect( client_fd, (universalNetworkStruct*)&server_addr, serverSize );
    if( statusOfConnect < 0 ){
        colorPrintf( NOMODE, RED, "Error of connect client with server:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        close( client_fd );
        return 0;
    }

    communicationWithServer( client_fd );

    close( client_fd );
    return 0;
}

struct sockaddr_in initServerStruct( char* ipAddress ){
    socketStruct server_addr = {};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton( AF_INET, ipAddress, &server_addr.sin_addr );

    return server_addr;
}

void communicationWithServer( int client_fd ){

    char* buffer = (char*)calloc( sizeOfBuffer, sizeof( char ) );
    buffer[ maxIndexInBuffer ] = '\0';

    registrationClient( client_fd, buffer );

    statusOfChat status = CONTINUE_CHAT;
    while( true ){

        fd_set readfds;                         // descriptor for reading

        FD_ZERO( &readfds);
        FD_SET( client_fd, &readfds );          // waiting data from the server
        FD_SET( stdinFileno, &readfds );        // waiting data from keyboard

        long unsigned int biggerSocket = client_fd > stdinFileno ? client_fd: stdinFileno;

        int count = select( biggerSocket + 1, &readfds, NULL, NULL, NULL );

        if( FD_ISSET( client_fd, &readfds ) ){
            status = readMessageFromServer( client_fd, buffer );
        }
        if( status == STOP_CHAT ){
            break;
        }

        if( FD_ISSET( stdinFileno, &readfds ) ){
            status = sendMessageForServer( client_fd, buffer );
        }
        if( status == STOP_CHAT ){
            break;
        }
    }

    free( buffer );
}

statusOfChat readMessageFromServer( int client_fd, char* buffer ){
    assert( buffer );

    ssize_t statusOfReading = recv( client_fd, buffer, sizeOfBuffer, 0 );
    if( statusOfReading <= 0 ){
        colorPrintf( NOMODE, RED, "Recv error:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return STOP_CHAT;
    }
    if( strcmp( "STOP", buffer ) == 0 ){
        colorPrintf( NOMODE, YELLOW, "Interlocutor finish the chat\n" );
        return STOP_CHAT;
    }
    if( statusOfReading > 0 ){
        colorPrintf( NOMODE, PURPLE, "Message from server: %s\n", buffer );
    }
    memset( buffer, 0, sizeOfBuffer );
    return CONTINUE_CHAT;
}

statusOfChat sendMessageForServer( int client_fd, char* buffer ){
    assert( buffer );

    fgets( buffer, sizeOfBuffer, stdin );
    char* ptrOnNewLine = strchr( buffer, '\n' );
    *ptrOnNewLine = '\0';

    size_t bufferLenWithNullSymbol = strlen( buffer ) + 1;
    send( client_fd, buffer, bufferLenWithNullSymbol, standardShipping);
    if( strcmp( "STOP", buffer ) == 0 ){
        colorPrintf( NOMODE, YELLOW, "Interlocutor finish the chat\n" );
        return STOP_CHAT;
    }
    memset( buffer, 0, sizeOfBuffer );
    return CONTINUE_CHAT;
}

void registrationClient( int client_fd, char* buffer ){
    assert( buffer );

    colorPrintf( NOMODE, BLUE, "Enter your ID: " );

    size_t countOfBytes = sizeOfBuffer;
    ssize_t messageLen = getlineWrapper( &buffer, &countOfBytes, stdin );
    if( messageLen == -1 ){
        colorPrintf( NOMODE, RED, "Error of getline:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return ;
    }

    send( client_fd, buffer, (size_t)messageLen, standardShipping);

    memset( buffer, 0, sizeOfBuffer );

    colorPrintf( NOMODE, BLUE, "Enter your interlocutor's ID: " );


    countOfBytes = sizeOfBuffer;
    messageLen = getlineWrapper( &buffer, &countOfBytes, stdin );
    if( messageLen == -1 ){
        colorPrintf( NOMODE, RED, "Error of getline:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return;
    }

    send( client_fd, buffer, (size_t)messageLen, standardShipping);

    memset( buffer, 0, sizeOfBuffer );
}
