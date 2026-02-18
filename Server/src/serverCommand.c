#include <stdio.h>

#include "serverCommand.h"
#include "server.h"

commandAndTheyFunction arrayWithServerCommand[] = {
    {   "/join"       ,       joinToRoom    },                                   // join to room
    {   "/list"       ,       getRoomList   },                                   // show the list of participants
    {   "/leave"      ,       leaveFromRoom },                                   // the user leaves the room
    {   "/stop"       ,       stopChat      }
};

const size_t sizeOfArrayWithServerCommand = sizeof(arrayWithServerCommand) / sizeof(commandAndTheyFunction);
