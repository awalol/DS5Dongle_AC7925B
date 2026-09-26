#include "system/includes.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "esco_recoder.h"
#include "bt_slience_detect.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btctrler/btctrler_task.h"
#include "btctrler/btcontroller_modules.h"
#include "hci_event.h"
#include "bt_common.h"
#include "btstack/bluetooth.h"
#include "event/key_event.h"
#include "event/device_event.h"
#include "file_player.h"
#include "app_tone.h"
#include "app_msg.h"
#include "vol_sync.h"
#include "asm/rf_coexistence_config.h"
#include "app_config.h"
#if (THIRD_PARTY_PROTOCOLS_SEL & MULTI_BOX_ADV_EN)
#include "multi_box_adv/multi_box_adv.h"
#endif


#if TCFG_APP_BT_EN

#define LOG_TAG             "[APP_BT]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


typedef struct {
    //phone
    u8 phone_ring_flag;         //来电响铃标志
    u8 phone_num_flag;          //通话号码获取标志
    u8 phone_income_flag;       //来电标志
    u8 phone_call_dec_begin;    //来电esco解码标志
    u8 phone_ring_sync_tws;     //tws来电铃声同步标志
    u8 phone_ring_addr[6];      //响铃的手机地址

    u8 inband_ringtone;         //带内铃声,表示支不支持手机端铃声
    u8 phone_vol;               //手机音量
    u8 last_call_type;
    u8 income_phone_num[30];    //通话手机号码
    u8 income_phone_len;        //通话手机号码长度
    u8 siri_stu;
    u8 remote_dev_company;
    u8 esco_play_delay_addr[6];
    u16 esco_play_delay_timer;
    u16 phone_timer_id;         //查询手机号码定时器
    u16 auto_connection_timer;  //回连定时器
    u8 auto_connection_addr[6]; //回连地址

#if TCFG_USER_TWS_ENABLE
    u16 tws_con_timer;
    u8 tws_start_con_cnt;
    bool search_tws_ing;
    u8 tws_conn_state;          //tws连接状态
    u8 tws_role;
    char tws_channel;
    u8 multi_box_adv_cnt;
    u8 multi_box_role;
    bool multi_box_role_lock;
#endif
    u16 multi_box_adv_timer;
    u16 sniff_timer;            //进入sniff模式定时器
    u16 get_music_player_timer;
    u16 exit_check_timer;
    u16 vol_sync_timer;

    pitch_level_t pitch_mode;
    bool fast_test_mode;
    bool enable;
    bool pair_enable;
    bool le_audio_enable;
    bool init_ok;               //蓝牙初始化完成标志
    bool exiting;               //蓝牙正在退出
    bool suspend_flag;
    bool back_to_prev_app;
} bt_mode_t;

static bt_mode_t bt_hdl;
#define __this  (&bt_hdl)

u8 a2dp_support_delay_report = 1;

int bt_get_unactive_remote_addr(bd_addr_t addr);
u8 get_unactive_device_call_status(void);
void wl_set_beaconlosttime(int sec);

#include "bt_tws.c"
#include "bt_test.c"
#include "bt_event_func.c"
#include "bt_msg_func.c"
#if TCFG_USER_BT_CLASSIC_ENABLE && TCFG_BT_SUPPORT_PROFILE_A2DP
#include "a2dp_play.c"
#include "tws_a2dp_play.c"
#endif
#if TCFG_USER_BT_CLASSIC_ENABLE && TCFG_BT_SUPPORT_PROFILE_HFP
#include "phone_call.c"
#include "tws_phone_call.c"
#endif
#if TCFG_USER_TWS_ENABLE
#include "multi_box_strategy.c"
#endif


