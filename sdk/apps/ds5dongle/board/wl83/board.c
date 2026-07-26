#include "app_config.h"
#include "system/includes.h"
#include "device/includes.h"
#include "device/uart.h"
#include "device/iic.h"
#include "asm/includes.h"


// *INDENT-OFF*

#if TCFG_UART0_ENABLE
UART0_PLATFORM_DATA_BEGIN(uart0_data)
#ifdef CONFIG_DEBUG_ENABLE
    .disable_tx_irq         = 1,
#endif
    .baudrate               = TCFG_UART0_BAUDRATE,
    .tx_pin                 = TCFG_UART0_TX_IO,
    .rx_pin                 = TCFG_UART0_RX_IO,
    .max_continue_recv_cnt  = 1024,
    .idle_sys_clk_cnt       = 500000,
    .flags                  = UART_DEBUG,
    .parity                 = TCFG_UART0_PARITY,
UART0_PLATFORM_DATA_END();
#endif // TCFG_UART0_ENABLE


#if TCFG_UART2_ENABLE
UART2_PLATFORM_DATA_BEGIN(uart2_data)
    .flow_ctl_enable        = TCFG_UART2_FLOW_CTL_ENABLE,
    .rts_hw_enable          = TCFG_UART2_RTS_HW_ENABLE,
    .baudrate               = TCFG_UART2_BAUDRATE,
    .tx_pin                 = TCFG_UART2_TX_IO,
    .rx_pin                 = TCFG_UART2_RX_IO,
    .rts_pin                = TCFG_UART2_RTS_IO,
    .cts_pin                = TCFG_UART2_CTS_IO,
    .rts_idle_level         = TCFG_UART2_RTS_IDLE_LEVEL,
    .cts_idle_level         = TCFG_UART2_CTS_IDLE_LEVEL,
    .rx_thresh              = TCFG_UART2_RTS_RX_THRESH,
    .max_continue_recv_cnt  = TCFG_UART2_MAX_CONTINUE_RECV_CNT,
    .idle_sys_clk_cnt       = 500000,
    .parity                 = TCFG_UART2_PARITY,
UART2_PLATFORM_DATA_END();
#endif // TCFG_UART2_ENABLE


#if TCFG_CAN_ENABLE
CAN_PLATFORM_DATA_BEGIN(can_data)
    .can_mode               = TCFG_CAN_MODE,
    .rx_pin                 = TCFG_CAN_RX_IO,
    .tx_pin                 = TCFG_CAN_TX_IO,
    .stm                    = TCFG_CAN_STM_ENABLE,
    .lom                    = TCFG_CAN_LOM_ENABLE,
    .baudrate = {
       .brp                 = TCFG_CAN_BAUD_BRP,
       .sjw                 = TCFG_CAN_BAUD_SJW,
       .sam                 = TCFG_CAN_BAUD_SAM,
       .tseg2               = TCFG_CAN_BAUD_PBS2,
       .tseg1               = TCFG_CAN_BAUD_PTS_PBS1,
    },
    .filter = {
        .filter_mode        = TCFG_CAN_FILTER_MODE,
#if TCFG_CAN_MODE
        .accept_id0         = TCFG_CAN_FILTER_ACC_ID0,
        .accept_id1         = TCFG_CAN_FILTER_ACC_ID1,
        .accept_rtr0        = TCFG_CAN_FILTER_ACC_RTR0,
        .accept_rtr1        = TCFG_CAN_FILTER_ACC_RTR1,
        .mask_id0           = TCFG_CAN_FILTER_MASK_ID0,
        .mask_id1           = TCFG_CAN_FILTER_MASK_ID1,
        .mask_rtr0          = TCFG_CAN_FILTER_MASK_RTR0,
        .mask_rtr1          = TCFG_CAN_FILTER_MASK_RTR1,
#else
        .accept_id0         = TCFG_CAN_FILTER_ACC_ID0,
        .mask_id0           = TCFG_CAN_FILTER_MASK_ID0,
#endif
	},
    .priority               = 5,
    .cpu_id                 = 0,
CAN_PLATFORM_DATA_END()
#endif // TCFG_CAN_ENABLE


