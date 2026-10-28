#ifndef KITCHEN_CALENDAR_H
#define KITCHEN_CALENDAR_H

#include <stdint.h>

/* Gregorian dates accepted by this UI, expressed as Unix days since 1970-01-01.
 * This module does not read an RTC or wall clock and does not advance dates.
 * 2026-01-01 is only a starting value when no valid date is available. */
#define KCAL_MIN_YEAR 2020
#define KCAL_MAX_YEAR 2099
#define KCAL_MIN_DAY INT32_C(18262)
#define KCAL_MAX_DAY INT32_C(47481)
#define KCAL_DEFAULT_DAY INT32_C(20454)

typedef struct {
  int year;
  int month;
  int day;
} kcal_date;

typedef enum { KCAL_YEAR, KCAL_MONTH, KCAL_DAY, KCAL_CONFIRM } kcal_field;
typedef enum { KCAL_UP, KCAL_DOWN, KCAL_OK, KCAL_CANCEL } kcal_key;
typedef enum {
  KCAL_EDITING,
  KCAL_CONFIRMED,
  KCAL_CANCELED,
  KCAL_ERROR
} kcal_status;

typedef struct {
  kcal_date date;
  kcal_field field;
  kcal_status status;
} kcal_editor;

/* Return 1 on success and 0 for null pointers or an out-of-range/invalid date.
 * On failure, the caller's output remains untouched. */
int kcal_date_to_day(const kcal_date *date, int32_t *epoch_day);
int kcal_day_to_date(int32_t epoch_day, kcal_date *date);

/* Set an existing editor to the supplied date or to 2026-01-01 if invalid.
 * Return 1 when epoch_day was valid, 0 when the input default was used, or -1
 * for a null editor. A default date is not evidence of the actual date. */
int kcal_editor_begin(kcal_editor *editor, int32_t epoch_day);

/* UP/DOWN wrap the selected year, month, or day; changing year/month clamps
 * the day to the new month's last day. OK advances year -> month -> day ->
 * confirm; another OK on confirm accepts. CANCEL (the caller maps double UP)
 * cancels without returning a date. UP/DOWN do nothing on the confirm screen.
 * The caller maps the button gestures and handles entry into this editor.
 * Returns the new status, or KCAL_ERROR for a null/corrupt/inactive editor or
 * invalid key. A rejected key leaves the editor unchanged. */
kcal_status kcal_editor_key(kcal_editor *editor, kcal_key key);

/* Extract the day and month only after confirmation. Return 0 for invalid
 * arguments or an unconfirmed/canceled editor, leaving outputs untouched. */
int kcal_editor_result(const kcal_editor *editor, int32_t *epoch_day,
                       unsigned *month);

#endif
