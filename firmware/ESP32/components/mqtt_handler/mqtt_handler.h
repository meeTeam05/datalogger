/**
 * @file mqtt_handler.h
 *
 * @brief MQTT5 Handler Library for ESP32
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

/* INCLUDES ------------------------------------------------------------------*/

#include "mqtt_client.h"
#include <stdbool.h>
#include <stdint.h>

/* DEFINES ------------------------------------------------------------------*/

#define MQTT_MAX_TOPIC_LEN 64
#define MQTT_MAX_DATA_LEN 256

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @typedef mqtt_data_callback_t
 *
 * @brief Callback function type for handling MQTT data reception
 *
 * @param topic    The MQTT topic the message was received on
 * @param data     Pointer to the received message payload
 * @param data_len Length of the received message payload in bytes
 */
typedef void (*mqtt_data_callback_t)(const char *topic, const char *data, int data_len);

/**
 * @typedef mqtt_handler_t
 *
 * @brief Main MQTT handler structure containing client state and configuration
 *
 * @details Contains all necessary information to manage an MQTT connection:
 *          - ESP32 MQTT client handle
 *          - Callback for message handling
 *          - Connection status
 *          - Unique client identifier
 */
typedef struct
{
  esp_mqtt_client_handle_t client;    /*!< ESP32 MQTT client handle */
  mqtt_data_callback_t data_callback; /*!< Callback for incoming messages */
  bool connected;                     /*!< Connection status */
  char client_id[32];                 /*!< Unique client identifier */
  uint32_t retry_count;               /*!< Retry attempt counter (for exponential backoff) */
  uint32_t last_retry_time_ms;        /*!< Timestamp of last retry attempt */
} mqtt_handler_t;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize MQTT handler
 *
 * @param mqtt MQTT handler structure
 * @param broker_url MQTT broker URL
 * @param username Username (can be NULL)
 * @param password Password (can be NULL)
 * @param callback Data received callback function
 *
 * @return true if successful
 */
bool MQTT_Handler_Init(mqtt_handler_t *mqtt, const char *broker_url,
                       const char *username, const char *password,
                       mqtt_data_callback_t callback);

/**
 * @brief Start MQTT client
 *
 * @param mqtt MQTT handler structure
 *
 * @return true if successful
 */
bool MQTT_Handler_Start(mqtt_handler_t *mqtt);

/**
 * @brief Subscribe to MQTT topic
 *
 * @param mqtt MQTT handler structure
 * @param topic Topic to subscribe
 * @param qos QoS level (0-2)
 *
 * @return Message ID if successful, -1 if failed
 */
int MQTT_Handler_Subscribe(mqtt_handler_t *mqtt, const char *topic, int qos);

/**
 * @brief Publish data to MQTT topic
 *
 * @param mqtt MQTT handler structure
 * @param topic Topic to publish
 * @param data Data to publish
 * @param data_len Data length (0 for null-terminated string)
 * @param qos QoS level (0-2)
 * @param retain Retain flag
 *
 * @return Message ID if successful, -1 if failed
 */
int MQTT_Handler_Publish(mqtt_handler_t *mqtt, const char *topic,
                         const char *data, int data_len, int qos, int retain);

/**
 * @brief Check if MQTT is connected
 *
 * @param mqtt MQTT handler structure
 *
 * @return true if connected
 */
bool MQTT_Handler_IsConnected(mqtt_handler_t *mqtt);

/**
 * @brief Reconnect MQTT with exponential backoff
 *
 * @param mqtt MQTT handler structure
 *
 * @return true if reconnection initiated, false if backoff delay not elapsed
 */
bool MQTT_Handler_Reconnect(mqtt_handler_t *mqtt);

/**
 * @brief Stop MQTT client
 *
 * @param mqtt MQTT handler structure
 */
void MQTT_Handler_Stop(mqtt_handler_t *mqtt);

/**
 * @brief Deinitialize MQTT handler
 *
 * @param mqtt MQTT handler structure
 */
void MQTT_Handler_Deinit(mqtt_handler_t *mqtt);

#endif /* MQTT_HANDLER_H */