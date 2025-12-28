
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
  return true; // keep transaction object
}

// Helper: attende il completamento (con timeout) eseguendo mbRTU.task()
static bool rtu_wait(uint32_t timeoutMs) {
  uint32_t t0 = millis();
  while (!rtu_done && (millis() - t0) < timeoutMs) {
    mbRTU.task();
    delay(1);
  }
  return rtu_done && rtu_ok;
}

// Lettura sincrona (tramite callback + attesa)
static bool mbRTU_syncRead(uint8_t id, uint16_t start, uint16_t* dest, uint16_t qty, uint32_t timeoutMs){
  rtu_done = false; rtu_ok = false;
  // Avvio transazione: versione con buffer e callback
  // Nota: la libreria copia internamente i dati letti in 'dest' quando completata
  if (!mbRTU.readHreg(id, start, dest, qty, rtu_cb)) return false;
  return rtu_wait(timeoutMs);
}

// Scrittura sincrona (tramite callback + attesa)
static bool mbRTU_syncWrite(uint8_t id, uint16_t start, const uint16_t* src, uint16_t qty, uint32_t timeoutMs){
  rtu_done = false; rtu_ok = false;
  // La write usa il buffer passato (meglio copiarlo su stack per sicurezza)
  uint16_t tmp[WORDS_OUT];
  for (uint16_t i=0; i<qty; i++) tmp[i] = src[i];

  if (!mbRTU.writeHreg(id, start, tmp, qty, rtu_cb)) return false;
  return rtu_wait(timeoutMs);
}

void TaskModbusRTU(void*){
  // RS-485 transceiver: DE/RE su PIN_RE_DE (HIGH=TX). La libreria può gestirlo automaticamente
  pinMode(PIN_RE_DE, OUTPUT);
  digitalWrite(PIN_RE_DE, LOW); // ricezione di default

  // UART2
  SerialRTU.begin(RTU_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);

  // In emelianov: begin(Stream*, int8_t TxEnablePin) abilita l'auto-toggle DE/RE
  mbRTU.begin(&SerialRTU, PIN_RE_DE);
  mbRTU.master();
  // Nessun setTimeout() nella RTU; gestiamo noi i timeout con rtu_wait()

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

    // --- WRITE 20 word a 100..119 ---
    uint16_t outbuf[WORDS_OUT];
    pi_lock(); memcpy(outbuf, PI.rtu_out[idx], sizeof(outbuf)); pi_unlock();

    bool okW = mbRTU_syncWrite(RTU_CFG[idx].id, HREG_WRITE_START, outbuf, WORDS_OUT, MB_TIMEOUT_MS);
    if(!okW){
      Serial.printf("[RTU %u] WRITE fail/timeout (ID=%u)\n", idx, RTU_CFG[idx].id);
    }

    idx = (uint8_t)((idx + 1) % NUM_RTU);
    vTaskDelayUntil(&last, pdMS_TO_TICKS(RTU_CYCLE_MS));
  }
}
