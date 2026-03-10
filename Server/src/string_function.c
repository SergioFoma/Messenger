#include "string_function.h"

#include <malloc.h>
#include <assert.h>
#include <ctype.h>

ssize_t getline_wrapper( char** line, size_t* n, FILE* stream ) {

    ssize_t line_size = getline( line, n, stream );
    if( line_size == -1 ){
        return -1;
    }

    if( (*line)[ line_size - 1 ] == '\n' ){
        (*line)[ line_size - 1 ] = '\0';
    }
    return line_size;
}
