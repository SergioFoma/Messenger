#ifndef FILE_COMMANDS_H
#define FILE_COMMANDS_H

#include <uv.h>

#include "logging.h"
#include "server_functions.h"

typedef struct file_transfer_s {
    uv_tcp_t* sender_handle;
    uv_tcp_t** recipient_handles;
    uv_tcp_t** open_files;
    const char* file_name;
    unsigned long transfer_id;
    size_t open_files_cap;
    size_t recipients_capacity;
    size_t recipients_count;
    size_t accepted_number;
    size_t not_accepted_number;
} file_transfer_t;

error init_transfers();

error add_transfer( unsigned long transfer_id, const char* file_name );

ssize_t find_free_transfer();

error init_one_transfer( file_transfer_t** transfer, unsigned long transfer_id, const char* fine_name );

ssize_t realloc_transfers( ssize_t free_index );

ssize_t realloc_recipients( file_transfer_t* transfer, ssize_t free_index );

void connect_file_channel( uv_stream_t* server, int status );

void file_alloc_cb( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf );

error realloc_file_buf( client_t* client, size_t suggested_size );

void read_file_ch( uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf );

void parse_message( client_t* client, ssize_t nread, void (*on_cmd)( client_t* client, const char* string ) );

void parse_instruction( client_t* client, const char* string );

error send_file_part( client_t* client, const char* string );

error init_recipient( client_t* client, const char* string );

error add_recipient( uv_tcp_t* recipient_handle, unsigned long transfer_id, file_transfer_t** current_transfer  );

error init_sender( client_t* client, const char* string );

error add_sender( uv_tcp_t* sender_handle, unsigned long transfer_id );

error send_agreement( client_t* client, const char* string  );

error add_open_file( client_t* client, file_transfer_t* file_transfer );

ssize_t find_free_place( file_transfer_t* file_transfer );

ssize_t check_enough_memory( file_transfer_t* file_transfer, ssize_t free_index );

error get_refusal( client_t* client, const char* string );

error check_download_responses( file_transfer_t* current_transfer );

void shutdown_channel( uv_shutdown_t* shutdown_req, int status );

void delete_client( client_t* client );

file_transfer_t* find_transfer( uv_tcp_t* client_handle );

file_transfer_t* find_id( unsigned long transfer_id );

ssize_t find_free_elem( file_transfer_t* file_transfer );

file_transfer_t* find_recipient( file_transfer_t* current_transfer, uv_tcp_t* client_handle );

void destroy_transfers();

error destroy_channel( client_t* client, const char* string );

void destroy_transfer( file_transfer_t** file_transfer );

void close_sockets( file_transfer_t* file_transfer );

void send_file_data( uv_tcp_t* file_handle, const char* format, ... );

void record_cb( uv_write_t* write_req, int status );

void finish_cb( uv_handle_t* handle );

#endif
