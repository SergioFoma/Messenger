#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "malloc.h"

#include <sys/socket.h>                                 // lib for socket
#include <sys/socket.h>                                 // lib for consts for socket
#include <netinet/in.h>                                 // sockaddr_in
#include <unistd.h>                                     // close
#include <arpa/inet.h>                                  // htons

#include "server.h"
#include "paint.h"
#include "myStringFunction.h"
#include "serverCommand.h"

const size_t port = 27008;                              // number of port, for console
const int backLog = 2;                                  // connection queue
const size_t countOfRoomsForInitialization = 10;
const size_t countOfUsersInRoomForInitialization = 2;
const int noTime = -1;                                 // for poll
const int userNotConnected = -1;
const size_t bytesForMessage = 100;
const size_t bytesForRead = bytesForMessage - 2;       // we read 2 bytes less than the message size, so that we can put '\0' later

int main(){

    roomsInfo conditionOfTheRooms = {};
    initializationRoomsArrray( &conditionOfTheRooms );

    if( conditionOfTheRooms.roomsError != CORRECT ){
        colorPrintf( NOMODE, RED, "Error of initialization rooms: %s, %s, %d\n", __FILE__, __func__ ,__LINE__ );
        return 0;
    }

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
    error serverStatus = startWorkWithClients( server_fd, &conditionOfTheRooms );

    if( serverStatus == CORRECT ){
        colorPrintf( NOMODE, GREEN, "Server secussful was started on port %lu\n", port );
    }

    destroyRoomsArray( &conditionOfTheRooms );
    return 0;
}

 struct sockaddr_in initServerStruct(){

    socketStruct server_addr = {};
    server_addr.sin_family = AF_INET;             // console
    server_addr.sin_port = htons(port);           // number of port
    server_addr.sin_addr.s_addr = INADDR_ANY;     // binding the socket to all network interfaces

    return server_addr;
}

void initializationRoomsArrray( roomsInfo* conditionOfTheRooms ){
    assert( conditionOfTheRooms );

    conditionOfTheRooms->countOfRooms = countOfRoomsForInitialization;
    conditionOfTheRooms->currentFreeRoom = 0;
    conditionOfTheRooms->rooms = (room*)calloc( countOfRoomsForInitialization, sizeof(room) );
    conditionOfTheRooms->roomsError = CORRECT;

    if( ! conditionOfTheRooms->rooms ){
        conditionOfTheRooms->roomsError = ROOMS_INITIALIZATION_ERROR;
        return ;
    }

    size_t roomIndex = 0;
    for( ; roomIndex < countOfRoomsForInitialization; roomIndex++ ){
        room* currentRoom = &((conditionOfTheRooms->rooms)[roomIndex]);
        currentRoom->roomName = NULL;
        currentRoom->numberOfParticipantsInRoom = 0;
        currentRoom->sizeOfUsersArray = countOfUsersInRoomForInitialization;
        currentRoom->usersArray = (int*)calloc( countOfUsersInRoomForInitialization, sizeof(int) );
        initializationUsersArray( currentRoom );
    }
}

void initializationUsersArray( room* currentRoom ){
    assert( currentRoom );

    size_t userIndex = 0;

    for( ; userIndex < currentRoom->sizeOfUsersArray; userIndex++ ){
        currentRoom->usersArray[userIndex] = userNotConnected;
    }
}

error startWorkWithClients( int server_fd, roomsInfo* conditionOfTheRooms ){
    assert( conditionOfTheRooms );

    pollFuncInfo pollInformation = {};
    initializationPollStruct( &pollInformation, server_fd );
    socketStruct client_addr = {};

    int returnFromPoll = 0;
    while( true ){

        returnFromPoll = poll( pollInformation.participants, pollInformation.freeIndexNow, noTime  );
        if( returnFromPoll < 0 ){
            return POLL_RET_BAD_VALUE;
        }
        else{
            colorPrintf( NOMODE, GREEN, "Connection has occurred\n" );
        }

        if( initializationNewParticipant( &pollInformation, server_fd, &client_addr  ) )        continue;
        if( workWithOldParticipant( &pollInformation, conditionOfTheRooms ) )                   continue;
    }
    return CORRECT;
}

