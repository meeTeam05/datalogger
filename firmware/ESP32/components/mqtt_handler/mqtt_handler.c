/**
 * @file mqtt_handler.c
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "mqtt_handler.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <string.h>

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static const char *TAG = "MQTT_HANDLER";

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief MQTT event handler for processing client events
 *
 * @details Handles events:
 * - MQTT_EVENT_CONNECTED: Updates connection status
 * - MQTT_EVENT_DISCONNECTED: Updates disconnection status
 * - MQTT_EVENT_SUBSCRIBED/UNSUBSCRIBED: Logs subscription status
 * - MQTT_EVENT_DATA: Processes incoming messages, forwards to user callback
 * - MQTT_EVENT_ERROR: Logs errors, marks as disconnected
 *
 * @param handler_args MQTT handler instance (mqtt_handler_t*)
 * @param event_id Event type
 * @param event_data Event data containing message info
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
  mqtt_handler_t *mqtt = (mqtt_handler_t *)handler_args;
  esp_mqtt_event_handle_t event = event_data;

  switch (event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT Connected");
    mqtt->connected = true;
    break;

  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT Disconnected");
    mqtt->connected = false;
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT Subscribed, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT Unsubscribed, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_PUBLISHED:
    ESP_LOGD(TAG, "MQTT Published, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "<- MQTT: %.*s = %.*s", event->topic_len, event->topic,
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
    ESP_LOGE(TAG, "MQTT Error");
    mqtt->connected = false;
    break;

  default:
    ESP_LOGD(TAG, "MQTT Event: %ld", event_id);
    break;
  }
}

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

  // Generate client ID from MAC
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  snprintf(mqtt->client_id, sizeof(mqtt->client_id), "ESP32_%02X%02X%02X",
           mac[3], mac[4], mac[5]);

  // Configure MQTT client
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = broker_url,
      .session.protocol_ver = MQTT_PROTOCOL_V_5,
      .network.disable_auto_reconnect = false,
      .network.reconnect_timeout_ms = 5000,
      .network.timeout_ms = 10000,
      .credentials.client_id = mqtt->client_id,
      .session.keepalive = 60,
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

  ESP_LOGI(TAG, "MQTT Handler initialized: %s (%s)", broker_url,
           mqtt->client_id);
  return true;
}

/**
 * @brief Start MQTT client
 *
 * @param mqtt MQTT handler structure
 *
 * @return true if successful
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
    ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(ret));
    return false;
  }

  ESP_LOGI(TAG, "MQTT client started");
  return true;
}

/**
 * @brief Subscribe to MQTT topic
 *
 * @param mqtt MQTT handler structure
 * @param topic Topic to subscribe
 * @param qos QoS level (0-2)
 *
 * @return Message ID if successful, -1 if failed
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
    ESP_LOGI(TAG, "Subscribed to %s, msg_id=%d", topic, msg_id);
  }
  else
  {
    ESP_LOGE(TAG, "Failed to subscribe to %s", topic);
  }

  return msg_id;
}

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
  if (msg_id >= 0)
  {
    ESP_LOGD(TAG, "Published to %s: %.*s, msg_id=%d", topic,
             data_len, data, msg_id);
  }
  else
  {
    ESP_LOGE(TAG, "Failed to publish to %s", topic);
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

  esp_mqtt_client_stop(mqtt->client);
  ESP_LOGI(TAG, "MQTT client stopped");
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
  ESP_LOGI(TAG, "MQTT handler deinitialized");
}