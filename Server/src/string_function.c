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

int clear_line( char* line_for_clear ){
    if( line_for_clear == NULL ){
        return -1;
    }

    size_t line_index = 0;

    while( line_for_clear[ line_index ] != '\0' ){
        line_for_clear[ line_index ] = '\0';
        ++line_index;
    }

    line_for_clear[ line_index ] = '\0';
    return 0;
}

void clear_buffer(){
    int symbol = '\0';
    while( ( symbol = getchar() ) != '\n' );
}
