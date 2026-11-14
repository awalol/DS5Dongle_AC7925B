//
// Created by awalol on 2026/9/30.
//

#include "asm/cpu.h"
#include "ds5_report.h"
#include "spinlock.h"

#include "bt.h"

#define LOG_TAG "[DS5_REPORT]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"
#include "ds5_state.h"

struct report
{
    u8 id;
    u8 data[63];
    u8 size;
};

static struct report feature_report[] = {
    {.id = 0x05},
    {.id = 0x09},
    {.id = 0x20},
    {.id = 0x22}
};

static DEFINE_SPINLOCK(feature_report_lock);
static USBGetStateData state;
static const uint8_t state_init_data[63] = {
    0x7f, 0x7d, 0x7f, 0x7e, 0x00, 0x00, 0xa7,
    0x08, 0x00, 0x00, 0x00, 0x52, 0x43, 0x30, 0x41,
    0x01, 0x00, 0x0e, 0x00, 0xef, 0xff, 0x03, 0x03,
    0x7b, 0x1b, 0x18, 0xf0, 0xcc, 0x9c, 0x60, 0x00,
    0xfc, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x00, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xa7, 0xad, 0x60, 0x00, 0x29, 0x18, 0x00,
    0x53, 0x9f, 0x28, 0x35, 0xa5, 0xa8, 0x0c, 0x8b
};
static u8 reportSeqCounter = 0;
static u8 packetCounter = 0;

void ds5_report_feature_update(u8 report_id, const u8* data, u8 size)
{
    for (int i = 0;i < sizeof(feature_report) / sizeof(struct report);i++)
    {
        struct report *report = &feature_report[i];
        if (report->id != report_id)
        {
            continue;
        }
        u8 len = MIN(size, sizeof(report->data));
        spin_lock(&feature_report_lock);
        memcpy(report->data, data, len);
        report->size = len;
        spin_unlock(&feature_report_lock);
        break;
    }
}

u8 ds5_report_feature_copy(u8 report_id, u8* dst, u8 size)
{
    for (int i = 0;i < sizeof(feature_report) / sizeof(struct report);i++)
    {
        struct report *report = &feature_report[i];
        if (report->id != report_id)
        {
            continue;
        }
        spin_lock(&feature_report_lock);
        u8 len = MIN(size, report->size);
        memcpy(dst, report->data, len);
        spin_unlock(&feature_report_lock);
        return len;
    }
    return 0;
}

void ds5_report_state_update(const u8* data, const u8 len)
{
    u8 size = MIN(len, sizeof(SetStateData));
    memcpy(&state, data, size);
}

u8 ds5_report_state_copy(u8* dst, u8 len)
{
    u8 size = MIN(len, sizeof(SetStateData));
    memcpy(dst, &state, size);
    return size;
}

void ds5_report_state_reset()
{
    memcpy(&state, state_init_data, sizeof(state_init_data));
}

void ds5_report_send_state(const SetStateData* state)
{
    u8 packet[78] = {0};
    packet[0] = 0x31;
    packet[1] = reportSeqCounter << 4;
    reportSeqCounter = (reportSeqCounter + 1) & 0x0F;
    packet[2] = 0x10;
    memcpy(packet + 3,state,sizeof(SetStateData));
    bt_hid_interrupt_send(packet,sizeof(packet));
}

void ds5_report_send_state_raw(const u8 *data, const u8 len)
{
    u8 packet[78] = {0};
    packet[0] = 0x31;
    packet[1] = reportSeqCounter << 4;
    reportSeqCounter = (reportSeqCounter + 1) & 0x0F;
    packet[2] = 0x10;
    memcpy(packet + 3,data,len);
    bt_hid_interrupt_send(packet,sizeof(packet));
}

void ds5_report_send_haptics(const s8* haptics)
{
    uint8_t pkt[142] = {0};
    pkt[0] = 0x32;
    pkt[1] = reportSeqCounter << 4;
    reportSeqCounter = (reportSeqCounter + 1) & 0x0F;
    pkt[2] = 0x11 | 0 << 6 | 1 << 7;
    pkt[3] = 6;
    pkt[4] = false ? 0b01111111 : 0b01111110;
    const u8 buf_len = 48;
    pkt[5] = buf_len; // VolumeHeadphones - guess but no work
    pkt[6] = buf_len; // VolumeMic - guess but no work
    pkt[7] = buf_len; // VolumeSpeaker - guess but no work
    pkt[8] = buf_len; // AudioBufferLength
    pkt[9] = packetCounter++;
    pkt[10] = 0x12 | 0 << 6 | 1 << 7;
    pkt[11] = 64;
    memcpy(pkt + 12, haptics, 64);
    bt_hid_interrupt_send(pkt,sizeof(pkt));
}

