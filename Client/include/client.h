#ifndef H_CLIENT
#define H_CLIENT

#include <stdio.h>

#include <netinet/in.h>             // sockaddr_in

enum statusOfChat {
    STOP_CHAT           =   0,
    CONTINUE_CHAT       =   1
};

struct sockaddr_in initServerStruct( char* ipAddress );

void printInstruction();

void communicationWithServer( int client_fd );

statusOfChat readMessageFromServer( int client_fd, char* buffer );

statusOfChat sendMessageForServer( int client_fd, char* buffer );

#endif
