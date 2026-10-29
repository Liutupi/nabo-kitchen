#include "offline_kitchen.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_pins.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "key_guard.h"
#include "kitchen/kitchen_bundle.h"
#include "kitchen/kitchen_metadata.h"
#include "kitchen/kitchen_view.h"
#include "kitchen/nabo_kitchen_mixer.h"
#include "kitchen/nabo_kitchen_store.h"
#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

extern const uint8_t
    startup_pcm_start[] asm("_binary_nabo_kitchen_startup_pcm_start");
extern const uint8_t
    startup_pcm_end[] asm("_binary_nabo_kitchen_startup_pcm_end");
static const char *TAG = "offline_kitchen";
#define WIDTH 240
#define HEIGHT 320
#define STRIP 8
#define FRAME_BYTES (192 * 192)
#define EDGE_CAP 32

typedef struct {
  uint8_t key, pressed;
  int64_t when;
} key_edge;
typedef struct {
  kb_assets resources;
  esp_partition_mmap_handle_t mapping;
  krc_pack recipes;
  kfn_pack fonts;
  ksp_pack dish, icons, ui_art;
  kan_pack animation;
  kan_action action;
  kan_player player;
  kc_catalog catalog;
  kc_item *items;
  ku_ui ui;
  ki_input input;
  kv_assets assets;
  kv_scene scene;
  kj_journal journal;
  kj_io store;
  uint8_t *frame;
  uint16_t *strip;
  char action_name[16];
  int64_t next_animation, last_save, last_second, last_flash, last_confetti,
      voice_due, last_battery;
  int redraw, storage_fault, battery;
  bool storage_ready, audio_ready, boot_voice, test_mode, capture, clock_lost;
  bool capture_clean;
  ku_ui *test_original;
} kitchen_state;
#define KEY_GUARD_MAX_US (10LL * 1000 * 1000)
static key_guard_t wake_guard;
static volatile bool input_ready;
static QueueHandle_t edge_queue;
static atomic_bool edge_overflow;
static atomic_uint voice_request, alarm_request, audio_volume, audio_reset;
/* Audio worker owns mixer/hardware; publish only scalar diagnostic snapshots. */
static atomic_uint voice_requested, voice_started, voice_samples, voice_nonzero;
static atomic_uint audio_writes, audio_write_errors;
static atomic_bool audio_available;
static atomic_int audio_last_error;
static kitchen_state *state;

static void initialization_pause(void *context) {
  (void)context;
  /* A positive tick blocks the worker and lets IDLE run even at a low tick
   * rate. taskYIELD()/vTaskDelay(0) alone cannot give a lower priority CPU. */
  vTaskDelay(1);
}

