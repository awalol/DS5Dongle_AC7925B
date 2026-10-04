#include "os/os_api.h"
#include "usb/device/usb_stack.h"
#include "usb/device/hid.h"
#include "usb_config.h"
#include "app_config.h"
#include "circular_buf.h"
#include "usb/device/custom_hid.h"

#if TCFG_USB_CUSTOM_HID_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE

#include "debug.h"

#define USB_HID_EP_DMA_BUFFER_KEEP_ENABLE  0

typedef void (*hid_rx_handle_t)(const usb_dev usb_id, void *hdl, u8 *buffer, u32 len);
typedef void (*hid_get_report_handle_t)(const usb_dev usb_id, void *hdl, u8 *buffer, u32 len);
typedef void (*hid_set_report_handle_t)(const usb_dev usb_id, void *hdl, u8 *buffer, u32 len);
struct custom_hid_hdl {
    u8 cfg_done;
    void *priv_hdl;
    hid_rx_handle_t hid_rx_hook;

    void *get_report_priv;
    hid_get_report_handle_t get_report_hook;
    void *set_report_priv;
    hid_set_report_handle_t set_report_hook;
};
static struct custom_hid_hdl *custom_hid_info[USB_MAX_HW_NUM];
#if USB_MALLOC_ENABLE
#else
static struct custom_hid_hdl _custom_hid_info;
#endif


static const u8 sHIDDescriptor[] = {
//HID
    //InterfaceDeszcriptor:
    USB_DT_INTERFACE_SIZE,     // Length
    USB_DT_INTERFACE,          // DescriptorType
    0x00,                       // bInterface number
    0x00,                      // AlternateSetting
    0x02,                      // NumEndpoint
    USB_CLASS_HID,             // Class = Human Interface Device
    0x00,                      // Subclass, 0 No subclass, 1 Boot Interface subclass
    0x00,                      // Procotol, 0 None, 1 Keyboard, 2 Mouse
    0x00,                      // Interface Name

    //HIDDescriptor:
    0x09,                      // bLength
    USB_HID_DT_HID,            // bDescriptorType, HID Descriptor
    0x01, 0x02,                // bcdHID, HID Class Specification release NO.
    0x00,                      // bCuntryCode, Country localization (=none)
    0x01,                       // bNumDescriptors, Number of descriptors to follow
    0x22,                       // bDescriptorType, Report Desc. 0x22, Physical Desc. 0x23
    0, 0,                       // wDescriptorLength

    //EndpointDescriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_IN | CUSTOM_HID_EP_IN,     // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_CUSTOM_HIDIN), HIBYTE(MAXP_SIZE_CUSTOM_HIDIN),// Maximum packet size
    HID_INTR_INTERVAL,                       // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms

    //Endpoint Descriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    CUSTOM_HID_EP_OUT,   // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_CUSTOM_HIDOUT), HIBYTE(MAXP_SIZE_CUSTOM_HIDOUT),// Maximum packet size
    HID_INTR_INTERVAL,                       // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms
};

