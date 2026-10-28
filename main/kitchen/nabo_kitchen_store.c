#include "nabo_kitchen_store.h"
#include "nvs.h"
static const char* key(unsigned slot) { return slot == 0 ? "state0" : "state1"; }
static int read_slot(void* ctx, unsigned slot, uint8_t* buf, size_t cap, size_t* length) {
    (void)ctx;
    if (slot > 1 || !buf || !length)
        return -1;
    nvs_handle_t handle;
    esp_err_t err = nvs_open("nabo_kitchen", NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
        return 0;
    if (err != ESP_OK)
        return -1;
    size_t needed = 0;
    err = nvs_get_blob(handle, key(slot), NULL, &needed);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(handle);
        return 0;
    }
    if (err != ESP_OK || needed > cap) {
        nvs_close(handle);
        return -1;
    }
    err = nvs_get_blob(handle, key(slot), buf, &needed);
    nvs_close(handle);
    if (err != ESP_OK)
        return -1;
    *length = needed;
    return 1;
}
static int write_slot(void* ctx, unsigned slot, const uint8_t* buf, size_t length) {
    (void)ctx;
    if (slot > 1 || !buf || !length)
        return 0;
    nvs_handle_t handle;
    esp_err_t err = nvs_open("nabo_kitchen", NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return 0;
    err = nvs_set_blob(handle, key(slot), buf, length);
    if (err == ESP_ERR_NVS_NOT_ENOUGH_SPACE) {
        /* The journal only writes its inactive slot. NVS's atomic replacement
         * otherwise needs three full blobs (both slots plus the new version).
         * Reclaim only the stale destination; the active slot remains durable
         * through an erase, retry failure, or power loss. Never erase a namespace
         * or partition to make room for kitchen records. */
        err = nvs_erase_key(handle, key(slot));
        if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) {
            err = nvs_commit(handle);
            if (err == ESP_OK)
                err = nvs_set_blob(handle, key(slot), buf, length);
        }
    }
    if (err == ESP_OK)
        err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}
kj_io nabo_kitchen_store_io(void) { return (kj_io){NULL, read_slot, write_slot}; }
