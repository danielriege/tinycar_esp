#include "socket_al.h"

void create_socket(void* pvParameters) {
    connection_data_t* conn_data = (connection_data_t*)pvParameters;
    int addr_family = 0;
    int ip_protocol = 0;
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = conn_data->ip;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    while(1) {
        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            // if socket cant be created, it does not matter if we try again 1 second later
            ESP_LOGE("tinycar:swiftrobot_interface", "Unable to create socket: errno %d", errno);
            break;
        }

        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(dest_addr.sin_addr), str, INET_ADDRSTRLEN);
        ESP_LOGI("tinycar:swiftrobot_interface", "Socket created, connecting to %s:%d", str, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            // if connection failed, we need to close the socket and start again
            ESP_LOGD("tinycar:swiftrobot_interface", "Socket unable to connect: errno %d", errno);
            close(sock);
            vTaskDelay(1000/portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI("tinycar:swiftrobot_interface", "Successfully connected");
        conn_data->socket_fd = sock;
        break;
    }
    vTaskDelete(NULL);
}

int close_socket(int socket_fd) {
    if (socket_fd > 0) {        
        return close(socket_fd);
    }
    return 0;
}

int send_image_buffer(int socket_fd, char* image_buffer, size_t image_buffer_length) {
    // TODO: send protocol overhead
    int err = send(socket_fd, (uint32_t*)&image_buffer_length, 4, 0);
    if (err < 0) {
        ESP_LOGE("tinycar:swiftrobot_interface", "Error occurred during sending: errno %d", errno);
        return err;
    }

    err = send(socket_fd, image_buffer, image_buffer_length, 0);
    if (err < 0) {
        ESP_LOGE("tinycar:swiftrobot_interface", "Error occurred during sending: errno %d", errno);
        return err;
    }
    return 0;
}