#if TCFG_BT_DUAL_CONN_ENABLE
u8 *lmp_get_inactive_remote_addr(void)
{
    if (bt_get_total_connect_dev() < 2) {
        return NULL;
    }

    static u8 active_addr[6];
    static u8 inactive_addr[6];
    u8 addr[6];

    memset(active_addr, 0, sizeof(active_addr));
    memset(inactive_addr, 0, sizeof(inactive_addr));

    memcpy(active_addr, bt_get_current_remote_addr(), 6);
    bt_get_unactive_remote_addr(inactive_addr);

    if (esco_player_get_btaddr(addr)) {
        if (!memcmp(active_addr, addr, 6)) {
            return inactive_addr;
        }
        if (!memcmp(inactive_addr, addr, 6)) {
            return active_addr;
        }
        return NULL;
    }

    if (a2dp_player_get_btaddr(addr)) {
        if (!memcmp(active_addr, addr, 6)) {
            return inactive_addr;
        }
        if (!memcmp(inactive_addr, addr, 6)) {
            return active_addr;
        }
        return NULL;
    }

    if (!memcmp(get_g_play_addr(), active_addr, 6)) {
        return inactive_addr;
    }
    if (!memcmp(get_g_play_addr(), inactive_addr, 6)) {
        return active_addr;
    }

    return NULL;
}
#endif

#if !TCFG_USER_TWS_ENABLE
static int bt_power_manage_event_handler(void *evt)
{
    struct device_event *event = (struct device_event *)evt;

    switch (event->event) {
    case POWER_EVENT_POWER_CHANGE:
        bt_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
        break;
    }

    return FALSE;
}

REGISTER_APP_EVENT_HANDLER(bt_power_manage_event) = {
    .event      = SYS_DEVICE_EVENT,
    .from       = DEVICE_EVENT_FROM_POWER,
    .handler    = bt_power_manage_event_handler,
};
#endif


#if TCFG_BT_VOL_SYNC_ENABLE
static void bt_connect_vol_sync(void *p)
{
    __this->vol_sync_timer = 0;

#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        free(p);
        return;
    }
#endif

    //连接手机后主动请求一次音量同步
    bt_cmd_prepare_for_addr((u8 *)p, USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);
    free(p);
}
#endif

/*
 * 对应原来的状态处理函数，连接，电话状态等
 */
static int bt_connction_status_event_handler(void *evt)
{
    struct bt_event *bt = (struct bt_event *)evt;
    u8 a2dp_vol_mac[6];

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /* 蓝牙初始化完成 */
        log_info("BT_STATUS_INIT_OK");
        bt_status_init_ok();
        break;
    case BT_STATUS_SECOND_CONNECTED:
        bt_clear_current_poweron_memory_search_index(0);
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED");
#if TCFG_USER_TWS_ENABLE
        bt_tws_phone_connected();
#endif
#if TCFG_BT_SUPPORT_PROFILE_MAP
        bt_cmd_prepare(USER_CTRL_MAP_READ_TIME, 0, NULL);
#endif
#if TCFG_BT_VOL_SYNC_ENABLE
        u8 *vol_addr = malloc(6);
        if (vol_addr) {
            memcpy(vol_addr, bt->args, 6);
            __this->vol_sync_timer = sys_timeout_add(vol_addr, bt_connect_vol_sync, 2000);
        }
#endif
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info("BT_STATUS_DISCONNECT");
#if TCFG_USER_EMITTER_ENABLE
        rf_coexistence_scene_exit(RF_COEXISTENCE_SCENE_A2DP_SOURCE);
#endif
        break;
    case BT_STATUS_CONN_A2DP_CH:
        log_info("BT_STATUS_CONN_A2DP_CH");
        memcpy(a2dp_vol_mac, bt->args, 6);
        app_audio_bt_volume_save_mac(a2dp_vol_mac);
#if TCFG_BT_SUPPORT_PROFILE_BIP
        if (!a2dp_player_runing()) {
            __bip_info->timer = sys_timeout_add(NULL, __bip_file_request_update, 1500);
        }
#endif
        break;
    case BT_STATUS_DISCON_A2DP_CH:
        log_info("BT_STATUS_DISCON_A2DP_CH");
        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
#define AVC_VOLUME_UP			0x41
#define AVC_VOLUME_DOWN			0x42
#define AVC_PLAY			    0x44
#define AVC_PAUSE			    0x46
        log_info("BT_STATUS_AVRCP_INCOME_OPID:0x%x", bt->value);
        if (bt->value == AVC_VOLUME_UP) {

        } else if (bt->value == AVC_VOLUME_DOWN) {

        } else if (bt->value == AVC_PLAY) {
            bt_music_post_msg_to_ui("music_state_on");
            bt_music_player_time_timer_deal(1);
#if TCFG_BT_SUPPORT_PROFILE_BIP
            bip_file_request_update(bt->args);
#endif
        } else if (bt->value == AVC_PAUSE) {
            bt_music_post_msg_to_ui("music_state_off");
            bt_music_player_time_timer_deal(0);
        }
        break;
    case BT_STATUS_AVRCP_VOL_CHANGE:
        if (!__this->vol_sync_timer &&
#if TCFG_USER_TWS_ENABLE
            tws_api_get_role() != TWS_ROLE_SLAVE &&
#endif
            !a2dp_player_get_btaddr(a2dp_vol_mac)) {
            bt_music_post_msg_to_ui("music_volume", bt->value);
            bt_set_music_device_volume(bt->value);
        }
        break;
    case BT_STATUS_CONN_HCRP_CH:
        log_info("BT_STATUS_CONN_HCRP_CH");
        break;
    case BT_STATUS_DISCONN_HCRP_CH:
        log_info("BT_STATUS_DISCONN_HCRP_CH");
        break;
    default:
        log_info("BT STATUS DEFAULT : 0x%x", bt->event);
        break;
    }

    return FALSE;
}

