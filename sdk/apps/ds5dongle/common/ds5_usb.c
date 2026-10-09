//
// Created by awalol on 2026/9/30.
//
#define LOG_TAG "[DS5_HID]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

#include "task.h"
#include "os/os_api.h"
#include "usb/device/custom_hid.h"
#include "ds5_report.h"
#include "ds5_usb.h"

static u8 state_data[64];

static bool task_created = false;
static OS_SEM hid_tx_sem;

void ds5_usb_tx_done(void)
{
    os_sem_post(&hid_tx_sem);
}

static void ds5_hid_report_task(void* priv)
{
    while (1)
    {
        if (!custom_hid_get_ready(1))
        {
            os_sem_pend(&hid_tx_sem, 1); // 发送完成时唤醒，超时后重查配置/端点状态
            continue;
        }
        ds5_report_state_copy(state_data + 1, 63);

        u32 len = custom_hid_tx_data(1, state_data, sizeof(state_data));

        if (len != sizeof(state_data))
        {
            os_time_dly(1); // 失败后退让，避免连续重试
            log_error("HID report data size mismatch: %d", len);
        }
    }
}

void ds5_usb_get_report(u8* buf, u32 len)
{
    ds5_report_feature_copy(buf[0],buf + 1,len - 1);
}

void ds5_usb_set_report(u8* buf, u32 len)
{
}

void ds5_usb_interrupt(u8* buf, u32 len)
{
    switch (buf[0])
    {
    case 0x02:
        {
            SetStateData state;
            memcpy(&state,buf + 1,len - 1);
            ds5_report_send_state(&state);
            break;
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
    os_sem_create(&hid_tx_sem, 0);
    int err = task_create(ds5_hid_report_task, NULL, "hid_report");
    if (err)
    {
        log_error("HID report task create failed: %d\n", err);
        return;
    }
    task_created = true;
    log_info("hid_report task created");
}
