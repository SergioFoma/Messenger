#include "myStringFunction.h"

#include <malloc.h>
#include <assert.h>
#include <ctype.h>

ssize_t getlineWrapper( char** line, size_t* n, FILE* stream ) {

    ssize_t sizeOfLine = getline( line, n, stream );
    if( sizeOfLine == -1 ){
        return -1;
    }

    if( (*line)[ sizeOfLine - 1 ] == '\n' ){
        (*line)[ sizeOfLine - 1 ] = '\0';
    }

    return sizeOfLine;
}

int cleanLine( char* lineForClean ){
    if( lineForClean == NULL ){
        return -1;
    }

    size_t cleanLineIndex = 0;

    while( lineForClean[ cleanLineIndex ] != '\0' ){
        lineForClean[ cleanLineIndex ] = '\0';
        ++cleanLineIndex;
    }

    lineForClean[ cleanLineIndex ] = '\0';

    return 0;
}

void cleanBuffer(){
    int symbol = '\0';
    while( ( symbol = getchar() ) != '\n' );
}