void initializationPollStruct( pollFuncInfo* pollInformation, int server_fd ){
    assert( pollInformation );

    pollInformation->participants = (struct pollfd*)calloc( countOfUsersInRoomForInitialization, sizeof(struct pollfd) );
    pollInformation->capacity = countOfUsersInRoomForInitialization;
    pollInformation->participants[0].fd = server_fd;
    pollInformation->participants[0].events = POLLIN;
    pollInformation->freeIndexNow = 1;
}

bool initializationNewParticipant( pollFuncInfo* pollInformation, int server_fd, socketStruct* client_addr ){
    assert( pollInformation );
    assert( client_addr );

    if( pollInformation->participants[0].revents != POLLIN ){
        return false;
    }

    socklen_t clientSize = sizeof(client_addr);
    int client_fd = accept( server_fd, (universalNetworkStruct*)client_addr, &clientSize );
    if( client_fd < 0 ){
        colorPrintf( NOMODE, RED, "Error of accept client:%s, %s, %d\n", __FILE__, __func__, __LINE__ );
        return false;
    }

    checkFreeMemoryAndReallocPollfd( pollInformation );
    pollInformation->participants[pollInformation->freeIndexNow].fd = client_fd;
    pollInformation->participants[pollInformation->freeIndexNow].events = POLLIN;
    ++(pollInformation->freeIndexNow);

    colorPrintf( NOMODE, GREEN, "New participant was initialized with fd: %d\n", client_fd );
    return true;
}

void checkFreeMemoryAndReallocPollfd( pollFuncInfo* pollInformation ){
    assert( pollInformation );

    if( pollInformation->freeIndexNow == pollInformation->capacity - 1 ){
        pollInformation->capacity *= 2;
        pollInformation->participants = (struct pollfd*)realloc( pollInformation->participants, pollInformation->capacity * sizeof(struct pollfd) );
    }
}

bool workWithOldParticipant( pollFuncInfo* pollInformation, roomsInfo* conditionOfTheRooms ){
    assert( pollInformation );

    functionTypes whatFunction = NO_FUNCTION;
    size_t userIndex = 1;
    char* message = (char*)calloc( bytesForMessage, sizeof(char) );

    for( ; userIndex < pollInformation->freeIndexNow; userIndex++ ){
        if( pollInformation->participants[userIndex].revents == POLLIN ){
            read( pollInformation->participants[userIndex].fd, message, bytesForRead );
            message[ bytesForRead + 1 ] = '\0';
            whatFunction = commandExecution( pollInformation->participants[userIndex].fd, message, conditionOfTheRooms );
            checkOnStopChat( &(pollInformation->participants[userIndex]), whatFunction );
        }
    }
    free( message );
    return true;
}

void checkOnStopChat( struct pollfd* participant, functionTypes whatFunction ){
    assert( participant );

    if( whatFunction == STOP_CHAT ){
        participant->fd = userNotConnected;
        participant->events = 0;
    }

}

functionTypes commandExecution( int client_fd, char* message, roomsInfo* conditionOfTheRooms ){
    assert( message );
    assert( conditionOfTheRooms );

    if( message[0] != '/' ){
        writeSentenceInChat( client_fd, message, conditionOfTheRooms );
        return WRITE_MESSAGE;
    }

    size_t commandIndex = 0;
    commandAndTheyFunction commandInfo = {};
    for( ; commandIndex < sizeOfArrayWithServerCommand; commandIndex++ ){
        commandInfo = arrayWithServerCommand[commandIndex];
        if( strncmp( message, commandInfo.nameOfCommand, strlen( commandInfo.nameOfCommand ) ) == 0 ){
            return commandInfo.function( client_fd, message, conditionOfTheRooms );
        }
    }
    return NO_FUNCTION;
}