static bool map_resources(kitchen_state *s) {
  const esp_partition_t *p = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "kitchen_assets");
  if (!p) {
    ESP_LOGE(TAG, "kitchen_assets partition missing");
    return false;
  }
  uint8_t h[16];
  if (esp_partition_read(p, 0, h, sizeof(h)) != ESP_OK ||
      memcmp(h, "KAB1", 4)) {
    ESP_LOGE(TAG, "asset header missing or invalid");
    return false;
  }
  uint32_t size = (uint32_t)h[8] | (uint32_t)h[9] << 8 | (uint32_t)h[10] << 16 |
                  (uint32_t)h[11] << 24;
  if (size < sizeof(h) || size > p->size) {
    ESP_LOGE(TAG, "asset size invalid");
    return false;
  }
  const void *data = NULL;
  if (esp_partition_mmap(p, 0, size, ESP_PARTITION_MMAP_DATA, &data,
                         &s->mapping) != ESP_OK) {
    ESP_LOGE(TAG, "asset mapping failed");
    return false;
  }
  if (!kb_open_cooperative(data, size, &s->resources, initialization_pause, NULL)) {
    ESP_LOGE(TAG, "asset container failed validation");
    esp_partition_munmap(s->mapping);
    s->mapping = 0;
    return false;
  }
  ESP_LOGI(TAG, "asset bundle validated: %u bytes", (unsigned)size);
  return true;
}
static bool load_resources(kitchen_state *s) {
  const kb_assets *r = &s->resources;
  if (!krc_open(&s->recipes, r->recipes.data, r->recipes.size))
    return false;
  initialization_pause(NULL);
  if (!kfn_open(&s->fonts, r->fonts.data, r->fonts.size))
    return false;
  initialization_pause(NULL);
  if (!ksp_open(&s->dish, r->dish.data, r->dish.size))
    return false;
  initialization_pause(NULL);
  if (!ksp_open(&s->icons, r->icons.data, r->icons.size))
    return false;
  initialization_pause(NULL);
  if (!ksp_open(&s->ui_art, r->ui.data, r->ui.size))
    return false;
  initialization_pause(NULL);
  if (!kan_open(&s->animation, r->animation.data, r->animation.size) ||
      s->recipes.count > KR_MAX_RECIPES || s->animation.width != 192 ||
      s->animation.height != 192)
    return false;
  /* Resource CRC/decompression is CPU work on this single-core board. Let the
   * idle task run between initialization stages instead of starving its WDT. */
  initialization_pause(NULL);
  s->items = heap_caps_calloc(s->recipes.count, sizeof(kc_item),
                              MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!s->items)
    return false;
  s->catalog = (kc_catalog){s->items, s->recipes.count, 0};
  if (!kc_load_cooperative(&s->catalog, &s->recipes, initialization_pause, NULL) ||
      !kv_assets_init(&s->assets, &s->catalog, &s->recipes, &s->fonts, &s->dish,
                      &s->icons, &s->ui_art, &s->animation))
    return false;
  initialization_pause(NULL);
  /* Stable device identity plus persisted draw_count, without a calendar. */
  uint8_t device_id[6];
  uint32_t device_seed = UINT32_C(0x4e41424f);
  if (esp_efuse_mac_get_default(device_id) == ESP_OK) {
    device_seed = UINT32_C(2166136261);
    for (unsigned i = 0; i < sizeof(device_id); ++i)
      device_seed = (device_seed ^ device_id[i]) * UINT32_C(16777619);
  }
  if (!ku_init(&s->ui, &s->catalog, &s->recipes, 0, 1, device_seed,
               km_ingredient_order, km_ingredient_order_count, km_fridge_order,
               km_fridge_order_count))
    return false;
  s->ui.sound = 1;
  s->store = nabo_kitchen_store_io();
  if (s->storage_ready) {
    uint8_t *buffer =
        heap_caps_malloc(KU_SAVE_MAX, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ku_ui *scratch =
        heap_caps_malloc(sizeof(ku_ui), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buffer || !scratch) {
      heap_caps_free(buffer);
      heap_caps_free(scratch);
      return false;
    }
    int lost = 0;
    kj_result result = kj_load(&s->journal, &s->store, &s->ui, scratch, buffer,
                               esp_timer_get_time() / 1000, -1, &lost);
    heap_caps_free(scratch);
    heap_caps_free(buffer);
    if (result != KJ_OK && result != KJ_EMPTY && result != KJ_RECOVERED) {
      s->storage_fault = 1;
      s->ui.message = KU_STORE_ERROR;
    } else if (result == KJ_RECOVERED)
      s->ui.message = KU_RECOVERED;
    if (lost) {
      /* Keep the warning until a real user acknowledges it after the boot
       * animation. */
      s->clock_lost = true;
      s->ui.dirty = 1;
    }
  } else {
    s->storage_fault = 1;
    s->ui.message = KU_STORE_ERROR;
  }
  /* Legacy calendar fields remain placeholders; no date confirmation is
   * presented after a reboot. */
  s->ui.date_unconfirmed = 0;
  s->frame =
      heap_caps_malloc(FRAME_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  s->strip = heap_caps_malloc(WIDTH * STRIP * sizeof(uint16_t),
                              MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!s->frame || !s->strip)
    return false;
  ki_init(&s->input);
  int64_t now = esp_timer_get_time() / 1000;
  s->voice_due = now + 1400;
  s->boot_voice = true;
  ku_startup(&s->ui, now);
  s->redraw = 1;
  s->battery = -1;
  return true;
}
static void save(kitchen_state *s, int64_t now) {
  if (s->test_mode || s->clock_lost || !s->ui.dirty || s->storage_fault)
    return;
  uint8_t *buffer =
      heap_caps_malloc(KU_SAVE_MAX, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!buffer) {
    s->ui.message = KU_STORE_ERROR;
    s->redraw = 1;
    return;
  }
  kj_result result = kj_store(&s->journal, &s->store, &s->ui, buffer, now, -1);
  heap_caps_free(buffer);
  if (result == KJ_OK)
    s->ui.dirty = 0;
  else {
    s->ui.message = KU_STORE_ERROR;
    s->redraw = 1;
    ESP_LOGE(TAG, "save failed (%d)", result);
  }
}
static void dispatch(void *context, ki_key key, ki_kind kind) {
  kitchen_state *s = context;
  if (s->clock_lost && !s->test_mode && s->ui.page != 0 && key == KI_OK &&
      kind == KI_SHORT) {
    s->clock_lost = false;
    ku_acknowledge_clock_loss(&s->ui);
    /* Dispatch runs in the UI worker, never in the button callback. Commit the
     * acknowledgement now rather than leaving a 1.5 second reset window. */
    save(s, esp_timer_get_time() / 1000);
    s->redraw = 1;
    return;
  }
  if (key == KI_UP && kind == KI_LONG && s->ui.page != 0) {
    /* This is a standalone application: long UP returns home, never exits the
     * timer service. */
    s->ui.page = 1;
    s->ui.focus = 0;
    s->ui.stack_count = 0;
    s->ui.outside = 0;
    s->ui.clear_stage = 0;
    s->ui.settings_mode = 0;
    s->ui.step_scroll = s->ui.step_scroll_max = s->ui.paper_scroll =
        s->ui.paper_scroll_max = 0;
  } else {
    ku_key(&s->ui, key, kind, esp_timer_get_time() / 1000);
  }
  s->redraw = 1;
}
static void on_button(bsp_btn_t key, bsp_btn_ev_t event, void *user) {
  (void)user;
  /* Release must disarm the wake guard even if input dispatch is not ready yet.
   */
  if (key_guard_consume(&wake_guard, event == BSP_BTN_RELEASE,
                        esp_timer_get_time()))
    return;
  if (!input_ready || !edge_queue)
    return;
  if (event != BSP_BTN_PRESS && event != BSP_BTN_RELEASE)
    return;
  key_edge e = {(uint8_t)key, event == BSP_BTN_PRESS,
                esp_timer_get_time() / 1000};
  if (xQueueSend(edge_queue, &e, 0) != pdTRUE)
    atomic_store(&edge_overflow, true);
}
static const char *animation_for(unsigned page) {
  return page == 3                ? "toss"
         : page == 8              ? "serve"
         : page == 6 || page == 7 ? "tend"
                                  : "idle";
}
static int next_animation(kitchen_state *s, int64_t now) {
  const char *name = animation_for(s->ui.page);
  if (strcmp(name, s->action_name)) {
    if (!kan_find(&s->animation, name, &s->action) ||
        !kan_start(&s->player, &s->animation, &s->action, s->frame,
                   FRAME_BYTES))
      return -1;
    snprintf(s->action_name, sizeof(s->action_name), "%s", name);
    s->next_animation = 0;
  }
  if (now < s->next_animation)
    return 0;
  int result = kan_next(&s->player);
  if (result == 0 && s->action.loop) {
    if (!kan_start(&s->player, &s->animation, &s->action, s->frame,
                   FRAME_BYTES))
      return -1;
    result = kan_next(&s->player);
  }
  if (result < 0)
    return -1;
  s->next_animation = now + 1000 / s->animation.fps;
  return result;
}
static bool render(kitchen_state *s, int64_t now, bool full, bool capture,
                   bool clean) {
  /* Presentation applies to this captured frame only. It never exits the
   * isolated test session or changes the user's saved state. */
  clean = capture && clean;
  if (!kv_prepare(&s->scene, &s->ui, &s->assets, now, s->battery,
                  clean ? "" : "--:--",
                  s->frame))
    return false;
  int first = 0, last = HEIGHT;
  if (!full) {
    if (s->ui.page == 3) {
      first = 88;
      last = 288;
    } else if (s->ui.page == 4 || s->ui.page == 7 || s->ui.page == 8)
      first = 192;
    else
      return true;
  }
  if (capture)
    printf("\n@kitchen.capture begin 240 320 RGB565\n");
  for (int y = first; y < last; y += STRIP) {
    kg_canvas canvas;
    kg_init(&canvas, s->strip, WIDTH, y, STRIP, 0x18c6);
    if (!kv_draw(&canvas, &s->scene))
      return false;
    if (canvas.error)
      return false;
    if (s->test_mode && !clean) {
      kg_rect(&canvas, 0, 0, 48, 16, 0xf800);
      kg_text(&canvas, &s->fonts, &s->assets.latin12, 3, 0, 0, 0xffff, "TEST");
    }
    for (unsigned i = 0; i < WIDTH * STRIP; ++i)
      s->strip[i] = (uint16_t)((s->strip[i] << 8) | (s->strip[i] >> 8));
    if (capture) {
      static const char hex[] = "0123456789abcdef";
      for (int row = 0; row < STRIP; ++row) {
        char line[WIDTH * 4 + 1];
        for (int x = 0; x < WIDTH; ++x) {
          uint16_t p = s->strip[row * WIDTH + x];
          line[x * 4] = hex[(p >> 4) & 15];
          line[x * 4 + 1] = hex[p & 15];
          line[x * 4 + 2] = hex[p >> 12];
          line[x * 4 + 3] = hex[(p >> 8) & 15];
        }
        line[WIDTH * 4] = 0;
        printf("@kitchen.row %d %s\n", y + row, line);
      }
    }
    if (esp_lcd_panel_draw_bitmap(bsp_display_panel(), 0, y, WIDTH, y + STRIP,
                                  s->strip) != ESP_OK ||
        esp_lcd_panel_io_tx_param(bsp_display_io(), -1, NULL, 0) != ESP_OK)
      return false;
  }
  if (capture)
    printf("@kitchen.capture end\n");
  return true;
}
static void audio_task(void *arg) {
  (void)arg;
  nk_mixer mixer;
  nk_mixer_init(&mixer, startup_pcm_start,
                (size_t)(startup_pcm_end - startup_pcm_start));
  bool available = state->audio_ready;
  atomic_store(&audio_available, available);
  for (;;) {
    int16_t samples[320] = {0};
    nk_mixer_fill(&mixer, samples, 320, atomic_exchange(&audio_reset, 0),
                  atomic_exchange(&voice_request, 0),
                  atomic_exchange(&alarm_request, 0),
                  atomic_load(&audio_volume));
    atomic_store(&voice_started, mixer.voice_starts);
    atomic_store(&voice_samples, mixer.voice_samples);
    atomic_store(&voice_nonzero, mixer.voice_nonzero_samples);
    esp_err_t error = available ? bsp_audio_write(samples, sizeof(samples)) : ESP_OK;
    if (available) {
      atomic_fetch_add(&audio_writes, 1);
      atomic_store(&audio_last_error, error);
      if (error != ESP_OK)
        atomic_fetch_add(&audio_write_errors, 1);
    }
    if (!available || error != ESP_OK) {
      if (available)
        ESP_LOGE(TAG, "audio playback stopped after device error");
      available = false;
      atomic_store(&audio_available, false);
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}
static void diagnostic(kitchen_state *s, const char *line, int64_t now) {
  if (!strcmp(line, ">kitchen.status")) {
    unsigned running = 0, paused = 0, done = 0;
    for (unsigned i = 0; i < s->ui.timers.count; ++i) {
      running += s->ui.timers.items[i].status == KT_RUNNING;
      paused += s->ui.timers.items[i].status == KT_PAUSED;
      done += s->ui.timers.items[i].status == KT_DONE;
    }
    printf(
        "\n@kitchen.status "
        "{\"running\":1,\"page\":%u,\"test\":%d,\"heap\":%u,\"largest\":%u,"
        "\"stack_free_min\":%u,\"recipes\":%u,\"ingredients\":%u,\"dirty\":%u,"
        "\"clock_lost\":%u,\"storage_fault\":%u,\"save_sequence\":%" PRIu32 ","
        "\"timers_running\":%u,\"timers_paused\":%u,\"timers_done\":%u,"
        "\"sound\":%u,\"volume\":%u,\"audio_gain\":%u,\"audio_ready\":%u,\"audio_available\":%u,"
        "\"boot_voice_pending\":%u,\"voice_requests\":%u,\"voice_starts\":%u,"
        "\"voice_samples\":%u,\"voice_nonzero_samples\":%u,\"audio_writes\":%u,"
        "\"audio_write_errors\":%u,\"audio_last_error\":%d}\n",
        s->ui.page, s->test_mode, (unsigned)esp_get_free_heap_size(),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        (unsigned)uxTaskGetStackHighWaterMark(NULL), (unsigned)s->catalog.count,
        (unsigned)km_ingredient_count, (unsigned)s->ui.dirty,
        (unsigned)s->clock_lost, (unsigned)s->storage_fault, s->journal.sequence,
        running, paused, done, (unsigned)s->ui.sound, (unsigned)s->ui.volume,
        atomic_load(&audio_volume), (unsigned)s->audio_ready,
        (unsigned)atomic_load(&audio_available), (unsigned)s->boot_voice,
        atomic_load(&voice_requested), atomic_load(&voice_started),
        atomic_load(&voice_samples), atomic_load(&voice_nonzero), atomic_load(&audio_writes),
        atomic_load(&audio_write_errors), atomic_load(&audio_last_error));
  } else if (!strcmp(line, ">kitchen.test=on")) {
    if (s->test_mode) {
      printf("\n@kitchen.test on\n");
      return;
    }
    if (s->ui.dirty) {
      printf("\n@kitchen.test pending-save\n");
      return;
    }
    for (unsigned i = 0; i < s->ui.timers.count; ++i) {
      const kt_timer *timer = &s->ui.timers.items[i];
      if (timer->status == KT_RUNNING ||
          (timer->status == KT_DONE && !timer->acknowledged)) {
        printf("\n@kitchen.test timer-active\n");
        return;
      }
    }
    ku_ui *backup =
        heap_caps_malloc(sizeof(ku_ui), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!backup) {
      printf("\n@kitchen.test no-memory\n");
      return;
    }
    *backup = s->ui;
    if (!ku_init(&s->ui, &s->catalog, &s->recipes, 19000, 1, 1,
                 km_ingredient_order, km_ingredient_order_count,
                 km_fridge_order, km_fridge_order_count)) {
      s->ui = *backup;
      heap_caps_free(backup);
      printf("\n@kitchen.test failed\n");
      return;
    }
    s->test_original = backup;
    s->test_mode = true;
    s->ui.sound = 1;
    atomic_store(&voice_request, 0);
    atomic_store(&alarm_request, 0);
    atomic_store(&audio_reset, 1);
    s->boot_voice = false;
    s->action_name[0] = 0;
    s->next_animation = 0;
    ki_init(&s->input);
    ku_startup(&s->ui, now);
    s->redraw = 1;
    printf("\n@kitchen.test on\n");
  } else if (!strcmp(line, ">kitchen.reset_test")) {
    if (s->test_mode && s->test_original) {
      s->ui = *s->test_original;
      heap_caps_free(s->test_original);
      s->test_original = NULL;
      s->test_mode = false;
      s->capture = false;
      s->capture_clean = false;
      s->action_name[0] = 0;
      s->next_animation = 0;
      atomic_store(&voice_request, 0);
      atomic_store(&alarm_request, 0);
      atomic_store(&audio_reset, 1);
      ki_init(&s->input);
      s->redraw = 1;
    }
    printf("\n@kitchen.test off\n");
  } else if (!strncmp(line, ">kitchen.seed ", 14)) {
    const char *value = line + 14;
    uint32_t seed = 0;
    bool valid = *value != 0;
    for (; *value; ++value) {
      if (*value < '0' || *value > '9' ||
          seed > (UINT32_MAX - (unsigned)(*value - '0')) / 10) {
        valid = false;
        break;
      }
      seed = seed * 10 + (unsigned)(*value - '0');
    }
    if (s->test_mode && valid) {
      s->ui.daily_seed = seed;
      printf("\n@kitchen.seed ok\n");
    } else
      printf("\n@kitchen.seed test-mode-required-or-invalid\n");
  } else if (!strcmp(line, ">kitchen.capture") ||
             !strcmp(line, ">kitchen.capture clean")) {
    if (s->test_mode) {
      s->capture = true;
      s->capture_clean = !strcmp(line, ">kitchen.capture clean");
      s->redraw = 1;
      printf("\n@kitchen.capture queued\n");
    } else
      printf("\n@kitchen.capture test-mode-required\n");
  } else if (!strncmp(line, ">kitchen.inject ", 16)) {
    const char *token = line + 16;
    unsigned key = !strncmp(token, "up", 2)     ? KI_UP
                   : !strncmp(token, "down", 4) ? KI_DOWN
                   : !strncmp(token, "ok", 2)   ? KI_OK
                                                : KI_KEYS;
    const char *suffix = key == KI_DOWN  ? token + 4
                         : key < KI_KEYS ? token + 2
                                         : token;
    bool valid = key < KI_KEYS &&
                 (!*suffix || !strcmp(suffix, "+") || !strcmp(suffix, "-"));
    if (s->test_mode && valid) {
      if (!strcmp(suffix, "-"))
        ki_release(&s->input, (ki_key)key, 0, now, dispatch, s);
      else {
        ki_press(&s->input, (ki_key)key, now, dispatch, s);
        if (!*suffix)
          ki_release(&s->input, (ki_key)key, 0, now, dispatch, s);
      }
      printf("\n@kitchen.inject queued\n");
    } else
      printf("\n@kitchen.inject test-mode-required-or-invalid\n");
  }
}
static void read_console(kitchen_state *s, int64_t now) {
  static char line[96];
  static unsigned used;
  char bytes[64];
  int count = usb_serial_jtag_read_bytes(bytes, sizeof(bytes), 0);
  for (int i = 0; i < count; ++i) {
    char ch = bytes[i];
    if (ch == '\n' || ch == '\r') {
      line[used] = 0;
      if (used)
        diagnostic(s, line, now);
      used = 0;
    } else if (used + 1 < sizeof(line))
      line[used++] = ch;
    else
      used = 0;
  }
}
static void worker(void *arg) {
  kitchen_state *s = arg;
  bool mapped = map_resources(s);
  initialization_pause(NULL);
  if (!mapped || !load_resources(s)) {
    ESP_LOGE(TAG, "kitchen assets or memory unavailable");
    /* A red screen is an explicit failure, rather than falsely showing old UI.
     */
    if (!s->strip) {
      heap_caps_free(s->frame);
      s->frame = NULL;
      s->strip = heap_caps_malloc(WIDTH * STRIP * sizeof(uint16_t),
                                  MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    }
    if (s->strip) {
      for (unsigned i = 0; i < WIDTH * STRIP; ++i)
        s->strip[i] = 0x00f8;
      for (int y = 0; y < HEIGHT; y += STRIP) {
        if (esp_lcd_panel_draw_bitmap(bsp_display_panel(), 0, y, WIDTH,
                                      y + STRIP, s->strip) != ESP_OK)
          break;
        if (esp_lcd_panel_io_tx_param(bsp_display_io(), -1, NULL, 0) != ESP_OK)
          break;
      }
      bsp_display_backlight(80);
    }
    vTaskDelete(NULL);
    return;
  }
  int64_t now = esp_timer_get_time() / 1000;
  s->last_save = now;
  s->last_second = -1;
  s->last_flash = -1;
  s->last_confetti = -1;
  bool lit = false;
  for (;;) {
    now = esp_timer_get_time() / 1000;
    read_console(s, now);
    uint32_t done_ids[3] = {0};
    unsigned done_count = 0;
    for (unsigned i = 0; i < s->ui.timers.count; ++i)
      if (s->ui.timers.items[i].status == KT_DONE)
        done_ids[done_count++] = s->ui.timers.items[i].id;
    unsigned page = s->ui.page;
    ku_tick(&s->ui, now);
    if (nk_boot_voice_due(&s->boot_voice, now, s->voice_due, s->test_mode)) {
      if (s->ui.sound && s->ui.volume && s->audio_ready) {
        atomic_store(&audio_volume, s->ui.volume);
        atomic_store(&voice_request, 1);
        atomic_fetch_add(&voice_requested, 1);
      }
    }
    if (s->ui.page != page)
      s->redraw = 1;
    key_edge edge;
    while (xQueueReceive(edge_queue, &edge, 0) == pdTRUE) {
      int64_t when = edge.when > now ? now : edge.when;
      if (when < s->input.last_now)
        when = s->input.last_now;
      if (edge.pressed)
        ki_press(&s->input, (ki_key)edge.key, when, dispatch, s);
      else
        ki_release(&s->input, (ki_key)edge.key, 0, when, dispatch, s);
    }
    if (atomic_exchange(&edge_overflow, false)) {
      ki_blur(&s->input, now, dispatch, s);
      s->ui.message = KU_CAPACITY_ERROR;
      s->redraw = 1;
    }
    ki_poll(&s->input, now, dispatch, s);
    unsigned done_after = 0;
    bool new_alarm = false, flash_active = false;
    for (unsigned i = 0; i < s->ui.timers.count; ++i) {
      const kt_timer *t = &s->ui.timers.items[i];
      if (t->status != KT_DONE)
        continue;
      ++done_after;
      if (!t->acknowledged && now >= t->deadline_ms &&
          now - t->deadline_ms < 2100)
        flash_active = true;
      bool seen = false;
      for (unsigned j = 0; j < done_count; ++j)
        seen |= done_ids[j] == t->id;
      if (!seen && !t->acknowledged)
        new_alarm = true;
    }
    if (new_alarm) {
      /* Timer alarms redraw immediately on every page. */
      s->redraw = 1;
      if (s->ui.sound && s->ui.volume && s->audio_ready) {
        atomic_store(&audio_volume, s->ui.volume);
        atomic_store(&alarm_request, 1);
      }
    }
    int animated = next_animation(s, now);
    if (animated < 0) {
      ESP_LOGE(TAG, "animation decoding failed");
      break;
    }
    if (s->ui.page == 0 && animated)
      s->redraw = 1;
    if (now / 1000 != s->last_second) {
      s->last_second = now / 1000;
      s->redraw = 1;
    }
    if (s->ui.page == 8 && now / 240 != s->last_confetti) {
      s->last_confetti = now / 240;
      s->redraw = 1;
    }
    if (done_after && flash_active && now / 350 != s->last_flash) {
      s->last_flash = now / 350;
      s->redraw = 1;
    }
    if (now - s->last_battery >= 5000) {
      s->last_battery = now;
      s->battery = bsp_battery_soc();
      s->redraw = 1;
    }
    if (s->clock_lost && !s->test_mode && s->ui.page != 0) {
      if (s->ui.message != KU_CLOCK_LOST) {
        s->ui.message = KU_CLOCK_LOST;
        s->redraw = 1;
      }
      s->ui.message_due = 0;
    }
    if (s->storage_fault && s->ui.message == KU_NO_MESSAGE) {
      s->ui.message = KU_STORE_ERROR;
      s->redraw = 1;
    }
    if (s->ui.dirty && now - s->last_save >= 1500) {
      save(s, now);
      s->last_save = now;
    }
    bool capture = s->test_mode && s->capture;
    bool clean = capture && s->capture_clean;
    s->capture = false;
    s->capture_clean = false;
    if ((s->redraw || animated || capture) &&
        !render(s, now, s->redraw || capture, capture, clean)) {
      ESP_LOGE(TAG, "screen rendering failed");
      break;
    }
    if (!lit) {
      bsp_display_backlight(80);
      lit = true;
    }
    /* Restore the visible isolation badge immediately after a clean capture. */
    s->redraw = clean ? 1 : 0;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  save(s, esp_timer_get_time() / 1000);
  ESP_LOGE(TAG, "kitchen worker stopped; restart required");
  vTaskDelete(NULL);
}
bool offline_kitchen_start(bool storage_ready, bool audio_ready) {
  if (state)
    return false;
  edge_queue = xQueueCreate(EDGE_CAP, sizeof(key_edge));
  state = heap_caps_calloc(1, sizeof(*state),
                           MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!edge_queue || !state)
    return false;
  state->storage_ready = storage_ready;
  state->audio_ready = audio_ready;
  if (!usb_serial_jtag_is_driver_installed()) {
    usb_serial_jtag_driver_config_t config = {.tx_buffer_size = 2048,
                                              .rx_buffer_size = 512};
    if (usb_serial_jtag_driver_install(&config) != ESP_OK)
      ESP_LOGW(TAG, "USB diagnostics unavailable");
  }
  if (usb_serial_jtag_is_driver_installed())
    usb_serial_jtag_vfs_use_driver();
  TaskHandle_t audio_handle = NULL;
  if (xTaskCreate(audio_task, "kitchen_audio", 4096, NULL, 7, &audio_handle) !=
      pdPASS) {
    state->audio_ready = false;
    ESP_LOGE(TAG, "audio task unavailable; continuing silently");
  }
  /* A 192-dish draw needs a larger temporary recipe array; retain headroom for
   * the search, renderer and storage calls on the same task stack. */
  if (xTaskCreate(worker, "kitchen_ui", 12288, state, 5, NULL) != pdPASS) {
    if (audio_handle)
      vTaskDelete(audio_handle);
    return false;
  }
  if (esp_sleep_get_wakeup_causes() & (1u << ESP_SLEEP_WAKEUP_GPIO))
    key_guard_arm(&wake_guard, esp_timer_get_time(), KEY_GUARD_MAX_US);
  if (bsp_button_init(on_button, NULL) != ESP_OK) {
    ESP_LOGE(TAG, "buttons unavailable");
    return false;
  }
  if (wake_guard.armed) {
    int mv = bsp_button_read_mv();
    key_guard_sync_released(&wake_guard, mv >= BSP_BTN_MV_RELEASED_MIN);
  }
  input_ready = true;
  return true;
}
