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
#define LOG_DEBUG_ENABLE
#include "debug.h"


static void custom_hid_rx_handler(const usb_dev usb_id, void *priv, u8 *buf, u32 len)
{
    log_debug("custom_hid_rx_handler >");
    log_debug_hexdump(buf,len);
}

static void custom_hid_get_report_handler(const usb_dev usb_id, void *priv, u8 *buf, u32 len) {
    log_debug("custom_hid_get_report_handler >");
    log_debug_hexdump(buf,len);
}

static void custom_hid_set_report_handler(const usb_dev usb_id, void *priv, u8 *buf, u32 len) {
    log_debug("custom_hid_set_report_handler >");
    log_debug_hexdump(buf,len);
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
    if (class == 0) {
        class = USB_DEVICE_CLASS_CONFIG_2_0;
    }
    usb_device_mode(usbfd, class);
    custom_hid_set_rx_hook(usbfd, NULL, custom_hid_rx_handler);
    custom_hid_set_get_report_hook(usbfd, NULL, custom_hid_get_report_handler);
    custom_hid_set_set_report_hook(usbfd, NULL, custom_hid_set_report_handler);

    ds5_usb_init();
}

void usb_init() {
}