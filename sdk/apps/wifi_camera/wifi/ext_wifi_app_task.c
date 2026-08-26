#include "app_config.h"

#if TCFG_EXT_WIFI_ENABLE
#include "system/includes.h"
#include "device/device.h"
#include "wireless_ext/wifi_dev.h"
#include "wireless_ext/wifi_connect.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "http/http_server.h"
#include "server/ctp_server.h"
#include "server/net_server.h"
#include "dev_desc.h"
#include "video_rt_tcp.h"

#define LOG_TAG             "[EXT_WIFI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_RTL8822ES_10M_ENABLE
#define AP_SSID "HF_GPS5G-123456"        //配置 AP模式的SSID前缀
#define AP_PWD  "12345678"               //配置 AP模式的密码
#else
#define AP_SSID EXT_WIFI_AP_SSID
#define AP_PWD  EXT_WIFI_AP_PWD
#endif

#define STA_SSID EXT_WIFI_STA_SSID
#define STA_PWD  EXT_WIFI_STA_PWD

#define WIFI_APP_TASK_NAME "ext_wifi_app_task"

enum WIFI_APP_MSG_CODE {
    WIFI_MSG_TICK_1_SEC,
    WIFI_MSG_SMP_CFG_COMPLETED,
    WIFI_MSG_STA_SCAN_COMPLETED,
    WIFI_MSG_STA_NETWORK_STACK_DHCP_SUCC,
};

static struct server *ctp = NULL;
static const struct ctp_server_info server_info = {
    .ctp_vaild = true,
    .ctp_port = CTP_CTRL_PORT,
    .cdp_vaild = true,
    .cdp_port = CDP_CTRL_PORT,
    .k_alive_type = CTP_ALIVE,
    /*.k_alive_type = CDP_ALIVE,*/
};

static void *wifi_dev = NULL;

extern unsigned int time_lapse(unsigned int *handle, unsigned int time_out);
extern u32 ext_wifi_get_upload_rate(void);
extern u32 ext_wifi_get_download_rate(void);
extern int wifi_module_is_init(void);


static void wifi_app_timer_func(void *p)
{
    os_taskq_post(WIFI_APP_TASK_NAME, 1, WIFI_MSG_TICK_1_SEC);
}

static int network_user_callback(void *network_ctx, enum WIFI_EVENT state, void *priv)
{
    struct cfg_info info = {0};

    switch (state) {

    case WIFI_EVENT_MODULE_INIT:
        log_info("ext_network_user_callback->WIFI_EVENT_MODULE_INIT");

        info.port_status = 0;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);
        msleep(10);
        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER, (u32)&info);

        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_CS, (u32)&info);

        info.port_status = 1;
        dev_ioctl(wifi_dev, DEV_SET_WIFI_WKUP, (u32)&info);

        msleep(100);

//        info.mode = AP_MODE;
//        info.ssid = AP_SSID;
//        info.pwd  = AP_PWD;
//        info.force_default_mode = 1;
//        dev_ioctl(wifi_dev, DEV_SET_DEFAULT_MODE, (u32)&info);
        break;

    case WIFI_EVENT_MODULE_START:
        log_info("ext_network_user_callback->WIFI_EVENT_MODULE_START");
