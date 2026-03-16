#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "database.h"
#include "logging.h"
#include "string_function.h"

const char* FOLDER_NAME = "../Database";
mode_t DIR_MODE = 0777;                             // for mkdir
mode_t FILE_MODE = 0644;                            // for open
const size_t memory_reserve = 10;                   // for snprintf
const long FILE_BEGIN = 0;                          // for fseek
const int DELETE_FILE = 0;                          // for unlinkat
const size_t NUMBER_INIT = 10;                      // for calloc
const size_t MAX_HISTORY_LINE = 1024;               // for snprintf


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

    if( mkdir( FOLDER_NAME, DIR_MODE ) != 0 ){
        log_panic( "error creating folder" );
        return MKDIR_CREATE_ERR;
    }
    return SUCCESS;
}

database_err delete_dir(){

    DIR* data_folder = opendir( FOLDER_NAME );                                      // open the directory to read its contents
    if( data_folder == NULL ){
        log_warning( "there is no folder with room histories" );
        return NO_FOLDER;
    }

    int folder_fd = open( FOLDER_NAME, O_RDONLY );                                  // getting a file descriptor for a folder
    int result;
    struct dirent* current_file = NULL;
    while( ( current_file = readdir( data_folder ) ) != NULL ){
        if( current_file->d_name[0] == '.' || current_file->d_name[0] == '..' ){   // check that we received the correct file.
            continue;
        }
        result = unlinkat( folder_fd, current_file->d_name, DELETE_FILE );         // delete file
    }

    rmdir( FOLDER_NAME );                                                          // delete folder
    closedir( data_folder );                                                       // close folder
    return SUCCESS;
}

file_info_t create_history( char* room_name ){
    assert( room_name );

    file_info_t file_info = {};
    char path[PATH_MAX] = {};
    snprintf( path, PATH_MAX, "%s/%s.txt", FOLDER_NAME, room_name );

    file_info.write_fd = open( path, O_WRONLY | O_CREAT, FILE_MODE );             // write data and create file
    file_info.read_fd = fopen( path, "r" );                                       // read data
    log_debug( "open res: %d", file_info.write_fd );
    return file_info;
}

database_err save_message( const char* message, int write_fd ){
    assert( message );

    if( write_fd == -1 ){
        log_panic( "ptr on file with message history is null ptr" );
        return NO_ROOM;
    }

    time_t current_time = time(NULL);                                                        // get current time
    time_data_t* time_data = localtime(&current_time);
    if( time_data == NULL ){
        log_panic( "function localtime returned error" );
        return PERIOD_ERR;
    }

    char* history_line = (char*)calloc( MAX_HISTORY_LINE, sizeof(char) );
    int line_len = snprintf( history_line, MAX_HISTORY_LINE, "%d %s [%lu]\n", time_data->tm_yday, message, hash( message ) );
    write( write_fd, history_line, line_len );
    return SUCCESS;
}


char* get_today_messages( FILE* message_history ){
    assert( message_history );

    time_t current_time = time(NULL);
    time_data_t* time_data = localtime(&current_time);
    search_history_t search_history = { NULL, time_data->tm_yday, TODAY, TODAY };

    return get_history( message_history, &search_history, FIXED_TIME );
}

char* get_yesterday_messages( FILE* message_history ){
    assert( message_history );

    time_t current_time = time(NULL);
    time_data_t* time_data = localtime(&current_time);
    search_history_t search_history = { NULL, time_data->tm_yday, YESTERDAY, YESTERDAY };

    return get_history( message_history, &search_history, FIXED_TIME );
}

char* get_week_messages( FILE* message_history ){
    assert( message_history );

    time_t current_time = time(NULL);
    time_data_t* time_data = localtime(&current_time);
    search_history_t search_history = { NULL, time_data->tm_yday, 0, WEEK };

    return get_history( message_history, &search_history, FIXED_TIME );
}

char* get_history( FILE* message_history, search_history_t* search_history, period_type what_message ){
    assert( message_history );

    history_t history_info = { NULL, NUMBER_INIT, 0 };
    history_info.history = (char*)calloc( NUMBER_INIT, sizeof(char) );
    database_err read_status = SUCCESS;
    if( what_message == FIXED_TIME ){
        read_status = fixed_period( message_history, &history_info, search_history );
    }
    else if( what_message == ALL_UNREAD_SMS ){
        read_status = unread_messages( message_history, &history_info, search_history );
    }
    else if( what_message == ALL_SMS ){
        read_status = all_messages( message_history, &history_info );
    }
    if( read_status != SUCCESS ){
        return NULL;
    }

    rewind( message_history );              // return to the beginning of the file
    return history_info.history;
}

database_err fixed_period( FILE* message_history,  history_t* history_info , search_history_t* search_history){
    assert( message_history );
    assert( history_info );

    database_err read_status = read_history( message_history, history_info, search_history );
    if( read_status != SUCCESS ){
        log_panic( "read_history returned null ptr" );
        return read_status;
    }
    return SUCCESS;
}

