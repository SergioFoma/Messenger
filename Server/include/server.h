#include <stdio.h>
#include <netinet/in.h>                                 // sockaddr_in

struct StructArrayWithFD {
    int client_fd;
    int sendersID;
    int recipientID;
};

typedef struct StructArrayWithFD arrayWithFD;

struct sockaddr_in initServerStruct();

int makeConnectWithClient( int server_fd, struct sockaddr_in* client_addr );

int getID( char* bufferWithID );

void initializationClient( arrayWithFD* currentClient );

void communicationWithClients( arrayWithFD* arrayWithUsersID, size_t currentClientIndex );

int getRecipientFd( arrayWithFD* arrayWithUsersID, size_t currentClientIndex );

void swapClient( int* currentClientIndex, int* nextClientIndex );

