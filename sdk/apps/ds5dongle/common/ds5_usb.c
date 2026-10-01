//
// Created by awalol on 2026/9/30.
//
#define LOG_TAG "[DS5_USB]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

#include "task.h"
#include "usb/device/custom_hid.h"
#include "ds5_report.h"
#include "ds5_usb.h"

static u8 state_data[64];

static bool task_created = false;

static void ds5_hid_report_task(void* priv)
{
    while (1)
    {
        if (!custom_hid_get_ready(1))
        {
            os_time_dly(1); // 未就绪时让出 CPU
            continue;
        }
        ds5_report_state_copy(state_data + 1, 63);

        u32 len = custom_hid_tx_data(1, state_data, sizeof(state_data));

        if (len != sizeof(state_data))
        {
            os_time_dly(1); // 失败后退让，避免连续重试
        }
    }
}

void ds5_usb_init()
{
    memset(state_data, 0, sizeof(state_data));
    state_data[0] = 0x01; // USB Input Report ID
    ds5_report_state_reset();

    if (task_created)
    {
        return;
    }
    int err = task_create(ds5_hid_report_task, NULL, "hid_report");
    if (err)
    {
        log_error("HID report task creation failed: %d\n", err);
        return;
    }
    task_created = true;
    log_info("hid_report task created");
}
