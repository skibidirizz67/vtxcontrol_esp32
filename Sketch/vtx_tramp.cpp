#include "vtx_tramp.h"

TrampParams params_buf;
uint16_t temp_buf;

uint8_t tramp_crc(const uint8_t *buf) {
    uint8_t cksum = 0;
    const uint8_t *endp = buf + 14;
    for (buf += 1; buf < endp; ++buf) {
        cksum += *buf;
    }
    return cksum; 
}

bool tramp_write(const uint8_t *buf) {
    bool success = VTX_Serial.write(buf, TRAMP_FRAME_SIZE) == TRAMP_FRAME_SIZE;
    VTX_Serial.flush();
    VTX_Serial.read(); // clear garbage byte
    return success;
}

bool tramp_read(uint8_t *buf) {
    return VTX_Serial.read(buf, TRAMP_FRAME_SIZE) == TRAMP_FRAME_SIZE; 
}

bool tramp_send_cmd(uint8_t cmd, uint16_t param) {
    uint8_t buf[TRAMP_FRAME_SIZE];
    buf[0] = TRAMP_SYNC_START;
    buf[1] = cmd;
    buf[2] = param & 0xFF;
    buf[3] = (param >> 8) & 0xFF;
    buf[14] = tramp_crc(buf);
    buf[15] = TRAMP_SYNC_STOP;
    return tramp_write(buf);
}

bool tramp_receive_rsp(uint8_t *buf) {
    size_t waiting_ms = 0;
    while (VTX_Serial.available() < TRAMP_FRAME_SIZE) {
        if (waiting_ms >= TRAMP_RSP_TIMEOUT_MS) break;
        waiting_ms += 100;
        delay(100);
        Serial.println("too little frame, retrying");
        Serial.println(VTX_Serial.available());
    }

    if (VTX_Serial.available() < TRAMP_FRAME_SIZE) return false;
    Serial.println("enough frame");
    Serial.println(VTX_Serial.available());

    if (!buf) return false;
    Serial.println("buf not empty");
    if (!tramp_read(buf)) return false;
    Serial.println("read done");
    if (buf[0] != TRAMP_SYNC_START) return false;
    Serial.println("sync good");
    if (buf[14] != tramp_crc(buf)) return false;
    Serial.println("crc good");
    if (buf[15] != TRAMP_SYNC_STOP) return false;
    Serial.println("stop good");

    return true;
}

// TODO: make it work
TrampFrame buf2frame(const uint8_t *buf) {
    TrampFrame frame = {0};
    if (!buf) return frame;

    frame.sync_start = buf[0];
    frame.cmd = buf[1];
    memcpy(&frame.pld, buf+2, TRAMP_PLD_SIZE);
    frame.crc = buf[14];
    frame.sync_stop = buf[15];
}

bool tramp_handle_rsp(const TrampFrame *frame) {
    Serial.println("handling response");
    switch (frame->cmd) {
        case TRAMP_CMD_CMD_RF: {Serial.println("ignoring init");} break;
        case TRAMP_CMD_GET_CONFIG: {
            Serial.println("got config");
            if (frame->pld.freq == 0) break; 
            memcpy(&params_buf, &frame->pld.buf, sizeof(params_buf));
            Serial.println("updated config");
            Serial.println(frame->pld.freq);
            Serial.println(frame->pld.params.freq);
            Serial.println(frame->pld.params.pow);
            Serial.println(frame->pld.params.control_mode);
            Serial.println(frame->pld.params.pit_mode);
            Serial.println(frame->pld.params.act_pow);
        } break;
        case TRAMP_CMD_CMD_SENSOR: {
            Serial.println("got sensor");
            if (frame->pld.temp == 0) break; 
            Serial.println("updated senor");
            temp_buf = frame->pld.temp;
        } break;
        default: {
            Serial.println("sum garbage");
            return false;
        } break; 
    }
    Serial.println("done reponse");
    return true;
}

bool tramp_update() {
    if (!tramp_send_cmd(TRAMP_CMD_GET_CONFIG, 0)) return false;

    uint8_t buf[TRAMP_FRAME_SIZE];
    TrampFrame frame;

    if (!tramp_receive_rsp(buf)) return false;
    frame = buf2frame(buf);
    memcpy(&frame, buf, TRAMP_FRAME_SIZE);
    if (!tramp_handle_rsp(&frame)) return false;

    return true;
}

bool tramp_init() {
    if (!tramp_send_cmd(TRAMP_CMD_CMD_RF, 0)) return false;
    Serial.println("init sent");
    
    uint8_t buf[TRAMP_FRAME_SIZE];
    if (!tramp_receive_rsp(buf)) return false;
    Serial.println("rsp received");
    TrampFrame frame = buf2frame(buf);
    memcpy(&frame, buf, TRAMP_FRAME_SIZE);
    if (!tramp_handle_rsp(&frame)) return false;
    Serial.println("rsp handled");

    return true;
}

// clear uart from any leftover garbage
void tramp_clear_uart() {
    uint8_t tmp[1024];
    VTX_Serial.flush();
    VTX_Serial.read(tmp, 1024);
}
