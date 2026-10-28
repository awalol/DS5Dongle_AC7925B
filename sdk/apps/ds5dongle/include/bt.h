#ifndef DS5DONGLE_BT_H
#define DS5DONGLE_BT_H

/* 从应用事件上下文启动搜索；未就绪、搜索中或已连接时忽略。 */
void bt_start_inquiry(void);
// SET_REPORT
void bt_hid_control_set(const u8 *data, u16 size);
// GET_REPORT
void bt_hid_control_get(const u8 reportId);
void bt_hid_interrupt_send(const u8 *data, u16 size);

/* FM8625H RF 开关控制：0 = 内部天线(RF_INT), 1 = 外部天线(RF_EXT) */
void switch_ext_rf(int use_external);

#endif
