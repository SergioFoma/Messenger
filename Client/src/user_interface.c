#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>

#include "logger.h"
#include "user_interface.h"

const size_t ONE = 1;
const size_t MAX_IP_LEN = 20;
const size_t MAX_NAME_LEN = 30;
const size_t INIT_COUNT_ROOMS = 5;
const int ROOM_NLINES = 5;

rooms_info_t rooms_info = {};                       // for room list
input_line_pos_t input_line_pos = {};               // for work with user and companion window

/*
* number of lines in my console: 61
* number of columns in my console: 248
*/

void init_colors(){
    init_pair( BLACK_BACK, COLOR_BLACK, COLOR_BLACK );
    init_pair( MAIN_BACK, COLOR_WHITE, COLOR_BLACK );
    init_pair( MAIN_BOX, COLOR_WHITE, COLOR_WHITE );
    init_pair( NAME_BACK, COLOR_BLACK, COLOR_CYAN );
    init_pair( NAME_BOX, COLOR_WHITE, COLOR_CYAN);
    init_pair( REG_BACK, COLOR_WHITE, COLOR_BLUE );
    init_pair( ROOM_BACK, COLOR_YELLOW, COLOR_BLUE );
    init_pair( MENU_BACK, COLOR_WHITE, COLOR_MAGENTA );
    init_pair( KEY, COLOR_GREEN, COLOR_MAGENTA );
    init_pair( ROOM_NAME_BACK, COLOR_YELLOW, COLOR_BLACK );
    init_pair( ONE_ROOM, COLOR_BLACK, COLOR_GREEN );
    init_pair( HISTORY_BACK, COLOR_BLACK, COLOR_YELLOW );
    init_pair( CHAT_BACK, COLOR_CYAN, COLOR_BLACK );
    init_pair( OPTION, COLOR_MAGENTA, COLOR_BLACK );
}

winsize_t* get_console_size(){

    winsize_t* default_size = (winsize_t*)calloc( ONE, sizeof(winsize_t) );

    // STDOUT_FILENO - stdout file descriptor, TIOCGWINSZ - getting window size.
    ioctl( STDOUT_FILENO, TIOCGWINSZ, default_size );

    log_info( "default console size: nlines = %lu, ncolumns = %lu", default_size->ws_row, default_size->ws_col );
    return default_size;
}

windows_t* create_background( winsize_t* console_size ){
    assert( console_size );

    ui_stat_t status = CORRECT_STATE;
    windows_t* windows = (windows_t*)calloc( ONE, sizeof(windows_t) );
    WINDOW* main_win =  initscr();                                              // screen initialization
    windows->main_win = main_win;
    if( has_colors() == false ){
        log_fatal( "terminal doesn't support color\n" );
        return NULL;
    }
    start_color();
    init_colors();
    keypad( stdscr, TRUE );                                                     // special key
    curs_set(0);                                                                // hide the cursor
    echo();

    bkgd( COLOR_PAIR( MAIN_BACK ) );                                            // set background color

    wattron( main_win, COLOR_PAIR( MAIN_BOX ) );
    box( main_win, 0, 0 );
    wattroff( main_win, COLOR_PAIR( MAIN_BOX ) );
    refresh();

    if( ( status = create_name_win( console_size, windows ) ) != CORRECT_STATE ){
        return NULL;
    }
    return windows;
}

ui_stat_t create_name_win( winsize_t* console_size, windows_t* windows ){
    assert( console_size );

    int nlines = 0;
    int ncolumns = 0;
    int begin_y = 0;
    int begin_x = 0;

    nlines = 4, ncolumns = 70, begin_x = console_size->ws_col / 3 + 5, begin_y = 0;
    WINDOW* name_win = newwin( nlines, ncolumns, begin_y, begin_x );
    if( name_win == NULL ){
        log_fatal( "window with messenger name is null ptr\n" );
        return WINDOW_ERR;
    }
    windows->name_win = name_win;
    wbkgd( name_win, COLOR_PAIR( NAME_BACK ) );

    begin_x = 25, begin_y = 2;
    wattron( name_win, A_BOLD );
    mvwaddstr( name_win, begin_y, begin_x, "MESSENGER SAVA" );             // moves the cursor to position (x;y) and prints the string
    wattroff( name_win, A_BOLD );

    wattron( name_win, COLOR_PAIR( NAME_BOX ) );
    box( name_win, 0, 0 );
    wattroff( name_win, COLOR_PAIR( NAME_BOX ) );

    wrefresh( name_win );
    return CORRECT_STATE;
}

user_info_t* client_registration( winsize_t* console_size, windows_t* windows ){
    assert( console_size );
    assert( windows );

    int nlines = 10;
    int ncolumns = 40;
    int begin_x = console_size->ws_col / 3 + 15;
    int begin_y = console_size->ws_row / 3;

    WINDOW* reg_win = newwin( nlines, ncolumns, begin_y, begin_x );
    if( reg_win == NULL ){
        log_fatal( "registration window is null ptr" );
        return NULL;
    }
    windows->reg_win = reg_win;

    wbkgd( reg_win, COLOR_PAIR( REG_BACK ) );
    keypad( reg_win, TRUE );                                                     // special key

    begin_x = 10, begin_y = 1;
    wattron( reg_win, A_BOLD );
    mvwaddstr( reg_win, begin_y, begin_x, "PLEASE, REGISTER");
    begin_x = 0, begin_y = 2;
    mvwhline( reg_win, begin_y, begin_x, ACS_HLINE, ncolumns );     // ACS_HLINE - for a solid line
    begin_x = 2, begin_y = 4;
    mvwaddstr( reg_win, begin_y, begin_x, "IP: " );
    box( reg_win, 0, 0 );
    wrefresh( reg_win );

    char* ip = (char*)calloc( MAX_IP_LEN, sizeof(char) );
    wgetnstr( reg_win, ip, MAX_IP_LEN - 1 );
    wattroff( reg_win, A_BOLD );

    user_info_t* user_data = (user_info_t*)calloc( ONE, sizeof(user_info_t) );
    user_data->ip = ip;
    user_data->status = CORRECT_STATE;
    return user_data;
}