//        info.mode = AP_MODE;
//        info.ssid = AP_SSID;
//        info.pwd  = AP_PWD;
//        info.force_default_mode = 1;
//        dev_ioctl(wifi_dev, DEV_SAVE_DEFAULT_MODE, (u32)&info);

        wpa_supplicant_set_connect_timeout(20);
        break;

    case WIFI_EVENT_MODULE_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_MODULE_STOP");
        break;

    case WIFI_EVENT_AP_START:
        log_info("ext_network_user_callback->WIFI_EVENT_AP_START");
        break;

    case WIFI_EVENT_AP_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_AP_STOP");
        break;

    case WIFI_EVENT_STA_START:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_START");
        break;

    case WIFI_EVENT_MODULE_START_ERR:
        log_info("ext_network_user_callback->WIFI_EVENT_MODULE_START_ERR");
        break;

    case WIFI_EVENT_STA_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_STOP");
        break;

    case WIFI_EVENT_STA_DISCONNECT:
        log_info("ext_network_user_callback->WIFI_STA_DISCONNECT");
        break;

    case WIFI_EVENT_STA_SCAN_COMPLETED:
        log_info("ext_network_user_callback->WIFI_STA_SCAN_COMPLETED");
        os_taskq_post(WIFI_APP_TASK_NAME, 1, WIFI_MSG_STA_SCAN_COMPLETED);
        break;

    case WIFI_EVENT_STA_CONNECT_SUCC:
        dev_ioctl(wifi_dev, DEV_GET_WIFI_CHANNEL, (u32)&info);
        log_info("ext_network_user_callback->WIFI_STA_CONNECT_SUCC,CH=%d", info.sta_channel);
        break;

    case WIFI_EVENT_MP_TEST_START:
        log_info("ext_network_user_callback->WIFI_EVENT_MP_TEST_START");
        break;

    case WIFI_EVENT_MP_TEST_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_MP_TEST_STOP");
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        log_info("ext_network_user_callback->WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID");
        break;

    case WIFI_EVENT_STA_CONNECT_ASSOCIAT_TIMEOUT:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_CONNECT_ASSOCIAT_TIMEOUT");
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_SUCC");
        os_taskq_post(WIFI_APP_TASK_NAME, 1, WIFI_MSG_STA_NETWORK_STACK_DHCP_SUCC);
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_TIMEOUT:
        log_info("ext_network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_TIMEOUT");
        break;

    case WIFI_EVENT_P2P_START:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_START");
        break;

    case WIFI_EVENT_P2P_STOP:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_STOP");
        break;

    case WIFI_EVENT_P2P_GC_DISCONNECTED:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_GC_DISCONNECTED");
#if TCFG_AIC8800_ENABLE
        wlan_enable_p2p(0);
#endif
        break;

    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC");
        break;

    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT:
        log_info("ext_network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT");
        break;

    case WIFI_EVENT_SMP_CFG_TIMEOUT:
        log_info("ext_network_user_callback->WIFI_EVENT_SMP_CFG_TIMEOUT");
        break;

    case WIFI_EVENT_SMP_CFG_COMPLETED:
        log_info("ext_network_user_callback->WIFI_EVENT_SMP_CFG_COMPLETED");
        os_taskq_post(WIFI_APP_TASK_NAME, 1, WIFI_MSG_SMP_CFG_COMPLETED);
        break;

    case WIFI_EVENT_PM_SUSPEND:
        log_info("ext_network_user_callback->WIFI_EVENT_PM_SUSPEND");
        break;

    case WIFI_EVENT_PM_RESUME:
        log_info("ext_network_user_callback->WIFI_EVENT_PM_RESUME");
        break;

    case WIFI_EVENT_AP_ON_ASSOC:
        struct eth_addr *hwaddr = (struct eth_addr *)network_ctx;
        log_info("ext_network_user_callback->WIFI_EVENT_AP_ON_ASSOC hwaddr = %02x:%02x:%02x:%02x:%02x:%02x",
                 hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
#if TCFG_LTE_PHY_ENABLE && IP_NAPT_EXT
        extern void check_dhcps_client_ipaddr(u8 * hwaddr);
        check_dhcps_client_ipaddr(hwaddr->addr);
#endif
        break;

    case WIFI_EVENT_AP_ON_DISCONNECTED:
        hwaddr = (struct eth_addr *)network_ctx;
        log_info("ext_network_user_callback->WIFI_EVENT_AP_ON_DISCONNECTED hwaddr = %02x:%02x:%02x:%02x:%02x:%02x",
                 hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
        break;

    default:
        break;
    }

    return 0;
}

