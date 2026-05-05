#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <unistd.h>
#include <uv.h>

#include "callbacks.h"
#include "logger.h"
#include "network_functions.h"
#include "user_interface.h"
#include "sending_files.h"

static const bool CLOSE_WINDOW = true;
static const bool NOT_CLOSE_WINDOW = false;
static const size_t ONE = 1;
static const int DEC = 10;
static const size_t INIT_BUF_LEN = 10;

srv_command_t srv_commands[] = {
    { "/recipients_ID"      ,       connect_recipient       }
};
size_t command_count = sizeof(srv_commands) / sizeof(srv_command_t);

// global vars for user interface
static winsize_t* console_size;
static windows_t* windows;
static user_info_t* user_data;
static room_position_t* room_pos;

user_info_t* start_registration(){

    console_size = get_console_size();
    windows = create_background( console_size );
    user_data = client_registration( console_size, windows );
    log_info( "IP: %s", user_data->ip );
    close_window( windows->reg_win );

    return user_data;
}

// req - request
void connect_cb(uv_connect_t* req, int status ){
    assert( req );

    if( status < 0 ){
        log_fatal( "connect callback get negative status" );
        return ;
    }

    room_pos = create_room_list( console_size );
    status = create_menu( windows, console_size );

    client_t* client = (client_t*)calloc( ONE, sizeof(client_t) );
    if( client == NULL ){
        log_fatal( "calloc return null ptr\n" );
        return ;
    }
    client_init( client );
    client->handle = (uv_tcp_t*)req->handle;
    client->handle->data = client;                                                                  // save client info in free field of the struct
    int read_server = uv_read_start( (uv_stream_t*)client->handle, alloc_srv_cb, read_srv_cb );     // req->handle - file descriptor wrapper
    if( read_server < 0 ){
        log_warning( "uv_read_start return negative value" );
        destroy_client( client );
        return ;
    }

    client->stdin_handle->data = client;
    client->app_state = MENU;
    int poll_status = uv_poll_init( req->handle->loop, client->stdin_handle, STDIN_FILENO );
    if( poll_status < 0 ){
        log_fatal( "poll init return file description initialization error" );
        destroy_client( client );
        return ;
    }
    int read_screen = uv_poll_start( client->stdin_handle, UV_READABLE, poll_cb );
    if( read_screen < 0 ){
        log_fatal( "uv_poll_start return negative value" );
        destroy_client( client );
        return ;
    }
    log_info( "read start" );
}

void client_init( client_t* client ){
    assert( client );

    client->stdin_handle = (uv_poll_t*)calloc( ONE, sizeof(uv_poll_t) );
    client->is_stopped = false;
    client->stopped_file_ch = false;
    client->write_bytes = 0;
    client->offset = 0;
    client->file_capacity = 0;
    client->file_data = NULL;
    client->full_path = NULL;
    client->chunk_data = NULL;
    client->active_writes = 0;;
    // init server buf
    client->srv_buf = NULL;
    client->srv_buf_len = 0;
    client->srv_buf_cap = 0;
    // init file buf
    client->file_buf = NULL;
    client->file_buf_len = 0;
    client->file_buf_cap = 0;
    // init screen buf
    client->scr_buf_len = 0;
    client->scr_buf_cap = INIT_BUF_LEN;
    client->scr_buf = (char*)calloc( INIT_BUF_LEN, sizeof(char) );
}

void alloc_srv_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    client_t* client = (client_t*)handle->data;
    client_err_t client_err = NO_ERR;
    size_t predicted_size = client->srv_buf_len + suggested_size + 1;
    if( client->srv_buf_cap > 0 ){
        if( ( client_err = memory_reallocation( client, predicted_size ) ) != NO_ERR ){
            return ;
        }
    }
    else{
        client->srv_buf = (char*)calloc( predicted_size, sizeof(char) );
        if( !client->srv_buf ){
            log_fatal( "can not allocate memory for client buf" );
            return ;
        }
    }

    client->srv_buf_cap = client->srv_buf_cap >= predicted_size
                          ? client->srv_buf_cap
                          : predicted_size;
    buf->base = client->srv_buf + client->srv_buf_len;                                    // ptr of free place
    buf->len = client->srv_buf_cap - client->srv_buf_len - 1;                             // maximum bytes for reading
}