REGISTER_APP_EVENT_HANDLER(bt_connction_status_event) = {
    .event      = SYS_BT_EVENT,
    .from       = BT_EVENT_FROM_CON,
    .handler    = bt_connction_status_event_handler,
};

static int bt_hci_event_handler(void *evt)
{
    struct bt_event *bt = (struct bt_event *)evt;

    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("bt_hci_event_handler reason 0x%x 0x%x", bt->event, bt->value);

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info("HCI_EVENT_INQUIRY_COMPLETE");
        bt_hci_event_inquiry(bt);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info("HCI_EVENT_USER_CONFIRMATION_REQUEST");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info("HCI_EVENT_USER_PASSKEY_REQUEST");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info("HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST:
        log_info("HCI_EVENT_PIN_CODE_REQUEST");
        break;
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR:
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR");
        bt_hci_event_disconnect(bt);
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE");
        if (!__this->enable) {
            break;
        }
        if (bt->value == ERROR_CODE_CONNECTION_TIMEOUT) {
            bt_hci_event_connection_timeout(bt);
        }
        bt_hci_event_disconnect(bt);
        break;
    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info("HCI_EVENT_CONNECTION_COMPLETE");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS:
            log_info("ERROR_CODE_SUCCESS");
            testbox_in_ear_detect_test_flag_set(0);
            break;
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED:
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST:
        case ERROR_CODE_AUTHENTICATION_FAILURE:
            //case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt);
            break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info("ERROR_CODE_CONNECTION_TIMEOUT");
            bt_hci_event_connection_timeout(bt);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return FALSE;
}

REGISTER_APP_EVENT_HANDLER(bt_hci_event) = {
    .event      = SYS_BT_EVENT,
    .from       = BT_EVENT_FROM_HCI,
    .handler    = bt_hci_event_handler,
};

bool bt_app_exit_check(void)
{
    if (__this->siri_stu && __this->siri_stu != 3 && bt_get_esco_coder_busy_flag()) {
        // siri不退出
        return FALSE;
    }

    u8 esco_state = bt_get_call_status();
    if (esco_state == BT_CALL_OUTGOING  ||
        esco_state == BT_CALL_ALERT     ||
        esco_state == BT_CALL_INCOMING  ||
        esco_state == BT_CALL_ACTIVE) {
        // 通话不退出
        return FALSE;
    }

    return TRUE;
}

bool get_bt_connction_enable_status(void)
{
    return __this->enable;
}

static void bt_connction_enable(void)
{
    if (__this->enable) {
        return;
    }

    play_tone_file_alone(get_tone_files()->bt_open);

#if TCFG_BT_CONNECTION_CLOSE_ALL
    bt_set_stack_exiting(0);
#if TCFG_USER_TWS_ENABLE
    bt_tws_poweron();
#endif
#endif

    struct bt_event e = {0};
    e.event = BT_STATUS_INIT_OK;
    bt_event_notify(BT_EVENT_FROM_CON, &e);

    __this->enable = TRUE;
}

static void bt_connction_disable(void)
{
    if (!__this->enable) {
        return;
    }

    __this->enable = FALSE;

#if TCFG_USER_TWS_ENABLE
    tws_dual_conn_close();
#if TCFG_BT_CONNECTION_CLOSE_ALL
    bt_tws_disable();
    /* bt_tws_poweroff(); */
#endif
#else
    void dual_conn_close(void);
    dual_conn_close();
#endif
#if TCFG_BT_CONNECTION_CLOSE_ALL
    bt_set_stack_exiting(1);
#endif
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);

#if (TCFG_KBOX_1T3_MODE_EN == 0 && TCFG_EDR_CONNECTION_CLOSE_ALL)        //三合一退出模式不关闭le audio
#if (TCFG_LEA_BIG_CTRLER_TX_EN || TCFG_LEA_BIG_CTRLER_RX_EN)
    app_broadcast_close(APP_BROADCAST_STATUS_STOP);
    app_broadcast_uninit();
#endif

#if (TCFG_LEA_CIG_CENTRAL_EN || TCFG_LEA_CIG_PERIPHERAL_EN)
    app_connected_close_all(APP_CONNECTED_STATUS_STOP);
    app_connected_uninit();
#endif
#endif

    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    if (__this->auto_connection_timer) {
        sys_timeout_del(__this->auto_connection_timer);
        __this->auto_connection_timer = 0;
    }

    play_tone_file_alone(get_tone_files()->bt_close);
}