#if TCFG_GPCNT_ENABLE
static const struct gpcnt_platform_data gpcnt_data = {
    .gpio                   = TCFG_GPCNT_INPUT_IO,
    .gss_clk_source         = TCFG_GPCNT_GSS_CLK_SOURCE,
    .css_clk_source         = TCFG_GPCNT_CSS_CLK_SOURCE,
    .cycle                  = TCFG_GPCNT_GSS_CYCLE,
};
#endif // TCFG_GPCNT_ENABLE


#if TCFG_IIC_ENABLE
HW_IIC0_PLATFORM_DATA_BEGIN(hw_iic0_data) ///< 硬件IIC0为增强型IIC
#if TCFG_HW_IIC0_ROLE == IIC_ROLE_MASTER
    .role                   = IIC_ROLE_MASTER,
#else
    .role                   = IIC_ROLE_SLAVE,
    .slave_addr             = TCFG_HW_IIC0_SLAVE_ADDR,
    .adv_enable             = 0,
    .stretch_disable        = TCFG_HW_IIC0_STRETCH_DISABLE,
#endif
    .hd_level               = TCFG_HW_IIC0_HD_LEVEL,
    .clk_pin                = TCFG_HW_IIC0_SCL_IO,
    .dat_pin                = TCFG_HW_IIC0_SDA_IO,
    .io_filter              = 2,
    .internal_pull_up_enable= 1,
    .fast_resp              = 0,
    .ie_enable              = TCFG_HW_IIC0_IRQ_ENABLE,
    .baudrate               = TCFG_HW_IIC0_BAUDRATE,
    .timeout_cnt            = 40,
HW_IIC0_PLATFORM_DATA_END()

HW_IIC1_PLATFORM_DATA_BEGIN(hw_iic1_data) ///< 硬件IIC1为普通型IIC
    .hd_level               = TCFG_HW_IIC1_HD_LEVEL,
    .clk_pin                = TCFG_HW_IIC1_SCL_IO,
    .dat_pin                = TCFG_HW_IIC1_SDA_IO,
    .baudrate               = TCFG_HW_IIC1_BAUDRATE,
    .internal_pull_up_enable= 1,
    .timeout_cnt            = 40,
HW_IIC1_PLATFORM_DATA_END()

SW_IIC_PLATFORM_DATA_BEGIN(sw_iic2_data)
    .clk_pin                = TCFG_SW_IIC2_SCL_IO,
    .dat_pin                = TCFG_SW_IIC2_SDA_IO,
    .sw_iic_delay           = TCFG_SW_IIC2_DELAY,
    .sw_iic_byte_gap_time   = TCFG_SW_IIC2_BYTE_GAP_TIME,
    .hd_level               = TCFG_SW_IIC2_HD_LEVEL,
SW_IIC_PLATFORM_DATA_END()
#endif // TCFG_IIC_ENABLE


#if TCFG_PWM1_ENABLE
PWM_PLATFORM_DATA_BEGIN(pwm_data1)
    .timer_pwm_port         = TCFG_PWM1_TIMER_PWM_REMAP_IO,
    .mcpwm_port             = TCFG_PWM1_MCPWM_REMAP_IO,
    .pwm_config = {
        .pwm_ch             = TCFG_PWM1_CH_MAPPING,
        .freq               = TCFG_PWM1_FREQUENCY,
        .duty               = TCFG_PWM1_DUTY,
        .point_bit          = TCFG_PWM1_DECIMAL_POINT,
    },
PWM_PLATFORM_DATA_END()
#endif // TCFG_PWM1_ENABLE


#if TCFG_RTC_ENABLE
static void alarm_wakeup_cb_in_irq(void)
{
    printf("alarm_wakeup_cb_in_irq\n");
}

static void pr_wakeup_cb_in_irq(int port)
{
    printf("pr_wakeup_cb_in_irq\n");
}

RTC_PLATFORM_DATA_BEGIN(rtc_data)
    .default_sys_time       = {
        .year               = 2024,
        .month              = 1,
        .day                = 1,
        .hour               = 0,
        .min                = 0,
        .sec                = 0,
    },
    .alarm_wakeup_cb_in_irq = alarm_wakeup_cb_in_irq,
    .pr_wakeup_cb_in_irq    = pr_wakeup_cb_in_irq,
    .clk_sel                = TCFG_RTC_CLK_SEL,
    .x32xs                  = TCFG_RTC_X32XS_LEVEL,