client_err_t memory_reallocation( client_t* client, size_t predicted_size ){
    assert( client );

    char* realloc_buf = NULL;
    if( client->srv_buf_cap < predicted_size ){
        realloc_buf = (char*)realloc( client->srv_buf, predicted_size * sizeof(char) );
        if( !realloc_buf ){
            log_fatal( "realloc return NULL ptr" );
            return MEMORY_ERR;
        }
        client->srv_buf = realloc_buf;
    }
    return NO_ERR;
}

void read_srv_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf ){
    assert( handle );
    assert( buf );

    int status = 0;
    client_t* client = (client_t*)handle->data;                             // restoring the client's structure
    if( nread < 0 ){
        log_warning( "server closed connection" );
        uv_shutdown_t* shutdown_req = (uv_shutdown_t*)calloc( ONE, sizeof(uv_shutdown_t) );
        if( ( status = uv_shutdown( shutdown_req, handle, shutdown_cb ) ) < 0 ){
            free( shutdown_req );
        }
        return;
    }

    client->srv_buf_len += nread;
    client->srv_buf[client->srv_buf_len] = '\0';
    show_server_message( client, client->srv_buf, client->srv_buf_len );
}

void poll_cb( uv_poll_t* handle, int status, int events ){
    assert( handle );

    if( status < 0 ){
        log_error( "poll callback get negative status" );
        return ;
    }

    client_t* client = (client_t*)handle->data;
    ui_stat_t state = CORRECT_STATE;
    int available_symbol = -1;
    switch( client->app_state ){
        case MENU:
            state = parse_request( windows->menu_win, user_data, CLOSE_WINDOW, &available_symbol );
            check_ui_state( handle, state, client );
            break;
        case GET_ROOM_NAME:
            read_name( client, CREATE_ROOM, room_name );
            check_app_state( handle, client );
            break;
        case MANAGE_MENU:
            state = parse_request( windows->manage_menu_win, user_data, NOT_CLOSE_WINDOW, &available_symbol );
            check_ui_state( handle, state, client );
            break;
        case JOIN_ROOM_NAME:
            read_name( client, JOIN_CHAT, room_name );
            check_app_state( handle, client );
            break;
        case USER_ACTION:
            state = parse_request( windows->der_user_win, user_data, NOT_CLOSE_WINDOW, &available_symbol );
            check_ui_state( handle, state, client );
            read_key( client, state, handle, &available_symbol );
            break;
        case WAITING_EXIT:
            state = waiting_exit( windows );
            check_ui_state( handle, state, client );
            break;
        case READ_PATH:
            read_name( client, CONNECT_SENDER, file_path );
            check_app_state( handle, client );
            break;
        case WAITING_ID:
            break;
        case WRITE_HISTORY:
            break;
        case FILE_REQUEST:
            state = file_request( windows->der_file_win );
            check_ui_state( handle, state, client );
            break;
        case READ_NEW_PATH:
            read_name( client, CREATE_FILE, file_path );
            check_app_state( handle, client );
	    break;
	case COMPLETE_DOWNLOAD:
	    state = file_request( windows->der_file_win );
	    check_ui_state( handle, state, client );
	    break;
	case WAIT_DISPATCH_COMPLET:
	    state = file_request( windows->der_file_win );
	    check_ui_state( handle, state, client );
	    break;
	case READ_BOT_PATH:
	    read_name( client, SEND_BOT_PATH, file_path );
	    check_app_state( handle, client );
	    break;
	case BOT_RESPONSE:
	    break;
        default:
            break;
    }
}

