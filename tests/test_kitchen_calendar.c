/* Host test for the independent Gregorian calendar and its button state machine. */
#include "kitchen_calendar.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

static void expect_day(int year, int month, int day, int32_t expected) {
    kcal_date date = {year, month, day}, decoded = {0, 0, 0};
    int32_t actual = INT32_MIN;
    assert(kcal_date_to_day(&date, &actual) == 1 && actual == expected);
    assert(kcal_day_to_date(expected, &decoded) == 1);
    assert(decoded.year == year && decoded.month == month && decoded.day == day);
}

static void test_boundaries_and_leaps(void) {
    expect_day(2020, 1, 1, 18262);
    expect_day(2020, 2, 29, 18321);
    expect_day(2024, 2, 29, 19782);
    expect_day(2026, 1, 1, 20454);
    expect_day(2099, 12, 31, 47481);
    int32_t value = 91;
    kcal_date date = {2019, 12, 31};
    assert(!kcal_date_to_day(&date, &value) && value == 91);
    date = (kcal_date){2100, 1, 1};
    assert(!kcal_date_to_day(&date, &value) && value == 91);
    date = (kcal_date){2023, 2, 29};
    assert(!kcal_date_to_day(&date, &value) && value == 91);
    date = (kcal_date){2024, 4, 31};
    assert(!kcal_date_to_day(&date, &value) && value == 91);
    date = (kcal_date){2024, 0, 1};
    assert(!kcal_date_to_day(&date, &value) && value == 91);
    date = (kcal_date){2024, 13, 1};
    assert(!kcal_date_to_day(&date, &value) && value == 91);
    date = (kcal_date){2024, 1, 0};
    assert(!kcal_date_to_day(&date, &value) && value == 91);
    date = (kcal_date){42, 43, 44};
    assert(!kcal_day_to_date(KCAL_MIN_DAY - 1, &date));
    assert(date.year == 42 && date.month == 43 && date.day == 44);
    assert(!kcal_day_to_date(KCAL_MAX_DAY + 1, &date));
    assert(!kcal_day_to_date(INT32_MIN, &date));
    assert(!kcal_day_to_date(INT32_MAX, &date));
    assert(!kcal_day_to_date(KCAL_MIN_DAY, NULL));
    assert(!kcal_date_to_day(NULL, &value));
    assert(!kcal_date_to_day(&date, NULL));
}

static void test_all_supported_days(void) {
    for (int32_t day = KCAL_MIN_DAY; day <= KCAL_MAX_DAY; ++day) {
        kcal_date date;
        int32_t roundtrip = INT32_MIN;
        assert(kcal_day_to_date(day, &date));
        assert(kcal_date_to_day(&date, &roundtrip));
        assert(day == roundtrip);
    }
}

static void test_confirmation_and_cancel(void) {
    kcal_editor e;
    int32_t day = -9;
    unsigned month = 99;
    assert(kcal_editor_begin(NULL, KCAL_DEFAULT_DAY) == -1);
    assert(kcal_editor_begin(&e, -1) == 0);
    assert(e.date.year == 2026 && e.date.month == 1 && e.date.day == 1);
    assert(e.field == KCAL_YEAR && e.status == KCAL_EDITING);
    assert(!kcal_editor_result(&e, &day, &month) && day == -9 && month == 99);
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_EDITING && e.field == KCAL_MONTH);
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_EDITING && e.date.month == 2);
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_EDITING && e.field == KCAL_DAY);
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_EDITING && e.date.day == 2);
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_EDITING && e.field == KCAL_CONFIRM);
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_EDITING && e.date.day == 2);
    assert(!kcal_editor_result(&e, &day, &month));
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_CONFIRMED);
    assert(kcal_editor_result(&e, &day, &month) && day == 20486 && month == 2);
    assert(!kcal_editor_result(&e, NULL, &month));
    assert(!kcal_editor_result(&e, &day, NULL));
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_ERROR);
    assert(kcal_editor_begin(&e, KCAL_MIN_DAY) == 1);
    assert(kcal_editor_key(&e, KCAL_CANCEL) == KCAL_CANCELED);
    day = -9; month = 99;
    assert(!kcal_editor_result(&e, &day, &month) && day == -9 && month == 99);
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_ERROR);
}

static void test_wrap_clamp_and_corruption(void) {
    kcal_editor e;
    int32_t leap_day;
    kcal_date date = {2024, 2, 29};
    assert(kcal_date_to_day(&date, &leap_day));
    assert(kcal_editor_begin(&e, leap_day) == 1);
    assert(kcal_editor_key(&e, KCAL_DOWN) == KCAL_EDITING);
    assert(e.date.year == 2023 && e.date.month == 2 && e.date.day == 28);
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_EDITING);
    assert(kcal_editor_key(&e, KCAL_DOWN) == KCAL_EDITING);
    assert(e.date.year == 2023 && e.date.month == 1 && e.date.day == 28);
    assert(kcal_editor_begin(&e, KCAL_MAX_DAY) == 1);
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_EDITING);
    assert(e.date.year == 2020 && e.date.month == 12 && e.date.day == 31);
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_EDITING);
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_EDITING && e.date.month == 1);
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_EDITING);
    assert(e.date.month == 2 && e.date.day == 29);
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_EDITING);
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_EDITING && e.date.day == 1);
    assert(kcal_editor_key(&e, KCAL_DOWN) == KCAL_EDITING && e.date.day == 29);
    assert(kcal_editor_key(&e, KCAL_OK) == KCAL_EDITING && e.field == KCAL_CONFIRM);
    assert(kcal_editor_key(&e, KCAL_CANCEL) == KCAL_CANCELED);
    assert(kcal_editor_begin(&e, KCAL_DEFAULT_DAY) == 1);
    assert(kcal_editor_key(&e, (kcal_key)99) == KCAL_ERROR);
    assert(kcal_editor_key(&e, (kcal_key)-1) == KCAL_ERROR);
    assert(e.field == KCAL_YEAR && e.date.year == 2026 && e.status == KCAL_EDITING);
    e.field = (kcal_field)-1;
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_ERROR && e.date.year == 2026);
    e.field = KCAL_YEAR;
    e.date.month = 0;
    assert(kcal_editor_key(&e, KCAL_UP) == KCAL_ERROR && e.date.month == 0);
    assert(kcal_editor_key(NULL, KCAL_UP) == KCAL_ERROR);
}

int main(void) {
    test_boundaries_and_leaps();
    test_all_supported_days();
    test_confirmation_and_cancel();
    test_wrap_clamp_and_corruption();
    puts("PASS: calendar bounds, leap years, 29220-day roundtrip and editor");
    return 0;
}
