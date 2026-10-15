# include "engine/usb_link.h"

# include "pico/stdlib.h"
# include "pico/stdio/driver.h"
# include "pico/bootrom.h"
# include "tusb.h"

# define LINK_PORT  0
# define DEBUG_PORT 1
# define BOOTSEL_MAGIC_BAUD 1200

static BCILinkParser parser;

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

// --- 1200 baud on either port: reboot into BOOTSEL (replaces the SDK's stdio_usb feature)

void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *coding)
{
    (void)itf;
    if (coding->bit_rate == BOOTSEL_MAGIC_BAUD) rom_reset_usb_boot(0, 0);
}

// ---

void initializeUsbLink(void)
{
    resetBCILinkParser(&parser);
    tud_init(0); // root hub port 0: the RP2350 has one USB controller
    stdio_set_driver_enabled(&debugDriver, true);
}

void updateUsbLink(void)
{
    tud_task();
    tud_cdc_n_write_flush(LINK_PORT);
    tud_cdc_n_write_flush(DEBUG_PORT);
}

bool isUsbLinkOpen(void)
{
    return tud_cdc_n_connected(LINK_PORT);
}

bool sendUsbLinkFrame(const uint8_t frame[BCI_LINK_FRAME_SIZE])
{
    if (!isUsbLinkOpen()) return false;
    if (tud_cdc_n_write_available(LINK_PORT) < BCI_LINK_FRAME_SIZE)
    {
        tud_task(); // give TinyUSB a chance to drain, then retry once
        tud_cdc_n_write_flush(LINK_PORT);
        if (tud_cdc_n_write_available(LINK_PORT) < BCI_LINK_FRAME_SIZE) return false;
    }
    tud_cdc_n_write(LINK_PORT, frame, BCI_LINK_FRAME_SIZE);
    return true;
}

bool tryReceiveUsbLinkMessage(BCILinkMessage *message)
{
    uint8_t byte;
    while (tud_cdc_n_available(LINK_PORT) > 0)
    {
        if (tud_cdc_n_read(LINK_PORT, &byte, 1) != 1) break;
        if (pushBCILinkParserByte(&parser, byte, message)) return true;
    }
    return false;
}