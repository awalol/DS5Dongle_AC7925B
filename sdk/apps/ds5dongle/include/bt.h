#ifndef DS5DONGLE_BT_H
#define DS5DONGLE_BT_H

#include "typedef.h"

struct sys_event;

int bt_event_handler(struct sys_event *event);

/* DS5 解析/USB 转发模块可以覆盖；packet 只在回调期间有效。 */
void ds5_hid_input_report(const u8 *packet, u16 size);
void ds5_hid_control_data(const u8 *packet, u16 size);

#endif
