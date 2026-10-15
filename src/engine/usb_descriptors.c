/*
 * USB descriptors: one device, two serial (CDC-ACM) ports.
 *   port 0 "TinyBCI Link"  : binary bci_link frames
 *   port 1 "TinyBCI Debug" : printf text
 */

# include "tusb.h"
# include "pico/unique_id.h"

// Raspberry Pi vendor ID. The product ID is not officially allocated: fine for
// a lab prototype; request one (github.com/raspberrypi/usb-pid) before sharing.
# define USB_VENDOR_ID  0x2E8A
# define USB_PRODUCT_ID 0x10C1
# define USB_BCD_DEVICE 0x0100

enum
{
    ITF_LINK = 0, ITF_LINK_DATA,
    ITF_DEBUG,    ITF_DEBUG_DATA,
    ITF_TOTAL
};

# define EP_LINK_NOTIF  0x81
# define EP_LINK_OUT    0x02
# define EP_LINK_IN     0x82
# define EP_DEBUG_NOTIF 0x83
# define EP_DEBUG_OUT   0x04
# define EP_DEBUG_IN    0x84

# define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN)

enum { STR_LANG = 0, STR_MANUFACTURER, STR_PRODUCT, STR_SERIAL, STR_LINK, STR_DEBUG, STR_COUNT };

static const tusb_desc_device_t deviceDescriptor = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,       // IAD: required for composite CDC
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VENDOR_ID,
    .idProduct          = USB_PRODUCT_ID,
    .bcdDevice          = USB_BCD_DEVICE,
    .iManufacturer      = STR_MANUFACTURER,
    .iProduct           = STR_PRODUCT,
    .iSerialNumber      = STR_SERIAL,
    .bNumConfigurations = 1
};

static const uint8_t configurationDescriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 250),
    TUD_CDC_DESCRIPTOR(ITF_LINK,  STR_LINK,  EP_LINK_NOTIF,  8, EP_LINK_OUT,  EP_LINK_IN,  64),
    TUD_CDC_DESCRIPTOR(ITF_DEBUG, STR_DEBUG, EP_DEBUG_NOTIF, 8, EP_DEBUG_OUT, EP_DEBUG_IN, 64),
};

static const char *strings[STR_COUNT] = {
    [STR_MANUFACTURER] = "TinyBCI",
    [STR_PRODUCT]      = "TinyBCI Engine",
    [STR_SERIAL]       = NULL, // board unique ID, filled at runtime
    [STR_LINK]         = "TinyBCI Link",
    [STR_DEBUG]        = "TinyBCI Debug",
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&deviceDescriptor;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return configurationDescriptor;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    static uint16_t descriptor[33];
    uint8_t length = 0;

    if (index == STR_LANG)
    {
        descriptor[1] = 0x0409; // English (US)
        length = 1;
    }
    else
    {
        if (index >= STR_COUNT) return NULL;

        char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
        const char *text = strings[index];
        if (index == STR_SERIAL)
        {
            pico_get_unique_board_id_string(serial, sizeof(serial));
            text = serial;
        }
        while (text[length] && length < 32)
        {
            descriptor[1 + length] = (uint8_t)text[length];
            length++;
        }
    }

    descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * length + 2));
    return descriptor;
}