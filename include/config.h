
#pragma once
#include <Arduino.h>
#include <IPAddress.h>

// ------------------- Rete ETH (WT32-ETH01) -------------------
static const uint8_t BASE_MAC[6] = { 0x02, 0xA5, 0xD7, 0xEF, 0x11, 0x01 };
static const IPAddress STATIC_IP (192,168,1,85);
static const IPAddress GATEWAY   (192,168,1,254);
static const IPAddress SUBNET    (255,255,255,0);
static const IPAddress DNS1      (8,8,8,8);
static const IPAddress DNS2      (8,8,4,4);

// ------------------- Mappa Modbus -----------------------------
// 20 word in lettura (HREG 0..19) e 20 in scrittura (HREG 100..119)
static const uint16_t WORDS_IN          = 20;
static const uint16_t WORDS_OUT         = 20;
static const uint16_t HREG_READ_START   = 0;    // offset 0-based  (≃ 40001)
static const uint16_t HREG_WRITE_START  = 50;  // offset 0-based  (≃ 40101)

// ------------------- Timing ----------------------------------
static const uint32_t TCP_CYCLE_MS   = 201;   // polling round-robin
static const uint32_t RTU_CYCLE_MS   = 251;
static const uint32_t LOGIC_MS       = 1000;
static const uint32_t MB_TIMEOUT_MS  = 600;

// ------------------- Modbus TCP (3 slave) --------------------
static const uint8_t  NUM_TCP = 3;
struct TcpCfg {
  IPAddress ip;
  uint16_t  port;
  uint8_t   unitId;
};
static const TcpCfg TCP_CFG[NUM_TCP] = {
  { IPAddress(192,168,1,95), 5022, 1 },
  { IPAddress(192,168,1,95), 5022, 2 },
  { IPAddress(192,168,1,95), 5022, 3 }
};

// ------------------- Modbus RTU (3 slave) --------------------
static const uint8_t  NUM_RTU = 3;
struct RtuCfg {
  uint8_t id;
};
static const RtuCfg RTU_CFG[NUM_RTU] = { {1}, {2}, {3} };

static const uint32_t RTU_BAUD   = 19200;
static const int PIN_RE_DE       = 33;  // HIGH=TX (DE/RE)
static const int PIN_UART_RX     = 5;  // UART2 RX
static const int PIN_UART_TX     = 17;  // UART2 TX;
