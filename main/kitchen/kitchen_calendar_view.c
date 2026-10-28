#include "kitchen_calendar_view.h"
#include "kitchen_view_internal.h"
#include <stdbool.h>
#include <stdio.h>

void kcal_view_draw(kg_canvas *c, const kv_scene *s,
                    const kcal_editor *editor) {
  if (!c || !s || !editor)
    return;
  kg_rect(c, 0, 0, 240, 320, BG);
  kv_header(c, s, "设置日期", GOLD);
  kv_label(c, s, &s->assets->han16, 17, 49, 205, 25, PAPER,
           "请手动确认今天日期");
  static const char *names[] = {"年", "月", "日"};
  int values[] = {editor->date.year, editor->date.month, editor->date.day};
  for (unsigned i = 0; i < 3; ++i) {
    int x = 12 + (int)i * 76;
    bool selected = editor->field == (kcal_field)i;
    kg_round_rect(c, x, 94, 68, 70, 9, selected ? GOLD : PANEL);
    char value[12];
    snprintf(value, sizeof(value), i ? "%02d" : "%04d", values[i]);
    kv_label(c, s, &s->assets->latin12, x + 13, 102, 55, 25,
             selected ? INK : PAPER, value);
    kv_label(c, s, &s->assets->han16, x + 24, 132, 30, 25,
             selected ? INK : PAPER, names[i]);
  }
  if (editor->field == KCAL_CONFIRM) {
    kg_round_rect(c, 30, 175, 180, 31, 8, TOMATO);
    kv_label(c, s, &s->assets->han16, 57, 179, 150, 25, INK, "OK 确认日期");
  } else {
    kv_label(c, s, &s->assets->han12, 28, 181, 190, 20, DIM,
             "上下调整 · OK 下一项");
  }
  kv_label(c, s, &s->assets->han12, 18, 230, 210, 20, PAPER,
           "日期不会自动变化");
  kv_label(c, s, &s->assets->han12, 18, 254, 210, 20, DIM, "断电后请重新确认");
  kv_footer(c, s, "双击上取消 · OK 确认");
}
void kcal_view_hint(kg_canvas *c, const kv_scene *s, int unconfirmed) {
  if (!c || !s)
    return;
  kv_label(c, s, &s->assets->han12, 18, 105, 120, 18, DIM,
           unconfirmed ? "长按下键设日期" : "长按下键调整日期");
}