room_position_t* create_room_list( winsize_t* console_size ){
    assert( console_size );

    int ncolumns = 40;
    int begin_x = 1;
    int begin_y = 1;
    WINDOW* room_win = newwin( console_size->ws_row - 2, ncolumns, begin_y, begin_x );
    if( room_win == NULL ){
        log_fatal( "room window is null ptr" );
        return NULL;
    }
    wbkgd( room_win, COLOR_PAIR( ROOM_BACK ) );

    wattron( room_win, A_BOLD );
    begin_x = 13, begin_y = 1;
    mvwaddstr( room_win, begin_y, begin_x, "ROOMS LIST" );
    begin_x = 0, begin_y = 2;
    mvwhline( room_win, begin_y, begin_x, ACS_HLINE, ncolumns );

    box( room_win, 0, 0 );
    wrefresh( room_win );
    wattroff( room_win, A_BOLD );

    room_position_t* room_pos = (room_position_t*)calloc( ONE, sizeof(room_position_t) );
    room_pos->room_win = room_win;
    room_pos->begin_x = 1;
    room_pos->begin_y = begin_y + 3;
    room_pos->ncolumns = ncolumns;
    room_pos->nlines = ROOM_NLINES;
    room_pos->status = CORRECT_STATE;
    init_rooms_info();
    return room_pos;
}

void init_rooms_info(){

    rooms_info.room_names = (char**)calloc( INIT_COUNT_ROOMS, sizeof(char*) );
    if( rooms_info.room_names == NULL ){
        log_error( "memory allocation error" );
    }
    rooms_info.chat_windows = (WINDOW**)calloc( INIT_COUNT_ROOMS, sizeof(WINDOW*) );
    if( rooms_info.chat_windows == NULL ){
        log_error( "memory allocation error" );
    }
    rooms_info.len = 0;
    rooms_info.capacity = INIT_COUNT_ROOMS;
}

void destroy_rooms_info(){

    char** room_names_begining = rooms_info.room_names;
    char** current_room_name = room_names_begining;
    size_t count_rooms = rooms_info.len;

    for(; current_room_name < room_names_begining + count_rooms; current_room_name++ ){
        if( *current_room_name ){
            free( *current_room_name );
        }
    }
    if( rooms_info.room_names ){
        free( rooms_info.room_names );
    }
    if( rooms_info.chat_windows ){
        free( rooms_info.chat_windows );
    }
    rooms_info.len = 0;
    rooms_info.capacity = 0;
}

ui_stat_t check_connect_possibility( char* room_name ){
    assert( room_name );

    char** room_names_begining = rooms_info.room_names;
    char** current_room_name = room_names_begining;
    size_t count_rooms = rooms_info.len;

    for(; current_room_name < room_names_begining + count_rooms; current_room_name++ ){
        log_debug( "current name = '%s', room name = '%s'", *current_room_name, room_name );
        if( strcmp( *current_room_name, room_name) == 0 ){
            log_info( "room was found" );
            return CORRECT_STATE;
        }
    }

    log_fatal( "room was not found" );
    return CONNECT_ERR;
}

ui_stat_t create_menu( windows_t* windows, winsize_t* console_size ){
    assert( console_size );

    int nlines = 20;
    int ncolumns = 70;
    int begin_x = console_size->ws_col / 3 + 5;
    int begin_y = console_size->ws_row / 3;

    WINDOW* menu_win = newwin( nlines, ncolumns, begin_y, begin_x );
    if( menu_win == NULL ){
        log_fatal( "menu window is null ptr" );
        return WINDOW_ERR;
    }
    wbkgd( menu_win, COLOR_PAIR( MENU_BACK ) );
    keypad( menu_win, TRUE );

    wattron( menu_win, A_BOLD );
    begin_x = 25, begin_y = 1;
    mvwaddstr( menu_win, begin_y, begin_x, "SELECT AN OPTION" );
    begin_x = 0, begin_y = 2;
    mvwhline( menu_win, begin_y, begin_x, ACS_HLINE, ncolumns );
    begin_y = begin_y + 2;
    show_option( menu_win, "*CREATE A NEW CHAT: ", "F1", begin_y );
    begin_y = begin_y + 2;
    show_option( menu_win, "*LEAVE THE MESSENGER: ", "F12", begin_y );

    box( menu_win, 0, 0 );
    wrefresh( menu_win );
    wattroff( menu_win, A_BOLD );
    windows->menu_win = menu_win;

    return CORRECT_STATE;
}

