//
// Created by awalol on 2026/9/30.
//

#include "asm/cpu.h"
#include "ds5_report.h"
#include "spinlock.h"

#include "bt.h"

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
u8 reportSeqCounter = 0;
u8 packetCounter = 0;

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
