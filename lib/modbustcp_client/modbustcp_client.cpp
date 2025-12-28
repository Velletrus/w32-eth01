
#include <Arduino.h>
#include <WiFiClient.h>
#include <ModbusIP_ESP8266.h>
#include <string.h>
#include "config.h"
#include "process_image.h"

static ModbusIP mbTCP;                 // unico stack TCP per più server
static uint32_t nextTry[NUM_TCP] = {0,0,0};

static bool mbTCP_syncRead(IPAddress ip, uint16_t start, uint16_t* dest, uint16_t qty, uint8_t unit, uint32_t toMs){
  uint16_t tid = mbTCP.readHreg(ip, start, dest, qty, nullptr, unit);
  if(!tid) return false;
  uint32_t t0 = millis();
  while(mbTCP.isTransaction(tid) && (millis()-t0)<toMs){ mbTCP.task(); delay(1); }
  return !mbTCP.isTransaction(tid);
}

static bool mbTCP_syncWrite(IPAddress ip, uint16_t start, const uint16_t* src, uint16_t qty, uint8_t unit, uint32_t toMs){
  uint16_t tmp[WORDS_OUT];
  for(uint16_t i=0;i<qty;i++) tmp[i] = src[i];
  uint16_t tid = mbTCP.writeHreg(ip, start, tmp, qty, nullptr, unit);
  if(!tid) return false;
  uint32_t t0 = millis();
  while(mbTCP.isTransaction(tid) && (millis()-t0)<toMs){ mbTCP.task(); delay(1); }
  return !mbTCP.isTransaction(tid);
}

void TaskModbusTCP(void*){
  mbTCP.client();
  uint8_t idx = 0;
  TickType_t last = xTaskGetTickCount();

  for(;;){
    // Gestione connessione
    if(!mbTCP.isConnected(TCP_CFG[idx].ip) && millis() >= nextTry[idx]){
      Serial.printf("[TCP %u] connect %s:%u (UID=%u)\n",
        idx, TCP_CFG[idx].ip.toString().c_str(), TCP_CFG[idx].port, TCP_CFG[idx].unitId);
      mbTCP.connect(TCP_CFG[idx].ip, TCP_CFG[idx].port);
      nextTry[idx] = millis() + 2000;
    }

    // Se connesso: READ 20 da 0..19, WRITE 20 a 100..119
    if(mbTCP.isConnected(TCP_CFG[idx].ip)){
      uint16_t inbuf[WORDS_IN];

      bool okR = mbTCP_syncRead(TCP_CFG[idx].ip, HREG_READ_START, inbuf, WORDS_IN, TCP_CFG[idx].unitId, MB_TIMEOUT_MS);
      if(okR){
        pi_lock();
        memcpy(PI.tcp_in[idx], inbuf, sizeof(inbuf));
        PI.tcp_stat[idx] = 1;
        pi_unlock();
      } else {
        pi_lock(); PI.tcp_stat[idx] = 0; pi_unlock();
        Serial.printf("[TCP %u] READ fail/timeout\n", idx);
      }

      uint16_t outbuf[WORDS_OUT];
      pi_lock(); memcpy(outbuf, PI.tcp_out[idx], sizeof(outbuf)); pi_unlock();
      bool okW = mbTCP_syncWrite(TCP_CFG[idx].ip, HREG_WRITE_START, outbuf, WORDS_OUT, TCP_CFG[idx].unitId, MB_TIMEOUT_MS);
      if(!okW){
        Serial.printf("[TCP %u] WRITE fail/timeout\n", idx);
      }
    }

    mbTCP.task(); // importante
    idx = (uint8_t)((idx + 1) % NUM_TCP);
    vTaskDelayUntil(&last, pdMS_TO_TICKS(TCP_CYCLE_MS));
  }
}
