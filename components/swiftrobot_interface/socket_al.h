#ifndef socket_al_h
#define socket_al_h

#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"

#define PORT CONFIG_MASTER_PORT

typedef struct connection_data {
    int socket_fd;
    uint32_t ip;
} connection_data_t;

/// @brief creates a socket client to swiftrobot master
void create_socket(void*);

int close_socket(int socket_fd);

/// @brief sends image buffer via the swiftrobot protocol
/// @param socket_fd 
/// @param image_buffer 
/// @param image_buffer_length 
/// @return sent bytes or -1 on error
int send_image_buffer(int socket_fd, char* image_buffer, size_t image_buffer_length);

#endif