ui_stat_t parse_request( WINDOW* window, user_info_t* user_data, const bool is_closing, int* available_symbol ){
    assert( user_data );
    assert( available_symbol );
    assert( window );

    nodelay( window, TRUE );                        // disabling blocking input
    int user_choice = wgetch( window );
    if( user_choice == ERR ){
        log_info( "command was not read" );
        return READ_ERR;
    }
    if( is_closing ){
        close_window( window );
    }

    *available_symbol = -1;                            // believe, that request was read
    switch( user_choice ){
        case KEY_F(1):
            return START_CREATE_ROOM;
        case KEY_F(2):                                 // join chat
            return START_JOIN_CHAT;
        case KEY_F(3):                                 // leave chat
            return START_LEAVE_CHAT;
        case KEY_F(4):                                 // room info (/list)
            return START_ROOM_LIST;
        case KEY_F(5):
            return VIEW_TODAY_HISTORY;
        case KEY_F(6):
            return VIEW_YESTERDAY_HISTORY;
        case KEY_F(7):
            return VIEW_WEEK_HISTORY;
        case KEY_F(8):
            return VIEW_ALL_UNREAD;
        case KEY_F(9):
            return START_SEND_FILE;
	case KEY_F(10):
	    return START_RECOGNIZE_PHOTO; 
        case KEY_F(12):
            return leave_app( user_data );
        default:
            break;
    }

    log_warning( "request was not recognized. User choice = %d", user_choice );
    *available_symbol = user_choice;                   // if this is not a request, we saved read char
    return READ_MESSAGE;
}

ui_stat_t file_request( WINDOW* window ){
    assert( window );

    nodelay( window, TRUE );
    int user_choice = wgetch( window );
    if( user_choice == ERR ){
        log_info( "command was not read" );
        return READ_ERR;
    }

    int upper = toupper( user_choice );
    log_debug( "user choice = %c", upper );
    if( upper == 'Y' ){
        return REQUEST_ACCEPTED;
    }
    else if( upper == 'N' ){
        return REQUEST_NOT_ACCEPTED;
    }
    else if( upper == 'C' ){
	return FINISH_DOWNLOAD;
    }
    return NO_REQUEST;
}

ui_stat_t leave_app( user_info_t* user_data ){
    assert( user_data );

    destroy_background();
    destroy_rooms_info();

    return CLOSE_MESSENGER;
}

ui_stat_t create_room( room_position_t* room_pos, winsize_t* console_size, char* room_name ){
    assert( room_pos );
    assert( console_size );
    assert( room_name );

    log_info( "room name: %s", room_name );
    WINDOW* room_win = newwin( room_pos->nlines, room_pos->ncolumns, room_pos->begin_y, room_pos->begin_x );
    if( room_win == NULL ){
        log_fatal( "room window is null ptr" );
        return WINDOW_ERR;
    }
    wbkgd( room_win, COLOR_PAIR( ONE_ROOM ) );

    int begin_x = 10;
    int begin_y = 1;
    wattron( room_win, A_BOLD );
    mvwaddstr( room_win, begin_y, begin_x, "CHAT" );
    begin_x = 0, begin_y = 2;
    mvwhline( room_win, begin_y, begin_x, ACS_HLINE, room_pos->ncolumns );
    begin_x = 2, begin_y = 3;
    mvwprintw( room_win, begin_y, begin_x, "NAME: %s", room_name );
    box( room_win, 0, 0 );
    wrefresh( room_win );

    add_chat( room_name, room_win );
    room_pos->begin_x = room_pos->begin_x;
    room_pos->begin_y = room_pos->begin_y + room_pos->nlines;
    room_pos->status = CORRECT_STATE;
    return FIN_CREATE_ROOM;
}

void add_chat( char* room_name, WINDOW* chat_win ){
    assert( chat_win );
    assert( room_name );

    names_reallocation();
    chats_reallocation();
    rooms_info.room_names[ rooms_info.len ] = strdup( room_name );
    rooms_info.chat_windows[ rooms_info.len ] = chat_win;
    ++rooms_info.len;

    log_debug( "added room: %s", room_name );
}

void names_reallocation(){

    char** realloc_buf = NULL;
    if( rooms_info.len == rooms_info.capacity - 1 ){
        rooms_info.capacity *= 2;
        realloc_buf = (char**)realloc( rooms_info.room_names, sizeof(char*) * rooms_info.capacity );
        if( realloc_buf == NULL ){
            log_fatal( "realloc return NULL ptr" );
            return;
        }
        rooms_info.room_names = realloc_buf;
    }
}

void chats_reallocation(){

    WINDOW** realloc_windows = NULL;
    if( rooms_info.len == rooms_info.capacity - 1 ){
        rooms_info.capacity *= 2;
        realloc_windows = (WINDOW**)realloc( rooms_info.chat_windows, sizeof(WINDOW*) );
        if( realloc_windows == NULL ){
            log_fatal( "realloc return NULL ptr" );
            return ;
        }
        rooms_info.chat_windows = realloc_windows;
    }
}

