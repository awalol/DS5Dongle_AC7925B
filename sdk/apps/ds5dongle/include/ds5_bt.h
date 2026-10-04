//
// Created by awalol on 2026/9/29.
//

#ifndef SDK_DS5_BT_H
#define SDK_DS5_BT_H

void ds5_hid_input_report(const u8 *packet, u16 size);
void ds5_hid_control_data(const u8 *packet, u16 size);
void ds5_on_connected(void);
void ds5_on_disconnected(void);

#endif //SDK_DS5_BT_H
