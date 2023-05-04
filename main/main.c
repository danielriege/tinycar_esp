#include <stdio.h>

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs_flash.h>
#include <esp_timer.h>
#include <sys/param.h>
#include <string.h>
#include <arpa/inet.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "access_point.h"
#include "socket_al.h"
#include "esp_camera.h"

static const char *MAIN_TAG = "tinycar:main";

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#define TEST_DATA_LEN 100000
#define CROP_OFFSET 36480

static connection_data_t connection = {-1,0};
TaskHandle_t socket_connection_task_handle = NULL;

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_YUV422, // YUV420
    //.frame_size = FRAMESIZE_CIF,    // 400x296
    .frame_size = FRAMESIZE_HQVGA, // 240 x 176

    .jpeg_quality = 63, //0-63 lower number means higher qualitys
    .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

void start_swiftrobotm_connection() {
    xTaskCreate(create_socket, "SWIFTROBOT_MASTER_CONNECTION_CREATION", 4096, (void*)&connection, 1, &socket_connection_task_handle);
}

void stop_swiftrobotm_connection() {
    // we need to ask for socket_fd since the task is already dead if we have a socket
    if (socket_connection_task_handle != NULL && connection.socket_fd <= 0) {
        vTaskDelete(socket_connection_task_handle);
    }
    socket_connection_task_handle = NULL;
    if (close_socket(connection.socket_fd) < 0) {
        ESP_LOGE(MAIN_TAG, "socket connection closing failed");
    }
    connection.socket_fd = -1;
}

// event handler for new assigned IP addresses
void new_wifi_connection_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
        printf("SYSTEM_EVENT_AP_STAIPASSIGNED STA connected to ESP IP Assigned " IPSTR "\n", IP2STR(&event->ip));

        stop_swiftrobotm_connection();
        connection.ip = event->ip.addr;
        start_swiftrobotm_connection();
    }
}

void app_main(void) {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // create wifi network
    wifi_init_access_point();

    // start the camera
    ESP_ERROR_CHECK(esp_camera_init(&camera_config));

    // create event handler for assigned IP addresses (triggered from esp_wifi component)
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_AP_STAIPASSIGNED,
                                                        &new_wifi_connection_handler,
                                                        NULL,
                                                        NULL));

    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    //char rx_buffer[10];
    //static char test_data[TEST_DATA_LEN] = {0};
    while (1) {
        if (connection.socket_fd > 0) {
            ESP_LOGI(MAIN_TAG, "Taking picture...");
            uint64_t start = esp_timer_get_time();

            camera_fb_t *pic = esp_camera_fb_get();
            // use pic->buf to access the image
            //ESP_LOGI(MAIN_TAG, "Picture taken! Its size was: %zu bytes", pic->len-CROP_OFFSET);
            if(send_image_buffer(connection.socket_fd, (char*)pic->buf+CROP_OFFSET, pic->len-CROP_OFFSET) < 0) {
                stop_swiftrobotm_connection();
            }
            esp_camera_fb_return(pic);
            uint64_t stop = esp_timer_get_time();
            ESP_LOGI(MAIN_TAG, "Image sent. Took %lld us", stop-start);

            // int len = recv(connection.socket_fd, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // // Error occurred during receiving
            // if (len < 0) {
            //     ESP_LOGE(TAG, "recv failed: errno %d", errno);
            //     break;
            // }
            // if(send_image_buffer(connection.socket_fd, &test_data, TEST_DATA_LEN) < 0) {
            //     stop_swiftrobotm_connection();
            // }
        }
        vTaskDelay(100  / portTICK_PERIOD_MS);
    }
    
    // get camera frames and send them peridocially to last saved socket_fd
    // while (1) {
    //     ESP_LOGI(MAIN_TAG, "Taking picture...");
    //     camera_fb_t *pic = esp_camera_fb_get();
    //     printf("%p \n",pic);

    //     // use pic->buf to access the image
    //     ESP_LOGI(MAIN_TAG, "Picture taken! Its size was: %zu bytes", pic->len);
    //     esp_camera_fb_return(pic);

    //     vTaskDelay(5000 / portTICK_PERIOD_MS);
    // }
}