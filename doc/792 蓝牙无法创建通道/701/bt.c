//*********************************************************************************//
//                                头文件与日志配置                                 //
//*********************************************************************************//

#include "app_config.h"
#include "system/includes.h"

#include "app_core.h"
#include "bt.h"
#include "btstack/avctp_user.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_task.h"
#include "classic/hci_lmp.h"
#include "btcontroller_modules.h"
#include "event.h"
#include "btstack/btstack_event.h"
#include "ibtstack.h"

#define LOG_TAG_CONST       BT
#include "debug.h"
#include "ble/hci_ll.h"

//*********************************************************************************//
//                                 常量与状态定义                                    //
//*********************************************************************************//

#define MTU_CONTROL                          0xffff
#define MTU_INTERRUPT                        0xffff

static bool bt_ready;
static bool bt_searching;
static bool new_pair;
static bd_addr_t current_device_addr;
static u16 hid_control_cid;
static u16 hid_interrupt_cid;
static bool hid_control_pending;    // Control 通道创建请求已提交，等待结果，防止 CID 未返回时重复创建
static bool hid_interrupt_pending;  // Interrupt 通道创建请求已提交，等待结果，防止 CID 未返回时重复创建

#define DS5_INQUIRY_LENGTH       10  /* 10 * 1.28 s */

static const char ds5_device_name[] = "DualSense Wireless Controller";

static void l2cap_packet_handler(u8 packet_type, u16 channel,
                                 u8 *packet, u16 size);

//*********************************************************************************//
//                                  HID 数据回调                                    //
//*********************************************************************************//

static void ds5_hid_interrupt_data(const u8 *packet, u16 size) {
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
        start_ms = now_ms;
        packets = 0;
    }


}

static void ds5_hid_control_data(const u8 *packet, u16 size) {
    (void) packet;
    (void) size;

    log_debug("Receive Control Data >>");
    log_debug_hexdump(packet,size);
}

static void ds5_on_connected() {
}

//*********************************************************************************//
//                               连接管理与线程投递                                   //
//*********************************************************************************//

/* Inquiry searches for peers; page scan lets a paired controller reconnect.
 * Call only after stack init and with bt_searching cleared.
 * get_total_connect_dev() excludes paging, so also check new_pair. */
static void bt_enable_reconnect(void) {
    if (new_pair || get_total_connect_dev()) {
        return;
    }

    u32 scan_ret = user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    u32 conn_ret = user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    log_info("restore reconnect scan: discoverable_ret=0x%x connectable_ret=0x%x",
             scan_ret, conn_ret);
}

static void bt_reset_connection(void) {
    bt_searching = false;
    new_pair = false;
    hid_control_cid = 0;
    hid_interrupt_cid = 0;
    hid_control_pending = false;
    hid_interrupt_pending = false;
    bt_enable_reconnect();
}

/* sys_event 不在 btstack task 上，首次 register/create 需要投递过去。 */
static int btstack_post_callback(void (*callback)(void *), void *priv) {
    int msg[] = {(int) callback, 1, (int) priv};

    return os_taskq_post_type("btstack", Q_CALLBACK, ARRAY_SIZE(msg), msg);
}

//*********************************************************************************//
//                              HID / L2CAP 通道管理                                //
//*********************************************************************************//

static void bt_l2cap_register_services(void *priv) {
    (void) priv;

    l2cap_register_service(l2cap_packet_handler,
                           PSM_HID_CONTROL, MTU_CONTROL);
    l2cap_register_service(l2cap_packet_handler,
                           PSM_HID_INTERRUPT, MTU_INTERRUPT);
    log_info("HID L2CAP services registered");
}

static void bt_l2cap_open_control(void *priv) {
    (void) priv;

    if (!new_pair || hid_control_cid || hid_control_pending) {
        return;
    }

    /* BR28 returns 1 for a queued request, not a local CID. Set pending
     * before calling because the stack can dispatch events synchronously. */
    hid_control_pending = true;
    u16 queued = l2cap_create_channel(l2cap_packet_handler,
                                           current_device_addr,
                                           PSM_HID_CONTROL,
                                           MTU_CONTROL);
    if (queued) {
        log_info("opening HID Control, waiting for channel-opened event");
    } else {
        hid_control_pending = false;
        log_error("HID Control create request failed");
    }
}

