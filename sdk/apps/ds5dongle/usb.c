//
// Created by awalol on 2026/9/28.
//
#include "app_config.h"
#include "usb/device/usb_stack.h"
#include "usb/device/custom_hid.h"
#include "ds5_usb.h"

#define LOG_TAG             "[DS5_USB]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_DISABLE
#include "debug.h"
#include "ds5_audio.h"
#include "usb/device/uac_audio.h"
#include "app_core.h"
#include "device_event.h"
#include "device/uac_stream.h"
#include "usb.h"

#include "ds5_state.h"


static void custom_hid_rx_handler(const usb_dev usb_id, void *priv, u8 *buf, u32 len)
{
    log_debug("custom_hid_rx_handler >");
    log_debug_hexdump(buf,len);

    ds5_usb_interrupt(buf, len);
}

static void custom_hid_get_report_handler(const usb_dev usb_id, void *priv, u8 *buf, u32 len) {
    log_debug("custom_hid_get_report_handler reply >");
    log_debug_hexdump(buf,len);

    ds5_usb_get_report(buf, len);
}

static void custom_hid_set_report_handler(const usb_dev usb_id, void *priv, u8 *buf, u32 len) {
    log_debug("custom_hid_set_report_handler >");
    log_debug_hexdump(buf,len);

    ds5_usb_set_report(buf, len);
}

static void uac_speaker_rx_handler(void *priv, void *data, int size)
{
    ds5_audio_rx(data,size);
}

static int uac_mic_tx_handler(void *priv, void *data, int size) {
    memset(data,0,size);
    return size;
}

/**
 * @brief      usb 进入从机模式
 * @param:     usbfd: usb 端口号 0: FUSB端口  1:HUSB端口
 * @param:     class: 从机类指定
 *             0:表示用板级头文件的usb设备类宏定义
 *             1:表示指定usb设备类宏定义
 * @return:    none
 **/
void usb_start(const usb_dev usbfd, u32 class)
{
    log_info("usb_start");
    if (class == 0) {
        class = USB_DEVICE_CLASS_CONFIG_2_0;
    }

    usb_device_mode(usbfd, class);
    custom_hid_set_rx_hook(usbfd, NULL, custom_hid_rx_handler);
    custom_hid_set_get_report_hook(usbfd, NULL, custom_hid_get_report_handler);
    custom_hid_set_set_report_hook(usbfd, NULL, custom_hid_set_report_handler);

    ds5_usb_init();
    ds5_audio_init();
}

void usb_pause(const usb_dev usb_id)
{
    log_info("usb_pause");

    usb_device_mode(usb_id,0);
}

void usb_init() {
}

int usb_audio_event_handler(void *evt)
{
    struct device_event *event = evt;
    int usb_id = (int)event->arg;
    int value = event->value;

    switch (event->event)
    {
    case USB_AUDIO_PLAY_OPEN:
        log_info("USB_AUDIO_PLAY_OPEN");
        set_uac_speaker_rx_handler(usb_id, NULL, uac_speaker_rx_handler);
        break;
    case USB_AUDIO_MIC_OPEN:
        log_info("USB_AUDIO_MIC_OPEN");
        set_uac_mic_tx_handler(usb_id,NULL,uac_mic_tx_handler);
        break;
    case USB_AUDIO_SET_PLAY_VOL:
            int l = (int)((u32)value & 0xffff);
            int r = (int)(((u32)value >> 16) & 0xffff);
            log_debug("USB_AUDIO_SET_PLAY%d_VOL : %d %d", usb_id, l, r);
            ds5_state_set_vol(l,255);
            break;
    default:
        log_info("%s> %d", __func__, event->event);
        break;
    }

    return FALSE;
}