static const u8 sDS5HIDReportDesc[] = {
    USAGE_PAGE(1, 0x01),        // Usage Page: Generic Desktop
    USAGE(1, 0x05),             // Usage: Game Pad
    COLLECTION(1, APPLICATION), // Collection: Application

    REPORT_ID(1, 0x01),          // Report ID: 0x01
    USAGE(1, 0x30),              // Usage: X
    USAGE(1, 0x31),              // Usage: Y
    USAGE(1, 0x32),              // Usage: Z
    USAGE(1, 0x35),              // Usage: Rz
    USAGE(1, 0x33),              // Usage: Rx
    USAGE(1, 0x34),              // Usage: Ry
    LOGICAL_MIN(1, 0x00),        // Logical Min: 0
    LOGICAL_MAX(2, 0xff, 0x00),  // Logical Max: 255
    REPORT_SIZE(1, 0x08),        // Report Size: 8 bits
    REPORT_COUNT(1, 0x06),       // Report Count: 6
    INPUT(1, 0x02),              // Input: Data, Variable, Absolute
    USAGE_PAGE(2, 0x00, 0xff),   // Usage Page: Vendor Defined 0xff00
    USAGE(1, 0x20),              // Usage: Vendor Defined 0x20
    REPORT_COUNT(1, 0x01),       // Report Count: 1
    INPUT(1, 0x02),              // Input: Data, Variable, Absolute
    USAGE_PAGE(1, 0x01),         // Usage Page: Generic Desktop
    USAGE(1, 0x39),              // Usage: Hat Switch
    LOGICAL_MIN(1, 0x00),        // Logical Min: 0
    LOGICAL_MAX(1, 0x07),        // Logical Max: 7
    PHYSICAL_MIN(1, 0x00),       // Physical Min: 0
    PHYSICAL_MAX(2, 0x3b, 0x01), // Physical Max: 315
    UNIT(1, 0x14),               // Unit: English Rotation, Degrees
    REPORT_SIZE(1, 0x04),        // Report Size: 4 bits
    REPORT_COUNT(1, 0x01),       // Report Count: 1
    INPUT(1, 0x42),              // Input: Data, Variable, Absolute, Null State
    UNIT(1, 0x00),               // Unit: None
    USAGE_PAGE(1, 0x09),         // Usage Page: Button
    USAGE_MIN(1, 0x01),          // Usage Min: 1
    USAGE_MAX(1, 0x0f),          // Usage Max: 15
    LOGICAL_MIN(1, 0x00),        // Logical Min: 0
    LOGICAL_MAX(1, 0x01),        // Logical Max: 1
    REPORT_SIZE(1, 0x01),        // Report Size: 1 bits
    REPORT_COUNT(1, 0x0f),       // Report Count: 15
    INPUT(1, 0x02),              // Input: Data, Variable, Absolute
    USAGE_PAGE(2, 0x00, 0xff),   // Usage Page: Vendor Defined 0xff00
    USAGE(1, 0x21),              // Usage: Vendor Defined 0x21
    REPORT_COUNT(1, 0x0d),       // Report Count: 13
    INPUT(1, 0x02),              // Input: Data, Variable, Absolute
    USAGE_PAGE(2, 0x00, 0xff),   // Usage Page: Vendor Defined 0xff00
    USAGE(1, 0x22),              // Usage: Vendor Defined 0x22
    LOGICAL_MIN(1, 0x00),        // Logical Min: 0
    LOGICAL_MAX(2, 0xff, 0x00),  // Logical Max: 255
    REPORT_SIZE(1, 0x08),        // Report Size: 8 bits
    REPORT_COUNT(1, 0x34),       // Report Count: 52
    INPUT(1, 0x02),              // Input: Data, Variable, Absolute

    REPORT_ID(1, 0x02),    // Report ID: 0x02
    USAGE(1, 0x23),        // Usage: Vendor Defined 0x23
    REPORT_COUNT(1, 0x2f), // Report Count: 47
    OUTPUT(1, 0x02),       // Output: Data, Variable, Absolute

    REPORT_ID(1, 0x05),    // Report ID: 0x05
    USAGE(1, 0x33),        // Usage: Vendor Defined 0x33
    REPORT_COUNT(1, 0x28), // Report Count: 40
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x08),    // Report ID: 0x08
    USAGE(1, 0x34),        // Usage: Vendor Defined 0x34
    REPORT_COUNT(1, 0x2f), // Report Count: 47
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x09),    // Report ID: 0x09
    USAGE(1, 0x24),        // Usage: Vendor Defined 0x24
    REPORT_COUNT(1, 0x13), // Report Count: 19
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x0a),    // Report ID: 0x0a
    USAGE(1, 0x25),        // Usage: Vendor Defined 0x25
    REPORT_COUNT(1, 0x1a), // Report Count: 26
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x0b),    // Report ID: 0x0b
    USAGE(1, 0x41),        // Usage: Vendor Defined 0x41
    REPORT_COUNT(1, 0x29), // Report Count: 41
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x0c),    // Report ID: 0x0c
    USAGE(1, 0x42),        // Usage: Vendor Defined 0x42
    REPORT_COUNT(1, 0x29), // Report Count: 41
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x20),    // Report ID: 0x20
    USAGE(1, 0x26),        // Usage: Vendor Defined 0x26
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x21),    // Report ID: 0x21
    USAGE(1, 0x27),        // Usage: Vendor Defined 0x27
    REPORT_COUNT(1, 0x04), // Report Count: 4
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x22),    // Report ID: 0x22
    USAGE(1, 0x40),        // Usage: Vendor Defined 0x40
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x80),    // Report ID: 0x80
    USAGE(1, 0x28),        // Usage: Vendor Defined 0x28
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x81),    // Report ID: 0x81
    USAGE(1, 0x29),        // Usage: Vendor Defined 0x29
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x82),    // Report ID: 0x82
    USAGE(1, 0x2a),        // Usage: Vendor Defined 0x2a
    REPORT_COUNT(1, 0x09), // Report Count: 9
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x83),    // Report ID: 0x83
    USAGE(1, 0x2b),        // Usage: Vendor Defined 0x2b
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x84),    // Report ID: 0x84
    USAGE(1, 0x2c),        // Usage: Vendor Defined 0x2c
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0x85),    // Report ID: 0x85
    USAGE(1, 0x2d),        // Usage: Vendor Defined 0x2d
    REPORT_COUNT(1, 0x02), // Report Count: 2
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0xa0),    // Report ID: 0xa0
    USAGE(1, 0x2e),        // Usage: Vendor Defined 0x2e
    REPORT_COUNT(1, 0x01), // Report Count: 1
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0xe0),    // Report ID: 0xe0
    USAGE(1, 0x2f),        // Usage: Vendor Defined 0x2f
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0xf0),    // Report ID: 0xf0
    USAGE(1, 0x30),        // Usage: Vendor Defined 0x30
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0xf1),    // Report ID: 0xf1
    USAGE(1, 0x31),        // Usage: Vendor Defined 0x31
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0xf2),    // Report ID: 0xf2
    USAGE(1, 0x32),        // Usage: Vendor Defined 0x32
    REPORT_COUNT(1, 0x0f), // Report Count: 15
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0xf4),    // Report ID: 0xf4
    USAGE(1, 0x35),        // Usage: Vendor Defined 0x35
    REPORT_COUNT(1, 0x3f), // Report Count: 63
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute

    REPORT_ID(1, 0xf5),    // Report ID: 0xf5
    USAGE(1, 0x36),        // Usage: Vendor Defined 0x36
    REPORT_COUNT(1, 0x03), // Report Count: 3
    FEATURE(1, 0x02),      // Feature: Data, Variable, Absolute
    END_COLLECTION,        // End Collection
};

