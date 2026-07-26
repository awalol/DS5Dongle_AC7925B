#include "app_config.h"
#include "system/includes.h"
#include "event/key_event.h"
#include "asm/power_interface.h"

/*中断列表 */
const struct irq_info irq_info_table[] = {
    //中断号   //优先级0-7   //注册的cpu(0或1)
#if CPU_CORE_NUM == 1
    { IRQ_SOFT5_IDX,      6,   0    }, //此中断强制注册到cpu0
    { IRQ_SOFT4_IDX,      6,   1    }, //此中断强制注册到cpu1
    { -2,     	         -2,  -2    }, //如果加入了该行, 那么只有该行之前的中断注册到对应核, 其他所有中断强制注册到CPU0
#endif

    { -1,     -1,   -1    },
};

/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            15,     2048,	  1024 },
    {"sys_event",           29,     512,	   0 },
    {"systimer",            14,     256, 	   0 },
    {"sys_timer",            9,     512,	  128 },

    {0, 0, 0, 0, 0},
};


static int key_long(struct key_event *key)
{
    switch (key->value) {
    case KEY_POWER:
        printf("KEY_POWER LONG\n");
        power_set_soft_poweroff(0);
        break;
    case KEY_MODE:
        printf("KEY_MODE LONG\n");
        break;
    case KEY_UP:
        printf("KEY_UP LONG\n");
        break;
    case KEY_DOWN:
        printf("KEY_DOWN LONG\n");
        break;
    case KEY_OK:
        printf("KEY_OK LONG\n");
        break;
    default:
        break;
    }
    return false;
}

static int key_click(struct key_event *key)
{
    switch (key->value) {
    case KEY_POWER:
        printf("KEY_POWER\n");
        break;
    case KEY_MODE:
        printf("KEY_MODE\n");
        break;
    case KEY_UP:
        printf("KEY_UP\n");
        break;
    case KEY_DOWN:
        printf("KEY_DOWN\n");
        break;
    case KEY_OK:
        printf("KEY_OK\n");
        break;
    default:
        break;
    }
    return false;
}

static int key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        key_click(key);
        break;
    case KEY_EVENT_DOUBLE_CLICK:
        break;
    case KEY_EVENT_TRIPLE_CLICK:
        break;
    case KEY_EVENT_LONG:
        key_long(key);
        break;
    case KEY_EVENT_HOLD:
        break;
    case KEY_EVENT_UP:
        break;
    default:
        break;
    }

    return TRUE;
}


/*
 * 默认的系统事件处理函数
 * 当所有活动的app的事件处理函数都返回false时此函数会被调用
 */
void app_default_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        key_event_handler((struct key_event *)event->payload);
        break;
    case SYS_TOUCH_EVENT:
        break;
    case SYS_DEVICE_EVENT:
        break;
    case SYS_NET_EVENT:
        break;
    case SYS_BT_EVENT:
        break;
    default:
        ASSERT(0, "unknow event type: %s\n", __func__);
        break;
    }
}

static void ds5dongle_task(void *p)
{
    while (1) {
        msleep(1000);
    }
}

/*
 * 应用程序主函数
 */
void app_main(void)
{
    printf("\n\n\n ------------ds5dongle run %s-------------\n\n\n", __TIME__);

    os_task_create(ds5dongle_task, NULL, 10, 1000, 0, "ds5dongle_task");
}