entry_t read_hist_line( char* hist_line ){
    assert( hist_line );

    char* whitespace = strchr( hist_line, ' ' );
    *whitespace = '\0';                                         // ' ' --> '\0'
    char* opening_bracket = strchr( whitespace + 1, '[' );
    *opening_bracket = '\0';                                    // '[' --> '\0'
    *(opening_bracket - 1) = '\n';                              // ' ' --> '\n'
    char* close_bracket = strchr( opening_bracket + 1, ']' );
    *close_bracket = '\0';                                      // ']' --> '\0'

    entry_t entry = {};
    entry.message = whitespace + 1;
    entry.hash = strtoul( opening_bracket + 1, NULL, 10);       // string --> unsigned
    entry.message_len = strlen( whitespace + 1);
    entry.day = atoi( hist_line );

    log_info( "message = %s, message_len = %lu, hash = %lu, day = %d", entry.message, entry.message_len, entry.hash, entry.day );
    return entry;
}

database_err read_history( FILE* message_history, history_t* history_info, search_history_t* search_history ){
    assert( message_history );
    assert( history_info );

    char* buffer = NULL;
    size_t buffer_capacity = 0;
    ssize_t buffer_len = 0 ;
    entry_t hist_line = {};
    FILE* hst = open_memstream( &history_info->history, &history_info->history_len );

    while( ( buffer_len = getline_wrapper( &buffer, &buffer_capacity, message_history ) ) != -1 ){
        hist_line = read_hist_line( buffer );                                                           //  parse history line
        if( search_history->min_difference <= search_history->current_day - hist_line.day &&
                                              search_history->current_day - hist_line.day <= search_history->max_difference ){

            if( history_info->history_capacity <= history_info->history_len + hist_line.message_len ){
                history_info->history_capacity = ( history_info->history_len + hist_line.message_len ) * 2;
                history_info->history = (char*)realloc( history_info->history, history_info->history_capacity * sizeof(char) );
            }
            history_info->history_len += hist_line.message_len;
            fprintf( hst, "%.*s", hist_line.message_len, hist_line.message );
        }
    }

    fclose( hst );
    free( buffer );
    return SUCCESS;
}

char* get_unread_messages( FILE* message_history, char** last_seen_message ){
    assert( message_history );
    assert( last_seen_message );

    time_t current_time = time(NULL);
    time_data_t* time_data = localtime(&current_time);
    search_history_t search_history = { last_seen_message, 0, 0, 0 };

    if( *last_seen_message == NULL ){
        return get_history( message_history, &search_history, ALL_SMS );              // If the client has just joined the room, display all messages.
    }
    return get_history( message_history, &search_history, ALL_UNREAD_SMS );
}

database_err unread_messages( FILE* message_history, history_t* history_info, search_history_t* search_history ){
    assert( message_history );
    assert( history_info );

    database_err read_status = scan_unread_message( message_history, history_info, search_history );
    if( read_status != SUCCESS ){
        log_panic( "scan_unread_message returned null ptr" );
        return read_status;
    }
    return SUCCESS;
}

database_err scan_unread_message( FILE* message_history, history_t* history_info, search_history_t* search_history ){
    assert( message_history );
    assert( history_info );

    char* buffer = NULL;
    size_t buffer_capacity = 0;
    ssize_t buffer_len = 0 ;
    unsigned long message_hash = hash( *(search_history->last_seen_message) );
    entry_t hist_line = {};
    FILE* hst = open_memstream( &history_info->history, &history_info->history_len );

    while( ( buffer_len = getline_wrapper( &buffer, &buffer_capacity, message_history ) ) != -1 ){
        hist_line = read_hist_line( buffer );
        if( message_hash == hist_line.hash ){
            break;
        }
    }

    char* last_read_message = NULL;
    while( ( buffer_len = getline_wrapper( &buffer, &buffer_capacity, message_history ) ) != -1 ){
        hist_line = read_hist_line( buffer );
        if( history_info->history_capacity <= history_info->history_len + hist_line.message_len ){
            history_info->history_capacity = ( history_info->history_len + hist_line.message_len ) * 2;
            history_info->history = (char*)realloc( history_info->history, history_info->history_capacity * sizeof(char) );
        }
        history_info->history_len += hist_line.message_len;
        fprintf( hst, "%.*s", hist_line.message_len, hist_line.message );
        last_read_message = buffer;
    }

    if( last_read_message != NULL ){
        free( *(search_history->last_seen_message) );
        *(search_history->last_seen_message) = strdup( last_read_message );
        log_info( "update last seen message" );
    }
    fclose( hst );
    free( buffer );
    return SUCCESS;
}

database_err all_messages( FILE* message_history, history_t* history_info ){
    assert( message_history );
    assert( history_info );

    database_err read_status = read_all_messages( message_history, history_info );
    if( read_status != SUCCESS ){
        log_panic( "read_all_messages returned null ptr" );
        return read_status;
    }
    return SUCCESS;
}

database_err read_all_messages( FILE* message_history, history_t* history_info ){
    assert( message_history );
    assert( history_info );

    char* buffer = NULL;
    size_t buffer_capacity = 0;
    ssize_t buffer_len = 0 ;
    entry_t hist_line = {};
    FILE* hst = open_memstream( &history_info->history, &history_info->history_len );

    while( ( buffer_len = getline_wrapper( &buffer, &buffer_capacity, message_history ) ) != -1 ){
        hist_line = read_hist_line( buffer );
        if( history_info->history_capacity <= history_info->history_len + hist_line.message_len ){
            history_info->history_capacity = ( history_info->history_len + hist_line.message_len ) * 2;
            history_info->history = (char*)realloc( history_info->history, history_info->history_capacity * sizeof(char) );
        }
        history_info->history_len += hist_line.message_len;
        fprintf( hst, "%.*s", hist_line.message_len, hist_line.message );
    }

    fclose( hst );
    free( buffer );
    return SUCCESS;
}

