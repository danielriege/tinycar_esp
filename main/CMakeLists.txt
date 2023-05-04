idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES wifi nvs_flash camera esp_timer swiftrobot_interface)