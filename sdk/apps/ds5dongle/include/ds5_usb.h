//
// Created by awalol on 2026/9/30.
//

#ifndef SDK_DS5_USB_H
#define SDK_DS5_USB_H

void ds5_usb_init();
void ds5_usb_tx_done(void);
void ds5_usb_get_report(u8 *buf, u32 len);
void ds5_usb_set_report(u8 *buf, u32 len);
void ds5_usb_interrupt(u8 *buf, u32 len);

#endif //SDK_DS5_USB_H