RTC_PLATFORM_DATA_END()
#endif // TCFG_RTC_ENABLE


#if TCFG_SPI1_ENABLE
SPI1_PLATFORM_DATA_BEGIN(spi1_data)
    .io = {
        .cs_pin             = TCFG_SPI1_CS_IO,
        .di_pin             = TCFG_SPI1_DI_IO,
        .do_pin             = TCFG_SPI1_DO_IO,
        .clk_pin            = TCFG_SPI1_CLK_IO,
        .d2_pin             = TCFG_SPI1_D2_IO,
        .d3_pin             = TCFG_SPI1_D3_IO,
    },
    .clk                    = TCFG_SPI1_BAUDRATE,
    .mode                   = TCFG_SPI1_MODE,
    .priority               = 6,
    .cpu_id                 = 1,
    .hd_level               = TCFG_SPI1_HD_LEVEL,
    .attr                   = TCFG_SPI1_ATTR,
    .role                   = TCFG_SPI1_ROLE,
SPI1_PLATFORM_DATA_END()
#endif // TCFG_SPI1_ENABLE


#if TCFG_SPI2_ENABLE
SPI2_PLATFORM_DATA_BEGIN(spi2_data)
    .io = {
        .cs_pin             = TCFG_SPI2_CS_IO,
        .di_pin             = TCFG_SPI2_DI_IO,
        .do_pin             = TCFG_SPI2_DO_IO,
        .clk_pin            = TCFG_SPI2_CLK_IO,
        .d2_pin             = TCFG_SPI2_D2_IO,
        .d3_pin             = TCFG_SPI2_D3_IO,
    },
    .clk                    = TCFG_SPI2_BAUDRATE,
    .mode                   = TCFG_SPI2_MODE,
    .priority               = 6,
    .cpu_id                 = 1,
    .hd_level               = TCFG_SPI2_HD_LEVEL,
    .attr                   = TCFG_SPI2_ATTR,
    .role                   = TCFG_SPI2_ROLE,
SPI2_PLATFORM_DATA_END()
#endif // TCFG_SPI2_ENABLE


#if TCFG_LEDC1_ENABLE
LEDC1_PLATFORM_DATA_BEGIN(ledc1_data)
    .port                   = TCFG_LEDC1_PORT,
    .idle_level             = TCFG_LEDC1_IDLE_LEVEL,
    .out_inv                = TCFG_LEDC1_OUT_INV,
    .bit_inv                = TCFG_LEDC1_BIT_INV,
    .time_base              = TCFG_LEDC1_TIME_BASE,
    .t1h_cnt                = TCFG_LEDC1_T1H_CNT,
    .t1l_cnt                = TCFG_LEDC1_T1L_CNT,
    .t0h_cnt                = TCFG_LEDC1_T0H_CNT,
    .t0l_cnt                = TCFG_LEDC1_T0L_CNT,
    .rst_time_cnt           = TCFG_LEDC1_RST_TIME_CNT,
LEDC1_PLATFORM_DATA_END()
#endif // TCFG_LEDC1_ENABLE

#if TCFG_ADKEY_ENABLE
#define ADC_VDDIO   (0x3FF)

#define ADKEY_V_9   ((TCFG_ADC_LEVEL_09 + ADC_VDDIO) / 2)
#define ADKEY_V_8   ((TCFG_ADC_LEVEL_08 + TCFG_ADC_LEVEL_09) / 2)
#define ADKEY_V_7   ((TCFG_ADC_LEVEL_07 + TCFG_ADC_LEVEL_08) / 2)
#define ADKEY_V_6   ((TCFG_ADC_LEVEL_06 + TCFG_ADC_LEVEL_07) / 2)
#define ADKEY_V_5   ((TCFG_ADC_LEVEL_05 + TCFG_ADC_LEVEL_06) / 2)
#define ADKEY_V_4   ((TCFG_ADC_LEVEL_04 + TCFG_ADC_LEVEL_05) / 2)
#define ADKEY_V_3   ((TCFG_ADC_LEVEL_03 + TCFG_ADC_LEVEL_04) / 2)
#define ADKEY_V_2   ((TCFG_ADC_LEVEL_02 + TCFG_ADC_LEVEL_03) / 2)
#define ADKEY_V_1   ((TCFG_ADC_LEVEL_01 + TCFG_ADC_LEVEL_02) / 2)
#define ADKEY_V_0   ((TCFG_ADC_LEVEL_00 + TCFG_ADC_LEVEL_01) / 2)

