// TODO: WILL ENABLE IN THE FUTURE

/**
 * @file coap_handler.h
 * 
 * @brief CoAP Handler Library for ESP32
 * 
 * @details Implements CoAP client for communicating with web servers.
 *          Supports sensor data publishing and command reception.
 *          Uses lightweight protocol suitable for embedded systems.
 */

#ifndef COAP_HANDLER_H
#define COAP_HANDLER_H

/* INCLUDES ------------------------------------------------------------------*/

#include "coap3/coap.h"
#include <stdbool.h>
#include <stdint.h>

/* DEFINES ------------------------------------------------------------------*/

#define COAP_MAX_PATH_LEN 64
#define COAP_MAX_DATA_LEN 512
#define COAP_DEFAULT_PORT 5683
#define COAP_TIMEOUT_MS 5000
#define COAP_MAX_RETRIES 3

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @typedef coap_data_callback_t
 *
 * @brief Callback function type for handling CoAP data reception
 *
 * @param path The CoAP resource path
 * @param data Pointer to the received message payload
 * @param data_len Length of the received message payload in bytes
 */
typedef void (*coap_data_callback_t)(const char *path, const char *data, int data_len);

/**
 * @typedef coap_handler_t
 *
 * @brief Main CoAP handler structure containing client state
 *
 * @details Contains all necessary information to manage a CoAP connection:
 *          - CoAP context for network operations
 *          - Server address and port
 *          - Callback for message handling
 *          - Connection status
 */
typedef struct
{
  coap_context_t *ctx;              /*!< CoAP context handle */
  coap_address_t server_addr;       /*!< Server address */
  coap_data_callback_t data_callback; /*!< Callback for incoming messages */
  bool connected;                   /*!< Connection status */
  char server_ip[16];               /*!< Server IP address (for display) */
  uint16_t server_port;             /*!< Server port number */
} coap_handler_t;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize CoAP handler
 *
 * @param coap CoAP handler structure
 * @param server_ip CoAP server IP address (e.g., "192.168.1.100")
 * @param server_port CoAP server port (default 5683)
 * @param callback Data received callback function
 *
 * @return true if successful, false otherwise
 */
bool CoAP_Handler_Init(coap_handler_t *coap, const char *server_ip,
                       uint16_t server_port, coap_data_callback_t callback);

/**
 * @brief Start CoAP client connection
 *
 * @param coap CoAP handler structure
 *
 * @return true if successful, false otherwise
 */
bool CoAP_Handler_Start(coap_handler_t *coap);

/**
 * @brief Publish data to CoAP resource
 *
 * @param coap CoAP handler structure
 * @param path Resource path (e.g., "/sensor/data")
 * @param data Data to publish
 * @param data_len Data length (0 for null-terminated string)
 * @param is_json Whether data is JSON format
 *
 * @return Message ID if successful, -1 if failed
 */
int CoAP_Handler_Publish(coap_handler_t *coap, const char *path,
                         const char *data, int data_len, bool is_json);

/**
 * @brief Subscribe to CoAP resource (observe)
 *
 * @param coap CoAP handler structure
 * @param path Resource path to observe
 *
 * @return true if successful, false otherwise
 */
bool CoAP_Handler_Subscribe(coap_handler_t *coap, const char *path);

/**
 * @brief Check if CoAP is connected
 *
 * @param coap CoAP handler structure
 *
 * @return true if connected, false otherwise
 */
bool CoAP_Handler_IsConnected(coap_handler_t *coap);

/**
 * @brief Stop CoAP client
 *
 * @param coap CoAP handler structure
 */
void CoAP_Handler_Stop(coap_handler_t *coap);

/**
 * @brief Deinitialize CoAP handler
 *
 * @param coap CoAP handler structure
 */
void CoAP_Handler_Deinit(coap_handler_t *coap);

#endif /* COAP_HANDLER_H */