#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

static const char *TAG = "SPRINT0";

void app_main(void)
{
    ESP_LOGI(TAG, "=====================================");
    ESP_LOGI(TAG, " ESP32-S3 USB-BLE Keyboard Bridge");
    ESP_LOGI(TAG, " Sprint 0: Environment OK!");
    ESP_LOGI(TAG, "=====================================");

    // In thông tin chip
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "Chip: %s, %d core(s), rev %d",
             CONFIG_IDF_TARGET, chip_info.cores, chip_info.revision);

    // In thông tin Flash
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    ESP_LOGI(TAG, "Flash size: %" PRIu32 " MB", flash_size / (1024 * 1024));

    // In thông tin heap
    ESP_LOGI(TAG, "Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());

    int count = 0;
    while (1) {
        ESP_LOGI(TAG, "Heartbeat: %d", count++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
