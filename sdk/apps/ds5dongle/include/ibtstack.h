#ifndef IBTSTACK_H
#define IBTSTACK_H

#include "btstack/btstack_typedef.h"

#define PSM_HID_CONTROL                      0x0011
#define PSM_HID_INTERRUPT                    0x0013

#ifndef L2CAP_DATA_PACKET
#define L2CAP_DATA_PACKET                    0x06
#endif
#ifndef L2CAP_EVENT_CHANNEL_OPENED
#define L2CAP_EVENT_CHANNEL_OPENED           0x70
#endif
#ifndef L2CAP_EVENT_CHANNEL_CLOSED
#define L2CAP_EVENT_CHANNEL_CLOSED           0x71
#endif
#ifndef L2CAP_EVENT_INCOMING_CONNECTION
#define L2CAP_EVENT_INCOMING_CONNECTION      0x72
#endif
#ifndef L2CAP_EVENT_CREDITS
/* Jieli Classic L2CAP send-ready notification.  The last byte contains the
 * controller credit indication; local_cid is stored at offsets 2..3. */
#define L2CAP_EVENT_CREDITS                  0x74
#endif

#define L2CAP_CONNECTION_REFUSED_RESOURCES   0x04

u16 l2cap_create_channel(btstack_packet_handler_t packet_handler,
                                  bd_addr_t addr, u16 psm, u16 mtu);
void l2cap_register_service(btstack_packet_handler_t packet_handler,
                                     u16 psm, u16 mtu);
void l2cap_accept_connection(u16 local_cid);
void l2cap_decline_connection(u16 local_cid, u8 reason);
u16 l2cap_get_remote_mtu_for_local_cid(u16 local_cid);

/* Classic Basic Mode adapter.  The SDK's exported l2cap_send is an LE
 * credit-based API.  Keep the source-level API while using a distinct link
 * symbol, then let ibtstack.c forward it to the internal Classic ABI. */
int l2cap_send(u16 local_cid, u8 *data, u16 len)
        __asm__("ds5_classic_l2cap_send");
/* AC792 btstack_task.c: immediate=1 posts the Classic credit-dispatch
 * message. stack_run_loop_resume() only runs registered profile callbacks,
 * so it cannot kick an application queue consumed by L2CAP_EVENT_CREDITS.
 * Dispatch is asynchronous and sends from the btstack task. */
void stack_handout_credit(u8 immediate);

#endif