functionTypes noFunction( int client_fd, char* message, roomsInfo* conditionOfTheRooms ){
    assert( message );
    assert( conditionOfTheRooms );
    return NO_FUNCTION;
}

functionTypes joinToRoom( int client_fd, char* message, roomsInfo* conditionOfTheRooms ){
    assert( message );
    assert( conditionOfTheRooms );

    char* whitespace = strchr( message, ' ' );
    char* nameOfRoom = (char*)calloc( bytesForMessage, sizeof(char) );
    strncpy(nameOfRoom, whitespace + 1, bytesForMessage - 1);
    colorPrintf( NOMODE, GREEN, "Client %d joined to room %s\n", client_fd, nameOfRoom );

    if( searchRoomForClient( client_fd, nameOfRoom, conditionOfTheRooms ) ){
        free( nameOfRoom );
        return JOIN_CHAT;
    }

    checkFreeMemoryAndReallocRooms( conditionOfTheRooms );
    room* currentRoom = &(conditionOfTheRooms->rooms[conditionOfTheRooms->currentFreeRoom]);
    checkFreeMemoryAndReallocUsersFd( currentRoom );

    currentRoom->roomName = nameOfRoom;
    currentRoom->usersArray[currentRoom->numberOfParticipantsInRoom] = client_fd;
    ++(currentRoom->numberOfParticipantsInRoom);
    ++(conditionOfTheRooms->currentFreeRoom);
    return JOIN_CHAT;
}

functionTypes stopChat( int client_fd, char* message, roomsInfo* conditionOfTheRooms ){
    leaveFromRoom( client_fd, message, conditionOfTheRooms );
    return STOP_CHAT;
}

functionTypes leaveFromRoom( int client_fd, char* message, roomsInfo* conditionOfTheRooms ){
    assert( message );
    assert( conditionOfTheRooms );

    size_t roomIndex = 0;
    size_t userIndex = 0;

    for( ; roomIndex < conditionOfTheRooms->currentFreeRoom; roomIndex++ ){
        room* currentRoom = &( conditionOfTheRooms->rooms[roomIndex] );
        for( ; userIndex < currentRoom->numberOfParticipantsInRoom; userIndex++ ){

            if( client_fd == currentRoom->usersArray[userIndex] ){
                currentRoom->usersArray[userIndex] = userNotConnected;
                return LEAVE_CHAT;
            }
        }
    }
    return NO_FUNCTION;
}

bool searchRoomForClient( int client_fd, char* nameOfRoom, roomsInfo* conditionOfTheRooms ){
    assert( nameOfRoom );

    size_t indexRoom = 0;

    for( ; indexRoom < conditionOfTheRooms->currentFreeRoom; indexRoom++ ){
        room* currentRoom = &(conditionOfTheRooms->rooms[indexRoom]);
        if( strcmp( currentRoom->roomName, nameOfRoom ) == 0 ){
            checkFreeMemoryAndReallocUsersFd( currentRoom );
            currentRoom->usersArray[currentRoom->numberOfParticipantsInRoom] = client_fd;
            ++(currentRoom->numberOfParticipantsInRoom);
            return true;
        }
    }
    return false;
}

void checkFreeMemoryAndReallocRooms( roomsInfo* conditionOfTheRooms ){
    assert( conditionOfTheRooms );

    if( conditionOfTheRooms->currentFreeRoom == conditionOfTheRooms->countOfRooms - 1 ){
        conditionOfTheRooms->countOfRooms *= 2;
        conditionOfTheRooms->rooms = (room*)realloc( conditionOfTheRooms->rooms, conditionOfTheRooms->countOfRooms * sizeof(room) );
    }
}

void checkFreeMemoryAndReallocUsersFd( room* currentRoom ){
    assert( currentRoom );

    if( currentRoom->numberOfParticipantsInRoom == currentRoom->sizeOfUsersArray - 1 ){
        currentRoom->sizeOfUsersArray *= 2;
        currentRoom->usersArray = (int*)realloc( currentRoom->usersArray, currentRoom->sizeOfUsersArray * sizeof(int) );
    }
}


