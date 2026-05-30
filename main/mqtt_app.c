
#include "mqtt_app.h"
#include "ota.h"
static const char *TAG = "MQTT"; // 或 "LED_MQTT"

// ---------- MQTT 事件回调 ----------
static void mqtt_event_handler(void *handler_args, 
                               esp_event_base_t base, 
                               int32_t event_id, 
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // 连接成功后可以订阅一个主题，方便以后接收云端命令
        esp_mqtt_client_subscribe(event->client, "/led/command", 0);
        esp_mqtt_client_subscribe(event->client, "/led/ota", 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        // 断线后会自动重连，这里先只打印
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s, DATA=%.*s\n", event->topic_len, event->topic,
                                          event->data_len, event->data);
        // 检查是否是 OTA 命令
        if (event->topic_len == strlen("/led/ota") &&
            memcmp(event->topic, "/led/ota", event->topic_len) == 0) {
            // 复制 URL，因为 event_data 在回调结束后会失效
            char *url = malloc(event->data_len + 1);
            if (url) {
                memcpy(url, event->data, event->data_len);
                url[event->data_len] = '\0';
                xTaskCreate(&ota_task, "ota_task", 8192, url, 5, NULL);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%" PRIi32, event_id);
        break;
    }
}

// ---------- 启动 MQTT 客户端 ----------
void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.emqx.io:1883",  // 测试服务器
    };

    g_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(g_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(g_client);
}
