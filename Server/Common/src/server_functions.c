#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>


#include "server_functions.h"

void client_init( client_t* client ){
    assert( client );

    client->last_seen_message = NULL;
    // chat buffer
    client->buf = NULL;
    client->len = 0;
    client->capacity = 0;
    client->transfer_id = 0;
    // buffer for file data
    client->file_buf = NULL;
    client->file_len = 0;
    client->file_buf_cap = 0;
    client->file_capacity = 0;
    //
    client->srv_file_data = NULL;
    client->chunk_line = NULL;
    client->offset = 0;
    client->is_stopped = false;
    client->in_room = false;
    client->is_bot = false;
}

unsigned long hash( const char* string ){
    assert( string );

    unsigned long hash = 5381;
    int c;
    while( ( c = *string++ ) ){
        hash = ( (hash << 5 ) + hash ) + c;
    }
    return hash;
}

