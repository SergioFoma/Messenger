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
    PERIOD_ERR          = 6
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

typedef struct search_info_s{
    char* last_seen_message;
    int current_day;
    int min_difference;
    int max_difference;
} search_info_t;

typedef struct tm time_data_t;

unsigned long hash( const char* string );

database_err make_dir();

database_err delete_dir();

FILE* create_history( char* room_name );

database_err save_message( const char* message, FILE* message_history );

char* get_today_messages( FILE* message_history );

char* get_yesterday_messages( FILE* message_history );

char* get_week_messages( FILE* message_history );

char* get_history( FILE* message_history, search_info_t search_info, period_type what_message );

database_err read_history( FILE* message_history, history_t* history_info, search_info_t search_info );

database_err fixed_period( FILE* message_history, history_t* history_info, search_info_t search_info );

char* get_unread_messages( FILE* message_history, const char* last_seen_message );

database_err unread_messages( FILE* message_history, history_t* history_info, search_info_t search_info );

database_err scan_unread_message( FILE* message_history, history_t* history_info, search_info_t search_info );

database_err all_messages( FILE* message_history, history_t* history_info, search_info_t search_info );

database_err read_all_messages( FILE* message_history, history_t* history_info, search_info_t search_info );

#endif