ui_stat_t create_room_name_win( winsize_t* console_size, windows_t* windows ){
        assert( console_size );

        int nlines = 5, ncolumns = 42;
        int der_nlines = nlines - 2, der_ncolumns = ncolumns - 2;
        int begin_x = console_size->ws_col / 3 + 15, begin_y = console_size->ws_row / 3;
        int der_begin_x = 1, der_begin_y = 1;

        WINDOW* room_name_win = newwin( nlines, ncolumns, begin_y, begin_x );
        WINDOW* der_name_win = derwin( room_name_win, der_nlines, der_ncolumns, der_begin_y, der_begin_x);      // create derwin to save box
        if( room_name_win == NULL || der_name_win == NULL ){
            log_fatal( "get room name error" );
            return WINDOW_ERR;
        }
        windows->room_name_win = room_name_win;
        windows->der_name_win = der_name_win;
        wbkgd( room_name_win, COLOR_PAIR( ROOM_NAME_BACK ) );
        wbkgd( der_name_win, COLOR_PAIR( ROOM_NAME_BACK ) );

        box( room_name_win, 0, 0 );

        begin_x = 1, begin_y = 2;
        mvwhline( room_name_win, begin_y, begin_x, ACS_HLINE, der_ncolumns );
        keypad( der_name_win, TRUE );

        wattron( der_name_win, A_BOLD );
        mvwaddstr( der_name_win, 0, 10, "ENTER THE ROOM NAME" );
        wattroff( der_name_win, A_BOLD );

        wrefresh( room_name_win );
        wrefresh( der_name_win );

        der_begin_x = 1, der_begin_y = 2;
        wmove( der_name_win, der_begin_y, der_begin_x );

        return START_CREATE_ROOM;
}

ui_stat_t create_manage_menu( windows_t* windows, winsize_t* console_size ){
    assert( windows );
    assert( console_size );

    int ncolumns = 40;
    int begin_x = console_size->ws_col - ncolumns;
    int begin_y = 1;
    WINDOW* manage_menu = newwin( console_size->ws_row - 2, ncolumns, begin_y, begin_x );
    if( manage_menu == NULL ){
        log_fatal( "error creating manage menu" );
        return WINDOW_ERR;
    }
    wbkgd( manage_menu, COLOR_PAIR( MENU_BACK ) );
    keypad( manage_menu, TRUE );

    wattron( manage_menu, A_BOLD );
    begin_x = 13, begin_y = 1;
    mvwaddstr( manage_menu, begin_y, begin_x, "MANAGE MENU" );
    begin_x = 0, begin_y = 2;
    mvwhline( manage_menu, begin_y, begin_x, ACS_HLINE, ncolumns );

    begin_y = begin_y + 2;
    show_option( manage_menu, "*CREATE A NEW CHAT: ", "F1", begin_y );
    begin_y = begin_y + 2;
    show_option( manage_menu, "*JOIN THE CHAT: ", "F2", begin_y );
    begin_y = begin_y + 2;
    show_option( manage_menu, "*LEAVE THE MESSENGER: ", "F12", begin_y );

    box( manage_menu, 0, 0 );
    wrefresh( manage_menu );
    wattroff( manage_menu, A_BOLD );
    windows->manage_menu_win = manage_menu;
    return CREATED_MANAGE_MENU;
}

ui_stat_t create_chat_background( windows_t* windows, winsize_t* console_size, char* room_name ){
    assert( windows );
    assert( console_size );
    assert( room_name );

    WINDOW* chat_back = newwin( console_size->ws_row, console_size->ws_col, 0, 0 );
    if( chat_back == NULL ){
        log_fatal( "char back creating err" );
        return WINDOW_ERR;
    }
    wbkgd( chat_back, COLOR_PAIR( REG_BACK ) );
    wrefresh( chat_back );

    create_chat_menu( windows, console_size );
    create_chat_name( windows, console_size, room_name );
    create_history_win( windows, console_size );
    create_companion_win( windows, console_size );
    create_user_win( windows, console_size );
    windows->chat_back_win = chat_back;
    return CORRECT_STATE;
}

ui_stat_t create_chat_name( windows_t* windows, winsize_t* console_size, char* room_name ){
    assert( console_size );
    assert( room_name );

    int nlines = 0;
    int ncolumns = 0;
    int begin_y = 0;
    int begin_x = 0;

    nlines = 4, ncolumns = 60, begin_x = console_size->ws_col / 3 + 15, begin_y = 0;
    WINDOW* name_win = newwin( nlines, ncolumns, begin_y, begin_x );
    if( name_win == NULL ){
        log_fatal( "window with messenger name is null ptr\n" );
        return WINDOW_ERR;
    }
    wbkgd( name_win, COLOR_PAIR( NAME_BACK ) );

    begin_x = 25, begin_y = 2;
    wattron( name_win, A_BOLD );
    // moves the cursor to position (x;y) and prints the string
    mvwprintw( name_win, begin_y, begin_x, "%s", make_uppercase_line( room_name ) );
    wattroff( name_win, A_BOLD );
    box( name_win, 0, 0 );
    wrefresh( name_win );

    windows->chat_name_win = name_win;
    return CORRECT_STATE;
}

char* make_uppercase_line( char* line ){
    assert( line );

    log_info( "%s", line );
    size_t line_index = 0;
    while( line[ line_index ] != '\0' ){
        line[ line_index ] = (char)toupper( (unsigned char)line[ line_index ] );
        ++line_index;
    }
    log_info( "%s", line );
    return line;
}

