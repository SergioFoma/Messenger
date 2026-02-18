#ifndef H_SERVER_COMMAND
#define H_SERVER_COMMAND

#include "server.h"

struct structCommandAndTheyFunction {
    const char* nameOfCommand;
    functionTypes (*function)( int client_fd, char* message, roomsInfo* conditionOfTheRooms );
};
typedef struct structCommandAndTheyFunction commandAndTheyFunction;

extern commandAndTheyFunction arrayWithServerCommand[];
extern const size_t sizeOfArrayWithServerCommand;

#endif