static void wifi_set_lan_setting_info(void)
{
    struct lan_setting lan_setting_info = {
        .WIRELESS_IP_ADDR0  = 192,
        .WIRELESS_IP_ADDR1  = 168,
        .WIRELESS_IP_ADDR2  = 1,
        .WIRELESS_IP_ADDR3  = 1,

        .WIRELESS_NETMASK0  = 255,
        .WIRELESS_NETMASK1  = 255,
        .WIRELESS_NETMASK2  = 255,
        .WIRELESS_NETMASK3  = 0,

        .WIRELESS_GATEWAY0  = 192,
        .WIRELESS_GATEWAY1  = 168,
        .WIRELESS_GATEWAY2  = 1,
        .WIRELESS_GATEWAY3  = 1,

        .SERVER_IPADDR1  = 192,
        .SERVER_IPADDR2  = 168,
        .SERVER_IPADDR3  = 1,
        .SERVER_IPADDR4  = 1,

        .CLIENT_IPADDR1  = 192,
        .CLIENT_IPADDR2  = 168,
        .CLIENT_IPADDR3  = 1,
        .CLIENT_IPADDR4  = 2,

        .SUB_NET_MASK1   = 255,
        .SUB_NET_MASK2   = 255,
        .SUB_NET_MASK3   = 255,
        .SUB_NET_MASK4   = 0,
    };

    struct cfg_info info;
    info.__lan_setting_info = &lan_setting_info;
    dev_ioctl(wifi_dev, DEV_SET_LAN_SETTING, (u32)&info);
}

#if !TCFG_WIFI_ENABLE
void net_app_init(void)
{
#ifdef CONFIG_MASS_PRODUCTION_ENABLE
    if (get_MassProduction()) {
        wifi_enter_sta_mode("wl83", "12345678");
#if 0
        /*     **代码段功能:修改RTSP的URL */
        /* **默认配置  :URL为rtsp://192.168.1.1/avi_pcm_rt/front.sd,//(avi_pcma_rt 传G7111音频)传JPEG实时流 */
        /* ** */
        /* * */
        const char *user_custom_name = "avi_pcm_rt";
        const char *user_custom_content =
            "stream\r\n"\
            "file_ext_name avi\r\n"\
            "media_source live\r\n"\
            "priority 1\r\n"\
            "payload_type 26\r\n"\
            "clock_rate 90000\r\n"\
            "encoding_name JPEG\r\n"\
            "coding_type frame\r\n"\
            "byte_per_pckt 1458\r\n"\
            "stream_end\r\n"\
            "stream\r\n"\
            "file_ext_name pcm\r\n"\
            "media_source live\r\n"\
            "priority 1\r\n"\
            "payload_type 8\r\n"\
            "encoding_name PCMA\r\n"\
            "clock_rate 8000\r\n"\
            "stream_end";
        extern void rtsp_modify_url(const char *user_custom_name, const char *user_custom_content);
        rtsp_modify_url(user_custom_name, user_custom_content);
#endif
        extern int stream_media_server_init(struct fenice_config * conf);
        extern int fenice_get_video_info(struct fenice_source_info * info);
        extern int fenice_get_audio_info(struct fenice_source_info * info);
        extern int fenice_set_media_info(struct fenice_source_info * info);
        extern int fenice_video_rec_setup(void);
        extern int fenice_video_rec_exit(void);
        struct fenice_config conf = {0};

        strncpy(conf.protocol, "UDP", 3);
        conf.exit = fenice_video_rec_exit;
        conf.setup = fenice_video_rec_setup;
        conf.get_video_info = fenice_get_video_info;
        conf.get_audio_info = fenice_get_audio_info;
        conf.set_media_info = fenice_set_media_info;
        conf.port = RTSP_PORT;  // 当为0时,用默认端口554
        stream_media_server_init(&conf);
    } else
#endif
#ifdef CONFIG_RTSP_TEST_ENABLE
        extern int stream_media_server_init(struct fenice_config * conf);
    extern int fenice_get_video_info(struct fenice_source_info * info);
    extern int fenice_get_audio_info(struct fenice_source_info * info);
    extern int fenice_set_media_info(struct fenice_source_info * info);
    extern int fenice_video_rec_setup(void);
    extern int fenice_video_rec_exit(void);
    struct fenice_config conf = {0};

    strncpy(conf.protocol, "UDP", 3);
    conf.exit = fenice_video_rec_exit;
    conf.setup = fenice_video_rec_setup;
    conf.get_video_info = fenice_get_video_info;
    conf.get_audio_info = fenice_get_audio_info;
    conf.set_media_info = fenice_set_media_info;
    conf.port = RTSP_PORT;  // 当为0时,用默认端口554
    stream_media_server_init(&conf);
#else
    {
        ctp = server_open("ctp_server", (void *)&server_info);
        if (!ctp) {
            printf("ctp server fail\n");
        }
        puts("http server init\n");
        extern int http_virfile_reg(const char *path, const char *contents, unsigned long len);

        http_virfile_reg(DEV_DESC_PATH, DEV_DESC_CONTENT, strlen(DEV_DESC_CONTENT)); //注册虚拟文件描述文档,可在dev_desc.h修改
        http_get_server_init(HTTP_PORT); //8080
        video_rt_tcp_server_init(2229);
#ifdef CONFIG_ENABLE_VLIST
        preview_init(VIDEO_PREVIEW_PORT, NULL); //2226
        playback_init(VIDEO_PLAYBACK_PORT, NULL);
#endif


        /* printf("ftpd server init \n"); */
        /*extern void ftpd_vfs_interface_cfg(void);*/
        /*ftpd_vfs_interface_cfg();*/
        /*stupid_ftpd_init("MAXUSERS=2\nUSER=FTPX 12345678     0:/      2   A\n", NULL);*/

        /* void ftpd_server_init(const char *user, const char *pass, const char *ota_name, int fifo_size); */
        /* ftpd_server_init("FTPX", "12345678", "update-ota.ufw", 4096); */
    }
#endif
}
#endif