static u32 get_hid_report_desc_len(u32 index)
{
    u32 len = 0;
    len = sizeof(sDS5HIDReportDesc);
    return len;
}

static void *get_hid_report_desc(u32 index)
{
    u8 *ptr  = NULL;
    ptr = (u8 *)sDS5HIDReportDesc;
    return ptr;
}

static int custom_hid_is_ready(const usb_dev usb_id)
{
    if (custom_hid_info[usb_id] == NULL) {
        return 0;
    }

    /*
     * Some iOS hosts do not send HID SET_IDLE.  SET_CONFIGURATION has already
     * completed when the host type becomes HOST_TYPE_IOS, so the interrupt
     * endpoint configured by the USB reset handler is ready for use.  Keep the
     * same compatibility rule as the SDK's standard HID implementation.
     */
    return custom_hid_info[usb_id]->cfg_done ||
           usb_get_host_type(usb_id) == HOST_TYPE_IOS;
}

u32 custom_hid_tx_data(const usb_dev usb_id, const u8 *buffer, u32 len)
{
    if (len > MAXP_SIZE_CUSTOM_HIDIN) {
        len = MAXP_SIZE_CUSTOM_HIDIN;
    }
    if (!custom_hid_is_ready(usb_id)) {
        return 0;
    }
    return usb_g_intr_write(usb_id, CUSTOM_HID_EP_IN, buffer, len);
}

static void custom_hid_rx_data(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 rx_buffer[MAXP_SIZE_CUSTOM_HIDOUT] = {0};
    u32 rx_len = usb_g_intr_read(usb_id, ep, rx_buffer, MAXP_SIZE_CUSTOM_HIDOUT, 0);
    if (custom_hid_info[usb_id] && custom_hid_info[usb_id]->hid_rx_hook) {
        custom_hid_info[usb_id]->hid_rx_hook(usb_id, custom_hid_info[usb_id]->priv_hdl, rx_buffer, rx_len);
    }
}

