#include "ibtstack.h"

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
extern int l2cap_send_internal(u16 local_cid, u8 *data, u16 len, u8 flag);

u16 l2cap_create_channel(btstack_packet_handler_t packet_handler,
                                  bd_addr_t addr, u16 psm, u16 mtu)
{
    return l2cap_create_channel_internal(NULL, packet_handler, addr, psm, mtu);
}

void l2cap_register_service(btstack_packet_handler_t packet_handler,
                                     u16 psm, u16 mtu)
{
    l2cap_register_service_internal(NULL, packet_handler, psm, mtu);
}

void l2cap_accept_connection(u16 local_cid)
{
    l2cap_accept_connection_internal(local_cid);
}

void l2cap_decline_connection(u16 local_cid, u8 reason)
{
    l2cap_decline_connection_internal(local_cid, reason);
}

int l2cap_send(u16 local_cid, u8 *data, u16 len)
{
    return l2cap_send_internal(local_cid, data, len, 0);
}

/* btstack.a 的弱函数（默认空实现）：收到对端 IO Capability Response 时调用。
 * 打印手柄的配对能力，用来确认 SSP 走的是哪种关联模型。
 */
void hci_event_io_capability_response_cb(u8 *addr, u8 io_cap, u8 oob_data, u8 auth_req)
{
    printf("[DS5_BT] remote IO capability: io_cap=%u oob=%u auth_req=0x%02x\n",
           io_cap, oob_data, auth_req);
    put_buf(addr, 6);
}

/* AC792 diagnostic tap: HCI ingress redirected by the diagnostic build rule.
 * This is the library HCI ingress, not the application's HID callback.
 * Never modify/drop packets or interpret controller-private event layouts.
 */
extern void l2cap_packet_handler(u8 type, u8 *packet, u16 size);

static u16 ds5_diag_le16(const u8 *p)
{
    return (u16)p[0] | ((u16)p[1] << 8);
}

static void ds5_diag_signaling(const char *direction, const u8 *packet, u16 size)
{
    if (size < 8) {
        return;
    }
    u16 handle_flags = ds5_diag_le16(packet);
    u16 acl_len = ds5_diag_le16(packet + 2);
    /* Continuations have no L2CAP header. Only decode complete PDUs here. */
    if (((handle_flags >> 12) & 3) == 1 ||
        acl_len < 4 || acl_len > size - 4 ||
        ds5_diag_le16(packet + 6) != 1) {
        return;
    }
    u16 payload_len = ds5_diag_le16(packet + 4);
    if (payload_len > acl_len - 4) {
        printf("[DS5_L2CAP_%s] fragmented signaling size=%u pdu=%u\n",
               direction, size, payload_len);
        return;
    }
    u16 end = 8 + payload_len;
    for (u16 pos = 8; pos + 4 <= end;) {
        const u8 *cmd = packet + pos;
        u16 len = ds5_diag_le16(cmd + 2);
        if (len > end - pos - 4) {
            printf("[DS5_L2CAP_%s] truncated command code=0x%02x len=%u\n",
                   direction, cmd[0], len);
            break;
        }
        printf("[DS5_L2CAP_%s] handle=0x%03x code=0x%02x id=%u len=%u\n",
               direction, handle_flags & 0xfff, cmd[0], cmd[1], len);
        if (cmd[0] == 3 && len >= 8) {
            printf("[DS5_L2CAP_%s] CONN_RSP dcid=0x%04x scid=0x%04x result=0x%04x status=0x%04x\n",
                   direction, ds5_diag_le16(cmd + 4), ds5_diag_le16(cmd + 6),
                   ds5_diag_le16(cmd + 8), ds5_diag_le16(cmd + 10));
        } else if (cmd[0] == 2 && len >= 4) {
            printf("[DS5_L2CAP_%s] CONN_REQ psm=0x%04x scid=0x%04x\n",
                   direction, ds5_diag_le16(cmd + 4), ds5_diag_le16(cmd + 6));
        }
        /* Bounded dump, only signaling: no HID reports or pairing keys. */
        put_buf(cmd, len > 16 ? 20 : len + 4);
        pos += len + 4;
    }
}

void ds5_diag_l2cap_packet_handler(u8 type, u8 *packet, u16 size)
{
    static u8 announced;
    if (!announced) {
        announced = 1;
        printf("[DS5_L2CAP_RX] diagnostic tap active v4\n");
    }
    if (type == 2 && packet) {
        ds5_diag_signaling("RX", packet, size);
    }
    l2cap_packet_handler(type, packet, size);
}

/* Called before hci_vendor consumes authentication/encryption events.
 * This internal ABI supplies a complete HCI event, with length at byte 1.
 */
void ds5_diag_hci_event(const u8 *packet)
{
    if (!packet) {
        return;
    }
    if (packet[0] == 8 && packet[1] >= 4) {
        printf("[DS5_HCI] ENCRYPTION_CHANGE status=0x%02x handle=0x%04x enabled=%u\n",
               packet[2], ds5_diag_le16(packet + 3), packet[5]);
    } else if (packet[0] == 6 && packet[1] >= 3) {
        printf("[DS5_HCI] AUTH_COMPLETE status=0x%02x handle=0x%04x\n",
               packet[2], ds5_diag_le16(packet + 3));
    } else if ((packet[0] == 0x12 || packet[0] == 0x36) && packet[1] >= 1) {
        printf("[DS5_HCI] event=0x%02x status=0x%02x\n", packet[0], packet[2]);
    } else if (packet[0] == 0xF7 && packet[1] >= 1) {
        /* 0xF7 is the controller's vendor encryption notification. It may
         * occur more than once during SSP/link-key authentication. */
        printf("[DS5_HCI] VENDOR_ENCRY_COMPLETE status=0x%02x\n", packet[2]);
    }
}

void ds5_diag_acl_tx(const u8 *packet, int size)
{
    if (packet && size >= 8 && size <= 65535) {
        ds5_diag_signaling("TX", packet, (u16)size);
    }
}