void check_ui_state( uv_poll_t* handle, ui_stat_t state, client_t* client ){
    assert( client );
    assert( handle );

    ui_stat_t new_state = CORRECT_STATE;
    switch( state ){
        case NO_REQUEST:
            log_warning( "user interface did not detect recognized" );
            break;
        case CLOSE_MESSENGER:
            leave_messenger( (uv_handle_t*)handle );
            log_info( "user close messenger" );
            break;
        case START_CREATE_ROOM:
            create_room_name_win( console_size, windows );
            client->app_state = GET_ROOM_NAME;
            break;
        case FIN_CREATE_ROOM:
            new_state = create_manage_menu( windows, console_size );
            check_ui_state( handle, new_state, client );
            break;
        case CREATED_MANAGE_MENU:
            client->app_state = MANAGE_MENU;
            break;
        case START_JOIN_CHAT:
            create_room_name_win( console_size, windows );
            client->app_state = JOIN_ROOM_NAME;
            break;
        case START_LEAVE_CHAT:
            leave_chat( client );
            client->app_state = MANAGE_MENU;
            break;
        case READ_MESSAGE:
            client->app_state = USER_ACTION;
            break;
        case START_ROOM_LIST:
            room_list( client );
            break;
        case CLOSE_ROOM_LIST:
            close_room_info( windows );
	    clear_input_line( windows->der_companion_win, COMPANION_ENTERS );
            show_companion_message( windows, client->srv_buf, client->srv_buf_len );
            clean_srv_buf( client );
            client->app_state = USER_ACTION;
            break;
        case VIEW_TODAY_HISTORY:
            view_history( client, view_today_hst );
            break;
        case VIEW_YESTERDAY_HISTORY:
            view_history( client, view_yesterday_hst );
            break;
        case VIEW_WEEK_HISTORY:
            view_history( client, view_week_hst );
            break;
        case VIEW_ALL_UNREAD:
            view_history( client, view_unread_message );
            break;
        case START_SEND_FILE:
            create_file_name_win( console_size, windows );
            client->app_state = READ_PATH;
            break;
        case REQUEST_ACCEPTED:
            close_window( windows->der_file_win );
            create_file_path_win( console_size, windows );
            client->app_state = READ_NEW_PATH;
            break;
        case REQUEST_NOT_ACCEPTED:
            close_file_windows( windows );
            //request_not_accepted( client );
	    client->app_state = USER_ACTION;
            break;
	case FINISH_DOWNLOAD:
	    close_file_windows( windows );
	    clear_input_line( windows->der_companion_win, COMPANION_ENTERS );
	    show_companion_message( windows, client->srv_buf, client->srv_buf_len );
	    clean_srv_buf( client );
	    client->app_state = USER_ACTION;
	    break;
	case START_RECOGNIZE_PHOTO:
	    create_file_name_win( console_size, windows );
	    client->app_state = READ_BOT_PATH;
	    break;
        default:
            break;
    }
}

void check_app_state( uv_poll_t* handle, client_t* client ){
    assert( client );
    assert( handle );

    ui_stat_t state = CORRECT_STATE;
    switch( client->app_state ){
        case CREATE_ROOM:
            close_window( windows->room_name_win );
            state = create_room( room_pos, console_size, client->scr_buf );
            clean_scr_buf( client );
            check_ui_state( handle, state, client );
            break;
        case JOIN_CHAT:
            close_window( windows->room_name_win );
            join_room( client );
            clean_scr_buf( client );
            client->app_state = USER_ACTION;
            break;
        case SEND_MESSAGE:
            send_message( client );
            break;
	case CONNECT_SENDER:
	    connect_sender( client );
	    break;
        case SEND_PATH:
            send_path( client );
            break;
        case CREATE_FILE:
	    create_receiver_file( client );
	    break;
	case SEND_BOT_PATH:
	    send_bot_path(client);
	    break;
        default:
            break;
    }
}

void join_room( client_t* client ){
    assert( client );

    if( check_connect_possibility( client->scr_buf ) != CORRECT_STATE ){
        log_error( "incorrectly entered name" );
        return ;
    }

    server_send( client, "/join %s\n", client->scr_buf );
    log_info( "client join room: %s", client->scr_buf );
    create_chat_background( windows, console_size, client->scr_buf );
}