int custom_hid_get_ready(const usb_dev usb_id)
{
    return custom_hid_is_ready(usb_id);
}

void custom_hid_set_rx_hook(const usb_dev usb_id, void *priv, void (*rx_hook)(const usb_dev usb_id, void *priv, u8 *buf, u32 len))
{
    if (custom_hid_info[usb_id]) {
        custom_hid_info[usb_id]->priv_hdl = priv;
        custom_hid_info[usb_id]->hid_rx_hook = rx_hook;
    }
}

void custom_hid_set_get_report_hook(const usb_dev usb_id, void *priv, void (*get_report_hook)(const usb_dev usb_id, void *priv, u8 *buf, u32 len))
{
    if (custom_hid_info[usb_id]) {
        custom_hid_info[usb_id]->get_report_priv = priv;
        custom_hid_info[usb_id]->get_report_hook = get_report_hook;
    }
}

void custom_hid_set_set_report_hook(const usb_dev usb_id, void *priv, void (*set_report_hook)(const usb_dev usb_id, void *priv, u8 *buf, u32 len))
{
    if (custom_hid_info[usb_id]) {
        custom_hid_info[usb_id]->set_report_priv = priv;
        custom_hid_info[usb_id]->set_report_hook = set_report_hook;
    }
}

static u32 custom_hid_recv_output_report(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    u8 read_ep[setup->wLength];
    usb_read_ep0(usb_id, read_ep, MIN(sizeof(read_ep), setup->wLength));

    custom_hid_info[usb_id]->set_report_hook(usb_id,
        custom_hid_info[usb_id]->set_report_priv,
                                    read_ep,
                                    setup->wLength);

    return USB_EP0_STAGE_SETUP;
}


static u8 *custom_hid_ep_in_dma[USB_MAX_HW_NUM];
static u8 *custom_hid_ep_out_dma[USB_MAX_HW_NUM];

static void custom_hid_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    usb_g_ep_config(usb_id, CUSTOM_HID_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT, 0, custom_hid_ep_in_dma[usb_id], MAXP_SIZE_CUSTOM_HIDIN);
    usb_g_ep_config(usb_id, CUSTOM_HID_EP_OUT, USB_ENDPOINT_XFER_INT, 1, custom_hid_ep_out_dma[usb_id], MAXP_SIZE_CUSTOM_HIDOUT);
    usb_g_set_intr_hander(usb_id, CUSTOM_HID_EP_OUT, custom_hid_rx_data);
    usb_enable_ep(usb_id, CUSTOM_HID_EP_OUT);
}

static void custom_hid_reset(struct usb_device_t *usb_device, u32 itf)
{
    log_info("%s", __func__);
#if USB_ROOT2
    usb_disable_ep(usb_id, HID_EP_OUT);
#else
    custom_hid_endpoint_init(usb_device, itf);
#endif
}

static u32 custom_hid_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:
            switch (HIBYTE(req->wValue)) {
            case USB_HID_DT_HID:
                tx_payload = (u8 *)sHIDDescriptor + USB_DT_INTERFACE_SIZE;
                tx_len = 9;
                tx_payload = usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                tx_payload[7] = LOBYTE(get_hid_report_desc_len(req->wIndex));
                tx_payload[8] = HIBYTE(get_hid_report_desc_len(req->wIndex));
                break;
            case USB_HID_DT_REPORT:
                tx_len = get_hid_report_desc_len(req->wIndex);
                tx_payload = get_hid_report_desc(req->wIndex);
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                break;
            }// USB_REQ_GET_DESCRIPTOR
            break;
        case USB_REQ_SET_DESCRIPTOR:
            /* usb_set_setup_phase(usb_device, USB_EP0_SET_STALL); */
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_SET_INTERFACE:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                //只有一个interface 没有Alternate
                if (req->wValue == 0) { //alt 0
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                } else {
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
            }
            break;
        case USB_REQ_GET_INTERFACE:
            if (req->wValue || (req->wLength != 1)) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            }
            break;
        case USB_REQ_GET_STATUS:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        }//bRequest @ USB_TYPE_STANDARD
        break;

    case USB_TYPE_CLASS:
        switch (req->bRequest) {
        case USB_REQ_SET_IDLE:
            custom_hid_endpoint_init(usb_device, LOBYTE(req->wIndex));
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            custom_hid_info[usb_id]->cfg_done = 1;
            break;
        case USB_REQ_GET_REPORT:
            if (custom_hid_info[usb_id]->get_report_hook) {
                tx_len = req->wLength;
                memset(tx_payload, 0, tx_len);
                tx_payload[0] = LOBYTE(req->wValue);
                custom_hid_info[usb_id]->get_report_hook(usb_id,
                    custom_hid_info[usb_id]->get_report_priv,
                                                tx_payload,
                                                tx_len);
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            } else {
                log_error("custom hid GET_REPORT haven't Hook. STALL");
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        case USB_REQ_SET_REPORT:
            if (custom_hid_info[usb_id]->set_report_hook) {
                usb_set_setup_recv(usb_device, custom_hid_recv_output_report);
            } else {
                log_error("custom hid SET_REPORT haven't Hook. STALL");
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
        }//bRequest @ USB_TYPE_CLASS
        break;

    }
    return 0;
}

