file(GLOB_RECURSE app_srcs *.c)

idf_component_register(
    SRCS ${app_srcs}
    INCLUDE_DIRS "."
    REQUIRES 
        esp_wifi 
        esp_netif 
        esp_event 
        nvs_flash 
        lwip
)