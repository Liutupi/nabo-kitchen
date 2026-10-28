#ifndef KITCHEN_CALENDAR_VIEW_H
#define KITCHEN_CALENDAR_VIEW_H
#include "kitchen_calendar.h"
#include "kitchen_view.h"
/* Render the date editor and home-page hint with the existing offline theme. */
void kcal_view_draw(kg_canvas *canvas, const kv_scene *scene,
                    const kcal_editor *editor);
void kcal_view_hint(kg_canvas *canvas, const kv_scene *scene,
                    int date_unconfirmed);
#endif