void ext_wifi_on(void)
{
    dev_ioctl(wifi_dev, DEV_NETWORK_START, 0);
}

void ext_wifi_off(void)
{
    dev_ioctl(wifi_dev, DEV_NETWORK_STOP, 0);
}

static void ext_wifi_app_task(void *priv)
{
    int msg[32];
    int res;
    unsigned int timehdl = 0;
    struct cfg_info info = {0};

//wifi app start
    wifi_dev = dev_open("wifi", NULL);

    info.cb = network_user_callback;
    info.net_priv = NULL;
    dev_ioctl(wifi_dev, DEV_SET_CB_FUNC, (u32)&info);

#if 0
    log_info(">>>> DEV_SET_WIFI_POWER_SAVE<<<<");
    dev_ioctl(wifi_dev, DEV_SET_WIFI_POWER_SAVE, 0);//打开就启用低功耗模式, 只有STA模式才有用
#endif



#if !TCFG_AIC8800_ENABLE
    log_info(">>>> DEV_SET_WIFI_TX_PWR_BY_RATE<<<");
    info.tx_pwr_lmt_enable = 0;//  解除WIFI发送功率限制
    dev_ioctl(wifi_dev, DEV_SET_WIFI_TX_PWR_LMT_ENABLE, (u32)&info);
    info.tx_pwr_by_rate = 1;// 设置WIFI根据不同datarate打不同power
    dev_ioctl(wifi_dev, DEV_SET_WIFI_TX_PWR_BY_RATE, (u32)&info);
#endif

    wifi_set_lan_setting_info();

#if !IP_NAPT_EXT || !TCFG_LTE_PHY_ENABLE
    ext_wifi_on();
#endif

#if !TCFG_WIFI_ENABLE
    net_app_init();
#endif

    sys_timer_add(NULL, wifi_app_timer_func, 1000);

#if (EXT_WIFI_TEST_MODE == AP_TEST_MODE)
    info.mode = AP_MODE;
    info.ssid = AP_SSID;
    info.pwd  = AP_PWD;
    info.force_default_mode = 1;
    dev_ioctl(wifi_dev, DEV_AP_MODE, (u32)&info);
#elif (EXT_WIFI_TEST_MODE == STA_TEST_MODE)
    info.mode = STA_MODE;
    info.ssid = STA_SSID;
    info.pwd  = STA_PWD;
    info.force_default_mode = 1;
    dev_ioctl(wifi_dev, DEV_STA_MODE, (u32)&info);
#elif (EXT_WIFI_TEST_MODE == P2P_TEST_MODE)
    info.p2p_role = 1;
    info.ssid = "AP79N-P2P-EXT";
    info.force_default_mode = 1;
    dev_ioctl(wifi_dev, DEV_P2P_MODE, (u32)&info);
#endif


#ifdef CONFIG_IPERF_ENABLE
    void iperf_test(void);
    iperf_test();
#endif

    while (1) {
        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));

        switch (msg[1]) {
        case WIFI_MSG_TICK_1_SEC:
            if (time_lapse(&timehdl, 3 * 1000)) {
                if (wifi_module_is_init()) {
                    log_info("WIFI U= %d KB/s, D= %d KB/s", ext_wifi_get_upload_rate() / 1024, ext_wifi_get_download_rate() / 1024);
#if 0
                    if (ext_get_cur_wifi_info()->mode == STA_MODE) {
                        get_rx_signal();
                    }
#endif
                }
            }

            break;

        default:
            break;
        }
    }
}
// #if TCFG_EXT_WIFI_ENABLE

