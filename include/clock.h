#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint8_t month;
    uint8_t day;
    uint32_t year;
} clock_datetime_t;

void clock_init(void);
void clock_task(void* pvParameter);

bool get_current_time(clock_datetime_t* time);
bool get_current_time12(clock_datetime_t* time);
char clock_getHourTens(void);
char clock_getHourOnes(void);
char clock_getMinuteTens(void);
char clock_getMinuteOnes(void);
char clock_getHourOnes12(void);
char clock_getHourTens12(void);