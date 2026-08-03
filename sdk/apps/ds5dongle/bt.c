#include "app_config.h"
#include "system/includes.h"

#include "app_core.h"
#include "bt.h"
#include "btstack/avctp_user.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_task.h"
#include "classic/hci_lmp.h"
#include "event/bt_event.h"
#include "hci_event.h"
#include "ibtstack.h"

#define LOG_TAG             "[DS5_BT]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"
#include "ble/hci_ll.h"

#define MTU_CONTROL                          0xffff
#define MTU_INTERRUPT                        0xffff

static bool bt_searching;
static bool new_pair;
static bd_addr_t current_device_addr;
static u16 hid_control_cid;
static u16 hid_interrupt_cid;

#define DS5_INQUIRY_LENGTH       10  /* 10 * 1.28 s */

static const char ds5_device_name[] = "Wireless Controller";

static void l2cap_packet_handler(u8 packet_type, u16 channel,
                                 u8 *packet, u16 size);

/* 后续 DS5 解析/USB 转发模块可以覆盖这两个弱回调。 */
__attribute__((weak))
void ds5_hid_input_report(const u8 *packet, u16 size) {
    (void) packet;
    (void) size;
}

__attribute__((weak))
void ds5_hid_control_data(const u8 *packet, u16 size) {
    (void) packet;
    (void) size;
}

static void bt_reset_connection(void) {
    bt_searching = false;
    new_pair = false;
    hid_control_cid = 0;
    hid_interrupt_cid = 0;
}

/* sys_event 不在 btstack task 上，首次 register/create 需要投递过去。 */
static int btstack_post_callback(void (*callback)(void *), void *priv) {
    int msg[] = {(int) callback, (int) priv};

    return os_taskq_post_type("btstack", Q_CALLBACK, ARRAY_SIZE(msg), msg);
}

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

    if (!new_pair || hid_control_cid) {
        return;
    }

    hid_control_cid = l2cap_create_channel(l2cap_packet_handler,
                                           current_device_addr,
                                           PSM_HID_CONTROL,
                                           MTU_CONTROL);
    if (hid_control_cid) {
        log_info("opening HID Control, provisional cid 0x%x",
                 hid_control_cid);
    } else {
        /* 杰理可能先排队建链，随后才在 CHANNEL_OPENED 中分配 CID。 */
        log_info("opening HID Control, local cid pending");
    }
}

static void bt_l2cap_handle_channel_opened(const u8 *packet, u16 size) {
    if (size < 24) {
        log_error("short L2CAP channel-opened event: %d", size);
        return;
    }

    u8 status = l2cap_event_channel_opened_get_status(packet);
    u16 psm = l2cap_event_channel_opened_get_psm(packet);
    u16 local_cid = l2cap_event_channel_opened_get_local_cid(packet);

    if (status) {
        log_error("Open failed psm=0x%04X status=0x%02X\n", psm, status);
        bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI,
                       sizeof(current_device_addr), current_device_addr);
        return;
    }

    if (psm == PSM_HID_CONTROL) {
        log_info("HID HID Control opened cid=0x%04X\n", local_cid);
        hid_control_cid = local_cid;
        const u16 mtu = l2cap_get_remote_mtu_for_local_cid(hid_control_cid);
        log_info("Remote Control MTU: %d\n", mtu);

        if (new_pair) {
            log_info("[L2CAP] Opening interrupt channel\n");
            hid_interrupt_cid = l2cap_create_channel(l2cap_packet_handler, current_device_addr, PSM_HID_INTERRUPT,
                                                     MTU_INTERRUPT);
        }
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
        log_warn("decline duplicate HID PSM 0x%x", psm);
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
            ds5_hid_input_report(packet, size);
        } else if (channel == hid_control_cid) {
            ds5_hid_control_data(packet, size);
        } else {
            log_warn("data on unknown L2CAP cid 0x%x", channel);
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

static void bt_start_inquiry(void) {
    u8 inquiry_length = DS5_INQUIRY_LENGTH;

    if (bt_searching || bt_get_total_connect_dev()) {
        return;
    }

    new_pair = false;
    if (bt_cmd_prepare(USER_CTRL_SEARCH_DEVICE, sizeof(inquiry_length), &inquiry_length)) {
        log_error("start inquiry command failed");
        return;
    }

    bt_searching = true;
    log_info("searching for DualSense");
}

static void bt_read_remote_name(u8 status, u8 *addr, u8 *name) {
    if (status) {
        log_warn("read remote name failed: %d", status);
        return;
    }

    log_info("remote name: %s", name);
    put_buf(addr, 6);
}

static int bt_init(void) {
    extern void bt_ble_module_init(void);
    bt_ble_module_init();

    bt_set_user_ctrl_conn_num(1);

    bt_set_page_timeout_value(8000);
    bt_set_super_timeout_value(8000);

    bt_set_simple_pair_param(3, 0, 2);
    lmp_set_sniff_disable();

    bt_read_remote_name_handle_register(bt_read_remote_name);
    bt_emitter_set_enable_flag(1);

    log_info("btstack init");
    return btstack_init();
}

static int bt_connection_status_event_handler(void *evt) {
    struct bt_event *bt = (struct bt_event *) evt;

    switch (bt->event) {
        case BT_STATUS_INIT_OK:
            log_info("BT_STATUS_INIT_OK");
            bt_inquiry_result_handle_register(bt_ds5_inquiry_result);
            bt_set_emitter_pin_code_flag(0);
            if (btstack_post_callback(bt_l2cap_register_services, NULL)) {
                log_error("post HID service registration failed");
            }
            bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
            bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
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

    switch (bt->event) {
        case HCI_EVENT_INQUIRY_COMPLETE:
            log_info("HCI_EVENT_INQUIRY_COMPLETE: 0x%x", bt->value);
            bt_searching = false;
            break;
        case HCI_EVENT_USER_CONFIRMATION_REQUEST: {
            u8 accept = 1;
            bt_emitter_cmd_prepare(USER_CTRL_PAIR, sizeof(accept), &accept);
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
            bt_searching = false;
            break;
        default:
            break;
    }

    return FALSE;
}

int bt_event_handler(struct sys_event *event) {
    switch (event->from) {
        case BT_EVENT_FROM_CON:
            return bt_connection_status_event_handler(event->payload);
        case BT_EVENT_FROM_HCI:
            return bt_hci_event_handler(event->payload);
        default:
            break;
    }

    return FALSE;
}

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
        case APP_STA_COMPLETE:
            break;
    }

    return 0;
}

static const struct application_operation bt_ops = {
    .state_machine = app_state_machine,
    .event_handler = NULL,
};

REGISTER_APPLICATION(bt) =
{
    .name = "bt",
    .ops = &bt_ops,
    .state = APP_STA_DESTROY,
};
