#include <stdio.h>

#include "logging.h"

void server_verify( error error_code ){

    switch( error_code ){
        case MEMORY_ALLOC_ERR:
            log_error( "memory allocation error" );
            break;
        case NULL_PTR:
            log_error( "server get a null pointer" );
            break;
        case SEARCH_ERROR:
            log_error( "search failed" );
            break;
        default:
            break;
    }
}
