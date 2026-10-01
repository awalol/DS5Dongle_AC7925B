//
// Created by awalol on 2026/9/30.
//

#include "asm/cpu.h"
#include "ds5_report.h"

struct report
{
    u8 data[63];
    u8 size;
};

// 0: 0x05 1:0x09 2:0x20 3:0x22
static struct report feature_report[4];
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

void ds5_report_feature_update(u8 report_id, const u8* data, u8 size)
{
    struct report* report;

    switch (report_id)
    {
    case 0x05:
        report = &feature_report[0];
        break;
    case 0x09:
        report = &feature_report[1];
        break;
    case 0x20:
        report = &feature_report[2];
        break;
    case 0x22:
        report = &feature_report[3];
        break;
    default:
        return;
    }

    u8 len = MIN(size, sizeof(report->data));
    memcpy(report->data, data, len);
    report->size = len;
}

u8 ds5_report_feature_copy(u8 report_id, u8* dst, u8 size)
{
    struct report* report;

    switch (report_id)
    {
    case 0x05:
        report = &feature_report[0];
        break;
    case 0x09:
        report = &feature_report[1];
        break;
    case 0x20:
        report = &feature_report[2];
        break;
    case 0x22:
        report = &feature_report[3];
        break;
    default:
        return 0;
    }

    u8 len = MIN(size, report->size);
    memcpy(dst, report->data, len);

    return len;
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
