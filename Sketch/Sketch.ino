#include "Arduino.h"

#include "vtx_tramp.h"

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  VTX_Serial.begin(9600, SERIAL_8N1, 16, 17);
  if (!VTX_Serial.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
    Serial.println("Failed to set RS485 mode");
  }

  while (!tramp_init()) {
    Serial.println("Could not initialize, retrying...");
    tramp_clear_uart();
    delay(100);
  }
  Serial.println("Initialized!");
}

void loop() {
  if (tramp_update()) {
    Serial.println(params_buf.freq);
    Serial.println(params_buf.pow);
    Serial.println(params_buf.control_mode);
    Serial.println(params_buf.pit_mode);
    Serial.println(params_buf.act_pow);
  }
  else {
    Serial.println("Could not update parameters...");
  }

  delay(1000);
}