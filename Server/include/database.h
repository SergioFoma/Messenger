#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <time.h>
#include <stdbool.h>

typedef enum database_err_e {
    SUCCESS             = 0,
    MKDIR_CREATE_ERR    = 1,
    MKDIR_DELETE_ERR    = 2,
    MEMORY_ERR          = 3,
    WRITE_IN_FILE_ERR   = 4,
    NO_ROOM             = 5,
    PERIOD_ERR          = 6,
    NO_FOLDER           = 7
} database_err;

typedef enum period_type_e {
    FIXED_TIME          = 0,
    ALL_UNREAD_SMS      = 1,
    ALL_SMS             = 2
} period_type;

typedef struct history_s {
    char* history;
    size_t history_capacity;
    size_t history_len;
} history_t;

typedef struct search_history_s{
    char** last_seen_message;
    int current_day;
    int min_difference;
    int max_difference;
} search_history_t;

typedef struct file_info_s{
    FILE* read_fd;
    int write_fd;
} file_info_t;

typedef struct tm time_data_t;

typedef struct entry_s {
    char* message;
    unsigned long hash;
    size_t message_len;
    int day;
} entry_t;

unsigned long hash( const char* string );

database_err make_dir();

database_err delete_dir();

file_info_t create_history( char* room_name );

database_err save_message( const char* message, int write_fd );

char* get_today_messages( FILE* message_history );

char* get_yesterday_messages( FILE* message_history );

char* get_week_messages( FILE* message_history );

char* get_history( FILE* message_history, search_history_t* search_history, period_type what_message );

database_err read_history( FILE* message_history, history_t* history_info, search_history_t* search_history );

database_err fixed_period( FILE* message_history, history_t* history_info, search_history_t* search_history );

char* get_unread_messages( FILE* message_history, char** last_seen_message );

database_err unread_messages( FILE* message_history, history_t* history_info, search_history_t* search_history );

database_err scan_unread_message( FILE* message_history, history_t* history_info, search_history_t* search_history );

database_err all_messages( FILE* message_history, history_t* history_info );

database_err read_all_messages( FILE* message_history, history_t* history_info );

entry_t read_hist_line( char* hist_line );

#endif