ui_stat_t create_chat_menu( windows_t* windows, winsize_t* console_size ){
    assert( windows );
    assert( console_size );

    int ncolumns = 40;
    int begin_x = console_size->ws_col - ncolumns;
    int begin_y = 0;
    WINDOW* chat_menu = newwin( console_size->ws_row, ncolumns, begin_y, begin_x );
    if( chat_menu == NULL ){
        log_fatal( "error creating manage menu" );
        return WINDOW_ERR;
    }
    wbkgd( chat_menu, COLOR_PAIR( MENU_BACK ) );
    keypad( chat_menu, TRUE );

    wattron( chat_menu, A_BOLD );
    begin_x = 13, begin_y = 1;
    mvwaddstr( chat_menu, begin_y, begin_x, "CHAT MENU" );
    begin_x = 0, begin_y = 2;
    mvwhline( chat_menu, begin_y, begin_x, ACS_HLINE, ncolumns );

    begin_y += 2;
    show_option( chat_menu, "*LEAVE THE CHAT: ", "F3", begin_y );
    begin_y += 2;
    show_option( chat_menu, "*VIEW ROOM INFORMATION: ", "F4", begin_y );
    begin_y += 2;
    show_option( chat_menu, "*VIEW MESSAGES FOR TODAY: ", "F5", begin_y );
    begin_y += 2;
    show_option( chat_menu, "*VIEW MESSAGES FROM YESTERDAY: ", "F6", begin_y );
    begin_y += 2;
    show_option( chat_menu, "*VIEW MESSAGES FOR WEEK: ", "F7", begin_y );
    begin_y += 2;
    show_option( chat_menu, "*VIEW ALL UNREAD MESSAGES: ", "F8", begin_y );
    begin_y += 2;
    show_option( chat_menu, "*SEND FILE: ", "F9", begin_y );
    begin_y += 2;
    show_option( chat_menu, "*RECOGNIZE PHOTO: ", "F10", begin_y );
    begin_y += 2;
    show_option( chat_menu, "*LEAVE THE MESSENGER: ", "F12", begin_y );

    box( chat_menu, 0, 0 );
    wrefresh( chat_menu );
    wattroff( chat_menu, A_BOLD );
    windows->chat_menu_win = chat_menu;
    return CORRECT_STATE;
}

ui_stat_t create_history_win( windows_t* windows, winsize_t* console_size ){
    assert( windows );
    assert( console_size );

    int ncolumns = 50, nlines = console_size->ws_row;
    int begin_x = 0, begin_y = 0;
    int der_ncolumns = ncolumns - 2, der_nlines = nlines - 2;
    int der_begin_x = 1, der_begin_y = 1;

    WINDOW* history_win = newwin( nlines, ncolumns, begin_y, begin_x );
    WINDOW* der_history_win = derwin( history_win, der_nlines, der_ncolumns, der_begin_y, der_begin_x );
    if( history_win == NULL || der_history_win == NULL ){
        log_fatal( "room window is null ptr" );
        return WINDOW_ERR;;
    }
    windows->chat_history_win = history_win;
    windows->der_history_win = der_history_win;

    wbkgd( history_win, COLOR_PAIR( HISTORY_BACK ) );
    wbkgd( der_history_win, COLOR_PAIR( HISTORY_BACK ) );

    wattron( der_history_win, A_BOLD );
    der_begin_x = 15, der_begin_y = 0;
    mvwaddstr( der_history_win, der_begin_y, der_begin_x, "CHAT HISTORY" );
    begin_x = 1, begin_y = 2;
    mvwhline( history_win, begin_y, begin_x, ACS_HLINE, der_ncolumns );
    wattroff( der_history_win, A_BOLD );

    box( history_win, 0, 0 );

    wrefresh( der_history_win );
    wrefresh( history_win );

    der_begin_x = 0, der_begin_y = 2;
    wmove( der_history_win, der_begin_y, der_begin_x );         // move to begining
    input_line_pos.history_x = der_begin_x;
    input_line_pos.history_y = der_begin_y;
    return CORRECT_STATE;
}

ui_stat_t create_companion_win( windows_t* windows, winsize_t* console_size ){
    assert( windows );
    assert( console_size );

    int nlines = 15, ncolumns = 60;
    int der_nlines = nlines - 2, der_ncolumns = ncolumns - 2;
    int begin_x = console_size->ws_col / 3 + 15, begin_y = console_size->ws_row / 5;
    int der_begin_x = 1, der_begin_y = 1;

    WINDOW* companion_win = newwin( nlines, ncolumns, begin_y, begin_x );
    WINDOW* der_companion_win = derwin( companion_win, der_nlines, der_ncolumns, der_begin_y, der_begin_x );
    if( companion_win == NULL || der_companion_win == NULL ){
        log_fatal( "registration window is null ptr" );
        return WINDOW_ERR;
    }
    windows->companion_win = companion_win;
    windows->der_companion_win = der_companion_win;

    wbkgd( companion_win, COLOR_PAIR( CHAT_BACK ) );
    wbkgd( der_companion_win, COLOR_PAIR( CHAT_BACK ) );
    keypad( der_companion_win, TRUE );

    der_begin_x = 15, der_begin_y = 0;
    wattron( der_companion_win, A_BOLD );
    mvwaddstr( der_companion_win, der_begin_y, der_begin_x, "MESSAGE FROM OTHER USERS");
    begin_x = 1, begin_y = 2;
    mvwhline( companion_win, begin_y, begin_x, ACS_HLINE, der_ncolumns );     // ACS_HLINE - for a solid line
    der_begin_x = 1, der_begin_y = 2;
    mvwaddstr( der_companion_win, der_begin_y, der_begin_x, "MESSAGE: " );
    wattroff( der_companion_win, A_BOLD );

    box( companion_win, 0, 0 );

    wrefresh( der_companion_win );
    wrefresh( companion_win );

    int x = -1, y = -1;                                                     // save the begining
    getyx( der_companion_win, y, x );                                       // of the iput line
    input_line_pos.companion_x = x;
    input_line_pos.companion_y = y;
    return CORRECT_STATE;
}

