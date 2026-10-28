#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "offline_kitchen.h"

static const char *TAG = "kitchen";

void app_main(void) {
    /* Never erase NVS automatically. A broken or incompatible store remains intact. */
    esp_err_t err = nvs_flash_init();
    bool storage_ready = err == ESP_OK;
    if (!storage_ready)
        ESP_LOGE(TAG, "NVS unavailable (%s); existing data retained", esp_err_to_name(err));
    if ((err = bsp_display_init()) != ESP_OK) {
        ESP_LOGE(TAG, "display unavailable (%s)", esp_err_to_name(err));
        return;
    }
    err = bsp_battery_init();
    if (err != ESP_OK)
        ESP_LOGW(TAG, "battery unavailable (%s)", esp_err_to_name(err));
    err = bsp_audio_init();
    if (err == ESP_OK)
        err = bsp_audio_set_format(16000, 16, 1);
    bool audio_ready = err == ESP_OK;
    if (audio_ready)
        bsp_audio_set_volume(80);
    else
        ESP_LOGE(TAG, "audio unavailable (%s); continuing silently", esp_err_to_name(err));
    if (!offline_kitchen_start(storage_ready, audio_ready))
        ESP_LOGE(TAG, "kitchen startup failed");
}
