/**
 * @file mqtt_handler.c
 *
 * @brief MQTT5 Handler Library Implementation for ESP32
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "mqtt_handler.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include <string.h>
#include <inttypes.h>

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static const char *TAG = "MQTT_HANDLER";

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief Reset reconnection backoff counter
 *
 * @param mqtt MQTT handler instance
 *
 * @details Resets the retry count and last retry timestamp.
 */
static void mqtt_reset_backoff(mqtt_handler_t *mqtt)
{
  if (mqtt)
  {
    mqtt->retry_count = 0;
    mqtt->last_retry_time_ms = 0;
  }
}

/**
 * @brief MQTT event handler for processing client events
 *
 * @param handler_args MQTT handler instance (mqtt_handler_t*)
 * @param event_id Event type
 * @param event_data Event data containing message info
 *
 * @details Handles events:
 *          - MQTT_EVENT_CONNECTED: Updates connection status
 *          - MQTT_EVENT_DISCONNECTED: Updates disconnection status
 *          - MQTT_EVENT_SUBSCRIBED/UNSUBSCRIBED: Logs subscription status
 *          - MQTT_EVENT_DATA: Processes incoming messages, forwards to user callback
 *          - MQTT_EVENT_ERROR: Logs errors, marks as disconnected
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
  mqtt_handler_t *mqtt = (mqtt_handler_t *)handler_args;
  esp_mqtt_event_handle_t event = event_data;

  switch (event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "Connected");
    mqtt->connected = true;
    mqtt_reset_backoff(mqtt); // Reset retry counter on successful connection
    break;

  case MQTT_EVENT_DISCONNECTED:
    // Only log if we were previously connected (avoid spam on startup)
    if (mqtt->connected)
    {
      ESP_LOGI(TAG, "Disconnected");
    }
    mqtt->connected = false;
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "Subscribed (id=%d)", event->msg_id);
    break;

  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "Unsubscribed (id=%d)", event->msg_id);
    break;

  case MQTT_EVENT_PUBLISHED:
    // Don't log every publish - too verbose
    break;

  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "RX %.*s: %.*s", event->topic_len, event->topic,
             event->data_len, event->data);

    // Call callback if available
    if (mqtt->data_callback)
    {
      // Create null-terminated strings
      char topic[MQTT_MAX_TOPIC_LEN];
      char data[MQTT_MAX_DATA_LEN];

      int topic_len = (event->topic_len < sizeof(topic) - 1)
                          ? event->topic_len
                          : sizeof(topic) - 1;

      int data_len = (event->data_len < sizeof(data) - 1) ? event->data_len
                                                          : sizeof(data) - 1;

      strncpy(topic, event->topic, topic_len);
      topic[topic_len] = '\0';

      strncpy(data, event->data, data_len);
      data[data_len] = '\0';

      mqtt->data_callback(topic, data, event->data_len);
    }
    break;

  case MQTT_EVENT_ERROR:
    // Only log error details if it's not a simple disconnect
    if (event->error_handle->error_type != MQTT_ERROR_TYPE_NONE)
    {
      ESP_LOGE(TAG, "Error (type=%d)", event->error_handle->error_type);
    }
    mqtt->connected = false;
    break;

  default:
    // Don't log every event - too verbose
    break;
  }
}

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize MQTT handler
 */
bool MQTT_Handler_Init(mqtt_handler_t *mqtt, const char *broker_url,
                       const char *username, const char *password,
                       mqtt_data_callback_t callback)
{
  if (!mqtt || !broker_url)
  {
    return false;
  }

  // Initialize structure
  mqtt->client = NULL;
  mqtt->data_callback = callback;
  mqtt->connected = false;
  mqtt->retry_count = 0;        // Initialize retry counter
  mqtt->last_retry_time_ms = 0; // Initialize retry timer

  // Generate client ID from MAC
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  snprintf(mqtt->client_id, sizeof(mqtt->client_id), "ESP32_%02X%02X%02X",
           mac[3], mac[4], mac[5]);

  // Configure MQTT client
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = broker_url,
      .session.protocol_ver = MQTT_PROTOCOL_V_5,
      .network.disable_auto_reconnect = true, // Disable auto-reconnect for manual control
      .network.reconnect_timeout_ms = 2000,   // 2s initial retry (will use exponential backoff)
      .network.timeout_ms = 5000,             // Connection timeout: 5s (was 15s - too long)
      .credentials.client_id = mqtt->client_id,
      .session.keepalive = 60, // KeepAlive: 60s heartbeat
  };

  // Set credentials if provided
  if (username)
  {
    mqtt_cfg.credentials.username = username;
  }

  if (password)
  {
    mqtt_cfg.credentials.authentication.password = password;
  }

  // Initialize MQTT client
  mqtt->client = esp_mqtt_client_init(&mqtt_cfg);
  if (mqtt->client == NULL)
  {
    ESP_LOGE(TAG, "Failed to initialize MQTT client");
    return false;
  }

  // Register event handler
  esp_err_t ret = esp_mqtt_client_register_event(mqtt->client, ESP_EVENT_ANY_ID,
                                                 mqtt_event_handler, mqtt);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register MQTT event handler: %s",
             esp_err_to_name(ret));
    esp_mqtt_client_destroy(mqtt->client);
    mqtt->client = NULL;
    return false;
  }

  ESP_LOGI(TAG, "Init: %s [%s]", broker_url, mqtt->client_id);
  return true;
}

