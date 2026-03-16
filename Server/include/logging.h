#ifndef LOGGING_H
#define LOGGING_H

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

#define COLORING( style, color ) style color

#define log_panic( message, ... ) fprintf( stderr, COLORING( BOLD, RED )                                                        \
                                           "[PANIC] File: %s , function: %s , line %d, message: " message RESET "\n",           \
                                           __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ )

#define log_error( message, ... ) fprintf( stderr, COLORING( BOLD, RED )                                                        \
                                           "[ERROR] File: %s , function: %s , line %d, message: " message RESET "\n",           \
                                           __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ )

#define log_warning( message, ... ) fprintf( stderr, COLORING( BOLD, BLUE )                                                     \
                                           "[WARNING] File: %s , function: %s , line %d, message: " message RESET "\n",         \
                                           __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ )

#define log_info( message, ... ) fprintf( stderr, COLORING( BOLD, YELLOW )                                                      \
                                           "[INFO] File: %s , function: %s , line %d, message: " message RESET "\n",            \
                                           __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ )

#ifdef _DEBUG
#define log_debug( message, ... ) fprintf( stderr, COLORING( BOLD, PURPLE )                                                     \
                                           "[DEBUG] File: %s , function: %s , line %d, message: " message RESET "\n",           \
                                           __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ )

void server_verify( error code_error );

#define error_check( server_error, return_value )             \
    do{                                                       \
        if( server_error != CORRECT ){                        \
            server_verify( server_error );                    \
            return return_value;                              \
        }                                                     \
    }while( false )

#else
#define log_debug( message, ... )
#define error_check( error ) do{}while( false )
#endif

#endif
