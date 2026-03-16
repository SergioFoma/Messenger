#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdbool.h>

typedef enum code_error_e {
    CORRECT                         = 0,
    MEMORY_ALLOC_ERR                = 1,
    NULL_PTR                        = 2,
    SEARCH_ERROR                    = 3
} error;

#define DEFAULT         "\033["
#define BOLD            "\033[1;"
#define FADED           "\033[2;"
#define ITALICS         "\033[3;"
#define UNDERLINED      "\033[4;"
#define BLINKING        "\033[5;"
#define CROSSED_OUT     "\033[9;"

#define RED             "31m"
#define GREEN           "32m"
#define YELLOW          "33m"
#define BLUE            "34m"
#define PURPLE          "35m"
#define RESET           "\033[0m"

extern FILE* file_logger;

#define COLORING( style, color ) style color

#define log_fatal( message, ... )                                                                                       \
    fprintf( file_logger, "[FATAL] File: %s , function: %s , line %d, message: " message "\n",                          \
                          __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ );                                            \
    fflush( file_logger );

#define log_error( message, ... )                                                                                       \
    fprintf( file_logger, "[ERROR] File: %s , function: %s , line %d, message: " message "\n",                          \
                          __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ );                                            \
    fflush( file_logger );

#define log_info( message, ... )                                                                                        \
    fprintf( file_logger, "[INFO] File: %s , function: %s , line %d, message: " message "\n",                           \
                          __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ );                                            \
    fflush( file_logger );

#define log_warning( message, ... )                                                                                     \
    fprintf( file_logger, "[WARNING] File: %s , function: %s , line %d, message: " message "\n",                        \
                          __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ );                                            \
    fflush( file_logger );

#ifdef _DEBUG
#define log_debug( message, ... )                                                                                       \
    fprintf( file_logger, "[DEBUG] File: %s , function: %s , line %d, message: " message "\n",                          \
                          __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ );                                            \
    fflush( file_logger );

#else
#define log_debug( message, ... )
#define error_check( error ) do{}while( false )
#endif

void open_log_file( const char* file_path );

void close_log_file();

#endif
