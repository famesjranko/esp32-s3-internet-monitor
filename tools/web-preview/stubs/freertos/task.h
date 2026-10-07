#pragma once
#include "FreeRTOS.h"
inline void vTaskDelay(TickType_t) {}
inline void vTaskDelete(TaskHandle_t) {}
template <typename... A> int xTaskCreatePinnedToCore(A...) { return 1; }
inline int xPortGetCoreID() { return 1; }
