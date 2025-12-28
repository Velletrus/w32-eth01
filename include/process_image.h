
#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"

// Process Image condivisa tra i task
struct ProcessImage {
  // TCP
  uint16_t tcp_in [NUM_TCP][WORDS_IN];    // letti da HREG 0..19
  uint16_t tcp_out[NUM_TCP][WORDS_OUT];   // scritti su HREG 100..119
  uint16_t tcp_stat[NUM_TCP];             // 0=down, 1=up (codici liberi)

  // RTU
  uint16_t rtu_in [NUM_RTU][WORDS_IN];
  uint16_t rtu_out[NUM_RTU][WORDS_OUT];
  uint16_t rtu_stat[NUM_RTU];             // 0=down, 1=up
};

// Dichiarazioni globali (definite in process_image.cpp)
#undef PI
extern ProcessImage PI;
extern SemaphoreHandle_t piMutex;

// API
void pi_init();
inline void pi_lock()   { xSemaphoreTake(piMutex, portMAX_DELAY); }
inline void pi_unlock() { xSemaphoreGive(piMutex); }
