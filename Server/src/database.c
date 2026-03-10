#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>

#include "database.h"
#include "logging.h"
#include "string_function.h"

const char* FOLDER_NAME = "../database";
mode_t MODE = 0777;
const size_t memory_reserve = 10;                   // for snprintf
const long FILE_BEGIN = 0;                          // for fseek
const size_t NUMBER_INIT = 10;                      // for calloc

const int TODAY     = 0;                            // messages written on the same day must have a difference in the number of days - 0
const int YESTERDAY = 1;                            // a message written yesterday should have a difference of 1 day
const int WEEK      = 6;                            // a message written this week must have a difference in days no more than 6

unsigned long hash( const char* string ){
    assert( string );

    unsigned long hash = 5381;
    int c;
    while( ( c = *string++ ) ){
        hash = ( (hash << 5 ) + hash ) + c;
    }
    return hash;
}

database_err make_dir(){

    if( mkdir( FOLDER_NAME, MODE ) != 0 ){
        log_panic( "error creating folder" );
        return MKDIR_CREATE_ERR;
    }
    return SUCCESS;
}

database_err delete_dir(){

    const char* system_command = "rm -rf";
    size_t command_size = strlen(system_command) + strlen(FOLDER_NAME) + memory_reserve;
    char* command = (char*)calloc( command_size, sizeof(char) );
    if( command == NULL ){
        return MEMORY_ERR;
    }
    snprintf( command, command_size, "%s %s", system_command, FOLDER_NAME );

    int result = system( command );
    if( result != 0 ){
        log_panic( "error delete folder" );
        free( command );
        return MKDIR_DELETE_ERR;
    }
    free( command );
    return SUCCESS;
}

FILE* create_history( char* room_name ){
    assert( room_name );

    char* path = (char*)calloc( PATH_MAX, sizeof(char) );
    if( file_name == NULL ){
        return NULL;
    }
    snprintf( path, PATH_MAX, "%s/%s.txt", FOLDER_NAME, room_name );

    FILE* message_history = fopen( path, "w+" );                                       // write and read data
    int result = setvbuf( message_history, NULL, _IONBF, 0 );
    if( result < 0 ){
        log_panic( "setvbuf error" );
        return NULL;
    }
    free( path );
    return message_history;
}

database_err save_message( const char* message, FILE* message_history){
    assert( message );
    assert( message_history );

    if( message_history == NULL ){
        log_panic( "ptr on file with message history is null ptr" );
        return NO_ROOM;
    }

    time_t current_time = time(NULL);                                                        // get current time
    time_data_t* time_data = localtime(&current_time);
    if( time_data == NULL ){
        log_panic( "function localtime returned error" );
        return PERIOD_ERR;
    }

    fprintf( message_history, "%d %s [%lu]\n", time_data->tm_yday, message, hash( message ) );
    return PERIOD_ERR;
}


char* get_today_messages( FILE* message_history ){
    assert( message_history );

    time_t current_time = time(NULL);
    time_data_t* time_data = localtime(&current_time);
    search_info_t search_info = { NULL, time_data->tm_yday, TODAY, TODAY };

    return get_history( message_history, search_info, FIXED_TIME );
}

char* get_yesterday_messages( FILE* message_history ){
    assert( message_history );

    time_t current_time = time(NULL);
    time_data_t* time_data = localtime(&current_time);
    search_info_t search_info = { NULL, time_data->tm_yday, YESTERDAY, YESTERDAY };

    return get_history( message_history, search_info, FIXED_TIME );
}

char* get_week_messages( FILE* message_history ){
    assert( message_history );

    time_t current_time = time(NULL);
    time_data_t* time_data = localtime(&current_time);
    search_info_t search_info = { NULL, time_data->tm_yday, 0, WEEK };

    return get_history( message_history, search_info, FIXED_TIME );
}

char* get_history( FILE* message_history, search_info_t search_info, period_type what_message ){
    assert( message_history );

    fpos_t pos = {};
    int status = fgetpos( message_history, &pos );                                     // save the current position in the file
    if( status != 0 ){
        log_panic( "function fgetpos returned error" );
        return NULL;
    }

    status = fseek( message_history, FILE_BEGIN, SEEK_SET );                          // moving the file pointer to the beginning
    if( status != 0 ){
        log_panic( "function fseek returned error" );
        return NULL;
    }

    history_t history_info = { NULL, NUMBER_INIT, 0 };
    history_info.history = (char*)calloc( NUMBER_INIT, sizeof(char) );
    database_err read_status = SUCCESS;
    if( what_message == FIXED_TIME ){
        read_status = fixed_period( message_history, &history_info, search_info );
    }
    else if( what_message == ALL_UNREAD_SMS ){
        read_status = unread_messages( message_history, &history_info, search_info );
    }
    else if( what_message == ALL_SMS ){
        read_status = all_messages( message_history, &history_info, search_info );
    }
    if( read_status != SUCCESS ){
        return NULL;
    }

    fsetpos( message_history, &pos );                                                   // return the pointer
    return history_info.history;
}

database_err fixed_period( FILE* message_history,  history_t* history_info , search_info_t search_info){
    assert( message_history );
    assert( history_info );

    database_err read_status = read_history( message_history, history_info, search_info );
    if( read_status != SUCCESS ){
        log_panic( "read_history returned null ptr" );
        return read_status;
    }
    return SUCCESS;
}

