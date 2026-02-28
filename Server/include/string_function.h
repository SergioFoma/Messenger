#ifndef H_MYSTRINGFUNCTION
#define H_MYSTRINGFUNCTION

#include <stdio.h>
#include <unistd.h>

ssize_t getline_rapper( char** line, size_t* n, FILE* stream );

int clear_line( char* lineForClean );

void clear_buffer();

#endif
