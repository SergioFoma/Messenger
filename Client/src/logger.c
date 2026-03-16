#include <stdio.h>

#include "logger.h"

FILE* file_logger = NULL;

void open_log_file( const char* file_path ){
    file_logger = fopen( file_path, "w" );
    if( file_logger == NULL ){
        log_fatal( "open logger file error\n" );
        return ;
    }
    log_info( "success open log file\n" );
}

void close_log_file(){
    fclose( file_logger );
}