void leave_chat( client_t* client ){
    assert( client );

    close_chat_windows( windows );
    server_send( client, "/leave\n" );
    update_original_windows( windows, room_pos );
}

void room_list( client_t* client ){
    assert( client );

    server_send( client, "/list\n" );
    client->app_state = ROOM_LIST;
}

void read_key( client_t* client, ui_stat_t state, uv_poll_t* handle, int* available_symbol ){
    assert( client );
    assert( handle );
    assert( available_symbol );

    if( state == READ_MESSAGE ){
        read_message( client, SEND_MESSAGE, *available_symbol );
        check_app_state( handle, client );
        *available_symbol = -1;                                           // updating read symbol
    }
}

void view_history( client_t* client, void(*func)( client_t* client ) ){
    log_debug( "ENTER VIEW HISTORY" );
    assert( client );

    func( client );

    client->app_state = WRITE_HISTORY;
    log_debug( "EXIT VIEW HISTORY" );
}

void view_today_hst( client_t* client ){
    assert( client );

    server_send( client, "/today\n" );
}

void view_yesterday_hst( client_t* client ){
    assert( client );

    server_send( client, "/yesterday\n" );
}

void view_week_hst( client_t* client ){
    assert( client );

    server_send( client, "/week\n" );
}

void view_unread_message( client_t* client ){
    assert( client );

    server_send( client, "/history\n" );
}

void send_path( client_t* client ){
    assert( client );

    client->sender_path = strdup( client->scr_buf );                  // save file path: home/documents/main.txt

    char* file_name = get_file_name( client->scr_buf );
    server_send( client, "/file %s\n", file_name );
    log_info( "file path: '%s'", client->sender_path );

    clean_scr_buf( client );
    clear_file_line( windows->der_file_win );
    client->app_state = WAITING_ID;
}

void send_bot_path( client_t* client ){
    assert( client  );

    log_info( "path of bot file = '%s'", client->scr_buf );
    server_send( client, "/bot_file %s\n", client->scr_buf );

    clean_scr_buf( client );
    clear_file_line( windows->der_file_win );
    client->app_state = BOT_RESPONSE;
}

char* get_file_name( char* path ){
    assert( path );

    char* find_name = NULL;
    char* name_begining = path;
    while( ( find_name = strchr( name_begining, '/' ) ) != NULL ){
        name_begining = find_name + 1;
        log_debug( "get file name: %s", name_begining );
    }

    log_debug( "file name from path: %s", name_begining );
    return name_begining;
}

void send_message( client_t* client ){
    assert( client );

    server_send( client, "%s\n", client->scr_buf );
    log_info( "client send message: %s", client->scr_buf );

    clean_scr_buf( client );

    client->app_state = USER_ACTION;
}

void leave_messenger( uv_handle_t* handle ){
    assert( handle );

    destroy_interface_data( user_data );
    if( !uv_is_closing( handle ) ){
        uv_close( handle, close_cb );
    }
}

void server_send( client_t* client, const char* format, ... ){
    assert( client );
    assert( format );

    uv_buf_t buffer = {};
    FILE* stream = open_memstream( &buffer.base, &buffer.len );                                         //creating a stream for recording
    va_list args = {};
    va_start( args, format );
    vfprintf( stream, format, args );
    fclose( stream );
    va_end( args );

    uv_write_t* req = (uv_write_t*)calloc( ONE, sizeof(uv_write_t) );
    req->data = buffer.base;
    if( uv_write( req, (uv_stream_t*)(client->handle), &buffer, ONE, write_cb ) < 0 ){                 //writing data to descriptor
        log_fatal( "write return negative value" );
        free( buffer.base );
        free( req );
    }
}

