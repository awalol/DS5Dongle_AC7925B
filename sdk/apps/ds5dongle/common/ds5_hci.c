#include <stdint.h>
#include "os/os_api.h"

/* AC792 内置 HCI 传输使用 btstack_task.c.o 的内部接口约定：
 * Q_USER + 2 消息携带 ACL 缓冲区指针及其长度（包含 4 字节 HCI ACL 头）。
 * os_taskq_post_type 会复制这两个消息参数。
 *
 * 默认的 hci_send_acl_data -> hci_packet_handler 路径遇到 OS_Q_FULL 时
 * 会休眠并重试。而 btstack 也需要向控制器队列投递“主机已处理包数”
 * 通知（0x0c35），两个队列同时满时可能导致 btctrler 与 btstack 相互等待。
 *
 * 此处立即返回队列投递结果。失败时，SDK 调用方
 * lmp_send_acl_u_packet_to_host 会清除缓冲区的已提交标志，
 * 并将数据包保留在控制器接收链表中。btstack 消费已入队的 ACL 包后，
 * 正常发送的完成通知会驱动控制器再次尝试投递。
 * 此处不得释放、复制 ACL 缓冲区，也不得提前确认数据包已处理。
 *
 * ds5dongle 构建时会在 LTO 之前重命名生成库中的原函数定义，
 * 使所有 ACL 调用方链接到此实现。其他 HCI 包类型保留原有处理路径，
 * 共享蓝牙静态库保持不变。
 */
int hci_send_acl_data(u8 *packet, u16 size)
{
    int msg[] = {(int)(uintptr_t)packet, size};

    return os_taskq_post_type("btstack", Q_USER + 2, 2, msg);
}
