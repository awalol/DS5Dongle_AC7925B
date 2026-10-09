#ifndef __CUSTOM_HID_H__
#define __CUSTOM_HID_H__

#include "typedef.h"
#include "asm/usb.h"

u32 custom_hid_tx_data(const usb_dev usb_id, const u8* buffer, u32 len);
/* HID configured and IN endpoint idle. */
int custom_hid_get_ready(const usb_dev usb_id);
/* Called in the IN endpoint interrupt. */
void custom_hid_set_tx_hook(const usb_dev usb_id, void (*tx_hook)(const usb_dev usb_id));
void custom_hid_set_rx_hook(
    const usb_dev usb_id,
    void* priv,
    void (*rx_hook)(const usb_dev usb_id, void* priv, u8* buf, u32 len)
);
void custom_hid_set_get_report_hook(
    const usb_dev usb_id,
    void* priv,
    void (*get_report_hook)(const usb_dev usb_id, void* priv, u8* buf, u32 len)
);
void custom_hid_set_set_report_hook(
    const usb_dev usb_id,
    void* priv,
    void (*set_report_hook)(const usb_dev usb_id, void* priv, u8* buf, u32 len)
);
u32 custom_hid_desc_config(const usb_dev usb_id, u8* ptr, u32* cur_itf_num);
u32 custom_hid_register(usb_dev usb_id);
void custom_hid_release(const usb_dev usb_id);

#endif
