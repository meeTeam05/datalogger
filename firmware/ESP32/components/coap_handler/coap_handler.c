// TODO: WILL ENABLE IN THE FUTURE

/**
 * @file coap_handler.c
 * 
 * @brief CoAP Handler Implementation for ESP32
 * 
 * @details This component is DISABLED for now due to ESP-IDF 5.4 compatibility issues
 *          with the CoAP library. The header and documentation are provided for
 *          future implementation when CoAP support is added via esp-idf-coap manager.
 *          
 *          MQTT is the primary protocol - use that for now!
 *          
 *          To enable CoAP in the future:
 *          1. idf.py add-dependency "espressif/coap"
 *          2. Update this implementation with proper libcoap bindings
 *          3. Re-enable component in CMakeLists.txt
 */

/* Temporarily disabled - use MQTT instead */
#if 0

#include "coap_handler.h"
#include "esp_log.h"
// #include "esp_timer.h"
#include <string.h>

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static const char *TAG = "COAP_HANDLER";

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief CoAP response handler for processing server responses
 *
 * @details Handles CoAP responses and calls user callback with received data
 */
static void coap_message_handler(coap_context_t *ctx,
                                 coap_session_t *session,
                                 coap_pdu_t *sent,
                                 coap_pdu_t *received,
                                 const coap_mid_t id)
{
  coap_handler_t *handler = (coap_handler_t *)coap_session_get_app_data(session);

  if (!handler || !received)
  {
    return;
  }

  // Extract response data
  size_t len = coap_get_data(received, (const uint8_t **)&received->data);

  if (len > 0 && handler->data_callback)
  {
    // Get resource path
    coap_string_t *uri_path = coap_get_uri_path(received);
    char path[COAP_MAX_PATH_LEN] = {0};

    if (uri_path && uri_path->length > 0)
    {
      int path_len = (uri_path->length < sizeof(path) - 1) ? uri_path->length : sizeof(path) - 1;
      strncpy(path, (char *)uri_path->s, path_len);
      path[path_len] = '\0';
    }

    // Create null-terminated data string
    char data[COAP_MAX_DATA_LEN] = {0};
    int data_len = (len < sizeof(data) - 1) ? len : sizeof(data) - 1;
    strncpy(data, (char *)received->data, data_len);
    data[data_len] = '\0';

    ESP_LOGI(TAG, "← %s: %s", path, data);

    // Call user callback
    handler->data_callback(path, data, len);
  }
}

/**
 * @brief Create CoAP request PDU
 */
static coap_pdu_t *coap_create_request(coap_context_t *ctx, const char *path,
                                       coap_request_t method)
{
  coap_pdu_t *pdu = coap_pdu_init(
      COAP_MESSAGE_CON,      // Confirmable message
      method,                 // Request method (GET, POST, PUT, DELETE)
      coap_new_message_id(ctx),
      coap_max_send_pdu_size(coap_get_session(ctx), NULL));

  if (!pdu)
  {
    return NULL;
  }

  // Add Uri-Path options
  size_t path_len = strlen(path);
  coap_add_option(pdu, COAP_OPTION_URI_PATH, path_len, (uint8_t *)path);

  return pdu;
}

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize CoAP handler
 */
bool CoAP_Handler_Init(coap_handler_t *coap, const char *server_ip,
                       uint16_t server_port, coap_data_callback_t callback)
{
  if (!coap || !server_ip)
  {
    return false;
  }

  // Store configuration
  strncpy(coap->server_ip, server_ip, sizeof(coap->server_ip) - 1);
  coap->server_port = server_port;
  coap->data_callback = callback;
  coap->connected = false;

  // Initialize CoAP address structure
  coap_address_init(&coap->server_addr);
  coap->server_addr.addr.sin.sin_family = AF_INET;
  coap->server_addr.addr.sin.sin_port = htons(server_port);

  // Convert IP string to address
  if (inet_pton(AF_INET, server_ip, &coap->server_addr.addr.sin.sin_addr) <= 0)
  {
    ESP_LOGE(TAG, "Invalid server IP: %s", server_ip);
    return false;
  }

  ESP_LOGI(TAG, "CoAP Handler initialized: %s:%d", server_ip, server_port);
  return true;
}

/**
 * @brief Start CoAP client connection
 */