void read_name( client_t* client, app_state_t app_state,  bool(*func)( client_t* client, int* key ) ){
    assert( client );

    char* realloc_buf = NULL;
    if( client->scr_buf_len == client->scr_buf_cap - 2 ){
        client->scr_buf_cap *= 2;
        realloc_buf = (char*)realloc( client->scr_buf, client->scr_buf_cap * sizeof(char) );
        if( !realloc_buf ){
            log_fatal( "realloc return NULL ptr" );
            return;
        }
        client->scr_buf = realloc_buf;
    }

    int key = 0;

    if( func( client, &key ) ){
        return ;
    }
    if( key != '\n' && key != KEY_ENTER ){
        client->scr_buf[ client->scr_buf_len++ ] = key;
        return;
    }
    client->scr_buf[ client->scr_buf_len++ ] = '\0';
    client->app_state = app_state;
}

bool room_name( client_t* client, int* key ){
    assert( client );
    assert( key );

    if( ( *key = wgetch( windows->der_name_win ) ) == KEY_BACKSPACE ){
        delete_symbol( &client->scr_buf_len, windows->der_name_win );
        return true;
    }
    return false;
}

bool file_path( client_t* client, int* key ){
    assert( client );
    assert( key );

    if( ( *key = wgetch( windows->der_file_win ) ) == KEY_BACKSPACE ){
        delete_symbol( &client->scr_buf_len, windows->der_file_win );
        return true;
    }
    return false;
}

void read_message( client_t* client, app_state_t app_state, int available_symbol ){
    assert( client );

    char* realloc_buf = NULL;
    if( client->scr_buf_len == client->scr_buf_cap - 2 ){
        client->scr_buf_cap *= 2;
        realloc_buf = (char*)realloc( client->scr_buf, client->scr_buf_cap * sizeof(char) );
        if( !realloc_buf ){
            log_fatal( "realloc return NULL ptr" );
            return;
        }
        client->scr_buf = realloc_buf;
    }

    if( available_symbol == KEY_BACKSPACE || available_symbol == 127 || available_symbol == '\b' ){
        delete_symbol( &client->scr_buf_len, windows->der_user_win );
        return ;
    }
    if( available_symbol != '\n' && available_symbol != KEY_ENTER ){
        client->scr_buf[ client->scr_buf_len++ ] = available_symbol;
        return ;
    }

    client->scr_buf[ client->scr_buf_len++ ] = '\0';
    client->app_state = app_state;
    clear_input_line( windows->der_user_win, USER_ENTERS );
}

void show_server_message( client_t* client, char* server_message, size_t nread ){
    assert( client );
    assert( server_message );

    log_debug( "Message from server = '%s'", server_message );

    if( find_srv_command( client, server_message ) ){
        clean_srv_buf( client );
        return ;
    }

    switch( client->app_state ){
        case ROOM_LIST:
            log_debug( "start waiting exit" );
            client->app_state = WAITING_EXIT;                                              // blocked reading messages
            show_room_list( windows, console_size, server_message, nread );
            clean_srv_buf( client );
            log_debug( "finish waiting exit" );
            break;
        case WAITING_EXIT:
            log_debug( "now waiting exit");
            break;
        case WRITE_HISTORY:
            clear_history_line( windows->der_history_win, console_size );
            show_history( windows, server_message, nread );
            clean_srv_buf( client );
            client->app_state = USER_ACTION;
            break;
	case COMPLETE_DOWNLOAD:
	    log_debug( "now waiting download complete" );
	    break;
        default:
            clear_input_line( windows->der_companion_win, COMPANION_ENTERS );
            show_companion_message( windows, server_message, nread );
            clean_srv_buf( client );
            break;
    }
}

bool find_srv_command( client_t* client, char* server_message ){
    assert( client );
    assert( server_message );

    size_t command_index = 0;
    char* command_name = NULL;
    for(; command_index < command_count; command_index++ ){
        command_name = srv_commands[command_index].command_name;
        if( strncmp( server_message, command_name, strlen( command_name ) ) == 0 ){
            srv_commands[command_index].func( client, server_message );
            return true;
        }
    }

    log_warning( "command was not found" );
    return false;
}