static void bt_l2cap_handle_channel_opened(const u8 *packet, u16 size) {
    /* BR28 emits 21 bytes, ending with remote_mtu at offsets 19..20.
     * It does not include the newer flush_timeout/incoming fields. */
    if (size < 21) {
        log_error("short L2CAP channel-opened event: %d", size);
        return;
    }

    u8 status = l2cap_event_channel_opened_get_status(packet);
    u16 psm = l2cap_event_channel_opened_get_psm(packet);
    u16 local_cid = l2cap_event_channel_opened_get_local_cid(packet);

    log_info("L2CAP opened: size=%u psm=0x%x status=0x%x cid=0x%x",
             size, psm, status, local_cid);
    if (psm == PSM_HID_CONTROL) {
        hid_control_pending = false;
    } else if (psm == PSM_HID_INTERRUPT) {
        hid_interrupt_pending = false;
    }

    if (status) {
        log_error("Open failed psm=0x%04X status=0x%02X", psm, status);
        user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI,
                       sizeof(current_device_addr), current_device_addr);
        return;
    }

    if (psm == PSM_HID_CONTROL) {
        log_info("HID Control opened cid=0x%04X", local_cid);
        hid_control_cid = local_cid;
        const u16 mtu = l2cap_get_remote_mtu_for_local_cid(hid_control_cid);
        log_info("Remote Control MTU: %d", mtu);

        if (new_pair && !hid_interrupt_cid && !hid_interrupt_pending) {
            log_info("[L2CAP] Opening interrupt channel");
            hid_interrupt_pending = true;
            if (!l2cap_create_channel(l2cap_packet_handler, current_device_addr,
                                      PSM_HID_INTERRUPT, MTU_INTERRUPT)) {
                hid_interrupt_pending = false;
                log_error("HID Interrupt create request failed");
            }
        }
    } else if (psm == PSM_HID_INTERRUPT) {
        hid_interrupt_cid = local_cid;
        log_info("HID Interrupt opened cid=0x%04X", local_cid);

        ds5_on_connected();
    }
}

static void bt_l2cap_handle_channel_closed(const u8 *packet, u16 size) {
    if (size < 4) {
        return;
    }

    u16 local_cid = l2cap_event_channel_closed_get_local_cid(packet);
    if (local_cid == hid_control_cid) {
        hid_control_cid = 0;
        log_info("HID Control closed");
    }
    if (local_cid == hid_interrupt_cid) {
        hid_interrupt_cid = 0;
        log_info("HID Interrupt closed");
    }
}

static void bt_l2cap_handle_incoming(const u8 *packet, u16 size) {
    if (size < 16) {
        return;
    }

    u16 psm = l2cap_event_incoming_connection_get_psm(packet);
    u16 local_cid = l2cap_event_incoming_connection_get_local_cid(packet);
    u16 *cid;

    if (psm == PSM_HID_CONTROL) {
        cid = &hid_control_cid;
    } else if (psm == PSM_HID_INTERRUPT) {
        cid = &hid_interrupt_cid;
    } else {
        l2cap_decline_connection(
            local_cid, L2CAP_CONNECTION_REFUSED_RESOURCES);
        return;
    }

    if (*cid) {
        log_info("decline duplicate HID PSM 0x%x", psm);
        l2cap_decline_connection(
            local_cid, L2CAP_CONNECTION_REFUSED_RESOURCES);
        return;
    }

    *cid = local_cid;
    l2cap_accept_connection(local_cid);
    log_info("accept incoming HID PSM 0x%x, cid 0x%x", psm, local_cid);
}

