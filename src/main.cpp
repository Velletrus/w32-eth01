
#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include "config.h"
#include "process_image.h"
#include "logical_task.h"
#include "modbustcp_client.h"
#include "modbusrtu_master.h"

static void printEthInfo() {
  Serial.printf("MAC: %s\n", ETH.macAddress().c_str());
  Serial.printf("IP : %s\n", ETH.localIP().toString().c_str());
  Serial.printf("GW : %s\n", ETH.gatewayIP().toString().c_str());
  Serial.printf("SN : %s\n", ETH.subnetMask().toString().c_str());
  Serial.printf("Link: %s, Speed: %d Mbps\n", ETH.linkUp() ? "UP" : "DOWN", ETH.linkSpeed());
}

void setup(){
  Serial.begin(115200);
  delay(200);

  // Ethernet
  esp_base_mac_addr_set((uint8_t*)BASE_MAC);  // prima della rete
  ETH.begin();
  ETH.config(STATIC_IP, GATEWAY, SUBNET, DNS1, DNS2);

  Serial.print("Attendo link...");
  while(!ETH.linkUp()){ delay(100); Serial.print("."); }
  Serial.println("\nLink OK");
  Serial.print("Attendo IP...");
  while(ETH.localIP() == IPAddress(0,0,0,0)){ delay(100); Serial.print("."); }
  Serial.println();
  printEthInfo();

  // Process Image
  pi_init();

  // Avvia task
  xTaskCreatePinnedToCore(TaskLogic,     "LOGIC",   4096, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskModbusTCP, "MB_TCP",  6144, nullptr, 1, nullptr, 1);
  xTaskCreatePinnedToCore(TaskModbusRTU, "MB_RTU",  6144, nullptr, 1, nullptr, 1);
}

void loop(){
  // tutto gira nei task
}