static const struct adkey_platform_data adkey_data = {
    .adkey_pin              = TCFG_ADKEY_INPUT_IO,
    .extern_up_en           = 1,
    .ad_channel             = TCFG_ADKEY_INPUT_CHANNEL,
    .ad_value               = {
                                ADKEY_V_0,
                                ADKEY_V_1,
                                ADKEY_V_2,
                                ADKEY_V_3,
                                ADKEY_V_4,
                                ADKEY_V_5,
                                ADKEY_V_6,
                                ADKEY_V_7,
                                ADKEY_V_8,
                                ADKEY_V_9,
    },
    .key_value              = {
                                TCFG_ADKEY_VALUE_0,
                                TCFG_ADKEY_VALUE_1,
                                TCFG_ADKEY_VALUE_2,
                                TCFG_ADKEY_VALUE_3,
                                TCFG_ADKEY_VALUE_4,
                                TCFG_ADKEY_VALUE_5,
                                TCFG_ADKEY_VALUE_6,
                                TCFG_ADKEY_VALUE_7,
                                TCFG_ADKEY_VALUE_8,
                                TCFG_ADKEY_VALUE_9,
    },
};

const struct adkey_platform_data *get_adkey_platform_data(void)
{
    return &adkey_data;
}
#endif

#if TCFG_IOKEY_ENABLE
static const struct iokey_port iokey_list[] = {
#if TCFG_IOKEY_INPUT0_ENABLE
    {
        .connect_way        = TCFG_IOKEY_INPUT0_CONNECT_WAY,
        .key_type.one_io.port = TCFG_IOKEY_INPUT0_IO,
        .key_value          = TCFG_IOKEY_INPUT0_VALUE,
    },
#endif
#if TCFG_IOKEY_INPUT1_ENABLE
    {
        .connect_way        = TCFG_IOKEY_INPUT1_CONNECT_WAY,
        .key_type.one_io.port = TCFG_IOKEY_INPUT1_IO,
        .key_value          = TCFG_IOKEY_INPUT1_VALUE,
    },
#endif
#if TCFG_IOKEY_INPUT2_ENABLE
    {
        .connect_way        = TCFG_IOKEY_INPUT2_CONNECT_WAY,
        .key_type.one_io.port = TCFG_IOKEY_INPUT2_IO,
        .key_value          = TCFG_IOKEY_INPUT2_VALUE,
    },
#endif
};

static const struct iokey_platform_data iokey_data = {
    .num                    = ARRAY_SIZE(iokey_list),
    .port                   = iokey_list,
};

const struct iokey_platform_data *get_iokey_platform_data(void)
{
    return &iokey_data;
}
#endif



REGISTER_DEVICES(device_table) = {

#if TCFG_UART0_ENABLE
    { "uart0", &uart_dev_ops, (void *)&uart0_data },
#endif

#if TCFG_UART2_ENABLE
    { "uart2", &uart_dev_ops, (void *)&uart2_data },
#endif

#if TCFG_CAN_ENABLE
    { "can", &can_dev_ops, (void *)&can_data },
#endif

#if TCFG_GPCNT_ENABLE
    { "gpcnt", &gpcnt_dev_ops, (void *)&gpcnt_data },
#endif

#if TCFG_IIC_ENABLE
    { "iic0",  &iic_dev_ops, (void *)&hw_iic0_data },
    { "iic1",  &iic_dev_ops, (void *)&hw_iic1_data },
    { "iic2",  &iic_dev_ops, (void *)&sw_iic2_data },
#endif

#if TCFG_PWM1_ENABLE
    { "pwm1",   &pwm_dev_ops,  (void *)&pwm_data1 },
#endif

#if TCFG_RTC_ENABLE
    { "rtc", &rtc_dev_ops, (void *)&rtc_data },
#endif

#if TCFG_SPI1_ENABLE
    { "spi1", &spi_dev_ops, (void *)&spi1_data },
#endif

#if TCFG_SPI2_ENABLE
    { "spi2", &spi_dev_ops, (void *)&spi2_data },
#endif

#if TCFG_LEDC1_ENABLE
    { "ledc1", &ledc_dev_ops, (void *)&ledc1_data },
#endif
};