bool CoAP_Handler_Start(coap_handler_t *coap)
{
  if (!coap)
  {
    return false;
  }

  // Create CoAP context
  coap->ctx = coap_new_context(NULL);
  if (!coap->ctx)
  {
    ESP_LOGE(TAG, "Failed to create CoAP context");
    return false;
  }

  // Create session to server
  coap_session_t *session = coap_new_client_session(
      coap->ctx, NULL, &coap->server_addr, COAP_PROTO_UDP);

  if (!session)
  {
    ESP_LOGE(TAG, "Failed to create CoAP session");
    coap_free_context(coap->ctx);
    coap->ctx = NULL;
    return false;
  }

  // Set response handler
  coap_register_response_handler(coap->ctx, coap_message_handler);

  // Store handler reference in session
  coap_session_set_app_data(session, coap);

  coap->connected = true;
  ESP_LOGI(TAG, "Started");
  return true;
}

/**
 * @brief Publish data to CoAP resource
 */
int CoAP_Handler_Publish(coap_handler_t *coap, const char *path,
                         const char *data, int data_len, bool is_json)
{
  if (!coap || !coap->ctx || !path || !data)
  {
    return -1;
  }

  if (data_len == 0)
  {
    data_len = strlen(data);
  }

  // Create PUT request (most common for publishing data)
  coap_pdu_t *pdu = coap_create_request(coap->ctx, path, COAP_REQUEST_PUT);
  if (!pdu)
  {
    ESP_LOGE(TAG, "Failed to create CoAP PDU");
    return -1;
  }

  // Add Content-Format option if JSON
  if (is_json)
  {
    // COAP_MEDIATYPE_APPLICATION_JSON = 50
    coap_add_option(pdu, COAP_OPTION_CONTENT_FORMAT, 1, (uint8_t *)"\x32");
  }

  // Add payload
  coap_add_data(pdu, data_len, (uint8_t *)data);

  // Send request
  coap_session_t *session = coap_get_session(coap->ctx, NULL, &coap->server_addr);
  if (!session)
  {
    ESP_LOGE(TAG, "Failed to get CoAP session");
    coap_delete_pdu(pdu);
    return -1;
  }

  coap_mid_t mid = coap_send(session, pdu);
  if (mid == COAP_INVALID_MID)
  {
    ESP_LOGE(TAG, "Failed to send CoAP message: %s", path);
    return -1;
  }

  // Don't log every publish - too verbose
  // ESP_LOGI(TAG, "→ %s", path);

  return (int)mid;
}

/**
 * @brief Subscribe to CoAP resource (observe)
 */
bool CoAP_Handler_Subscribe(coap_handler_t *coap, const char *path)
{
  if (!coap || !coap->ctx || !path)
  {
    return false;
  }

  // Create GET request with OBSERVE option
  coap_pdu_t *pdu = coap_create_request(coap->ctx, path, COAP_REQUEST_GET);
  if (!pdu)
  {
    ESP_LOGE(TAG, "Failed to create CoAP subscription PDU");
    return false;
  }

  // Add OBSERVE option (register for notifications)
  coap_add_option(pdu, COAP_OPTION_OBSERVE, 0, NULL);

  // Send request
  coap_session_t *session = coap_get_session(coap->ctx, NULL, &coap->server_addr);
  if (!session)
  {
    ESP_LOGE(TAG, "Failed to get CoAP session for subscription");
    coap_delete_pdu(pdu);
    return false;
  }

  coap_mid_t mid = coap_send(session, pdu);
  if (mid == COAP_INVALID_MID)
  {
    ESP_LOGE(TAG, "Failed to send CoAP subscription: %s", path);
    return false;
  }

  ESP_LOGI(TAG, "Subscribe: %s", path);
  return true;
}

/**
 * @brief Check if CoAP is connected
 */
bool CoAP_Handler_IsConnected(coap_handler_t *coap)
{
  return coap ? coap->connected : false;
}

/**
 * @brief Stop CoAP client
 */
void CoAP_Handler_Stop(coap_handler_t *coap)
{
  if (!coap || !coap->ctx)
  {
    return;
  }

  coap->connected = false;

  // Clean up sessions
  coap_session_t *session, *tmp;
  COAP_LIST_STRUCT_ITER(coap->ctx->sessions, session, tmp, {
    coap_session_release(session);
  });

  // Release context will be freed when all sessions are released
  coap_free_context(coap->ctx);
  coap->ctx = NULL;

  ESP_LOGI(TAG, "Stopped");
}

/**
 * @brief Deinitialize CoAP handler
 */
void CoAP_Handler_Deinit(coap_handler_t *coap)
{
  if (!coap)
  {
    return;
  }

  if (coap->ctx)
  {
    CoAP_Handler_Stop(coap);
  }

  coap->connected = false;
  ESP_LOGI(TAG, "CoAP handler deinitialized");
}

#endif  // End of disabled CoAP implementation