functionTypes getRoomList( int client_fd, char* message, roomsInfo* conditionOfTheRooms ){
    assert( message );
    assert( conditionOfTheRooms );

    size_t roomIndex = 0;
    size_t userIndex = 0;

    for( ; roomIndex < conditionOfTheRooms->currentFreeRoom; roomIndex++ ){
        room* currentRoom = &( conditionOfTheRooms->rooms[roomIndex] );
        for( ; userIndex < currentRoom->numberOfParticipantsInRoom; userIndex++ ){

            if( client_fd == currentRoom->usersArray[userIndex] ){
                dumpRoomInformation( client_fd, currentRoom );
                return SHOW_LIST;
            }
        }
    }
    return NO_FUNCTION;
}

void dumpRoomInformation( int client_fd, room* currentRoom ){
    assert( currentRoom );

    const char* roomInfo =  "\n_______________ROOOM INFO________________________\n"
                            "Number of participants in room: %lu\n"
                            "Size of room: %lu\n\n\n";
    size_t roomInfoLen = strlen(roomInfo);
    char* bufferWithRoomInfo = (char*)calloc( roomInfoLen + 1, sizeof(char) );
    bufferWithRoomInfo[ roomInfoLen - 1 ] = '\0';

    snprintf( bufferWithRoomInfo, roomInfoLen + 1, roomInfo,
              currentRoom->numberOfParticipantsInRoom, currentRoom->sizeOfUsersArray
            );
    write( client_fd, bufferWithRoomInfo, roomInfoLen + 1 );
    free( bufferWithRoomInfo );
}

void writeSentenceInChat( int client_fd, char* message, roomsInfo* conditionOfTheRooms ){
    assert( message );
    assert( conditionOfTheRooms );

    size_t clientIndex = 0;

    room* roomWhereSitClient = getRoom( client_fd, conditionOfTheRooms );
    if( roomWhereSitClient == NULL ){
        const char* warning = "You are not connected to any chat. Join the chat and send the message again\n";
        write( client_fd, warning, strlen(warning) + 1 );
        return ;
    }

    colorPrintf( NOMODE, GREEN, "Message for write: %s in room:% s\n", message, roomWhereSitClient->roomName );

    for( ; clientIndex < roomWhereSitClient->numberOfParticipantsInRoom; clientIndex++ ){
        if( client_fd == roomWhereSitClient->usersArray[clientIndex] || roomWhereSitClient->usersArray[clientIndex] == userNotConnected ){
            continue;
        }

        write( roomWhereSitClient->usersArray[clientIndex], message, bytesForMessage );
    }


}

room* getRoom( int client_fd, roomsInfo* conditionOfTheRooms ){
    assert( conditionOfTheRooms );

    size_t roomIndex = 0;
    size_t userIndex = 0;

    for( ; roomIndex < conditionOfTheRooms->currentFreeRoom; roomIndex++ ){
        room* currentRoom = &( conditionOfTheRooms->rooms[roomIndex] );
        for( ; userIndex < currentRoom->numberOfParticipantsInRoom; userIndex++ ){

            if( client_fd == currentRoom->usersArray[userIndex] ){
                return currentRoom;
            }
        }
    }
    return NULL;
}

void destroyRoomsArray( roomsInfo* conditionOfTheRooms ){
    assert( conditionOfTheRooms );

    size_t roomIndex = 0;

    for( ; roomIndex < conditionOfTheRooms->countOfRooms; roomIndex++ ){
        room* currentRoom = &(conditionOfTheRooms->rooms[roomIndex]);
        free( currentRoom->usersArray);
        free( currentRoom->roomName );
        currentRoom->numberOfParticipantsInRoom = 0;
        currentRoom->sizeOfUsersArray = 0;
    }

    conditionOfTheRooms->countOfRooms = 0;
    conditionOfTheRooms->currentFreeRoom = 0;
    conditionOfTheRooms->roomsError = ROOM_WAS_DESTROYED;
}
