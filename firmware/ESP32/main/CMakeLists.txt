# main/CMakeLists.txt

idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        mqtt_handler
        coap_handler
        stm32_uart
        relay_control
        json_sensor_parser
        json_utils
        button_handler
        esp_wifi
        esp_netif
        nvs_flash
        wifi_manager
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)