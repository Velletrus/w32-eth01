
#include <Arduino.h>
#include <HardwareSerial.h>
#include <ModbusRTU.h>
#include <string.h>
#include "config.h"
#include "process_image.h"

static HardwareSerial SerialRTU(2);
static ModbusRTU mbRTU;

// ---------------- Callbacks asincroni ----------------
static volatile bool rtu_done = false;
static volatile bool rtu_ok   = false;

static bool rtu_cb(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  // event == Modbus::EX_SUCCESS se tutto ok
  rtu_ok   = (event == Modbus::EX_SUCCESS);
  rtu_done = true;
  // IMPORTANT: non mantenere l'oggetto transazione per evitare blocchi dopo la prima richiesta
  return false; 
}

// Helper: attende il completamento (con timeout) eseguendo mbRTU.task()
static bool rtu_wait(uint32_t timeoutMs) {
  uint32_t t0 = millis();
  while (!rtu_done && (millis() - t0) < timeoutMs) {
    mbRTU.task();          // avanza lo state machine ModbusRTU
    vTaskDelay(1);         // cede CPU (meglio di delay() in FreeRTOS)
  }
  return rtu_done && rtu_ok;
}

// Lettura sincrona (tramite callback + attesa)
static bool mbRTU_syncRead(uint8_t id, uint16_t start, uint16_t* dest, uint16_t qty, uint32_t timeoutMs){
  rtu_done = false; rtu_ok = false;
  // Avvio transazione: versione con buffer e callback
  if (!mbRTU.readHreg(id, start, dest, qty, rtu_cb)) return false;
  return rtu_wait(timeoutMs);
}

// Scrittura sincrona (tramite callback + attesa)
static bool mbRTU_syncWrite(uint8_t id, uint16_t start, const uint16_t* src, uint16_t qty, uint32_t timeoutMs){
  rtu_done = false; rtu_ok = false;
  // Copia su stack per sicurezza
  uint16_t tmp[WORDS_OUT];
  for (uint16_t i=0; i<qty; i++) tmp[i] = src[i];

  if (!mbRTU.writeHreg(id, start, tmp, qty, rtu_cb)) return false;
  return rtu_wait(timeoutMs);
}

void TaskModbusRTU(void*){
  // UART2: inizializzazione UNA sola volta
  SerialRTU.begin(RTU_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

  // La libreria gestisce automaticamente DE/RE sul pin passato
  mbRTU.begin(&SerialRTU, PIN_RE_DE);
  mbRTU.master();

  // (opzionale) Log watermark per verificare stack
  Serial.printf("[MB_RTU] Stack watermark: %u\n", uxTaskGetStackHighWaterMark(NULL));

  uint8_t idx = 0;
  TickType_t last = xTaskGetTickCount();

  for(;;){
    // --- READ 20 word da 0..19 ---
    uint16_t inbuf[WORDS_IN];
    bool okR = mbRTU_syncRead(RTU_CFG[idx].id, HREG_READ_START, inbuf, WORDS_IN, MB_TIMEOUT_MS);
    if(okR){
      pi_lock();
      memcpy(PI.rtu_in[idx], inbuf, sizeof(inbuf));
      PI.rtu_stat[idx] = 1;
      pi_unlock();
    } else {
      pi_lock(); PI.rtu_stat[idx] = 0; pi_unlock();
      Serial.printf("[RTU %u] READ fail/timeout (ID=%u)\n", idx, RTU_CFG[idx].id);
    }

    // --- mini pump: qualche giro non bloccante tra le transazioni ---
    for (int i = 0; i < 3; ++i) {
      mbRTU.task();
      vTaskDelay(1);
    }

    // --- WRITE 20 word a 100..119 ---
    uint16_t outbuf[WORDS_OUT];
    pi_lock(); memcpy(outbuf, PI.rtu_out[idx], sizeof(outbuf)); pi_unlock();

    bool okW = mbRTU_syncWrite(RTU_CFG[idx].id, HREG_WRITE_START, outbuf, WORDS_OUT, MB_TIMEOUT_MS);
    if(!okW){
      Serial.printf("[RTU %u] WRITE fail/timeout (ID=%u)\n", idx, RTU_CFG[idx].id);
    }

    // --- mini pump: ancora qualche giro ---
    for (int i = 0; i < 3; ++i) {
      mbRTU.task();
      vTaskDelay(1);
    }

    // Round-robin sugli slave
    idx = (uint8_t)((idx + 1) % NUM_RTU);

    // Periodicità deterministica
    vTaskDelayUntil(&last, pdMS_TO_TICKS(RTU_CYCLE_MS));
  }
}