u32 custom_hid_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    log_info("custom hid interface num:%d", *cur_itf_num);

    memcpy(ptr, sHIDDescriptor, sizeof(sHIDDescriptor));

    ptr[2] = *cur_itf_num;
    ptr[USB_DT_INTERFACE_SIZE + 7] = LOBYTE(get_hid_report_desc_len(0));
    ptr[USB_DT_INTERFACE_SIZE + 8] = HIBYTE(get_hid_report_desc_len(0));

    if (usb_device->bSpeed == USB_SPEED_HIGH) {
        ptr[9 + 9 + 6] = HID_INTR_INTERVAL_HS;
        ptr[9 + 9 + 7 + 6] = HID_INTR_INTERVAL_HS;
    }

    if (usb_set_interface_hander(usb_id, *cur_itf_num, custom_hid_itf_hander) != *cur_itf_num) {
        ASSERT(0, "custom hid set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, custom_hid_reset) != *cur_itf_num) {
        ASSERT(0, "custom hid set interface_reset_hander fail");
    }

    *cur_itf_num = *cur_itf_num + 1;
    return sizeof(sHIDDescriptor);
}

u32 custom_hid_register(usb_dev usb_id)
{
    if (custom_hid_info[usb_id]) {
        return 0;
    }
#if USB_MALLOC_ENABLE
    custom_hid_info[usb_id] = malloc(sizeof(struct custom_hid_hdl));
    if (!custom_hid_info[usb_id]) {
        log_error("custom hid allocates memory fail 1\n");
        return -1;
    }
#else
    custom_hid_info[usb_id] = &_custom_hid_info;
#endif
    memset(custom_hid_info[usb_id], 0, sizeof(struct custom_hid_hdl));
    if (!custom_hid_ep_in_dma[usb_id]) {
        custom_hid_ep_in_dma[usb_id] = usb_alloc_ep_dmabuffer(usb_id, CUSTOM_HID_EP_IN | USB_DIR_IN, MAXP_SIZE_CUSTOM_HIDIN);
    }
    if (!custom_hid_ep_out_dma[usb_id]) {
        custom_hid_ep_out_dma[usb_id] = usb_alloc_ep_dmabuffer(usb_id, CUSTOM_HID_EP_OUT, MAXP_SIZE_CUSTOM_HIDOUT);
    }

    return 0;
}

void custom_hid_release(const usb_dev usb_id)
{
    if (custom_hid_info[usb_id] == NULL) {
        return;
    }
#if USB_MALLOC_ENABLE
#if !USB_HID_EP_DMA_BUFFER_KEEP_ENABLE
    if (custom_hid_ep_in_dma[usb_id]) {
        usb_free_ep_dmabuffer(usb_id, custom_hid_ep_in_dma[usb_id]);
        custom_hid_ep_in_dma[usb_id] = NULL;
    }
    if (custom_hid_ep_out_dma[usb_id]) {
        usb_free_ep_dmabuffer(usb_id, custom_hid_ep_out_dma[usb_id]);
        custom_hid_ep_out_dma[usb_id] = NULL;
    }
#endif
    free(custom_hid_info[usb_id]);
#else
    memset(custom_hid_info[usb_id], 0, sizeof(struct custom_hid_hdl));
#endif
    custom_hid_info[usb_id] = NULL;
}

#endif