void ds5_report_send_speaker_haptics(const u8* speaker,const s8* haptics)
{
    u16 i = 0;
    uint8_t pkt[334] = {0};
    pkt[i++] = 0x35;
    pkt[i++] = reportSeqCounter << 4;
    reportSeqCounter = (reportSeqCounter + 1) & 0x0F;
    pkt[i++] = 0x11 | 0 << 6 | 1 << 7;
    pkt[i++] = 6;
    pkt[i++] = false ? 0b01111111 : 0b01111110;
    const u8 buf_len = 48;
    pkt[i++] = buf_len; // VolumeHeadphones - guess but no work
    pkt[i++] = buf_len; // VolumeMic - guess but no work
    pkt[i++] = buf_len; // VolumeSpeaker - guess but no work
    pkt[i++] = buf_len; // AudioBufferLength
    pkt[i++] = packetCounter++;
    pkt[i++] = 0x13 | 0 << 6 | 1 << 7;
    pkt[i++] = 200;
    memcpy(pkt + i, speaker, 200);
    i += 200;
    pkt[i++] = 0x12 | 0 << 6 | 1 << 7;
    pkt[i++] = 64;
    memcpy(pkt + i, haptics, 64);

    bt_hid_interrupt_send(pkt,sizeof(pkt));
}

void ds5_report_send_state_speaker_haptics(const u8* speaker,const s8* haptics) {
    u16 i = 0;
    uint8_t pkt[398] = {0};
    pkt[i++] = 0x36;
    pkt[i++] = reportSeqCounter << 4;
    reportSeqCounter = (reportSeqCounter + 1) & 0x0F;
    pkt[i++] = 0x10 | 0 << 6 | 1 << 7;
    pkt[i++] = 63;
    ds5_state_copy_volume_state(pkt + i,63);
    i += 63;
    pkt[i++] = 0x11 | 0 << 6 | 1 << 7;
    pkt[i++] = 6;
    pkt[i++] = false ? 0b01111111 : 0b01111110;
    const u8 buf_len = 40;
    pkt[i++] = buf_len; // VolumeHeadphones - guess but no work
    pkt[i++] = buf_len; // VolumeMic - guess but no work
    pkt[i++] = buf_len; // VolumeSpeaker - guess but no work
    pkt[i++] = buf_len; // AudioBufferLength
    pkt[i++] = packetCounter++;
    pkt[i++] = 0x16 | 0 << 6 | 1 << 7;
    pkt[i++] = 200;
    memcpy(pkt + i, speaker, 200);
    i += 200;
    pkt[i++] = 0x12 | 0 << 6 | 1 << 7;
    pkt[i++] = 64;
    memcpy(pkt + i, haptics, 64);

    bt_hid_interrupt_send(pkt,sizeof(pkt));
}

void ds5_report_send_double_speaker_haptics(const u8* speaker, const u8* speaker2,const s8* haptics, const s8* haptics2)
{
    u16 i = 0;
    uint8_t pkt[547] = {0};
    pkt[i++] = 0x39;
    pkt[i++] = reportSeqCounter << 4;
    reportSeqCounter = (reportSeqCounter + 1) & 0x0F;
    pkt[i++] = 0x11 | 0 << 6 | 1 << 7;
    pkt[i++] = 6;
    pkt[i++] = false ? 0b01111111 : 0b01111110;
    const u8 buf_len = 40;
    pkt[i++] = buf_len; // VolumeHeadphones - guess but no work
    pkt[i++] = buf_len; // VolumeMic - guess but no work
    pkt[i++] = buf_len; // VolumeSpeaker - guess but no work
    pkt[i++] = buf_len; // AudioBufferLength
    pkt[i++] = packetCounter;
    packetCounter += 2;
    pkt[i++] = 0x13 | 1 << 6 | 1 << 7;
    pkt[i++] = 200;
    memcpy(pkt + i, speaker, 200);
    i += 200;
    memcpy(pkt + i, speaker2, 200);
    i += 200;
    pkt[i++] = 0x12 | 1 << 6 | 1 << 7;
    pkt[i++] = 64;
    memcpy(pkt + i, haptics, 64);
    i += 64;
    memcpy(pkt + i, haptics2, 64);

    bt_hid_interrupt_send(pkt,sizeof(pkt));

    u8 state[63];
    ds5_state_copy_volume_state(state,sizeof(state));
    ds5_report_send_state_raw(state,sizeof(state));
}