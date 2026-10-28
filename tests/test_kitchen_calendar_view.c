#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kitchen_calendar_view.h"

#define WIDTH 240
#define HEIGHT 320
#define ROWS 8
static uint8_t* read_file(const char* path, size_t* size) {
    FILE* f = fopen(path, "rb"); assert(f);
    assert(fseek(f, 0, SEEK_END) == 0);
    long n = ftell(f); assert(n > 0); rewind(f);
    uint8_t* data = malloc((size_t)n); assert(data);
    assert(fread(data, 1, (size_t)n, f) == (size_t)n); fclose(f);
    *size = (size_t)n; return data;
}
int main(void) {
    size_t size; uint8_t* bytes = read_file("assets/kitchen/kitchen_fonts.bin", &size);
    kfn_pack fonts; assert(kfn_open(&fonts, bytes, size));
    kv_assets assets = {0}; assets.fonts = &fonts;
    assert(kfn_find_font(&fonts, "han12", &assets.han12));
    assert(kfn_find_font(&fonts, "han15", &assets.han15));
    assert(kfn_find_font(&fonts, "han16", &assets.han16));
    assert(kfn_find_font(&fonts, "latin12", &assets.latin12));
    ku_ui ui = {0}; kv_scene scene = {0}; scene.state = &ui; scene.assets = &assets;
    scene.battery = -1; memcpy(scene.clock, "--:--", 6);
    kcal_editor editor; assert(kcal_editor_begin(&editor, 0) == 0);
    uint16_t* full = calloc(WIDTH * HEIGHT, sizeof(*full));
    uint16_t* strips = calloc(WIDTH * HEIGHT, sizeof(*strips)); assert(full && strips);
    for (int state = 0; state < 6; ++state) {
        kg_canvas c; kg_init(&c, full, WIDTH, 0, HEIGHT, 0);
        if (state < 4) { editor.field = (kcal_field)state; kcal_view_draw(&c, &scene, &editor); }
        else kcal_view_hint(&c, &scene, state == 4);
        assert(!c.error);
        for (int y = 0; y < HEIGHT; y += ROWS) {
            kg_init(&c, strips + y * WIDTH, WIDTH, y, ROWS, 0);
            if (state < 4) kcal_view_draw(&c, &scene, &editor);
            else kcal_view_hint(&c, &scene, state == 4);
            assert(!c.error);
        }
        assert(memcmp(full, strips, WIDTH * HEIGHT * sizeof(*full)) == 0);
    }
    free(strips); free(full); free(bytes);
    puts("PASS: calendar Chinese glyphs, all editor fields and stripe/full rendering");
}
