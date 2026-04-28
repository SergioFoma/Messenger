#ifndef NETWORK_FUNCTIONS_H
#define NETWORK_FUNCTIONS_H

#include <stdbool.h>
#include <uv.h>

typedef enum app_state_e {
    UNDEFINED                   =   0,
    MENU                        =   1,
    GET_ROOM_NAME               =   2,
    CREATE_ROOM                 =   3,
    MANAGE_MENU                 =   4,
    JOIN_ROOM_NAME              =   5,
    JOIN_CHAT                   =   6,
    USER_ACTION                 =   7,
    SEND_MESSAGE                =   8,
    ROOM_LIST                   =   9,
    WAITING_EXIT                =   10,
    WRITE_HISTORY               =   11,
    READ_PATH                   =   12,
    SEND_PATH                   =   13,
    WAITING_ID                  =   14,
    FILE_REQUEST                =   15,
    READ_NEW_PATH               =   16,
    CREATE_FILE                 =   17,
    COMPLETE_DOWNLOAD		=   18,
    WAIT_DISPATCH_COMPLET	=   19,
    READ_BOT_PATH		=   20,
    SEND_BOT_PATH		=   21,
    BOT_RESPONSE		=   22
} app_state_t;

typedef enum client_err_t {
    NO_ERR          =       0,
    MEMORY_ERR      =       1
} client_err_t;

typedef enum client_type_s {
    RECEIVER    	=   0,
    SENDER      	=   1,
    UNDEFINED_TYPE	=   2
} client_type_t;

typedef struct client_info_s {
    uv_tcp_t* handle;
    uv_tcp_t* file_handle;
    uv_poll_t* stdin_handle;
    char* file_name;                // name of the received/sent file
    char* sender_path;              // name and path, that entered sender
    char* receiver_path;            // name and path, that entered receiver
    int file_fd;                    // file descriptor on file that is being downloaded/sent
    char* srv_buf;                  // buffer for data from server
    char* scr_buf;                  // buffer for data from screen
    char* file_buf;                 // buffer for file data
    unsigned long transfer_id;      // to send and receive files
    size_t srv_buf_cap;
    size_t srv_buf_len;
    size_t scr_buf_cap;
    size_t scr_buf_len;
    size_t file_buf_len;
    size_t file_buf_cap;
    size_t write_bytes;		    // for receiver
    size_t sent_bytes;              // for sender
    size_t file_capacity;           // for sender
    app_state_t app_state;          // what awaits the application now
    client_type_t client_type;      // for joined callback
    bool is_stopped;
    bool stopped_file_ch;
} client_t;

typedef struct main_connection_s {
    uv_loop_t* loop;
    client_t* client;
} main_connection_t;

typedef struct srv_command_s {
    char* command_name;
    void (*func)( client_t* client, char* command_line );
} srv_command_t;

#endif
