//
// Created by awalol on 2026/9/29.
//

#define LOG_TAG             "[DS5_BT]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

#include "ds5_bt.h"

#include "bt.h"
#include "ibtstack.h"
#include "timer.h"

struct report
{
    u8 data[63];
    u8 size;
};
// 0: 0x05 1:0x09 2:0x20 3:0x22
static struct report feature_repot[4];


void ds5_hid_input_report(const u8 *packet, u16 size) {
    (void) packet;
    (void) size;

    static u32 start_ms;
    static u32 packets;
    static bool initialized;
    const u32 log_interval_ms = 1000; // 输出间隔，单位 ms
    u32 now_ms = sys_timer_get_ms();


    if (!initialized) {
        initialized = true;
        start_ms = now_ms;
        return; // 首包作为统计窗口起点
    }

    packets++;
    u32 elapsed_ms = now_ms - start_ms;
    if (elapsed_ms >= log_interval_ms) {
        u32 rate_x10 = (u32)((u64)packets * 10000U / elapsed_ms);
        log_info("HID Interrupt RX: packets=%u, interval=%u ms, rate=%u.%u Hz",
                 (unsigned int)packets, (unsigned int)elapsed_ms,
                 (unsigned int)(rate_x10 / 10U), (unsigned int)(rate_x10 % 10U));

        // log_debug_hexdump(packet,size);

        start_ms = now_ms;
        packets = 0;
    }


}

void ds5_hid_control_data(const u8 *packet, u16 size) {
    (void) packet;
    (void) size;

    log_debug("Receive Control Data >>");
    log_debug_hexdump(packet,size);
}

void ds5_on_connected(void) {
    bt_hid_control_get(0x05);
    bt_hid_control_get(0x09);
    bt_hid_control_get(0x20);
    bt_hid_control_get(0x22);
}
