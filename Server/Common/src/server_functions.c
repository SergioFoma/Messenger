#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>


#include "server_functions.h"

static const size_t ONE = 1;

void client_init( client_t* client ){
    assert( client );

    client->last_seen_message = NULL;
    // chat buffer
    client->buf = NULL;
    client->len = 0;
    client->capacity = 0;
    // buffer for file data
    client->file_buf = NULL;
    client->file_len = 0;
    client->file_capacity = 0;
    //
    client->is_stopped = false;
    client->in_room = false;
    client->is_bot = false;
}
