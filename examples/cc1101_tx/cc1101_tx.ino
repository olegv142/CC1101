#include "CC1101.h"

CC1101Transceiver g_tx(0xa55b, 7);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println(g_tx.begin());
  Serial.println(g_tx.get_state());
}

struct CC1101Packet g_pkt(5, 1);

void loop() {
  // put your main code here, to run repeatedly:
  CC1100State_t st = g_tx.get_state();
  if (st == st_IDLE) {
    ++g_pkt.data[0];
    g_tx.send_packet(&g_pkt);
    Serial.print("-> ");
    Serial.println(g_pkt.data[0]);
  } else {
    Serial.print("in state ");
    Serial.println(st);
  }
  delay(1000);
}

