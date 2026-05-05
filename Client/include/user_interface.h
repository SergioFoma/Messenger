#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <sys/ioctl.h>                  // winsize
#include <stdbool.h>
#include <ncurses.h>

typedef enum ui_stat_e {                // user interface status
    CORRECT_STATE           = 0,
    COLOR_ERR               = 1,
    WINDOW_ERR              = 2,
    CHOICE_ERR              = 3,
    CLOSE_MESSENGER         = 4,
    READ_ERR                = 5,
    ROOM_NAME_ERR           = 6,
    START_CREATE_ROOM       = 7,
    FIN_CREATE_ROOM         = 8,
    CREATED_MANAGE_MENU     = 9,
    START_JOIN_CHAT         = 10,
    START_LEAVE_CHAT        = 11,
    NO_REQUEST              = 12,
    CONNECT_ERR             = 13,
    READ_MESSAGE            = 14,
    USER_ENTERS             = 15,
    COMPANION_ENTERS        = 16,
    START_ROOM_LIST         = 17,
    CLOSE_ROOM_LIST         = 18,
    WAITING_ERR             = 19,
    VIEW_TODAY_HISTORY      = 20,
    VIEW_YESTERDAY_HISTORY  = 21,
    VIEW_WEEK_HISTORY       = 22,
    VIEW_ALL_UNREAD         = 23,
    START_SEND_FILE         = 24,
    REQUEST_ACCEPTED        = 25,
    REQUEST_NOT_ACCEPTED    = 26,
    FINISH_DOWNLOAD	    = 27,
    START_RECOGNIZE_PHOTO   = 28
} ui_stat_t;

typedef enum win_colors_e {
    MAIN_BACK       = 1,
    NAME_BACK       = 2,
    MAIN_BOX        = 3,
    REG_BACK        = 4,
    NAME_BOX        = 5,
    BLACK_BACK      = 6,
    ROOM_BACK       = 7,
    MENU_BACK       = 8,
    KEY             = 9,
    ROOM_NAME_BACK  = 10,
    ONE_ROOM        = 11,
    HISTORY_BACK    = 12,
    CHAT_BACK       = 13,
    OPTION          = 14
} win_colors_t;

typedef struct user_info_s {
    char* ip;
    ui_stat_t status;
} user_info_t;

typedef struct input_line_pos_s {
    int user_x;
    int user_y;
    int companion_x;
    int companion_y;
    int history_x;
    int history_y;
    int file_x;
    int file_y;
} input_line_pos_t;

typedef struct windows_s {
    WINDOW* main_win;
    WINDOW* name_win;
    WINDOW* reg_win;
    WINDOW* menu_win;
    WINDOW* room_name_win;
    WINDOW* der_name_win;
    WINDOW* manage_menu_win;
    WINDOW* chat_back_win;
    WINDOW* chat_history_win;
    WINDOW* der_history_win;
    WINDOW* chat_menu_win;
    WINDOW* chat_name_win;
    WINDOW* companion_win;
    WINDOW* der_companion_win;
    WINDOW* user_win;
    WINDOW* der_user_win;
    WINDOW* list_win;
    WINDOW* der_list_win;
    WINDOW* file_win;
    WINDOW* der_file_win;
} windows_t;

typedef struct room_position_s {
    WINDOW* room_win;
    ui_stat_t status;
    int begin_y;                        // current free row
    int begin_x;                        // column from which begining recording
    int ncolumns;                       // window width
    int nlines;
} room_position_t;

typedef struct rooms_info_s {
    char** room_names;
    WINDOW** chat_windows;
    size_t len;
    size_t capacity;
} rooms_info_t;

typedef struct winsize winsize_t;

typedef struct main_struct_s {
    windows_t* windows;
    winsize_t* console_size;
    user_info_t* user_data;
} main_struct_t;

void init_colors();

winsize_t* get_console_size();

windows_t* create_background( winsize_t* console_size );

ui_stat_t create_name_win( winsize_t* console_size, windows_t* windows );

user_info_t* client_registration( winsize_t* console_size, windows_t* windows );

room_position_t* create_room_list( winsize_t* console_size );

ui_stat_t create_menu( windows_t* windows, winsize_t* console_size );

ui_stat_t parse_request( WINDOW* window, user_info_t* user_data, const bool is_closing, int* available_symbol );

ui_stat_t file_request( WINDOW* window );

ui_stat_t leave_app( user_info_t* user_data );

ui_stat_t create_room( room_position_t* room_pos, winsize_t* console_size, char* room_name );

ui_stat_t create_room_name_win( winsize_t* console_size, windows_t* windows );

ui_stat_t create_manage_menu( windows_t* windows, winsize_t* console_size );

ui_stat_t create_chat_background( windows_t* windows, winsize_t* console_size, char* room_name );

ui_stat_t create_chat_name( windows_t* windows, winsize_t* console_size, char* room_name );

ui_stat_t create_chat_menu( windows_t* window, winsize_t* console_size );

ui_stat_t create_history_win( windows_t* windows, winsize_t* console_size );

ui_stat_t create_companion_win( windows_t* windows, winsize_t* console_size );

ui_stat_t create_user_win( windows_t* windows, winsize_t* console_size );

ui_stat_t create_list_win( windows_t* windows, winsize_t* console_size );

ui_stat_t create_file_path_win( winsize_t* console_size, windows_t* windows );

ui_stat_t create_file_name_win( winsize_t* console_size, windows_t* windows );

ui_stat_t create_get_file_win( winsize_t* console_size, windows_t* windows );

ui_stat_t file_window( winsize_t* console_size, windows_t* windows, char* info_line );

char* make_uppercase_line( char* line );

void init_rooms_info();

void close_chat_windows( windows_t* windows );

void update_original_windows( windows_t* windows, room_position_t* room_pos );

void update_window( WINDOW* win );

void add_chat( char* room_name, WINDOW* chat_win );

void names_reallocation();

void chats_reallocation();

void destroy_rooms_info();

void clear_input_line( WINDOW* win, ui_stat_t state );

void clear_history_line( WINDOW* win, winsize_t* console_size );

void clear_file_line( WINDOW* win );

ui_stat_t check_connect_possibility( char* room_name );

void show_option( WINDOW* window, const char* option, const char* key, int begin_y );

void show_list_option( WINDOW* window, const char* option, const char* key, int begin_y );

void show_companion_message( windows_t* windows, char* companion_message, size_t nread );

void show_history( windows_t* windows, char* hst, size_t nread );

void show_room_list( windows_t* windows, winsize_t* console_size, char* room_list, size_t nread );

void file_accept_request( WINDOW* file_win, char* file_name );

void waiting_download_win( windows_t* windows );

void download_complete( windows_t* windows );

void dispatch_notification( WINDOW* file_win );

void update_chat_windows( windows_t* windows );

ui_stat_t waiting_exit( windows_t* windows );

void close_room_info( windows_t* windows );

void close_file_windows( windows_t* windows );

void delete_symbol( size_t* buf_size, WINDOW* window );

void close_window( WINDOW* window );

void destroy_user( user_info_t* user_data );

void destroy_background();

#endif