void connect_recipient( client_t* client, char* command_line ){
    assert( client );
    assert( command_line );
    
    char* first_wh = strchr( command_line, ' ' );
    char* second_wh = strchr( first_wh + 1, ' ' );
    *second_wh = '\0';
    client->transfer_id = strtoul( first_wh + 1, NULL, DEC );
    char* file_name = strdup( second_wh + 1 );
    client->file_name = file_name;

    client->app_state = FILE_REQUEST;
    client->client_type = RECEIVER;

    create_get_file_win( console_size, windows );
    file_accept_request( windows->der_file_win, file_name );
}

void connect_sender( client_t* client ){
    assert( client );

    client->sender_path = strdup( client->scr_buf );                  // save file path: home/documents/main.txt
    char* file_name = get_file_name( client->scr_buf );		      // find file name: main.txt
    client->file_name = strdup( file_name );                          // save file name
    client->client_type = SENDER;				      // SENDER or RECEIVER

    log_info( "file path: '%s'", client->sender_path );

    clean_scr_buf( client );
    clear_file_line( windows->der_file_win );

    connecting_file_channel( client );
    client->app_state = FILE_REQUEST;
}

void connecting_file_channel( client_t* client ){
    assert( client );

    main_struct_t main_struct = { windows, console_size, user_data };
    main_connection_t main_connection = { client->handle->loop, client };
    open_new_connection( &main_struct, &main_connection );
}

void create_receiver_file( client_t* client ){
    assert( client );

    client->receiver_path = strdup( client->scr_buf );

    close_file_windows( windows );
    create_get_file_win( console_size, windows );
    waiting_download_win( windows );
    clean_scr_buf( client );
    connecting_file_channel( client );
    client->app_state = COMPLETE_DOWNLOAD;
}

void clean_scr_buf( client_t* client ){
    assert( client );

    memset( client->scr_buf, '\0', client->scr_buf_len );
    client->scr_buf_len = 0;
}

void clean_srv_buf( client_t* client ){
    assert( client );
    
    if( client->srv_buf ){
	memset( client->srv_buf, '\0', client->srv_buf_len );
        client->srv_buf_len = 0;
    }
}

void write_cb( uv_write_t* req, int status ){
    assert( req );

    if( status < 0 ){
        log_error( "can not write message for server" );
        free( req->data );
        free( req );
        return ;
    }

    client_t* client = (client_t*)req->handle->data;
    if( client->is_stopped ){
        destroy_interface_data( user_data );
        uv_close( (uv_handle_t*)client->handle, close_cb );
    }

    free( req->data );
    free( req );
}

void shutdown_cb( uv_shutdown_t* req, int status ){
    assert( req );

    if( status < 0 ){
        free( req );
        log_error( "shutdown callback get negative status" );
    }
    if( !uv_is_closing( (uv_handle_t*)req->handle ) ){
        uv_close( (uv_handle_t*)req->handle, close_cb );
    }
    free( req );
}

void close_cb( uv_handle_t* handle ){
    assert( handle );

    client_t* client = (client_t*)handle->data;                  // restoring the client's struct
    uv_stop( handle->loop );
    if( client->stdin_handle ){
        free( client->stdin_handle );
    }

    destroy_client( client );
}

void destroy_interface_data( user_info_t* client_data ){

    if( console_size )      free( console_size );
    if( windows)            free( windows );
    if( room_pos )          free( room_pos );

    destroy_user( client_data );
}

void destroy_client( client_t* client ){
    if( client == NULL ){
        return ;
    }

    if( client->scr_buf ){
        free( client->scr_buf );
        client->scr_buf = NULL;
    }
    if( client->srv_buf ){
        free( client->srv_buf );
        client->srv_buf = NULL;
    }
    if( client->file_buf ){
        free( client->file_buf );
        client->file_buf = NULL;
    }
    if( client->sender_path ){
	free( client->sender_path );
	client->sender_path = NULL;
    }
    if( client->receiver_path ){
	free( client->receiver_path );
	client->receiver_path = NULL;
    }
    if( client->file_name ){
	free( client->file_name );
	client->file_name = NULL;
    }
    if( client ){
        free( client );
    }
}