static const power_param_t power_param = {
    .config                 = TCFG_LOWPOWER_FUNCTION,
    .btosc_hz               = 24000000,
    .vddiom_lev             = TCFG_LOWPOWER_VDDIOM_LEVEL,
    .vddiow_lev             = TCFG_LOWPOWER_VDDIOW_LEVEL,
    .vdc14_lev              = TCFG_LOWPOWER_VDC14_LEVEL,
    .osc_type               = TCFG_LOWPOWER_OSC_TYPE,
    .osc_delay_us           = 8000,
#if TCFG_POWER_AVDD18_ENABLE
    .avdd18_enable          = 1,
#endif
#if TCFG_POWER_AVDD28_ENABLE
    .avdd28_enable          = 1,
#endif
};

static const power_pdata_t power_pdata = {
    .power_param            = &power_param,
};

#if TCFG_LOWPOWER_WAKEUP_PORT0_ENABLE
static void wakeup_port0_callback_in_irq(p33_io_wkup_edge_t edge)
{
    // 非pdown状态下关闭io wakeup，避免与按键冲突
    __p33_io_wakeup_enable(TCFG_LOWPOWER_WAKEUP_PORT0_IO, 0);
    printf("wakeup port0 callback\n");
}

static const p33_io_wakeup_config_t wakeup_port0 = {
    .pullup_down_enable     = 0,
    .filter                 = TCFG_LOWPOWER_WAKEUP_PORT0_FILTER,
    .edge                   = TCFG_LOWPOWER_WAKEUP_PORT0_EDGE,
    .gpio                   = TCFG_LOWPOWER_WAKEUP_PORT0_IO,
    .callback               = wakeup_port0_callback_in_irq,
};
#endif

static u32 fusb_io_con = 0;
static u32 husb_io_con = 0;

// 进入、退出低功耗函数回调状态，函数单核操作、关中断，请勿做耗时操作
void sleep_enter_callback(u8 step)
{
    /* 此函数禁止添加打印 */
    putchar('<');

    //USB IO打印引脚特殊处理
#if (defined CONFIG_DEBUG_ENABLE) && ((TCFG_DEBUG_UART_TX_PIN == IO_PORT_USB_DPA) || (TCFG_DEBUG_UART_TX_PIN == IO_PORT_USB_DMA))
    fusb_io_con = JL_PORTFUSB->DIR;
    gpio_close(PORTFUSB, 0xffff);
#endif
#if (defined CONFIG_DEBUG_ENABLE) && ((TCFG_DEBUG_UART_TX_PIN == IO_PORT_USB_DPB) || (TCFG_DEBUG_UART_TX_PIN == IO_PORT_USB_DMB))
    husb_io_con = JL_PORTHUSB->DIR;
    gpio_close(PORTHUSB, 0xffff);
#endif

#if TCFG_LOWPOWER_WAKEUP_PORT0_ENABLE
    __p33_io_wakeup_enable(TCFG_LOWPOWER_WAKEUP_PORT0_IO, 1);
#endif
}

void sleep_exit_callback(u32 usec)
{
    //USB IO打印引脚特殊处理
#if (defined CONFIG_DEBUG_ENABLE) && ((TCFG_DEBUG_UART_TX_PIN == IO_PORT_USB_DPA) || (TCFG_DEBUG_UART_TX_PIN == IO_PORT_USB_DMA))
    JL_PORTFUSB->DIR = fusb_io_con;
#endif
#if (defined CONFIG_DEBUG_ENABLE) && ((TCFG_DEBUG_UART_TX_PIN == IO_PORT_USB_DPB) || (TCFG_DEBUG_UART_TX_PIN == IO_PORT_USB_DMB))
    JL_PORTHUSB->DIR = husb_io_con;
#endif

#if TCFG_LOWPOWER_WAKEUP_PORT0_ENABLE
    // 如果io wakeup起pending，由中断回调disable io wakeup
    if (p33_io_wakeup_get_pending(TCFG_LOWPOWER_WAKEUP_PORT0_IO) == 0) {
        // 非pdown状态下关闭io wakeup，避免与按键冲突
        __p33_io_wakeup_enable(TCFG_LOWPOWER_WAKEUP_PORT0_IO, 0);
    }
#endif
    putchar('>');
}

