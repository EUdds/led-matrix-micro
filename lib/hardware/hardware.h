#pragma once

#include <stdint.h>

uint8_t hardware_getPotentiometerValue(void);
float  hardware_getPotentiometerValuef(void);
void hardware_task(void* pvParameters);