# include "engine/usb_link.h"

# include "pico/stdlib.h"
# include "pico/stdio/driver.h"
# include "pico/bootrom.h"
# include "tusb.h"

# define DEBUG_PORT 1
# define BOOTSEL_MAGIC_BAUD 1200

static const uint8_t linkPorts[USB_LINK_CLIENTS] = { 0, 2 };
static BCILinkParser parsers[USB_LINK_CLIENTS];

// --- printf -> debug port

static void debugOutChars(const char *buffer, int length)
{
    // Single-threaded firmware: write directly. Drop text nobody is reading,
    // and drop what doesn't fit rather than stall the pipeline.
    if (!tud_cdc_n_connected(DEBUG_PORT)) return;
    uint32_t room = tud_cdc_n_write_available(DEBUG_PORT);
    if ((uint32_t)length > room) length = (int)room;
    if (length > 0) tud_cdc_n_write(DEBUG_PORT, buffer, (uint32_t)length);
}

static void debugOutFlush(void)
{
    tud_cdc_n_write_flush(DEBUG_PORT);
}

static stdio_driver_t debugDriver = {
    .out_chars = debugOutChars,
    .out_flush = debugOutFlush,
# if PICO_STDIO_ENABLE_CRLF_SUPPORT
    .crlf_enabled = true,
# endif
};

// --- 1200 baud on any port: reboot into BOOTSEL (replaces the SDK's stdio_usb feature)

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *coding)
{
    (void)itf;
    if (coding->bit_rate == BOOTSEL_MAGIC_BAUD) rom_reset_usb_boot(0, 0);
}

// ---

void initializeUsbLink(void)
{
    for (uint8_t i = 0; i < USB_LINK_CLIENTS; i++) resetBCILinkParser(&parsers[i]);
    tud_init(0); // root hub port 0: the RP2350 has one USB controller
    stdio_set_driver_enabled(&debugDriver, true);
}

void updateUsbLink(void)
{
    tud_task();
    for (uint8_t i = 0; i < USB_LINK_CLIENTS; i++) tud_cdc_n_write_flush(linkPorts[i]);
    tud_cdc_n_write_flush(DEBUG_PORT);
}

bool isUsbLinkOpen(uint8_t client)
{
    return client < USB_LINK_CLIENTS && tud_cdc_n_connected(linkPorts[client]);
}

bool sendUsbLinkFrame(uint8_t client, const uint8_t frame[BCI_LINK_FRAME_SIZE])
{
    if (!isUsbLinkOpen(client)) return false;
    uint8_t port = linkPorts[client];
    if (tud_cdc_n_write_available(port) < BCI_LINK_FRAME_SIZE)
    {
        tud_task(); // give TinyUSB a chance to drain, then retry once
        tud_cdc_n_write_flush(port);
        if (tud_cdc_n_write_available(port) < BCI_LINK_FRAME_SIZE) return false;
    }
    tud_cdc_n_write(port, frame, BCI_LINK_FRAME_SIZE);
    return true;
}

bool tryReceiveUsbLinkMessage(uint8_t client, BCILinkMessage *message)
{
    if (client >= USB_LINK_CLIENTS) return false;
    uint8_t port = linkPorts[client];
    uint8_t byte;
    while (tud_cdc_n_available(port) > 0)
    {
        if (tud_cdc_n_read(port, &byte, 1) != 1) break;
        if (pushBCILinkParserByte(&parsers[client], byte, message)) return true;
    }
    return false;
}