//maskrom 使用到的io
static void __mask_io_cfg(void)
{
    struct boot_soft_flag_t g_boot_soft_flag = {0};

    g_boot_soft_flag.flag0.boot_ctrl.wdt_dis                    = 0;
    g_boot_soft_flag.flag0.boot_ctrl.lvd_en                     = 1;
    g_boot_soft_flag.flag0.boot_ctrl.sfc_fast_boot              = 0;
    g_boot_soft_flag.flag0.boot_ctrl.flash_power_keep           = 0;
    g_boot_soft_flag.flag0.boot_ctrl.skip_flash_reset           = 0;
    g_boot_soft_flag.flag0.boot_ctrl.sfc_flash_stable_delay_sel = 0;
    g_boot_soft_flag.flag0.boot_ctrl.flash_stable_delay_sel     = 0;    //0: 0mS;   1: 4mS

    g_boot_soft_flag.flag1.boot_ctrl.fusbdp                      = 0;
    g_boot_soft_flag.flag1.boot_ctrl.fusbdm                      = 0;

    g_boot_soft_flag.flag3.boot_ctrl.husbdp                      = 0;
    g_boot_soft_flag.flag3.boot_ctrl.husbdm                      = 0;

    mask_softflag_config(&g_boot_soft_flag);
}

static void gpio_config_soft_poweroff(void)
{
    PORT_TABLE(g);

#if TCFG_LOWPOWER_WAKEUP_PORT0_ENABLE
    PORT_PROTECT(TCFG_LOWPOWER_WAKEUP_PORT0_IO);
#endif

    __port_init((u32)gpio_config);
}

u8 power_soff_callback(void)
{
    printf("++++++++%s--------", __FUNCTION__);

    /* do_platform_uninitcall(); */

#if TCFG_ADKEY_ENABLE || TCFG_RTC_ENABLE
    adc_suspend();
#endif

#if TCFG_LOWPOWER_WAKEUP_PORT0_ENABLE
    p33_io_wakeup_enable(TCFG_LOWPOWER_WAKEUP_PORT0_IO, 1);
#endif

    __mask_io_cfg();

    gpio_config_soft_poweroff();

    /* gpio_config_uninit(); */

    return 0;
}

void power_early_flowing(void)
{
    PORT_TABLE(g);

    // 默认关闭长按复位0，由key_driver配置
    gpio_longpress_pin0_reset_config(IO_PORTD_01, 0, 0, 1, 1);

#if (defined CONFIG_DEBUG_ENABLE) && (TCFG_DEBUG_PORT != -1)
    PORT_PROTECT(TCFG_DEBUG_PORT);
#endif

    power_early_init((u32)gpio_config);

    power_set_mode(TCFG_POWER_MODE);

#if TCFG_RTC_ENABLE
    set_rtc_platform_data(&rtc_data);
#endif
}

static void board_power_init(void)
{
    /* gpio_config_init(); */
#if TCFG_RTC_ENABLE
    power_control(PCONTROL_RTC_CLK, TCFG_RTC_CLK_SEL);
#endif
    power_control(PCONTROL_PD_VDDIO_KEEP, VDDIO_KEEP_TYPE_NORMAL);
    power_control(PCONTROL_SF_VDDIO_KEEP, VDDIO_KEEP_TYPE_NORMAL);

    power_init(&power_pdata);

#if TCFG_LOWPOWER_WAKEUP_PORT0_ENABLE
    p33_io_wakeup_port_init(&wakeup_port0);
#endif
}


#ifdef CONFIG_DEBUG_ENABLE
void debug_uart_init(void)
{
#if TCFG_UART0_ENABLE
    uart_init(&uart0_data);
#endif
}
#endif

void board_early_init(void)
{
    board_power_init();

    devices_init();
}

void board_init(void)
{
#if TCFG_ADKEY_ENABLE || TCFG_RTC_ENABLE
    adc_init();
#endif

#if TCFG_RTC_ENABLE
    rtc_early_init();
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE)
    key_driver_init();
#endif
}