ui_stat_t create_user_win( windows_t* windows, winsize_t* console_size ){
    assert( windows );
    assert( console_size );

    int nlines = 15, ncolumns = 60;
    int begin_x = console_size->ws_col / 3 + 15, begin_y = console_size->ws_row / 5 + nlines * 1.5 + 5;
    int der_nlines = nlines - 2, der_ncolumns = ncolumns - 2;
    int der_begin_x = 1, der_begin_y = 1;

    WINDOW* user_win = newwin( nlines, ncolumns, begin_y, begin_x );
    WINDOW* der_user_win = derwin( user_win, der_nlines, der_ncolumns, der_begin_y, der_begin_x );
    if( user_win == NULL || der_user_win == NULL ){
        log_fatal( "registration window is null ptr" );
        return WINDOW_ERR;
    }
    windows->user_win = user_win;
    windows->der_user_win = der_user_win;

    wbkgd( user_win, COLOR_PAIR( CHAT_BACK ) );
    wbkgd( der_user_win, COLOR_PAIR( CHAT_BACK ) );
    keypad( der_user_win, TRUE );

    der_begin_x = 20, der_begin_y = 0;
    wattron( der_user_win, A_BOLD );
    mvwaddstr( der_user_win, der_begin_y, der_begin_x, "ENTER YOUR MESSAGE");
    begin_x = 1, begin_y = 2;
    mvwhline( user_win, begin_y, begin_x, ACS_HLINE, der_ncolumns );         // ACS_HLINE - for a solid line
    der_begin_x = 1, der_begin_y = 2;
    mvwaddstr( der_user_win, der_begin_y, der_begin_x, "MESSAGE: " );
    wattroff( der_user_win, A_BOLD );

    box( user_win, 0, 0 );

    wrefresh( der_user_win );
    wrefresh( user_win );

    int x = -1, y = -1;                                                     // save the begining
    getyx( der_user_win, y, x );                                            // of the iput line
    input_line_pos.user_x = x;
    input_line_pos.user_y = y;
    return CORRECT_STATE;
}

ui_stat_t create_list_win( windows_t* windows, winsize_t* console_size ){
    assert( windows );
    assert( console_size );

    int nlines = 40, ncolumns = 60;
    int begin_x = console_size->ws_col / 3 + 15, begin_y = console_size->ws_row / 5;
    int der_nlines = nlines - 2, der_ncolumns = ncolumns - 2;
    int der_begin_x = 1, der_begin_y = 1;

    WINDOW* list_win = newwin( nlines, ncolumns, begin_y, begin_x );
    WINDOW* der_list_win = derwin( list_win, der_nlines, der_ncolumns, der_begin_y, der_begin_x );
    if( list_win == NULL || der_list_win == NULL ){
        log_fatal( "registration window is null ptr" );
        return WINDOW_ERR;
    }
    windows->list_win = list_win;
    windows->der_list_win = der_list_win;

    wbkgd( list_win, COLOR_PAIR( CHAT_BACK ) );
    wbkgd( der_list_win, COLOR_PAIR( CHAT_BACK ) );
    keypad( der_list_win, TRUE );

    der_begin_x = 20, der_begin_y = 0;
    wattron( der_list_win, A_BOLD );
    mvwaddstr( der_list_win, der_begin_y, der_begin_x, "ROOM INFORMATION");
    der_begin_x = 0, der_begin_y = 1;
    mvwhline( der_list_win, der_begin_y, der_begin_x, ACS_HLINE, der_ncolumns );            // ACS_HLINE - for a solid line
    der_begin_y = 2;
    show_list_option( der_list_win, "CLOSE ROOM INFORMATION: ", "KEY UP", der_begin_y );
    der_begin_x = 0, der_begin_y = 3;
    mvwhline( der_list_win, der_begin_y, der_begin_x, ACS_HLINE, der_ncolumns );            // ACS_HLINE - for a solid line
    wattroff( der_list_win, A_BOLD );

    box( list_win, 0, 0 );

    wrefresh( der_list_win );
    wrefresh( list_win );

    der_begin_x = 0, der_begin_y = 4;
    wmove( der_list_win, der_begin_y, der_begin_x );
    return CORRECT_STATE;
}

ui_stat_t create_file_name_win( winsize_t* console_size, windows_t* windows ){
    assert( console_size );
    assert( windows );

    return file_window( console_size, windows, "ENTER THE FUL PATH TO FILE" );
}

ui_stat_t create_get_file_win( winsize_t* console_size, windows_t* windows ){
    assert( console_size );
    assert( windows );

    return file_window( console_size, windows, "NOTIFICATION" );
}

ui_stat_t create_file_path_win( winsize_t* console_size, windows_t* windows ){
    assert( console_size );
    assert( windows );

    return file_window( console_size, windows, "ENTER YOUR PATH (EXAMPLE: /home/user/)");
}

