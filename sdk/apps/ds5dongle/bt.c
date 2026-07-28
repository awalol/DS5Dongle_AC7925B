#include "app_config.h"
#include "system/includes.h"

#include "app_core.h"
#include "btstack/avctp_user.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_task.h"
#include "classic/hci_lmp.h"
#include "event/bt_event.h"
#include "hci_event.h"

#define LOG_TAG             "[DS5_BT]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

static bool bt_searching;

#define DS5_INQUIRY_LENGTH       10  /* 10 * 1.28 s */

static const char ds5_device_name[] = "Wireless Controller";

static u8 bt_name_is_ds5(const char *name, u8 name_len) {
    if (!name || !name_len) {
        return FALSE;
    }

    u32 expected_len = sizeof(ds5_device_name) - 1;
    return name_len == expected_len &&
           !memcmp(name, ds5_device_name, expected_len);
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

    if (!bt_name_is_ds5(name, name_len)) {
        return FALSE;
    }

    log_info("DualSense found, start Classic connection");
    bt_searching = false;
    lmp_set_sniff_establish_by_remote(1);

    /* 返回 TRUE 会让杰理 emitter 停止 Inquiry 并直接 Page 当前设备。 */
    return TRUE;
}

static void bt_start_inquiry(void) {
    u8 inquiry_length = DS5_INQUIRY_LENGTH;

    if (bt_searching || bt_get_total_connect_dev()) {
        return;
    }

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
            bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
            bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            bt_start_inquiry();
            break;
        case BT_STATUS_FIRST_CONNECTED:
            log_info("BT_STATUS_FIRST_CONNECTED");
            put_buf(bt->args, 6);
            bt_searching = false;
            /*
             * 此处只表示 ACL 已连接。DualSense 的 HID Control/Interrupt
             * L2CAP 通道还需要独立的 Classic HID Host 实现。
             */
            break;
        case BT_STATUS_FIRST_DISCONNECT:
            log_info("BT_STATUS_FIRST_DISCONNECT");
            bt_searching = false;
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
            bt_searching = false;
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

static int app_event_handler(struct application *app, struct sys_event *event) {
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
    .event_handler = app_event_handler,
};

REGISTER_APPLICATION(bt) =
{
    .name  = "bt",
    .ops   = &bt_ops,
    .state = APP_STA_DESTROY,
};