static void l2cap_packet_handler(u8 packet_type, u16 channel,
                                 u8 *packet, u16 size) {
    if (!packet || !size) {
        return;
    }
    if (packet_type == L2CAP_DATA_PACKET) {
        if (channel == hid_interrupt_cid) {
            ds5_hid_interrupt_data(packet, size);
        } else if (channel == hid_control_cid) {
            ds5_hid_control_data(packet, size);
        } else {
            log_info("data on unknown L2CAP cid 0x%x", channel);
        }
        return;
    }

    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    switch (packet[0]) {
        case L2CAP_EVENT_CHANNEL_OPENED:
            bt_l2cap_handle_channel_opened(packet, size);
            break;
        case L2CAP_EVENT_CHANNEL_CLOSED:
            bt_l2cap_handle_channel_closed(packet, size);
            break;
        case L2CAP_EVENT_INCOMING_CONNECTION:
            bt_l2cap_handle_incoming(packet, size);
            break;
        default:
            break;
    }
}

//*********************************************************************************//
//                               设备搜索与名称回调                                   //
//*********************************************************************************//

static u8 bt_ds5_inquiry_result(char *name, u8 name_len, u8 *addr,
                                u32 dev_class, char rssi) {
    char display_name[64] = {0};
    u8 copy_len = name_len;

    if (copy_len >= sizeof(display_name)) {
        copy_len = sizeof(display_name) - 1;
    }
    if (name && copy_len) {
        memcpy(display_name, name, copy_len);
    }

    log_info("inquiry: %s, class 0x%x, rssi %d",
             name ? display_name : "<no name>", dev_class, rssi);
    put_buf(addr, 6);

    if (!name || name_len != sizeof(ds5_device_name) - 1 ||
        memcmp(name, ds5_device_name, sizeof(ds5_device_name) - 1)) {
        return FALSE;
    }

    log_info("DualSense found, start Classic connection");
    bt_searching = false;
    new_pair = true;
    memcpy(current_device_addr, addr, sizeof(current_device_addr));
    lmp_set_sniff_establish_by_remote(1);

    /* 返回 TRUE 会让杰理 emitter 停止 Inquiry 并直接 Page 当前设备。 */
    return TRUE;
}

void bt_start_inquiry(void) {
    u8 inquiry_length = DS5_INQUIRY_LENGTH;

    u8 connected = get_total_connect_dev();
    if (!bt_ready || bt_searching || connected || new_pair) {
        log_info("inquiry skipped: ready=%u searching=%u connected=%u pairing=%u",
                 bt_ready, bt_searching, connected, new_pair);
        return;
    }

    bt_searching = true;
    if (user_send_cmd_prepare(USER_CTRL_SEARCH_DEVICE, sizeof(inquiry_length), &inquiry_length)) {
        bt_searching = false;
        log_error("start inquiry command failed");
        bt_enable_reconnect();
        return;
    }

    log_info("searching for DualSense");
}

static void bt_read_remote_name(u8 status, u8 *addr, u8 *name) {
    if (status) {
        log_info("read remote name failed: %d", status);
        return;
    }

    log_info("remote name: %s", name);
    put_buf(addr, 6);
}

//*********************************************************************************//
//                                   蓝牙初始化                                     //
//*********************************************************************************//

static int bt_init(void) {
    bt_max_pwr_set(10, 5, 8, 10);

    __set_user_ctrl_conn_num(1);

    __set_page_timeout_value(8000);
    __set_super_timeout_value(8000);

    __set_simple_pair_param(3, 0, 2);
    lmp_set_sniff_disable();

#if TCFG_BT_RF_USE_EXT_PA_ENABLE
    extern void bt_rf_PA_control_io_remap(u16 tx_io, u16 rx_io);
    bt_rf_PA_control_io_remap(TCFG_RF_PA_TX_PORT, TCFG_RF_PA_RX_PORT);
    bt_set_rxtx_status_enable(1);
#endif

    read_remote_name_handle_register(bt_read_remote_name);
    __set_emitter_enable_flag(1);

    __change_hci_class_type(BD_CLASS_REMOTE_CONTROL);

    log_info("btstack init");
    return btstack_init();
}

//*********************************************************************************//
//                                  蓝牙事件处理                                   //
//*********************************************************************************//