ui_stat_t file_window( winsize_t* console_size, windows_t* windows, char* info_line ){
    assert( console_size );
    assert( windows );
    assert( info_line );

    int nlines = 8, ncolumns = 60;
    int begin_x = console_size->ws_col / 3 + 15, begin_y = console_size->ws_row / 3 + 8;
    int der_nlines = nlines - 2, der_ncolumns = ncolumns - 2;
    int der_begin_x = 1, der_begin_y = 1;

    WINDOW* file_win = newwin( nlines, ncolumns, begin_y, begin_x );
    WINDOW* der_file_win = derwin( file_win, der_nlines, der_ncolumns, der_begin_y, der_begin_x );
    if( file_win == NULL || der_file_win == NULL ){
        log_fatal( "registration window is null ptr" );
        return WINDOW_ERR;
    }
    windows->file_win = file_win;
    windows->der_file_win = der_file_win;

    wbkgd( file_win, COLOR_PAIR( CHAT_BACK ) );
    wbkgd( der_file_win, COLOR_PAIR( CHAT_BACK ) );
    keypad( der_file_win, TRUE );

    der_begin_x = 15, der_begin_y = 0;
    wattron( der_file_win, A_BOLD );
    mvwprintw( der_file_win, der_begin_y, der_begin_x, "%s", info_line );
    der_begin_x = 1, der_begin_y = 1;
    mvwhline( der_file_win, der_begin_y, der_begin_x, ACS_HLINE, ncolumns );          // ACS_HLINE - for a solid line

    der_begin_x = 1, der_begin_y = 2;
    wmove( der_file_win, der_begin_y, der_begin_x );
    input_line_pos.file_x = der_begin_x;
    input_line_pos.file_y = der_begin_y;

    wattroff( der_file_win, A_BOLD );

    box( file_win, 0, 0 );

    wrefresh( der_file_win );
    wrefresh( file_win );

    return CORRECT_STATE;
}


void show_option( WINDOW* window, const char* option, const char* key, int begin_y ){
    assert( window );
    assert( option );
    assert( key );

    int begin_x = 0;
    mvwaddstr( window, begin_y, begin_x, option );
    wattron( window, COLOR_PAIR( KEY ) );
    waddstr( window, key );
    wattroff( window, COLOR_PAIR( KEY ) );
}

void show_list_option( WINDOW* window, const char* option, const char* key, int begin_y ){
    assert( window );
    assert( option );
    assert( key );

    int begin_x = 0;
    mvwaddstr( window, begin_y, begin_x, option );
    wattron( window, COLOR_PAIR( OPTION ) );
    waddstr( window, key );
    wattroff( window, COLOR_PAIR( OPTION ) );
}

void close_chat_windows( windows_t* windows ){
    assert( windows );

    close_window( windows->chat_back_win );
    close_window( windows->chat_history_win );
    close_window( windows->chat_menu_win );
    close_window( windows->chat_name_win );
    close_window( windows->companion_win );
    close_window( windows->user_win );
    close_window( windows->der_name_win );
    close_window( windows->der_companion_win );
    close_window( windows->der_user_win );
    close_window( windows->der_history_win );
}

void update_original_windows( windows_t* windows, room_position_t* room_pos ){
    assert( windows );
    assert( room_pos );

    update_window( windows->main_win );
    update_window( windows->name_win );
    update_window( windows->manage_menu_win );
    update_window( room_pos->room_win );

    WINDOW** chats_begining = rooms_info.chat_windows;
    WINDOW** current_chat = chats_begining;
    for(; current_chat < chats_begining + rooms_info.len; current_chat++ ){
        if( *current_chat != NULL ){
            update_window( *current_chat );
        }
    }
}

void update_window( WINDOW* win ){
    assert( win );

    touchwin( win );
    wrefresh( win );
}

void delete_symbol( size_t* buf_size, WINDOW* window ){
    assert(buf_size);
    assert(window);

    int x = -1;
    int y = -1;
    getyx( window, y, x );

    if( x > 0 && *buf_size > 0 ){
        waddch( window, ' ' );              // print whitespace, after command x = ( x - 1 ) + 1 = x
        wrefresh( window );
        wmove( window, y, x );              // move back 1 char
        wrefresh( window );
        --(*buf_size);
        log_info( "delete one symbol" );
    }
}

void show_companion_message( windows_t* windows, char* companion_message, size_t str_len ){
    assert( companion_message );
    assert( windows );

    int user_x = -1;
    int user_y = -1;
    getyx( windows->der_user_win, user_y, user_x );                                                 // save position in user window

    int der_companion_x = input_line_pos.companion_x;
    int der_companion_y = input_line_pos.companion_y;
    mvwprintw( windows->der_companion_win, der_companion_y, der_companion_x, "%.*s",
               str_len, companion_message );                                                        // write message from companion
    wrefresh( windows->der_companion_win );

    wmove( windows->der_user_win, user_y, user_x );                                                 // return to user window
}

void show_history( windows_t* windows, char* hst, size_t nread ){
    assert( windows );
    assert( hst );

    log_debug( "hst = %s", hst );
    log_debug( "nread = %lu", nread );

    int user_x = -1;
    int user_y = -1;
    getyx( windows->der_user_win, user_y, user_x );

    int der_hst_x = input_line_pos.history_x;
    int der_hst_y = input_line_pos.history_y;
    mvwprintw( windows->der_history_win, der_hst_y, der_hst_x, "%.*s",
               nread, hst );
    wrefresh( windows->der_history_win );

    wmove( windows->der_user_win, user_y, user_x );
}

void show_room_list( windows_t* windows, winsize_t* console_size, char* room_list, size_t nread ){
    assert( windows );
    assert( room_list );
    assert( console_size );

    create_list_win( windows, console_size );

    int x = -1;
    int y = -1;
    getyx( windows->der_list_win, y, x );

    mvwprintw( windows->der_list_win, y, x, "%.*s", nread, room_list );
    wrefresh( windows->der_list_win );
}

