#include "CC1101.h"

CC1101Transceiver g_tx(0xa55b, 5);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println(g_tx.begin());
  // Start receiving
  g_tx.receive();
}

struct CC1101RxPacket g_pkt;

void loop() {
  // put your main code here, to run repeatedly:
  CC1100State_t st = g_tx.get_state();
  Serial.print("state=");
  Serial.print(st);
  Serial.print(" ");
  if (st == st_IDLE) {
    // Transceiver goes to idle state on capturing packet from the air
    // So lets retrieve it and check
    if (g_tx.get_packet(&g_pkt)) {
      if (g_pkt.valid()) {
        struct CC1101PacketRxInfo const* i = g_pkt.info();
        Serial.print(g_pkt.payload_len());
        Serial.print(" bytes @");
        Serial.print(g_pkt.addr);
        Serial.print(" : ");
        Serial.print(g_pkt.data[0]);
        Serial.print(" rssi=");
        Serial.println(i->rssi);
      } else {
        Serial.println("bad packet");
      }
    } else {
      Serial.println("no packet");
    }
    // Restart receiving
    g_tx.receive();
  } else {
        Serial.print("idle channel rssi=");
        Serial.println(g_tx.get_rssi());
  }
  // The delay between status checks should be less than the packet sending interval
  delay(500);
}