database_err read_history( FILE* message_history, history_t* history_info, search_info_t search_info ){
    assert( message_history );
    assert( history_info );

    char* buffer = NULL;
    size_t buffer_capacity = 0;
    ssize_t buffer_len = 0 ;
    char* whitespace = NULL;
    char* opening_bracket = NULL;

    while( ( buffer_len = getline_wrapper( &buffer, &buffer_capacity, message_history ) ) != -1 ){
        whitespace = strchr( buffer, ' ' );
        *whitespace = '\0';
        int day = atoi( buffer );                                                                     //  read the days in a year
        if( search_info.min_difference <= search_info.current_day - day &&
                                      search_info.current_day - day <= search_info.max_difference ){

            opening_bracket = strchr( whitespace + 1, '[' );
            if( history_info->history_capacity <= history_info->history_len + opening_bracket - whitespace ){
                history_info->history_capacity *= 2;
                history_info->history = (char*)realloc( history_info->history, history_info->history_capacity * sizeof(char) );
            }
            *(opening_bracket - 1 ) = '\n';
            *opening_bracket = '\0';
            history_info->history_len += opening_bracket - whitespace;
            strncat( history_info->history, whitespace + 1, opening_bracket - whitespace );
        }
    }

    free( buffer );
    return SUCCESS;
}

char* get_unread_messages( FILE* message_history, const char* last_seen_message ){
    assert( message_history );

    time_t current_time = time(NULL);
    time_data_t* time_data = localtime(&current_time);
    search_info_t search_info = { (char*)last_seen_message, 0, 0, 0 };

    if( last_seen_message == NULL ){
        return get_history( message_history, search_info, ALL_SMS );              // If the client has just joined the room, display all messages.
    }
    return get_history( message_history, search_info, ALL_UNREAD_SMS );
}

database_err unread_messages( FILE* message_history, history_t* history_info, search_info_t search_info ){
    assert( message_history );
    assert( history_info );

    database_err read_status = scan_unread_message( message_history, history_info, search_info );
    if( read_status != SUCCESS ){
        log_panic( "scan_unread_message returned null ptr" );
        return read_status;
    }
    return SUCCESS;
}

database_err scan_unread_message( FILE* message_history, history_t* history_info, search_info_t search_info ){
    assert( message_history );
    assert( history_info );

    char* buffer = NULL;
    size_t buffer_capacity = 0;
    ssize_t buffer_len = 0 ;
    char* opening_bracket = NULL;
    char* close_bracket = NULL;
    char* whitespace = NULL;
    unsigned long message_hash = hash( search_info.last_seen_message );
    unsigned long read_hash = 0;

    while( ( buffer_len = getline_wrapper( &buffer, &buffer_capacity, message_history ) ) != -1 ){
        opening_bracket = strchr( buffer, '[' );
        close_bracket = strchr( buffer, ']' );
        *close_bracket = '\0';
        read_hash = strtoul( opening_bracket + 1, NULL, 10);
        if( message_hash == read_hash ){
            break;
        }
    }

    while( ( buffer_len = getline_wrapper( &buffer, &buffer_capacity, message_history ) ) != -1 ){
        opening_bracket = strchr( buffer, '[' );
        whitespace = strchr( buffer, ' ' );
        if( history_info->history_capacity <= history_info->history_len + opening_bracket - whitespace ){
            history_info->history_capacity *= 2;
            history_info->history = (char*)realloc( history_info->history, history_info->history_capacity * sizeof(char) );
        }
        history_info->history_len += opening_bracket - whitespace;
        *(opening_bracket - 1 ) = '\n';
        *opening_bracket = '\0';
        strncat( history_info->history, whitespace + 1, opening_bracket - whitespace );
    }

    free( buffer );
    return SUCCESS;
}

database_err all_messages( FILE* message_history, history_t* history_info, search_info_t search_info ){
    assert( message_history );
    assert( history_info );

    database_err read_status = read_all_messages( message_history, history_info, search_info );
    if( read_status != SUCCESS ){
        log_panic( "read_all_messages returned null ptr" );
        return read_status;
    }
    return SUCCESS;
}

database_err read_all_messages( FILE* message_history, history_t* history_info, search_info_t search_info ){
    assert( message_history );
    assert( history_info );

    char* buffer = NULL;
    size_t buffer_capacity = 0;
    ssize_t buffer_len = 0 ;
    char* opening_bracket = NULL;
    char* whitespace = NULL;

    while( ( buffer_len = getline_wrapper( &buffer, &buffer_capacity, message_history ) ) != -1 ){
        opening_bracket = strchr( buffer, '[' );
        whitespace = strchr( buffer, ' ' );
        if( history_info->history_capacity <= history_info->history_len + opening_bracket - whitespace ){
            history_info->history_capacity *= 2;
            history_info->history = (char*)realloc( history_info->history, history_info->history_capacity * sizeof(char) );
        }
        history_info->history_len += opening_bracket - whitespace;
        *(opening_bracket - 1 ) = '\n';
        *opening_bracket = '\0';
        strncat( history_info->history, whitespace + 1, opening_bracket - whitespace );
    }

    free( buffer );
    return SUCCESS;
}

