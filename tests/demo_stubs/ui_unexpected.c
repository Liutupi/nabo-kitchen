/* COFF linkers require definitions even for unused demo screen functions.
 * Lifecycle tests exercise workers, not screen creation: fail on any accidental
 * call instead of silently pretending that a UI operation succeeded. */
#include <stdlib.h>
static _Noreturn void unexpected_ui(void) { abort(); }
int esp_sleep_get_wakeup_cause(void) { unexpected_ui(); }
esp_err_t esp_wifi_scan_get_ap_num(uint16_t *n) { (void)n;unexpected_ui(); }
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *n,wifi_ap_record_t *r) {
    (void)n;(void)r;unexpected_ui();
}
uint32_t lv_color_hex(uint32_t c) { (void)c; unexpected_ui(); }
lv_obj_t *lv_obj_create(lv_obj_t *p) { (void)p; unexpected_ui(); }
lv_obj_t *lv_label_create(lv_obj_t *p) { (void)p; unexpected_ui(); }
void lv_obj_set_size(lv_obj_t *o,int w,int h) { (void)o;(void)w;(void)h;unexpected_ui(); }
void lv_obj_set_width(lv_obj_t *o,int w) { (void)o;(void)w;unexpected_ui(); }
#define STYLE_TRAP(name,type) \
    void name(lv_obj_t *o,type v,int s) { (void)o;(void)v;(void)s;unexpected_ui(); }
STYLE_TRAP(lv_obj_set_style_radius,int)
STYLE_TRAP(lv_obj_set_style_bg_color,uint32_t)
STYLE_TRAP(lv_obj_set_style_border_width,int)
STYLE_TRAP(lv_obj_set_style_text_color,uint32_t)
STYLE_TRAP(lv_obj_set_style_text_align,int)
STYLE_TRAP(lv_obj_set_style_text_font,const lv_font_t *)
#undef STYLE_TRAP
void lv_obj_center(lv_obj_t *o) { (void)o;unexpected_ui(); }
void lv_obj_align(lv_obj_t *o,int a,int x,int y) { (void)o;(void)a;(void)x;(void)y;unexpected_ui(); }
void lv_obj_delete(lv_obj_t *o) { (void)o;unexpected_ui(); }
void lv_label_set_text_fmt(lv_obj_t *o,const char *s,...) { (void)o;(void)s;unexpected_ui(); }
void lv_label_set_long_mode(lv_obj_t *o,int m) { (void)o;(void)m;unexpected_ui(); }
void lv_screen_load(lv_obj_t *o) { (void)o;unexpected_ui(); }
lv_timer_t *lv_timer_create(void (*cb)(lv_timer_t *),unsigned p,void *d) {
    (void)cb;(void)p;(void)d;unexpected_ui();
}
void lv_timer_delete(lv_timer_t *t) { (void)t;unexpected_ui(); }
lv_obj_t *ui_pixel_screen_create(const char *s) { (void)s;unexpected_ui(); }
lv_obj_t *ui_pixel_panel_create(lv_obj_t *p,int x,int y,int w,int h,uint32_t c) {
    (void)p;(void)x;(void)y;(void)w;(void)h;(void)c;unexpected_ui();
}
lv_obj_t *ui_pixel_label(lv_obj_t *p,const char *s,const lv_font_t *f,uint32_t c) {
    (void)p;(void)s;(void)f;(void)c;unexpected_ui();
}
lv_obj_t *ui_pixel_mascot_create(lv_obj_t *p,int x,int y) {
    (void)p;(void)x;(void)y;unexpected_ui();
}
