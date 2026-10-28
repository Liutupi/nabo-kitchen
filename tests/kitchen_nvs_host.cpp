/* Synthetic NOR adapter for the unmodified ESP-IDF NVS storage engine. */
#include "nvs_storage.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>

class MemoryPartition : public nvs::Partition {
  public:
    std::array<uint8_t, 0x6000> bytes;
    int fail_after = -1, mutations = 0;
    MemoryPartition() { bytes.fill(0xff); }
    const char *get_partition_name() override { return "nvs"; }
    uint32_t get_address() override { return 0x9000; }
    uint32_t get_size() override { return bytes.size(); }
    bool get_readonly() override { return false; }
    esp_err_t read_raw(size_t at, void *dst, size_t n) override {
        if (at > bytes.size() || n > bytes.size() - at)
            return ESP_FAIL;
        std::memcpy(dst, bytes.data() + at, n);
        return ESP_OK;
    }
    esp_err_t read(size_t at, void *dst, size_t n) override { return read_raw(at, dst, n); }
    bool cut() {
        if (fail_after >= 0 && mutations >= fail_after)
            return true;
        ++mutations;
        return false;
    }
    esp_err_t write_raw(size_t at, const void *src, size_t n) override {
        if (at > bytes.size() || n > bytes.size() - at)
            return ESP_FAIL;
        if (cut())
            return ESP_ERR_FLASH_OP_FAIL;
        const auto *p = static_cast<const uint8_t *>(src);
        for (size_t i = 0; i < n; ++i) {
            assert((bytes[at + i] & p[i]) == p[i]);
            bytes[at + i] &= p[i];
        }
        return ESP_OK;
    }
    esp_err_t write(size_t at, const void *src, size_t n) override { return write_raw(at, src, n); }
    esp_err_t erase_range(size_t at, size_t n) override {
        assert(at % 4096 == 0 && n % 4096 == 0);
        if (at > bytes.size() || n > bytes.size() - at)
            return ESP_FAIL;
        if (cut())
            return ESP_ERR_FLASH_OP_FAIL;
        std::fill_n(bytes.data() + at, n, 0xff);
        return ESP_OK;
    }
};
static MemoryPartition partition;
static std::unique_ptr<nvs::Storage> storage;
static std::array<uint8_t, 0x6000> snapshot;
extern "C" {
void test_nvs_reboot(void) {
    storage.reset(new (std::nothrow) nvs::Storage(&partition));
    partition.fail_after = -1;
    partition.mutations = 0;
    assert(storage->init(0, 6) == ESP_OK);
}
void test_nvs_reset(void) {
    storage.reset();
    partition.bytes.fill(0xff);
    test_nvs_reboot();
}
void test_nvs_snapshot(void) { snapshot = partition.bytes; }
void test_nvs_restore(void) {
    partition.bytes = snapshot;
    test_nvs_reboot();
}
void test_nvs_cut_after(int n) {
    partition.mutations = 0;
    partition.fail_after = n;
}
int test_nvs_mutations(void) { return partition.mutations; }
esp_err_t nvs_open(const char *ns, nvs_open_mode_t mode, nvs_handle_t *h) {
    uint8_t index;
    esp_err_t err = storage->createOrOpenNamespace(ns, mode == NVS_READWRITE, index);
    if (err == ESP_OK)
        *h = index;
    return err;
}
void nvs_close(nvs_handle_t) {}
esp_err_t nvs_commit(nvs_handle_t) { return ESP_OK; }
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *dst, size_t *n) {
    size_t size;
    esp_err_t err = storage->getItemDataSize(h, nvs::ItemType::BLOB, key, size);
    if (err != ESP_OK)
        return err;
    if (!dst) {
        *n = size;
        return ESP_OK;
    }
    if (*n < size) {
        *n = size;
        return ESP_ERR_NVS_INVALID_LENGTH;
    }
    *n = size;
    return storage->readItem(h, nvs::ItemType::BLOB, key, dst, size);
}
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *src, size_t n) {
    return storage->writeItem(h, nvs::ItemType::BLOB, key, src, n);
}
esp_err_t nvs_erase_key(nvs_handle_t h, const char *key) { return storage->eraseItem(h, key); }
}
