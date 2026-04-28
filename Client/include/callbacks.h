#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <stdbool.h>

#include "network_functions.h"
#include "user_interface.h"

user_info_t* start_registration();

void get_background();

void init_console_fd( uv_loop_t* loop );

void connect_cb( uv_connect_t* req, int status );

void alloc_srv_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

client_err_t memory_reallocation( client_t* client, size_t predicted_size );

void read_srv_cb( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf );

void client_init( client_t* client );

void server_send( client_t* client, const char* format, ... );

void shutdown_cb( uv_shutdown_t* req, int status );

void close_cb( uv_handle_t* handle );

void write_cb( uv_write_t* req, int status );

void destroy_client( client_t* client );

void poll_cb( uv_poll_t* handle, int status, int events );

void check_ui_state( uv_poll_t* handle, ui_stat_t state, client_t* client );

void leave_messenger( uv_handle_t* handle );

void check_app_state( uv_poll_t* handle, client_t* client );

void read_name( client_t* client, app_state_t app_state, bool(*func)( client_t* client, int* key ) );

bool room_name( client_t* client, int* key );

bool file_path( client_t* client, int* key );

void read_message( client_t* client, app_state_t app_state, int available_symbol );

void show_server_message( client_t* client, char* server_message, size_t nread );

void join_room( client_t* client );

void leave_chat( client_t* client );

void room_list( client_t* client );

void send_path( client_t* client );

void send_bot_path( client_t* client );

bool find_srv_command( client_t* client, char* server_message );

void connect_recipient( client_t* client, char* command_line );

void connect_sender( client_t* client, char* command_lie );

void connecting_file_channel( client_t* client, char* command_line, client_type_t client_type );

void send_message( client_t* client );

void read_key( client_t* client, ui_stat_t state, uv_poll_t* handle, int* available_symbol );

void view_history( client_t* client, void(*func)( client_t* client ) );

void view_today_hst( client_t* client );

void view_yesterday_hst( client_t* client );

void view_week_hst( client_t* client );

void view_unread_message( client_t* client );

void send_file_name( client_t* client );

char* get_file_name( char* path );

void clean_scr_buf( client_t* client );

void clean_srv_buf( client_t* client );

void destroy_interface_data( user_info_t* client_data );

#endif