/**
 * @brief Start MQTT client
 */
bool MQTT_Handler_Start(mqtt_handler_t *mqtt)
{
  if (!mqtt || !mqtt->client)
  {
    return false;
  }

  esp_err_t ret = esp_mqtt_client_start(mqtt->client);
  if (ret != ESP_OK)
  {
    // If already started, try to reconnect
    if (ret == ESP_FAIL)
    {
      ESP_LOGI(TAG, "Started (reconnecting...)");
      ret = esp_mqtt_client_reconnect(mqtt->client);
      if (ret != ESP_OK)
      {
        ESP_LOGE(TAG, "Reconnect failed: %s", esp_err_to_name(ret));
        return false;
      }
      return true;
    }
    ESP_LOGE(TAG, "Start failed: %s", esp_err_to_name(ret));
    return false;
  }

  ESP_LOGI(TAG, "Started");
  return true;
}

/**
 * @brief Reconnect MQTT with fixed retry interval
 *
 * @details Implements fixed 5-second retry strategy:
 *          - Each reconnection attempt is spaced 5 seconds apart
 *          - This prevents flooding the broker while still reconnecting quickly
 *          - No exponential backoff to keep timing predictable
 */
bool MQTT_Handler_Reconnect(mqtt_handler_t *mqtt)
{
  if (!mqtt || !mqtt->client)
  {
    return false;
  }

  uint32_t now_ms = esp_timer_get_time() / 1000;

// Fixed retry interval: 5 seconds between attempts
#define MQTT_RETRY_INTERVAL_MS 5000

  // Check if enough time has elapsed since last retry
  if ((now_ms - mqtt->last_retry_time_ms) < MQTT_RETRY_INTERVAL_MS)
  {
    // Not yet time to retry
    return false;
  }

  // Time to reconnect - attempt reconnect
  mqtt->last_retry_time_ms = now_ms;
  mqtt->retry_count++;

  ESP_LOGI(TAG, "Reconnect attempt #%" PRIu32, mqtt->retry_count);

  esp_err_t ret = esp_mqtt_client_reconnect(mqtt->client);
  if (ret != ESP_OK)
  {
    ESP_LOGW(TAG, "Reconnect failed");
    return false;
  }

  return true;
}

/**
 * @brief Subscribe to MQTT topic
 */
int MQTT_Handler_Subscribe(mqtt_handler_t *mqtt, const char *topic, int qos)
{
  if (!mqtt || !mqtt->client || !topic)
  {
    return -1;
  }

  int msg_id = esp_mqtt_client_subscribe(mqtt->client, topic, qos);
  if (msg_id >= 0)
  {
    ESP_LOGI(TAG, "Subscribe: %s", topic);
  }
  else
  {
    ESP_LOGE(TAG, "Subscribe failed: %s", topic);
  }

  return msg_id;
}

/**
 * @brief Publish data to MQTT topic
 */
int MQTT_Handler_Publish(mqtt_handler_t *mqtt, const char *topic,
                         const char *data, int data_len, int qos, int retain)
{
  if (!mqtt || !mqtt->client || !topic || !data)
  {
    return -1;
  }

  if (data_len == 0)
  {
    data_len = strlen(data);
  }

  int msg_id = esp_mqtt_client_publish(mqtt->client, topic,
                                       data, data_len, qos, retain);
  if (msg_id < 0)
  {
    ESP_LOGE(TAG, "Publish failed: %s", topic);
  }

  return msg_id;
}

bool MQTT_Handler_IsConnected(mqtt_handler_t *mqtt)
{
  return mqtt ? mqtt->connected : false;
}

void MQTT_Handler_Stop(mqtt_handler_t *mqtt)
{
  if (!mqtt || !mqtt->client)
  {
    return;
  }

  mqtt->connected = false; // Update status immediately before stopping
  esp_mqtt_client_stop(mqtt->client);
  ESP_LOGI(TAG, "Stopped");
}

void MQTT_Handler_Deinit(mqtt_handler_t *mqtt)
{
  if (!mqtt)
  {
    return;
  }

  if (mqtt->client)
  {
    esp_mqtt_client_destroy(mqtt->client);
    mqtt->client = NULL;
  }

  mqtt->connected = false;
  ESP_LOGI(TAG, "Deinitialized");
}