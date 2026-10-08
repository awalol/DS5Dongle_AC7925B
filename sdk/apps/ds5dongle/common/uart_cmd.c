#include "system/includes.h"
#include "app_config.h"
#include "device/uart.h"
#include "uart_cmd.h"
#include "bt.h"
#include "usb/device/usb_stack.h"

extern void go_mask_usb_updata(void);


static void cmd_update_enter(void)
{
    puts("UART UPDATE command received");
    go_mask_usb_updata();
}

static void cmd_usb_start(void)
{
    usb_start(1,0);
}

static void uart_cmd_post(void (*callback)(void))
{
    int msg[] = {(int)callback, 0};
    int err = os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("UART command queue failed: %d\n", err);
    }
}

static void uart_cmd_rx_callback(u8 *packet, u32 size)
{
    u32 i;

    for (i = 0; i < size; i++) {
        switch (packet[i]) {
        case 'U':
            uart_cmd_post(cmd_update_enter);
            break;
        case 'S':
            uart_cmd_post(cmd_usb_start);
            break;
        case 'I':
            uart_cmd_post(bt_start_inquiry);
            break;
        default:
            break;
        }
    }
}

/* AC792 uses the UART device API instead of uart_db_regiest_recieve_callback.
 * UART0 shares the board's debug port; commands use its configured RX pin. */
#if TCFG_UART0_ENABLE
static u8 uart_cmd_rx_buf[1024] __attribute__((aligned(32)));
static bool uart_cmd_started;

static void uart_cmd_task(void *priv)
{
    (void)priv;
    u8 packet[64];
    void *hdl = dev_open("uart0", NULL);
    if (!hdl) {
        puts("UART command: open uart0 failed\n");
        uart_cmd_started = false;
        return;
    }

    if (dev_ioctl(hdl, IOCTL_UART_SET_CIRCULAR_BUFF_ADDR, (u32)uart_cmd_rx_buf) ||
        dev_ioctl(hdl, IOCTL_UART_SET_CIRCULAR_BUFF_LENTH, sizeof(uart_cmd_rx_buf)) ||
        dev_ioctl(hdl, IOCTL_UART_SET_RECV_BLOCK, 1) ||
        dev_ioctl(hdl, IOCTL_UART_SET_RECV_TIMEOUT, 1000) ||
        dev_ioctl(hdl, IOCTL_UART_START, 0)) {
        puts("UART command: configure uart0 failed\n");
        dev_close(hdl);
        uart_cmd_started = false;
        return;
    }

    puts("UART commands ready: U=update, I=inquiry, S=usb\n");
    while (1) {
        int len = dev_read(hdl, packet, sizeof(packet));
        if (len > 0) {
            uart_cmd_rx_callback(packet, len);
        } else if (len == UART_CIRCULAR_BUFFER_WRITE_OVERLAY) {
            puts("UART command RX overflow\n");
            dev_ioctl(hdl, IOCTL_UART_FLUSH, 0);
        } else if (len != UART_RECV_TIMEOUT) {
            printf("UART command read failed: %d\n", len);
            os_time_dly(10);
        }
    }
}
#endif

void uart_cmd_init(void)
{
#if TCFG_UART0_ENABLE
    if (uart_cmd_started) {
        return;
    }
    uart_cmd_started = true;
    int err = os_task_create(uart_cmd_task, NULL, 10, 1024, 0, "uart_cmd");
    if (err) {
        uart_cmd_started = false;
        printf("UART command task creation failed: %d\n", err);
    }
#else
    puts("UART commands disabled: UART0 is disabled\n");
#endif
}
