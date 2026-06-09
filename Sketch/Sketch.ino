#include <stdint.h>

#include "Arduino.h"

#define TRAMP_SYNC_START            0x0F
#define TRAMP_SYNC_STOP             0x00
#define TRAMP_CMD_CMD_RF 'r'
#define TRAMP_CMD_GET_CONFIG    'v' // 0x76

uint8_t tramp_crc(const uint8_t *buf) {
  uint8_t cksum = 0;
  for (int i = 1; i < 14; i++) {
    cksum += buf[i];
  }
  return cksum;
}

void send_cmd(uint8_t cmd, uint16_t param = 0) {
  uint8_t buf[16] = {0};

  buf[0] = TRAMP_SYNC_START;
  buf[1] = cmd;
  buf[2] = (uint8_t)(param & 0xFF);
  buf[3] = (uint8_t)((param >> 8) & 0xFF);
  buf[14] = tramp_crc(buf);
  buf[15] = TRAMP_SYNC_STOP;

  while (Serial2.available()) Serial2.read();

  Serial2.write(buf, sizeof(buf));
  Serial2.flush();
}

size_t get_response(uint8_t *buf) {
  size_t cnt = 10;
  while (cnt--) {
    if (Serial2.available() >= 16) {
      size_t n = Serial2.readBytes(buf, 16);
      return n;
    }
    delay(50);
  }
  return 0;
}

static uint8_t rx_buf[16] = {0};

void setup() {
  Serial.begin(115200);

  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  if (!Serial2.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
    Serial.println("Failed to set RS485 mode");
  }

  Serial.println("sending init");
  send_cmd(TRAMP_CMD_CMD_RF);
  if (get_response(rx_buf) > 0) {
    Serial.println("received init response");
  }
}

void loop() {
  Serial.println("sending command");
  send_cmd(TRAMP_CMD_GET_CONFIG);
  if (get_response(rx_buf) > 0) {
    Serial.println("received response");
  }

  delay(5000);
}