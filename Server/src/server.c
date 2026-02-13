#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/socket.h>                                 // lib for socket
#include <sys/socket.h>                                 // lib for consts for socket
#include <netinet/in.h>                                 // sockaddr_in
#include <unistd.h>                                     // close
#include <arpa/inet.h>                                  // htons

#include "server.h"
#include "paint.h"
#include "myStringFunction.h"

const size_t port = 27007;                              // number of port, for console
const int backLog = 2;                                  // connection queue
const size_t sizeOfBuffer = 1024;                       // size of buffer for read from descriptor
const size_t maxIndexInBuffer = sizeOfBuffer - 1;

typedef struct sockaddr_in socketStruct;
typedef struct sockaddr universalNetworkStruct;

int main(){

    int server_fd = socket( AF_INET, SOCK_STREAM, 0 );     // AF_INET - address IPv4, SOCK_STREAM - reliable communication, 0 - protocol
    if( server_fd == -1 ){
        colorPrintf( NOMODE, RED, "Error of get socket:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return 0;
    }

    socketStruct server_addr = initServerStruct();                            // struct describe socket for protocol

    if( bind( server_fd, (universalNetworkStruct*)&server_addr, sizeof(server_addr) ) < 0 ){
        colorPrintf( NOMODE, RED, "Binding error:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return 0;
    }

    // Start of listening
    if( listen( server_fd, backLog ) < 0 ){
        colorPrintf( NOMODE, RED, "Listen error:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        return 0;
    }

    // Work with clients
    // Array with users INFO
    arrayWithFD* arrayWithUsersID = (arrayWithFD*)calloc( backLog, sizeof(arrayWithFD) );
    size_t clientIndex = 0;

    for( ; clientIndex < backLog; clientIndex++ ){

        socketStruct client_addr = {};
        colorPrintf( NOMODE, YELLOW, "Connection...\n" );
        int client_fd = makeConnectWithClient( server_fd, &client_addr );

        arrayWithUsersID[ clientIndex ].client_fd = client_fd;
        initializationClient( arrayWithUsersID + clientIndex );                         // ptr on arrayWithUsersInfo[ clientIndex ]
    }

    for( clientIndex = 0; clientIndex < ( backLog / 2 ); clientIndex++ ){
        communicationWithClients( arrayWithUsersID, clientIndex );
        close( arrayWithUsersID[ clientIndex ].client_fd );
    }

    colorPrintf( NOMODE, GREEN, "Server secussful was started on port %lu\n", port );
    close( server_fd );
    free( arrayWithUsersID );
    return 0;
}

 struct sockaddr_in initServerStruct(){

    socketStruct server_addr = {};
    server_addr.sin_family = AF_INET;             // console
    server_addr.sin_port = htons(port);           // number of port
    server_addr.sin_addr.s_addr = INADDR_ANY;     // binding the socket to all network interfaces

    return server_addr;
}

int makeConnectWithClient( int server_fd, struct sockaddr_in* client_addr ){
    assert( client_addr );

    socklen_t clientSize = sizeof( client_addr );
    // Connection
    int client_fd = accept( server_fd, (universalNetworkStruct*)client_addr, &clientSize );
    if( client_fd < 0 ){
        colorPrintf( NOMODE, RED, "Accept error:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
    }
    colorPrintf( NOMODE, GREEN, "Connection client: %s\n", inet_ntoa( client_addr->sin_addr ) );

    return client_fd;
}

void initializationClient( arrayWithFD* currentClient ){
    assert( currentClient );

    char* bufferForID = (char*)calloc( sizeOfBuffer, sizeof( char ) );

    ssize_t statusOfRead = read( currentClient->client_fd, bufferForID, maxIndexInBuffer );
    if( statusOfRead == -1 ){
        colorPrintf( NOMODE, RED, "Error of read:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
    }

    int sendersID = getID( bufferForID );
    colorPrintf( NOMODE, BLUE, "senders ID: %d\n", sendersID );

    memset( bufferForID, 0, sizeOfBuffer );

    statusOfRead = read( currentClient->client_fd, bufferForID, maxIndexInBuffer );
    if( statusOfRead == -1 ){
        colorPrintf( NOMODE, RED, "Error of read:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
    }

    int recipientID = getID( bufferForID );
    colorPrintf( NOMODE, BLUE, "recipient ID: %d\n", recipientID );

    currentClient->sendersID = sendersID;
    currentClient->recipientID = recipientID;
    free( bufferForID );
}

int getID( char* bufferWithID ){
    assert( bufferWithID );

    size_t bufferIndex = 0, bufferSize = sizeof( bufferWithID ) - 1;
    int number = 0;
    while( bufferIndex < bufferSize && '0' <= bufferWithID[ bufferIndex ] && bufferWithID[ bufferIndex ] <= '9' ){
        number = number * 10 + ( bufferWithID[ bufferIndex ] - '0' );
        ++bufferIndex;
    }

    return number;
}

void communicationWithClients( arrayWithFD* arrayWithUsersID, size_t currentClientIndex ){
    assert( arrayWithUsersID );

    char* buffer = (char*)calloc( sizeOfBuffer, sizeof(char) );
    buffer[ maxIndexInBuffer ] = '\0';

    int senders_fd = arrayWithUsersID[currentClientIndex].client_fd;
    int recipient_fd = getRecipientFd( arrayWithUsersID, currentClientIndex );

    while( true ){
        fd_set readfds;
        FD_ZERO( &readfds );

        FD_SET( senders_fd, &readfds );
        FD_SET( recipient_fd, &readfds );

        long unsigned int biggerSocket = senders_fd > recipient_fd ? senders_fd : recipient_fd;

        int count = select( biggerSocket + 1, &readfds, NULL, NULL, NULL );

        if( FD_ISSET( recipient_fd, &readfds ) ){
            swapClient( &senders_fd, &recipient_fd );
        }

        ssize_t statusOfRead = read( senders_fd, buffer, maxIndexInBuffer );
        if( statusOfRead == -1 ){
            colorPrintf( NOMODE, RED, "Error of read:%s, %s, %d\n", __func__, __FILE__, __LINE__ );
        }
        colorPrintf( NOMODE, PURPLE, "Message from client: %s\n", buffer );

        write( recipient_fd, buffer, sizeOfBuffer );
        if( strcmp( "STOP", buffer ) == 0 ){
            colorPrintf( NOMODE, YELLOW, "Client left the chat\n" );
            break;
        }

        memset( buffer, 0, maxIndexInBuffer );
        swapClient( &senders_fd, &recipient_fd );
    }

    free( buffer );
}

int getRecipientFd( arrayWithFD* arrayWithUsersID, size_t currentClientIndex ){
    assert( arrayWithUsersID );

    size_t clientIndex = 0;
    for( ; clientIndex < backLog; clientIndex++ ){
        if( arrayWithUsersID[ currentClientIndex ].recipientID == arrayWithUsersID[ clientIndex ].sendersID ){
            return arrayWithUsersID[ clientIndex ].client_fd;
        }
    }
    return -1;
}

void swapClient( int* senders_fd, int* recipnet_fd ){
    assert( senders_fd );
    assert( recipnet_fd );

    int save_fd = *senders_fd;
    *senders_fd = *recipnet_fd;
    *recipnet_fd = save_fd;
}