void bt_connction_disconnect(void)
{
    if (!__this->enable) {
        return;
    }

#if TCFG_USER_TWS_ENABLE
    tws_dual_conn_close();
#else
    void dual_conn_close(void);
    dual_conn_close();
#endif

    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    if (__this->auto_connection_timer) {
        sys_timeout_del(__this->auto_connection_timer);
        __this->auto_connection_timer = 0;
    }
}

static int bt_cfg_key_double_click_event(struct key_event *key)
{
    int ret = FALSE;

    switch (key->value) {
    case KEY_MODE:
        __this->enable ? bt_connction_disable() : bt_connction_enable();
        ret = TRUE;
        break;
    default:
        break;
    }

    return ret;
}

static int bt_cfg_key_triple_click_event(struct key_event *key)
{
    int ret = FALSE;

    switch (key->value) {
    case KEY_MODE:
#if TCFG_BT_CONNECTION_CLOSE_ALL
        if (!__this->enable) {
            break;
        }
#endif
#if TCFG_USER_TWS_ENABLE
        __this->pair_enable ? multiple_bt_pair_remove() : multiple_bt_pair_start();
#endif
        ret = TRUE;
        break;
    default:
        break;
    }

    return ret;
}

static int bt_cfg_key_event_handler(void *evt)
{
    struct key_event *key = (struct key_event *)evt;
    int ret = FALSE;

    switch (key->action) {
    case KEY_EVENT_DOUBLE_CLICK:
        ret = bt_cfg_key_double_click_event(key);
        break;
    case KEY_EVENT_TRIPLE_CLICK:
        ret = bt_cfg_key_triple_click_event(key);
        break;
    default:
        break;
    }

    return ret;
}