ui_stat_t waiting_exit( windows_t* windows ){
    assert( windows );

    nodelay( windows->der_list_win, TRUE );
    int exit_symbol = wgetch( windows->der_list_win );
    log_debug( "key in room info window: %d", exit_symbol );
    if( exit_symbol == KEY_UP ){
        return CLOSE_ROOM_LIST;
    }
    return WAITING_ERR;
}

void file_accept_request( WINDOW* file_win, char* file_name ){
    assert( file_win );

    int x = input_line_pos.file_x - 1;
    int y = input_line_pos.file_y;

    mvwprintw( file_win, y, x, "THE USER OF THE ROOM WANTS TO SHARE A FILE WITH YOU!\n"
                               "FILE NAME: %s\n", file_name );
    show_list_option( file_win, "START DOWNLOADING? ", "Y/N", y + 2 );

    wrefresh( file_win );
}

void waiting_download_win( windows_t* windows ){
    assert( windows );

    int x = input_line_pos.file_x;
    int y = input_line_pos.file_y;

    mvwaddstr( windows->der_file_win, y, x, "STATUS: DOWNLOAD..." );
    wrefresh( windows->der_file_win );

    int default_x = input_line_pos.user_x;
    int default_y = input_line_pos.user_y;

    wmove( windows->der_user_win, default_y, default_x );		// returned to user input line
}

void download_complete( windows_t* windows ){
    assert( windows  );

    clear_file_line( windows->der_file_win );
    int file_x = 0;
    int file_y = input_line_pos.file_y;
    mvwaddstr( windows->der_file_win, file_y, file_x, "FILE DOWNLOADED SUCCESSFULLY!\n" );
    show_list_option( windows->der_file_win, "CLOSE NOTIFICATION WINDOW: ", "C", file_y + 1 );

    wrefresh( windows->der_file_win );
}

void dispatch_notification( WINDOW* file_win, unsigned long accepted, unsigned long all ){
    assert( file_win );

    clear_file_line( file_win );
    int file_x = 0;
    int file_y = input_line_pos.file_y;
    mvwprintw( file_win, file_y, file_x, "%lu OUT OF %lu USERS RECIEVED FILE, REST REFUSED\n", accepted, all );
    show_list_option( file_win, "CLOSE NOTOFOCATION WINDOW: ", "C", file_y + 1 );

    wrefresh( file_win );
}

void close_room_info( windows_t* windows ){
    assert( windows );

    close_window( windows->der_list_win );
    close_window( windows->list_win );

    update_chat_windows( windows );
}

void close_file_windows( windows_t* windows ){
    assert( windows );

    close_window( windows->der_file_win );
    close_window( windows->file_win );

    update_chat_windows( windows );
}


void update_chat_windows( windows_t* windows ){
    assert( windows );

    update_window( windows->chat_back_win );
    update_window( windows->chat_name_win );
    update_window( windows->chat_menu_win );
    update_window( windows->chat_history_win );
    update_window( windows->companion_win );
    update_window( windows->der_companion_win );
    update_window( windows->user_win );
    update_window( windows->der_user_win );
}

void clear_input_line( WINDOW* win, ui_stat_t state ){
    assert( win );

    int x = -1, y = -1;
    if( state == USER_ENTERS ){
        x = input_line_pos.user_x;
        y = input_line_pos.user_y;
    }
    else if( state == COMPANION_ENTERS ){
        x = input_line_pos.companion_x;
        y = input_line_pos.companion_y;
    }
    wmove( win, y, x );

    const int ncolumns = 60;
    const int nlines = 15;
    // clear the user line after entering a message
    // max column: ncolumns - 1  - 1 - strlen( "MESSAGE: ") = 60 - 1 - 1 - 9 = 49
    mvwhline( win, y, x, ' ', 48 );                                          // ACS_HLINE - for a solid line

    int current_line = y + 1, current_column = 0;
    for(; current_line < nlines; current_line++ ){
        mvwhline( win, current_line, current_column, ' ', ncolumns );
    }
    wmove( win, y, x );                                                      // return to begining of the line
    wrefresh( win );
}

void clear_history_line( WINDOW* win, winsize_t* console_size ){
    assert( win );
    assert( console_size );

    int x = input_line_pos.history_x;
    int y = input_line_pos.history_y;

    wmove( win, y, x );

    const int ncolumns = 50;
    const int nlines = console_size->ws_row;

    int current_line = y, current_column = 0;
    for(; current_line < nlines; current_line++ ){
        mvwhline( win, current_line, current_column, ' ', ncolumns );
    }

    wmove( win, y, x );
    wrefresh( win );
}

void clear_file_line( WINDOW* win ){
    assert( win );

    int x = input_line_pos.file_x;
    int y = input_line_pos.file_y;

    wmove( win, y, x );

    const int ncolumns = 60;
    const int nlines = 6;

    int current_line = y, current_column = 0;
    for(; current_line < nlines; current_line++ ){
        mvwhline( win, current_line, current_column, ' ', ncolumns );
    }

    wmove( win, y, x );
    wrefresh( win );
}

void close_window( WINDOW* window ){
    if( window == NULL ){
        log_warning( "you want to delete an uncreated window" );
        return ;
    }

    wclear( window );                                                       // clear data
    wbkgd( window, COLOR_PAIR( BLACK_BACK ) );                              // background cleaning
    wrefresh( window );
    delwin( window );

}

void destroy_background(){

    clear();                                                               // disabling of attributes
    endwin();
}

void destroy_user( user_info_t* user_data ){
    if( user_data == NULL ){
        return ;
    }

    free( user_data->ip );
    free( user_data );
}
