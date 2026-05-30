#include "ota.h"
static const char *TAG = "OTA";  // 或 "LED_MQTT"，建议用 "OTA" 方便定位日志





void ota_task(void *param)
{
    esp_err_t ret = ESP_FAIL;
    esp_ota_handle_t ota_handle = 0;
    
    // 1. HTTP 配置，强调就是明文 TCP
    esp_http_client_config_t http_config = {
        .url = (const char *)param,
        .transport_type = HTTP_TRANSPORT_OVER_TCP, // 强制 TCP
        .timeout_ms = 15000,
        .keep_alive_enable = false,
    };
    
    esp_http_client_handle_t http_client = esp_http_client_init(&http_config);
    if (http_client == NULL) {
        ESP_LOGE(TAG, "OTA: HTTP client init failed");
        goto cleanup;
    }

    // 2. 发送 HTTP GET 请求
    ret = esp_http_client_open(http_client, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA: HTTP connect failed");
        goto cleanup;
    }

    int content_length = esp_http_client_fetch_headers(http_client);
    if (content_length <= 0) {
        ESP_LOGE(TAG, "OTA: No content or header error");
        goto cleanup;
    }

    ESP_LOGI(TAG, "OTA: Downloading %d bytes...", content_length);

    // 3. 启动 OTA 会话，指定写入 ota_1 分区
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "OTA: No OTA partition found");
        goto cleanup;
    }

    ret = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA: esp_ota_begin failed");
        goto cleanup;
    }

    // 4. 循环读取 HTTP 数据流，一块一块写入 Flash
    char buf[1024];
    int total_read = 0;
    while (1) {
        int read_len = esp_http_client_read(http_client, buf, sizeof(buf));
        if (read_len < 0) {
            ESP_LOGE(TAG, "OTA: HTTP read error");
            goto cleanup;
        }
        if (read_len == 0) break; // 数据读完了

        ret = esp_ota_write(ota_handle, buf, read_len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "OTA: Write error");
            goto cleanup;
        }
        total_read += read_len;
        ESP_LOGI(TAG, "OTA: Written %d/%d bytes", total_read, content_length);
    }

    // 5. 校验并结束 OTA
    ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA: esp_ota_end failed");
        goto cleanup;
    }

    // 6. 切换启动分区
    ret = esp_ota_set_boot_partition(update_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OTA: Set boot partition failed");
        goto cleanup;
    }

    ESP_LOGI(TAG, "OTA: Upgrade successful, restarting now!");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();

cleanup:
    if (ota_handle) {
        esp_ota_abort(ota_handle); // 失败时回滚
    }
    if (http_client) {
        esp_http_client_close(http_client);
        esp_http_client_cleanup(http_client);
    }
    free(param);
    vTaskDelete(NULL);
}
