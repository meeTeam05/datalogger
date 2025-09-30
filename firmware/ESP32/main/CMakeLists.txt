# main/CMakeLists.txt

idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        stm32_uart
        mqtt_handler
        relay_control
        sht3x_parser
        esp_wifi
        esp_netif
        nvs_flash
        wifi_manager
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)