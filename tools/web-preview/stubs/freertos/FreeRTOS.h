#pragma once
#include "../Arduino.h"
typedef int portMUX_TYPE;
typedef void* TaskHandle_t;
typedef unsigned TickType_t;
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
#define pdMS_TO_TICKS(ms) (ms)
