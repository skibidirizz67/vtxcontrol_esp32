#include "vtx_tramp.h"

/*
# vtxtable
vtxtable bands 6
vtxtable channels 8
vtxtable band 1 BOSCAM_A A CUSTOM 5865 5845 5825 5805 5785 5765 5745 5725
vtxtable band 2 BOSCAM_B B CUSTOM 5733 5752 5771 5790 5999 5828 5847 5866
vtxtable band 3 BOSCAM_E E CUSTOM 5705 5685 5665 5645 5885 5905 5925 5945
vtxtable band 4 FATSHARK F CUSTOM 5740 5760 5780 5800 5820 5840 5860 5880
vtxtable band 5 RACEBAND R CUSTOM 5658 5695 5732 5769 5806 5843 5880 5917
vtxtable band 6 LOWRACE L CUSTOM 5362 5399 5436 5473 5510 5547 5584 5621
vtxtable powerlevels 5
vtxtable powervalues 25 100 200 400 600
vtxtable powerlabels 25 400 800 1.5 2.5
*/

const uint16_t freq_table[48] = {
    5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // vtxtable band 1 BOSCAM_A A CUSTOM
    5733, 5752, 5771, 5790, 5999, 5828, 5847, 5866, // vtxtable band 2 BOSCAM_B B CUSTOM
    5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // vtxtable band 3 BOSCAM_E E CUSTOM
    5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // vtxtable band 4 FATSHARK F CUSTOM
    5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // vtxtable band 5 RACEBAND R CUSTOM
    5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621, // vtxtable band 6 LOWRACE L CUSTOM 
};
const uint16_t power_table[] = {25, 100, 200, 400, 600};
const char *power_labels[] = {"25", "400", "800", "1.5", "2.5"};

TrampParams fetched_params;
uint16_t fetched_temp;

uint16_t set_freq;
uint16_t set_pow;

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
    }

    if (VTX_Serial.available() < TRAMP_FRAME_SIZE) return false;

    if (!buf) return false;
    if (!tramp_read(buf)) return false;
    if (buf[0] != TRAMP_SYNC_START) return false;
    if (buf[14] != tramp_crc(buf)) return false;
    if (buf[15] != TRAMP_SYNC_STOP) return false;

    return true;
}

TrampFrame buf2frame(const uint8_t *buf) {
    TrampFrame frame = {0};
    if (!buf) return frame;

    frame.sync_start = buf[0];
    frame.cmd = buf[1];
    memcpy(&frame.pld, buf+2, TRAMP_PLD_SIZE);
    frame.crc = buf[14];
    frame.sync_stop = buf[15];

    return frame;
}

bool tramp_handle_rsp(const TrampFrame *frame) {
    switch (frame->cmd) {
        case TRAMP_CMD_CMD_RF: {} break;
        case TRAMP_CMD_GET_CONFIG: {
            if (frame->pld.freq == 0) break; 
            memcpy(&fetched_params, &frame->pld.buf, sizeof(fetched_params));
        } break;
        case TRAMP_CMD_CMD_SENSOR: {
            if (frame->pld.temp == 0) break; 
            fetched_temp = frame->pld.temp;
        } break;
        default: {
            return false;
        } break; 
    }
    return true;
}

bool tramp_update_params() {
    if (!tramp_send_cmd(TRAMP_CMD_GET_CONFIG, 0)) return false;

    uint8_t buf[TRAMP_FRAME_SIZE];
    TrampFrame frame;

    if (!tramp_receive_rsp(buf)) return false;
    frame = buf2frame(buf);
    memcpy(&frame, buf, TRAMP_FRAME_SIZE);
    if (!tramp_handle_rsp(&frame)) return false;

    return true;
}

bool tramp_apply_changes() {
    tramp_send_cmd(TRAMP_CMD_SET_FREQ, set_freq);
    tramp_send_cmd(TRAMP_CMD_SET_POW, set_pow);

    return true;
}

bool tramp_init() {
    if (!tramp_send_cmd(TRAMP_CMD_CMD_RF, 0)) return false;
    
    uint8_t buf[TRAMP_FRAME_SIZE];
    if (!tramp_receive_rsp(buf)) return false;
    TrampFrame frame = buf2frame(buf);
    memcpy(&frame, buf, TRAMP_FRAME_SIZE);
    if (!tramp_handle_rsp(&frame)) return false;

    return true;
}

// clear uart from any leftover garbage
void tramp_clear_uart() {
    uint8_t tmp[1024];
    VTX_Serial.flush();
    VTX_Serial.read(tmp, 1024);
}
