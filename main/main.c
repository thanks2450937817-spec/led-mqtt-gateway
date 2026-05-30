#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "mqtt_app.h"

// ---------- 配置 ----------
#define LED_GPIO   GPIO_NUM_1         // 你的LED引脚
static const char *TAG = "LED_MQTT";  // 日志标签

// ---------- 全局变量：MQTT客户端句柄 ----------
// 为了让 while(1) 能用到 mqtt_app_start 里创建的 client，这里设一个全局变量
esp_mqtt_client_handle_t g_client = NULL;
// ----------  OTA ---------------------------


// ---------- 主程序 ----------
void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    // 1. 底层初始化（网络项目的“三板斧”）
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    // 2. 连接 WiFi（会阻塞到成功拿到IP）
    ESP_ERROR_CHECK(example_connect());

    // 3. 启动 MQTT（后台会自动维持连接）
    mqtt_app_start();

    // 4. 初始化 LED GPIO
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    printf("LED MQTT Gateway started!\n");

    // 5. 主循环：闪灯 + 上报状态
    while (1) {
        // 亮灯 + 上报 "ON"
        gpio_set_level(LED_GPIO, 1);
        if (g_client != NULL) {
            esp_mqtt_client_publish(g_client, "/led/status", "ON", 0, 1, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));

        // 灭灯 + 上报 "OFF"
        gpio_set_level(LED_GPIO, 0);
        if (g_client != NULL) {
            esp_mqtt_client_publish(g_client, "/led/status", "OFF", 0, 1, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}