#TODO: WILL ENABLE IN THE FUTURE

idf_component_register(
    SRCS "coap_handler.c"
    INCLUDE_DIRS "."
    REQUIRES 
        lwip
        esp_event
        espressif__coap
)