static int bt_connection_status_event_handler(void *evt) {
    struct bt_event *bt = (struct bt_event *) evt;

    switch (bt->event) {
        case BT_STATUS_INIT_OK:
            log_info("BT_STATUS_INIT_OK");
            bt_ready = true;
            extern u8 l2cap_debug_enable;
            l2cap_debug_enable = 0xff;
            /* BR28's header declares void, but its emitter consumes the u8
             * match result (same ABI as the SDK emitter_search_result). */
            inquiry_result_handle_register((void (*)(char *, u8, u8 *, u32, char))
                                           bt_ds5_inquiry_result);
            if (btstack_post_callback(bt_l2cap_register_services, NULL)) {
                log_error("post HID service registration failed");
            }
            bt_enable_reconnect();
            bt_start_inquiry();
            break;
        case BT_STATUS_FIRST_CONNECTED:
            log_info("BT_STATUS_FIRST_CONNECTED");
            put_buf(bt->args, 6);
            bt_searching = false;
            memcpy(current_device_addr, bt->args,
                   sizeof(current_device_addr));
            break;
        case BT_STATUS_ENCRY_COMPLETE:
            log_info("BT_STATUS_ENCRY_COMPLETE");
            if (new_pair) {
                if (btstack_post_callback(bt_l2cap_open_control, NULL)) {
                    log_error("post HID Control connection failed");
                }
            } else {
                log_info("automatic reconnect: wait for incoming HID channels");
            }
            break;
        case BT_STATUS_FIRST_DISCONNECT:
            log_info("BT_STATUS_FIRST_DISCONNECT");
            bt_reset_connection();
            break;
        default:
            break;
    }

    return FALSE;
}

static int bt_hci_event_handler(void *evt) {
    struct bt_event *bt = (struct bt_event *) evt;

    log_info("HCI event=0x%x value=0x%x", bt->event, bt->value);

    switch (bt->event) {
        case HCI_EVENT_INQUIRY_COMPLETE:
            log_info("HCI_EVENT_INQUIRY_COMPLETE: 0x%x", bt->value);
            bt_searching = false;
            bt_enable_reconnect();
            break;
        case HCI_EVENT_USER_CONFIRMATION_REQUEST: {
            u8 accept = 1;
            u32 ret = user_send_cmd_prepare(USER_CTRL_PAIR, sizeof(accept), &accept);
            log_info("SSP user confirmation: accept=%u, cmd_ret=0x%x",
                     accept, ret);
            break;
        }
        case HCI_EVENT_CONNECTION_COMPLETE:
            log_info("HCI_EVENT_CONNECTION_COMPLETE: 0x%x", bt->value);
            if (bt->value) {
                bt_reset_connection();
            } else {
                bt_searching = false;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: 0x%x", bt->value);
            bt_reset_connection();
            break;
        default:
            break;
    }

    return FALSE;
}

static int bt_event_handler(struct application *app, struct sys_event *event) {
    (void) app;
    if (event->type != SYS_BT_EVENT) {
        return FALSE;
    }

    switch ((u32) event->arg) {
        case SYS_BT_EVENT_TYPE_CON_STATUS:
            bt_connection_status_event_handler(&event->u.bt);
            return TRUE;
        case SYS_BT_EVENT_TYPE_HCI_STATUS:
            bt_hci_event_handler(&event->u.bt);
            return TRUE;
        default:
            break;
    }

    return FALSE;
}

//*********************************************************************************//
//                               应用生命周期与注册                                //
//*********************************************************************************//

static int app_state_machine(struct application *app, enum app_state state,
                             struct intent *it) {
    switch (state) {
        case APP_STA_CREATE:
            return bt_init();
        case APP_STA_START:
        case APP_STA_PAUSE:
        case APP_STA_RESUME:
        case APP_STA_STOP:
        case APP_STA_DESTROY:
            break;
    }

    return 0;
}

static const struct application_operation bt_ops = {
    .state_machine = app_state_machine,
    .event_handler = bt_event_handler,
};

REGISTER_APPLICATION(bt) =
{
    .name = "bt",
    .ops = &bt_ops,
    .state = APP_STA_DESTROY,
};
