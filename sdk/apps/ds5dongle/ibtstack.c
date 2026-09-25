#include "ibtstack.h"

static uint8_t connection = 0;

/*
 * 杰理当前 SDK 没有提供 Classic L2CAP 的公开头文件，但 btstack.a
 * 导出了下列 internal ABI。此文件只做无状态的接口适配。
 */
extern u16 l2cap_create_channel_internal(void *connection,
        btstack_packet_handler_t packet_handler, bd_addr_t addr,
        u16 psm, u16 mtu);
extern void l2cap_register_service_internal(void *connection,
        btstack_packet_handler_t packet_handler, u16 psm, u16 mtu);
extern void l2cap_accept_connection_internal(u16 local_cid);
extern void l2cap_decline_connection_internal(u16 local_cid, u8 reason);

u16 l2cap_create_channel(btstack_packet_handler_t packet_handler,
                                  bd_addr_t addr, u16 psm, u16 mtu)
{
    return l2cap_create_channel_internal(&connection, packet_handler, addr, psm, mtu);
}

void l2cap_register_service(btstack_packet_handler_t packet_handler,
                                     u16 psm, u16 mtu)
{
    l2cap_register_service_internal(&connection, packet_handler, psm, mtu);
}

void l2cap_accept_connection(u16 local_cid)
{
    l2cap_accept_connection_internal(local_cid);
}

void l2cap_decline_connection(u16 local_cid, u8 reason)
{
    l2cap_decline_connection_internal(local_cid, reason);
}
