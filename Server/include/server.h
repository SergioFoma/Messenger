#ifndef H_SERVER
#define H_SERVER

#include <stdio.h>
#include <netinet/in.h>                                 // sockaddr_in
#include <poll.h>

enum codeError {
    CORRECT                         = 0,
    ROOMS_INITIALIZATION_ERROR      = 1,
    POLL_RET_BAD_VALUE              = 2,
    ROOM_WAS_DESTROYED              = 3
};
typedef enum codeError error;

enum enumFunctionTypes {
    NO_FUNCTION                     = 0,
    JOIN_CHAT                       = 1,
    SHOW_LIST                       = 2,
    LEAVE_CHAT                      = 3,
    STOP_CHAT                       = 4,
    WRITE_MESSAGE                   = 5
};
typedef enum enumFunctionTypes functionTypes;

struct structRoom {
    char* roomName;
    int* usersArray;                                   // array with file descriptors
    size_t numberOfParticipantsInRoom;                 // how many people in the room now
    size_t sizeOfUsersArray;
};
typedef struct structRoom room;

struct structRoomsInfo {
    size_t countOfRooms;
    size_t currentFreeRoom;
    room* rooms;
    error roomsError;
};
typedef struct structRoomsInfo roomsInfo;

struct structPollFuncInfo {
    struct pollfd* participants;
    size_t freeIndexNow;
    size_t capacity;
};
typedef struct structPollFuncInfo pollFuncInfo;

typedef struct sockaddr_in socketStruct;
typedef struct sockaddr universalNetworkStruct;

struct sockaddr_in initServerStruct();

void initializationRoomsArrray( roomsInfo* conditionOfTheRooms );

void initializationUsersArray( room* currentRoom );

error startWorkWithClients( int server_fd, roomsInfo* conditionOfTheRooms );

void initializationPollStruct( pollFuncInfo* pollInformation, int server_fd );

bool initializationNewParticipant( pollFuncInfo* pollInformation, int server_fd, socketStruct* client_addr );

void checkFreeMemoryAndReallocPollfd( pollFuncInfo* pollInformation );

bool workWithOldParticipant( pollFuncInfo* pollInformation, roomsInfo* conditionOfTheRooms );

functionTypes commandExecution( int client_fd, char* message, roomsInfo* conditionOfTheRooms );

void checkOnStopChat( struct pollfd* participant, functionTypes whatFunction );

functionTypes noFunction( int client_fd, char* message, roomsInfo* conditionOfTheRooms );

functionTypes joinToRoom( int client_fd, char* message, roomsInfo* conditionOfTheRooms );

functionTypes stopChat( int client_fd, char* message, roomsInfo* conditionOfTheRooms );

bool searchRoomForClient( int client_fd, char* nameOfRoom, roomsInfo* conditionOfTheRooms );

functionTypes leaveFromRoom( int client_fd, char* message, roomsInfo* conditionOfTheRooms );

void checkFreeMemoryAndReallocRooms( roomsInfo* conditionOfTheRooms );

void checkFreeMemoryAndReallocUsersFd( room* currentRoom );

functionTypes getRoomList( int client_fd, char* message, roomsInfo* conditionOfTheRooms );

void dumpRoomInformation( int client_fd, room* currentRoom );

void writeSentenceInChat( int client_fd, char* message, roomsInfo* conditionOfTheRooms );

room* getRoom( int client_fd, roomsInfo* conditionOfTheRooms );

void destroyRoomsArray( roomsInfo* conditionOfTheRooms );

#endif


