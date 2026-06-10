#include "Arduino.h"

#include "VTX_Tramp.h"

int pwr_Level = -1;//default value -1 means not updated (not requested from VTX)
int ch_index = -1;//default value -1 means not updated (not requested from VTX)
bool pitMode = false;
bool initialized = false;

bool trampPush(const uint8_t* packet)//const trampFrame_t* object)
{  
  //port->writeDummyByte();//to get port low before sending command
  //port->write((uint8_t)0x00);  
  //bool res = port->write((uint8_t*)&object, sizeof(trampFrame_t)) == sizeof(trampFrame_t);
  bool res = Serial2.write(packet, TRAMP_FRAME_LENGTH) == TRAMP_FRAME_LENGTH;  
  Serial2.flush();
  Serial2.read();
  return res;
}

bool trampReadResponse()
{  
  unsigned long currentTime = 0;
  while (Serial2.available() == 0)
  {
    if (currentTime > 10000)
    {
      //DEBUG("trampReadResponse, time out of waiting response");
      break;// return false;
    }
    delay(100);
    currentTime += 100;
  }
  
  //DEBUG("trampReadResponse, before fill rx buf");
  int16_t incoming_bytes_count = Serial2.available();
  if (incoming_bytes_count == 0)
  {    
    Serial.println("no bytes came");
    return false;
  }
  if (incoming_bytes_count == TRAMP_FRAME_LENGTH)
  {
    uint8_t tramp_rx_buf[TRAMP_FRAME_LENGTH];
    //fill out the buffer
    for (int i = 0; i < TRAMP_FRAME_LENGTH; i++)
    {
      uint8_t d = Serial2.read();
      tramp_rx_buf[i] = d;
    }
    // Buffer is full, calculate checksum
    const trampFrame_t* frame = (const trampFrame_t*)tramp_rx_buf;
    if (frame->header.syncStart != TRAMP_SYNC_START)
    {
      Serial.println("trampReadResponse, first header byte not equal SyncByte");;
      return false;
    }
    if (frame->footer.syncStop != TRAMP_SYNC_STOP)
    {
      Serial.println("trampReadResponse, last byte not equal Sync Stop");
      return false;
    }
    //const uint8_t crc = trampCrc(frame);
    const uint8_t crc = trampCrc(tramp_rx_buf);
    if (crc != frame->footer.crc) 
    {
      Serial.println("trampReadResponse, Incorrect checksum");
      return false;
    }    
    Serial.println("trampReadResponse: Update Data");
    //TODO:here we need to update data dependently of response code (cmd)
    switch (frame->header.command) 
    {
    case TRAMP_COMMAND_CMD_RF://'r', Init
    {
      //we ignore this response, just parse
      Serial.println("trampReadResponse: Response from Init: do nothing");
      break;
    }
    case TRAMP_COMMAND_GET_CONFIG://'v' //we update data for current settings just sending TRAMP_COMMAND_GET_CONFIG and reading request
    {
      Serial.println("trampReadResponse: Response from Get_CONFIG: Update data");
      const uint16_t freq = frame->payload.settings.frequency;
      // Check we're not reading the request (indicated by freq zero)
      if (freq != 0)
      {
        //update data
        //ch_index = getChannelIndex(frame->payload.settings.frequency);
        //pwr_Level = getPowerIndexFromMW(frame->payload.settings.power);
        pitMode = (bool)frame->payload.settings.pitModeEnabled;        
        Serial.println("trampReadResponse: Updated, Freq:" + (String)frame->payload.settings.frequency);
        Serial.println("trampReadResponse: Converted to Channel:" + (String)ch_index);
        Serial.println("trampReadResponse: Updated, Power in Mw:" + (String)frame->payload.settings.power);
        Serial.println("trampReadResponse: Converted to Power Level:" + (String)pwr_Level);
        Serial.println("trampReadResponse: Updated, PitMode:" + (String)frame->payload.settings.pitModeEnabled);
      }
      break;
    }
    case TRAMP_COMMAND_CMD_SENSOR://'s' - temperature sensor, currently not used
    {
      //currently we ignore this response, just parse
      Serial.println("trampReadResponse: Response from CMD_Sensor: do nothing");
      break;
    }
    }     
    //clear port
    Serial2.flush();    
    // successful response, wait another 100ms to give the VTX a chance to recover
    // before sending another command.
    delay(100);//çäåñü ìîæíî èñïîëüçîâàòü delay
    Serial.println("trampReadResponse:Succesfull");
    return true;
  }
  else
  {
    Serial.print("trampReadResponse2, incoming_bytes_count != TRAMP_FRAME_LENGTH: ");
    Serial.println(incoming_bytes_count);
  }
  //clear port
  Serial2.flush();    
  return false;
}

bool trampSendPacket(uint8_t * packet, bool respRequired)//(trampFrame_t* packet)
{    
  if (trampPush(packet))
  {        
    //with setPower and setChannel we don't wait a response, 
    //in this case we send UpdateParameters(GetConfig) to get new parameters
    bool res = respRequired ? trampReadResponse() : true;
    return res;
  }
  return false;
}

bool trampSendCmd(uint8_t cmd, uint16_t param) {
  uint8_t buf[TRAMP_FRAME_LENGTH];
  memset(buf, 0, TRAMP_FRAME_LENGTH);
  buf[0] = TRAMP_SYNC_START;
  buf[1] = cmd;
  if (param != 0)
  {
    buf[2] = param & 0xff;
    buf[3] = (param >> 8) & 0xff;
  }
  buf[14] = trampCrc(buf);
  buf[15] = TRAMP_SYNC_STOP;
  //we need a reponse for certain commands only
  //so calling UpdateParameters required after send command
  bool respRequired = cmd == TRAMP_COMMAND_GET_CONFIG || cmd == TRAMP_COMMAND_CMD_RF || TRAMP_COMMAND_CMD_SENSOR;
  return trampSendPacket(buf, respRequired);
}

bool trampInit() {
  if (trampSendCmd(TRAMP_COMMAND_CMD_RF, 0))
  { 
    Serial.println("TrampInit: Inititalized Successfully");
    initialized = true;
    return true;
  }  
	Serial.println("TrampInit: Not Inititalized");
	return false;
}

void setup() {
  Serial.begin(115200);

  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  if (!Serial2.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
    Serial.println("Failed to set RS485 mode");
  }
}

void loop() {
  for (int i = 0; i < 16; i++) {
    if (trampInit()) {
      Serial.println("initialized successfully");
      break;
    }
  }
  
  delay(300);
}