#ifndef H_MYSTRINGFUNCTION
#define H_MYSTRINGFUNCTION

#include <stdio.h>
#include <unistd.h>

ssize_t getline_wrapper( char** line, size_t* n, FILE* stream );

#endif