REGISTER_APP_EVENT_HANDLER(bt_cfg_key_event) = {
    .event      = SYS_KEY_EVENT,
    .from       = KEY_EVENT_FROM_KEY,
    .handler    = bt_cfg_key_event_handler,
};

#if TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END
static int bt_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (event == STREAM_EVENT_STOP) {
        bt_tws_poweron();
#if TCFG_LE_AUDIO_STREAM_ENABLE
        if (!__this->pair_enable) {
            syscfg_read(VM_LE_AUDIO_ONOFF_INDEX, &__this->le_audio_enable, sizeof(__this->le_audio_enable));
            if (__this->le_audio_enable) {
                __this->pair_enable = TRUE;
                le_audio_switch_onoff();
#if (THIRD_PARTY_PROTOCOLS_SEL & MULTI_BOX_ADV_EN)
                multi_box_in_bis_start_notify(__this->multi_box_role);
#endif
            }
        }
#endif
    }
    return 0;
}
#endif

static int bt_mode_init(void)
{
    __this->exiting = FALSE;

    if (__this->exit_check_timer) {
        sys_timer_del(__this->exit_check_timer);
        __this->exit_check_timer = 0;
        return 0;
    }

    __this->init_ok = FALSE;
    __this->phone_vol = 9;

    syscfg_read(CFG_BT_CALL_VOLUME, &__this->phone_vol, sizeof(__this->phone_vol));

    // tone_player_stop();

#if TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END
    play_tone_file_callback(get_tone_files()->bt_mode, NULL, bt_tone_play_end_callback);
#else
    play_tone_file(get_tone_files()->bt_mode);
#endif

    bt_function_select_init();
    bredr_handle_register();
    btstack_init();
#if TCFG_USER_TWS_ENABLE
    tws_profile_init();
#if TCFG_KBOX_1T3_MODE_EN
    set_tws_task_add_run_slot(6);
    tws_api_pure_monitor_enable(1);
#endif
#endif

    void bt_sniff_feature_init();
    bt_sniff_feature_init();

#if TCFG_PITCH_SPEED_NODE_ENABLE
    __this->pitch_mode = PITCH_0;    //设置变调初始模式
#endif
    /* app_send_message(APP_MSG_ENTER_MODE, APP_MODE_BT); */

    __this->enable = TRUE;

    return 0;
}

static void bt_no_background_exit_check(void *priv)
{
    if (__this->exiting == 0) {
        sys_timer_del(__this->exit_check_timer);
        __this->exit_check_timer = 0;
        return;
    }

    if (esco_player_runing() || a2dp_player_runing()) {
        return;
    }

#if THIRD_PARTY_PROTOCOLS_SEL
    multi_protocol_bt_exit();
#endif

    btstack_exit();

    bt_set_stack_exiting(0);

    sys_timer_del(__this->exit_check_timer);
    __this->exit_check_timer = 0;

    __this->init_ok = FALSE;
    __this->exiting = FALSE;
}

static int bt_mode_exit(void)
{
    __this->exiting = TRUE;
    __this->enable = FALSE;

#if TCFG_LOCAL_TWS_ENABLE
    local_tws_exit_mode();
#endif

#if TCFG_USER_TWS_ENABLE
    tws_dual_conn_close();
    bt_tws_poweroff();
#else
    void dual_conn_close(void);
    dual_conn_close();
#endif

    bt_set_stack_exiting(1);

    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);

    if (__this->auto_connection_timer) {
        sys_timeout_del(__this->auto_connection_timer);
        __this->auto_connection_timer = 0;
    }

    if (__this->exit_check_timer == 0) {
        __this->exit_check_timer = sys_timer_add(NULL, bt_no_background_exit_check, 40);
        log_info("set exit timer");
    }

    return 0;
}