static int ext_wireless_net_init(void)//主要是create wifi 线程的
{
    printf("\n--------------EXT_WIFI_APP_TASK START----------\n");

    return thread_fork(WIFI_APP_TASK_NAME, 10, 0x1000, 64, 0, ext_wifi_app_task, NULL);
}
late_initcall(ext_wireless_net_init);

#if !TCFG_WIFI_ENABLE
const char *get_rec_path_0()
{
    return NULL;
}
const char *get_rec_path_1()
{
    return CONFIG_REC_PATH_0;
}
const char *get_rec_path_2()
{
    return CONFIG_REC_PATH_1;
}
const char *get_rec_path_3()
{
    return CONFIG_REC_PATH_2;
}
#endif


//sdio驱动底层调用接口

//设置指定IO的强驱
int get_sdio_hd_value(void)
{
    printf("sdio hd level set\n");
    return 0;
}

//设置高速卡
int get_sdio_hs_enable(void)
{
    return 0;
}

//返回Hi3861L用的edge
int SDIO_DAT_EDGE_GET(void)
{
    return 0;
}

//CTU模式下连续读写报错时回调，用于过滤错误的报错信息
int sdio_wr_err_cb(int crc_status)
{
    if (crc_status == 1) {
        return 0;
    }
    printf("\n >>>crc_status = %d \n", crc_status);
    return -1;
}

//AIC8800需要软件判忙
int get_sdio_tx_ctu_enable(void)
{
#if TCFG_AIC8800_ENABLE
    return 0;
#else
    return 1;
#endif
}

int get_sdio_rx_ctu_enable(void)
{
    return 1;
}

void port_wakeup_reg_set_gpio_cb(int event, unsigned int gpio, int edge)
{
    if (event != 0) {
        gpio_direction_input(gpio);
        gpio_set_die(gpio, 1);
        if (edge == 0) {
            gpio_set_pull_down(gpio, 1);
            gpio_set_pull_up(gpio, 0);
        } else if (edge == 1) {
            gpio_set_pull_down(gpio, 0);
            gpio_set_pull_up(gpio, 1);
        }
    }
}

static OS_SEM busy_sem;

static void sdio_wait_busy_isr(void *priv)
{
//    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>1");
    os_sem_post(&busy_sem);
}

//软件判忙操作
void sdio_wait_busy(int gpio)
{
    int ret;
    int index = 0;

    if (!os_sem_valid(&busy_sem)) {
        os_sem_create(&busy_sem, 0);
    }

    index = exti_init(gpio, 1, sdio_wait_busy_isr, NULL);

    if (gpio_read(gpio)) {
        goto __exit;
    } else {
        ret = os_sem_pend(&busy_sem, 10);
        if (ret) {
            while (1) {
                puts("BUSY");
                msleep(100);
            }
        }
    }
__exit:

    exti_uninit(index);
    os_sem_set(&busy_sem, 0);
}

#endif
