#include "Arduino.h"

#include "vtx_tramp.h"

void dump_params() {
  Serial.print("Frequency: "); Serial.println(fetched_params.freq);
  Serial.print("Configured Power: "); Serial.println(fetched_params.pow);
  Serial.print("Control mode enabled (0/1): "); Serial.println(fetched_params.control_mode);
  Serial.print("Pit mode enabled (0/1): "); Serial.println(fetched_params.pit_mode);
  Serial.print("Actual Power: "); Serial.println(fetched_params.act_pow);
  Serial.println();
}

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

  for (int i = 0; i < 15; i++) {
    if (tramp_update_params()) dump_params();
    else Serial.println("Could not update parameters...");

    delay(1250);
  }
}

void loop() {
  Serial.println("===============================");
  if (tramp_update_params()) dump_params();
  else Serial.println("Could not update parameters...");

  set_freq = freq_table[25];
  tramp_apply_changes();
  delay(1000);

  if (tramp_update_params()) dump_params();
  else Serial.println("Could not update parameters...");

  Serial.println("Sleeping");
  delay(30000);
}