static void bt_music_app_suspend(void)
{
#if TCFG_LE_AUDIO_STREAM_ENABLE
    if ((a2dp_player_runing() || le_audio_player_is_playing()) && bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING) {
#else
    if (a2dp_player_runing() && bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING) {
#endif
        bt_cmd_prepare_for_addr(get_g_play_addr(), USER_CTRL_AVCTP_PAUSE_MUSIC, 0, NULL);
#if TCFG_USER_TWS_ENABLE
        tws_a2dp_slience_detect(get_g_play_addr(), 1);
#else
        a2dp_play_slience_detect(get_g_play_addr());
#endif
        __this->suspend_flag = 1;
        ai_app_local_event_notify(FILE_PLAYER_PAUSE);
    }
}

static void bt_music_app_resume(void)
{
    if (!__this->suspend_flag) {
        return;
    }

    if (bt_a2dp_get_status() == BT_MUSIC_STATUS_SUSPENDING) {
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        ai_app_local_event_notify(FILE_PLAYER_START);
        u8 *addr = get_g_play_addr();
        if (bt_slience_detect_get_result(addr) != BT_SLIENCE_NO_DETECTING) {
            bt_stop_a2dp_slience_detect(addr);
            bt_start_a2dp_slience_detect(addr, 0);
            a2dp_media_unmute(addr);
            a2dp_media_close(addr);
        }
    }

    __this->suspend_flag = 0;
}

static void bt_music_app_stop(void)
{
    if (a2dp_player_runing() && bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING) {
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
    }

    __this->suspend_flag = 0;
}

static int bt_msg_handler(struct application *app, int *msg)
{
    u8 *bt_addr = (u8 *)&msg[1];

    switch (msg[0]) {
    case APP_MSG_SUSPEND:
        bt_music_app_suspend();
        break;
    case APP_MSG_RESUME:
        bt_music_app_resume();
        break;
    case APP_MSG_STOP:
        bt_music_app_stop();
        break;
    case APP_MSG_BT_MUSIC_PP:
        log_info("music pp");
        bt_app_msg_music_pp();
        break;
    case APP_MSG_BT_MUSIC_PLAY:
        log_info("music play");
        bt_app_msg_music_play();
        break;
    case APP_MSG_BT_MUSIC_PAUSE:
        log_info("music pause");
        bt_app_msg_music_pause();
        break;
    case APP_MSG_BT_MUSIC_NEXT:
        log_info("music next");
        bt_app_msg_music_next();
        break;
    case APP_MSG_BT_MUSIC_PREV:
        log_info("music prev");
        bt_app_msg_music_prev();
        break;
    case APP_MSG_BT_VOL_UP:
        log_info("vol up");
        bt_app_msg_vol_up();
        break;
    case APP_MSG_BT_VOL_DOWN:
        log_info("vol down");
        bt_app_msg_vol_down();
        break;
    case APP_MSG_BT_VOL_SET:
        log_info("vol set %d", msg[1]);
        bt_app_msg_vol_set((u8)msg[1]);
        break;
#if TCFG_BT_SUPPORT_PROFILE_HFP
    case APP_MSG_BT_CALL_ANSWER:
        log_info("call answer");
        bt_app_msg_call_answer();
        break;
    case APP_MSG_BT_CALL_HANGUP:
        log_info("call hangup");
        bt_app_msg_call_hang_up();
        break;
    case APP_MSG_BT_CALL_LAST_NO:
        log_info("call last on");
        bt_app_msg_call_last_on();
        break;
    case APP_MSG_BT_OPEN_SIRI:
        log_info("open siri");
        bt_app_msg_call_siri();
        break;
    case APP_MSG_BT_CALL_THREE_WAY_ANSWER1:
        log_info("call three way anawer1");
        bt_app_msg_call_three_way_answer1();
        break;
    case APP_MSG_BT_CALL_THREE_WAY_ANSWER2:
        log_info("call three way anawer2");
        bt_app_msg_call_three_way_answer2();
        break;
    case APP_MSG_BT_CALL_SWITCH:
        log_info("call switch");
        bt_app_msg_call_switch();
        break;
#endif
    case APP_MSG_BT_HID_CONTROL:
        log_info("hid control");
        bt_app_msg_hid_control();
        break;
#if !TCFG_USER_TWS_ENABLE
    case APP_MSG_BT_PAGE_DEVICE:
        log_info("page device");
        void dual_conn_page_device_msg_handler(void);
        dual_conn_page_device_msg_handler();
#endif
        break;
#if TCFG_BT_SUPPORT_PROFILE_A2DP
    case APP_MSG_BT_MUSIC_PITCH_UP:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        log_info("a2dp pitch up");
        if (a2dp_player_runing()) {
            __this->pitch_mode = a2dp_file_pitch_up();
        }
#endif
        break;
    case APP_MSG_BT_MUSIC_PITCH_DOWN:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        log_info("a2dp pitch down");
        if (a2dp_player_runing()) {
            __this->pitch_mode = a2dp_file_pitch_down();
        }
#endif
        break;
    case APP_MSG_BT_A2DP_START:
        log_info("bt a2dp start");
        bt_app_msg_a2dp_start(bt_addr);
        break;
    case APP_MSG_BT_A2DP_PAUSE:
        log_info("bt a2dp pause");
        if (a2dp_player_is_playing(bt_addr)) {
#if TCFG_USER_TWS_ENABLE
            tws_a2dp_play_send_cmd(CMD_A2DP_MUTE, bt_addr, 6, 1);
#else
            a2dp_play_send_cmd(CMD_A2DP_MUTE, bt_addr, 6);
#endif
        }
        break;
    case APP_MSG_BT_A2DP_STOP:
        log_info("bt a2dp stop");
#if TCFG_USER_TWS_ENABLE
        tws_a2dp_play_send_cmd(CMD_A2DP_CLOSE, bt_addr, 6, 1);
#else
        a2dp_play_send_cmd(CMD_A2DP_CLOSE, bt_addr, 6);
#endif
        break;
    case APP_MSG_BT_A2DP_PLAY:
        log_info("bt a2dp play");
        if (memcmp(a2dp_energy_detect_addr, bt_addr, 6) == 0) {
            memset(a2dp_energy_detect_addr, 0xff, 6);
        }
#if TCFG_USER_TWS_ENABLE
        tws_a2dp_sync_play(bt_addr, 1);
#else
        a2dp_play(bt_addr);
#endif
        break;
    case APP_MSG_BT_SEND_A2DP_PLAY_CMD:
        log_info("bt send a2dp play cmd");
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
#endif
        void *device = btstack_get_conn_device(bt_addr);
        if (device) {
            btstack_device_control(device, USER_CTRL_AVCTP_OPID_PLAY);
        }
        break;
    case APP_MSG_BT_A2DP_NO_ENERGY:
        log_info("bt a2dp no energy");
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
#endif
        if (a2dp_player_is_playing(bt_addr)) {
#if TCFG_USER_TWS_ENABLE
            tws_a2dp_play_send_cmd(CMD_A2DP_SWITCH, bt_addr, 6, 1);
#else
            a2dp_play_send_cmd(CMD_A2DP_SWITCH, bt_addr, 6);
#endif
        }
        break;
    case APP_MSG_BT_LOW_LANTECY:
        log_info("low lantecy");
        int state = bt_get_low_latency_mode();
        bt_set_low_latency_mode(!state, 1, 300);
        break;
#endif
    case APP_MSG_BT_DISCONNECT:
        bt_connction_disconnect();
        break;
    default:
#if TCFG_USER_TWS_ENABLE
        int dual_conn_tws_app_event_handler(int *msg);
        dual_conn_tws_app_event_handler(msg);
#endif
        break;
    }

    return 0;
}

static int bt_key_click(struct key_event *key)
{
    int ret = FALSE;

    switch (key->value) {
    case KEY_OK:
        app_send_message(APP_MSG_BT_MUSIC_PP, 0);
        break;
    case KEY_PLAY:
        app_send_message(APP_MSG_BT_MUSIC_PLAY, 0);
        break;
    case KEY_PAUSE:
        app_send_message(APP_MSG_BT_MUSIC_PAUSE, 0);
        break;
    case KEY_VOLUME_DEC:
    case KEY_UP:
    case KEY_PREV:
        app_send_message(APP_MSG_BT_VOL_DOWN, 0);
        ret = TRUE;
        break;
    case KEY_VOLUME_INC:
    case KEY_DOWN:
    case KEY_NEXT:
        app_send_message(APP_MSG_BT_VOL_UP, 0);
        ret = TRUE;
        break;
    default:
        break;
    }

    return ret;
}

static int bt_key_long(struct key_event *key)
{
    int ret = FALSE;

    switch (key->value) {
    case KEY_VOLUME_DEC:
    case KEY_UP:
    case KEY_PREV:
        app_send_message(APP_MSG_BT_MUSIC_PREV, 0);
        break;
    case KEY_VOLUME_INC:
    case KEY_DOWN:
    case KEY_NEXT:
        app_send_message(APP_MSG_BT_MUSIC_NEXT, 0);
        break;
    default:
        break;
    }

    return ret;
}

static int bt_key_double_click(struct key_event *key)
{
    int ret = FALSE;

    switch (key->value) {
    case KEY_POWER:
    case KEY_ENC:
        app_send_message(APP_MSG_BT_OPEN_SIRI, 0);
        break;
    default:
        break;
    }

    return ret;
}

static int bt_key_event_handler(struct key_event *key)
{
    int ret = FALSE;

    switch (key->action) {
    case KEY_EVENT_CLICK:
        ret = bt_key_click(key);
        break;
    case KEY_EVENT_LONG:
        ret = bt_key_long(key);
        break;
    case KEY_EVENT_DOUBLE_CLICK:
        ret = bt_key_double_click(key);
        break;
    default:
        break;
    }

    return ret;
}

static int bt_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return bt_key_event_handler((struct key_event *)event->payload);
    default:
        return FALSE;
    }
}

