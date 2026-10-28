#include "kitchen_calendar.h"

#include <stddef.h>

static int leap_year(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static int month_days(int year, int month) {
  static const unsigned char lengths[] = {31, 28, 31, 30, 31, 30,
                                          31, 31, 30, 31, 30, 31};
  if (year < KCAL_MIN_YEAR || year > KCAL_MAX_YEAR || month < 1 || month > 12)
    return 0;
  return lengths[month - 1] + (month == 2 && leap_year(year));
}

static int valid_date(const kcal_date *date) {
  if (!date)
    return 0;
  int last = month_days(date->year, date->month);
  return last && date->day >= 1 && date->day <= last;
}

/* At most 130 iterations and no locale, timezone, heap, or time_t dependency.
 */
static int32_t year_start(int year) {
  int32_t result = 0;
  for (int i = 1970; i < year; ++i)
    result += leap_year(i) ? 366 : 365;
  return result;
}

int kcal_date_to_day(const kcal_date *date, int32_t *epoch_day) {
  if (!epoch_day || !valid_date(date))
    return 0;
  int32_t result = year_start(date->year);
  for (int m = 1; m < date->month; ++m)
    result += month_days(date->year, m);
  *epoch_day = result + date->day - 1;
  return 1;
}

int kcal_day_to_date(int32_t epoch_day, kcal_date *date) {
  if (!date || epoch_day < KCAL_MIN_DAY || epoch_day > KCAL_MAX_DAY)
    return 0;
  kcal_date result = {KCAL_MIN_YEAR, 1, 1};
  int32_t remaining = epoch_day - KCAL_MIN_DAY;
  while (remaining >= (leap_year(result.year) ? 366 : 365)) {
    remaining -= leap_year(result.year) ? 366 : 365;
    ++result.year;
  }
  while (remaining >= month_days(result.year, result.month)) {
    remaining -= month_days(result.year, result.month);
    ++result.month;
  }
  result.day = (int)remaining + 1;
  *date = result;
  return 1;
}

int kcal_editor_begin(kcal_editor *editor, int32_t epoch_day) {
  if (!editor)
    return -1;
  kcal_date date;
  int valid = kcal_day_to_date(epoch_day, &date);
  if (!valid)
    date = (kcal_date){2026, 1, 1};
  *editor = (kcal_editor){date, KCAL_YEAR, KCAL_EDITING};
  return valid;
}

kcal_status kcal_editor_key(kcal_editor *editor, kcal_key key) {
  if (!editor || editor->status != KCAL_EDITING || editor->field < KCAL_YEAR ||
      editor->field > KCAL_CONFIRM || key < KCAL_UP || key > KCAL_CANCEL ||
      !valid_date(&editor->date))
    return KCAL_ERROR;
  if (key == KCAL_CANCEL) {
    editor->status = KCAL_CANCELED;
  } else if (key == KCAL_OK) {
    if (editor->field == KCAL_CONFIRM)
      editor->status = KCAL_CONFIRMED;
    else
      editor->field = (kcal_field)(editor->field + 1);
  } else if (editor->field != KCAL_CONFIRM) {
    int delta = key == KCAL_UP ? 1 : -1;
    if (editor->field == KCAL_YEAR) {
      editor->date.year += delta;
      if (editor->date.year > KCAL_MAX_YEAR)
        editor->date.year = KCAL_MIN_YEAR;
      else if (editor->date.year < KCAL_MIN_YEAR)
        editor->date.year = KCAL_MAX_YEAR;
    } else if (editor->field == KCAL_MONTH) {
      editor->date.month += delta;
      if (editor->date.month > 12)
        editor->date.month = 1;
      else if (editor->date.month < 1)
        editor->date.month = 12;
    } else {
      int last = month_days(editor->date.year, editor->date.month);
      editor->date.day += delta;
      if (editor->date.day > last)
        editor->date.day = 1;
      else if (editor->date.day < 1)
        editor->date.day = last;
    }
    int last = month_days(editor->date.year, editor->date.month);
    if (editor->date.day > last)
      editor->date.day = last;
  }
  return editor->status;
}

int kcal_editor_result(const kcal_editor *editor, int32_t *epoch_day,
                       unsigned *month) {
  if (!editor || editor->status != KCAL_CONFIRMED ||
      editor->field != KCAL_CONFIRM || !epoch_day || !month)
    return 0;
  int32_t result;
  if (!kcal_date_to_day(&editor->date, &result))
    return 0;
  *epoch_day = result;
  *month = (unsigned)editor->date.month;
  return 1;
}
