
#include <Arduino.h>
#include <string.h>
#include "process_image.h"

ProcessImage PI;
SemaphoreHandle_t piMutex = nullptr;

void pi_init() {
  piMutex = xSemaphoreCreateMutex();
  memset(&PI, 0, sizeof(PI));
}