static int bt_state_machine(struct application *app, enum app_state state,
                            struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        bt_mode_init();
        break;
    case APP_STA_START:
        break;
    case APP_STA_PAUSE:
        bt_music_app_suspend();
#if TCFG_USER_EMITTER_ENABLE
        extern u8 *get_cur_connect_emitter_mac_addr(void);
        extern void emitter_open(void);
        if (get_cur_connect_emitter_mac_addr()) {
            emitter_open();
        }
#endif
        break;
    case APP_STA_RESUME:
#if TCFG_USER_EMITTER_ENABLE
        extern u8 *get_cur_connect_emitter_mac_addr(void);
        extern void emitter_close(void);
        if (get_cur_connect_emitter_mac_addr()) {
            emitter_close();
        }
#endif
#if TCFG_LE_AUDIO_STREAM_ENABLE
        le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);    //退出上一个模式
#endif
#if TCFG_LOCAL_TWS_ENABLE
        local_tws_enter_mode(get_tone_files()->bt_mode, APP_MODE_BT, NULL);
#endif
        bt_music_app_resume();
        break;
    case APP_STA_STOP:
        break;
    case APP_STA_DESTROY:
        bt_mode_exit();
        break;
    case APP_STA_COMPLETE:
#if TCFG_LE_AUDIO_STREAM_ENABLE
        le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER);
#endif
        break;
    }

    return 0;
}

static const struct application_operation bt_ops = {
    .state_machine  = bt_state_machine,
    .event_handler 	= bt_event_handler,
    .msg_handler 	= bt_msg_handler,
};

REGISTER_APPLICATION(bt) = {
    .name   = "bt",
    .ops    = &bt_ops,
    .state  = APP_STA_DESTROY,
};

#endif

