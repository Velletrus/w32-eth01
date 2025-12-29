
#include <Arduino.h>
#include "process_image.h"
#include "config.h"

void TaskLogic(void*){
  TickType_t last = xTaskGetTickCount();
  uint32_t ctr = 0;
  uint16_t registro1 = 0;
  uint16_t registro2 = 0;
  for(;;){
    // Esempio: genera due contatori per mostrare traffico
    pi_lock();
    PI.tcp_out[0][0] = (uint16_t)(ctr & 0xFFFF);     // pubblicato su TCP[0]
    PI.rtu_out[0][0] = (uint16_t)((ctr * 3) & 0xFFFF);  // pubblicato su RTU[0]
    registro1 = PI.tcp_in[0][0];
    registro2 = PI.rtu_in[0][0];
    pi_unlock();
    
    Serial.print("registro TCP[0][0]: "); Serial.println(registro1);
    Serial.print(" | registro RTU[0][0]: "); Serial.println(registro2);
    
    ctr++;
    vTaskDelayUntil(&last, pdMS_TO_TICKS(LOGIC_MS));
  }
}
