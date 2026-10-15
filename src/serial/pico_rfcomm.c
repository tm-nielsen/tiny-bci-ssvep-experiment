# ifdef TBCI_PICO

# include <stdint.h>
# include <stdbool.h>
# include <stddef.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include "serial/serial.h"
# include "pico/cyw43_arch.h"
# include "btstack.h"

# define CONNECT_TIMEOUT_MS   30000
# define INQUIRY_LENGTH       5       // units of 1.28 s
# define MAX_CANDIDATES       8
# define RX_BUFFER_SIZE       4096    // ~0.35 s of Unicorn data
# define PICO_SERIAL_HANDLE   1       // only one link at a time

typedef enum { LINK_CLOSED, LINK_OPENING, LINK_OPEN } LinkState;

static bool stackInitialized = false;
static bool stackReady = false;
static btstack_packet_callback_registration_t hciRegistration;
static void (*idleCallback)(void) = NULL;

static LinkState linkState = LINK_CLOSED;
static uint16_t rfcommCid = 0;

static uint8_t rxBuffer[RX_BUFFER_SIZE];
static uint32_t rxHead = 0;   // written by the RFCOMM callback
static uint32_t rxTail = 0;   // read by serialRead
static uint32_t rxOverflowBytes = 0;

static uint8_t *txData = NULL;
static uint16_t txLength = 0;
static bool txPending = false;

static uint32_t nowMs(void) { return to_ms_since_boot(get_absolute_time()); }

// Runs the Bluetooth stack once, plus whatever main needs during long waits.
static void pumpWhileWaiting(void)
{
    cyw43_arch_poll();
    if (idleCallback) idleCallback();
}

void setPicoSerialIdleCallback(void (*callback)(void)) { idleCallback = callback; }

typedef struct
{
    bd_addr_t address;
    uint8_t pageScanRepetitionMode;
    uint16_t clockOffset;
} Candidate;

static char wantedName[64];
static Candidate candidates[MAX_CANDIDATES];
static uint8_t candidateCount = 0;
static bool inquiryRunning = false;
static bool nameRequestRunning = false;
static bool deviceFound = false;
static bd_addr_t deviceAddress;
static bool addressCached = false;

static bool nameMatches(const char *name, size_t length)
{
    size_t expected = strlen(wantedName);
    return length >= expected && memcmp(name, wantedName, expected) == 0;
}

static void onInquiryResult(const uint8_t *packet)
{
    if (deviceFound) return;

    bd_addr_t address;
    gap_event_inquiry_result_get_bd_addr(packet, address);

    if (gap_event_inquiry_result_get_name_available(packet))
    {
        const char *name = (const char *)gap_event_inquiry_result_get_name(packet);
        if (nameMatches(name, gap_event_inquiry_result_get_name_len(packet)))
        {
            bd_addr_copy(deviceAddress, address);
            deviceFound = true;
            gap_inquiry_stop();
        }
        return;
    }

    // No name in the inquiry response: ask for it after the inquiry ends.
    if (candidateCount >= MAX_CANDIDATES) return;
    for (uint8_t i = 0; i < candidateCount; i++)
        if (bd_addr_cmp(candidates[i].address, address) == 0) return;

    Candidate *candidate = &candidates[candidateCount++];
    bd_addr_copy(candidate->address, address);
    candidate->pageScanRepetitionMode = gap_event_inquiry_result_get_page_scan_repetition_mode(packet);
    candidate->clockOffset = gap_event_inquiry_result_get_clock_offset(packet);
}

static void onRemoteNameComplete(const uint8_t *packet)
{
    nameRequestRunning = false;
    if (hci_event_remote_name_request_complete_get_status(packet) != ERROR_CODE_SUCCESS) return;

    const char *name = hci_event_remote_name_request_complete_get_remote_name(packet);
    if (nameMatches(name, strnlen(name, 248)))
    {
        hci_event_remote_name_request_complete_get_bd_addr(packet, deviceAddress);
        deviceFound = true;
    }
}

static void handleHciEvent(uint8_t packetType, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel; (void)size;
    if (packetType != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet))
    {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) stackReady = true;
            break;
        case GAP_EVENT_INQUIRY_RESULT:
            onInquiryResult(packet);
            break;
        case GAP_EVENT_INQUIRY_COMPLETE:
            inquiryRunning = false;
            break;
        case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
            onRemoteNameComplete(packet);
            break;
        case HCI_EVENT_PIN_CODE_REQUEST:
        {
            bd_addr_t address;
            hci_event_pin_code_request_get_bd_addr(packet, address);
            fprintf(stderr, "bluetooth: device asked for a PIN, rejecting\n");
            gap_pin_code_negative(address);
            break;
        }
        default:
            break;
    }
}

static bool findDevice(uint32_t deadline)
{
    deviceFound = false;
    while (!deviceFound && nowMs() < deadline)
    {
        candidateCount = 0;
        inquiryRunning = true;
        gap_inquiry_start(INQUIRY_LENGTH);
        while (inquiryRunning && nowMs() < deadline) pumpWhileWaiting();
        if (inquiryRunning) { gap_inquiry_stop(); return false; }

        for (uint8_t i = 0; i < candidateCount && !deviceFound && nowMs() < deadline; i++)
        {
            nameRequestRunning = true;
            gap_remote_name_request(
                candidates[i].address,
                candidates[i].pageScanRepetitionMode,
                candidates[i].clockOffset | 0x8000 // "clock offset valid" flag
            );
            while (nameRequestRunning && nowMs() < deadline) pumpWhileWaiting();
        }
    }
    return deviceFound;
}

static uint8_t serialPortChannel = 0;
static bool sdpRunning = false;

static void handleSdpEvent(uint8_t packetType, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel; (void)size;
    if (packetType != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet))
    {
        case SDP_EVENT_QUERY_RFCOMM_SERVICE:
            if (serialPortChannel == 0)
                serialPortChannel = sdp_event_query_rfcomm_service_get_rfcomm_channel(packet);
            break;
        case SDP_EVENT_QUERY_COMPLETE:
            sdpRunning = false;
            break;
        default:
            break;
    }
}

static void storeReceivedBytes(const uint8_t *data, uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        uint32_t next = (rxHead + 1) % RX_BUFFER_SIZE;
        if (next == rxTail) { rxOverflowBytes += size - i; return; }
        rxBuffer[rxHead] = data[i];
        rxHead = next;
    }
}

static void handleRfcommEvent(uint8_t packetType, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (packetType == RFCOMM_DATA_PACKET)
    {
        if (channel == rfcommCid) storeReceivedBytes(packet, size);
        return;
    }
    if (packetType != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet))
    {
        case RFCOMM_EVENT_CHANNEL_OPENED:
            if (rfcomm_event_channel_opened_get_status(packet) == ERROR_CODE_SUCCESS)
            {
                rfcommCid = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
                linkState = LINK_OPEN;
            }
            else linkState = LINK_CLOSED;
            break;

        case RFCOMM_EVENT_CAN_SEND_NOW:
            if (txPending)
            {
                rfcomm_send(rfcommCid, txData, txLength); // copies the data
                txPending = false;
            }
            break;

        case RFCOMM_EVENT_CHANNEL_CLOSED:
            linkState = LINK_CLOSED;
            rfcommCid = 0;
            txPending = false;
            break;

        default:
            break;
    }
}

static bool connectToDevice(uint32_t deadline)
{
    serialPortChannel = 0;
    sdpRunning = true;
    if (sdp_client_query_rfcomm_channel_and_name_for_service_class_uuid(
            handleSdpEvent, deviceAddress, BLUETOOTH_SERVICE_CLASS_SERIAL_PORT) != ERROR_CODE_SUCCESS)
        return false;
    while (sdpRunning && nowMs() < deadline) pumpWhileWaiting();
    if (serialPortChannel == 0) return false;

    linkState = LINK_OPENING;
    if (rfcomm_create_channel(handleRfcommEvent, deviceAddress, serialPortChannel, &rfcommCid) != ERROR_CODE_SUCCESS)
    {
        linkState = LINK_CLOSED;
        return false;
    }
    while (linkState == LINK_OPENING && nowMs() < deadline) pumpWhileWaiting();
    return linkState == LINK_OPEN;
}

// --- serial.h API

static bool startBluetoothStack(void)
{
    if (!stackInitialized)
    {
        l2cap_init();
        rfcomm_init();
        gap_set_local_name("TinyBCI Engine");
        gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
        gap_ssp_set_auto_accept(1);

        hciRegistration.callback = &handleHciEvent;
        hci_add_event_handler(&hciRegistration);
        hci_power_control(HCI_POWER_ON);
        stackInitialized = true;
    }

    uint32_t deadline = nowMs() + 5000;
    while (!stackReady && nowMs() < deadline) pumpWhileWaiting();
    if (!stackReady) fprintf(stderr, "bluetooth: controller did not start\n");
    return stackReady;
}

int serialOpen(SerialHandle *handle, const char *port, uint32_t readTimeout)
{
    (void)readTimeout; // reads are non-blocking, as on unix
    *handle = INVALID_HANDLE_VALUE;

    if (!startBluetoothStack()) return EXIT_FAILURE;
    if (linkState != LINK_CLOSED)
    {
        fprintf(stderr, "bluetooth: a link is already open\n");
        return EXIT_FAILURE;
    }

    uint32_t deadline = nowMs() + CONNECT_TIMEOUT_MS;
    rxHead = rxTail = 0;

    if (!addressCached || strcmp(wantedName, port) != 0)
    {
        snprintf(wantedName, sizeof(wantedName), "%s", port);
        printf("bluetooth: searching for '%s'...\n", wantedName);
        if (!findDevice(deadline))
        {
            fprintf(stderr, "bluetooth: '%s' not found\n", wantedName);
            return EXIT_FAILURE;
        }
        addressCached = true;
    }

    printf("bluetooth: connecting to %s...\n", bd_addr_to_str(deviceAddress));
    if (!connectToDevice(deadline))
    {
        fprintf(stderr, "bluetooth: connection to '%s' failed\n", wantedName);
        addressCached = false; // search again next time
        return EXIT_FAILURE;
    }

    *handle = PICO_SERIAL_HANDLE;
    return EXIT_SUCCESS;
}

int serialWaitReadable(SerialHandle *handle, uint32_t timeoutMs)
{
    if (isSerialHandleInvalid(handle)) return -1;

    uint32_t deadline = nowMs() + timeoutMs;
    while (rxTail == rxHead && linkState == LINK_OPEN && (int32_t)(deadline - nowMs()) > 0)
        pumpWhileWaiting();

    // >0 also when the link dropped, so serialRead reports the disconnect
    return (rxTail != rxHead || linkState != LINK_OPEN) ? 1 : 0;
}

int serialWrite(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    if (isSerialHandleInvalid(handle) || linkState != LINK_OPEN)
    {
        fprintf(stderr, "Error: Attempted to write to invalid serial handle\n");
        return 0;
    }

    uint16_t maxFrame = rfcomm_get_max_frame_size(rfcommCid);
    txData = buffer;
    txLength = (uint16_t)(bufferLength < maxFrame ? bufferLength : maxFrame);
    txPending = true;
    rfcomm_request_can_send_now_event(rfcommCid);

    // Block until sent, like write(): callers pass compound literals.
    while (txPending && linkState == LINK_OPEN) pumpWhileWaiting();
    return txPending ? 0 : txLength;
}

int serialRead(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    if (isSerialHandleInvalid(handle))
    {
        fprintf(stderr, "Error: Attempted to read from invalid serial handle\n");
        return 0;
    }

    cyw43_arch_poll(); // pull in anything the radio has received

    size_t count = 0;
    while (count < bufferLength && rxTail != rxHead)
    {
        buffer[count++] = rxBuffer[rxTail];
        rxTail = (rxTail + 1) % RX_BUFFER_SIZE;
    }

    if (count == 0 && linkState == LINK_CLOSED)
    {
        fprintf(stderr, "--\nSerial handle disconnected\n");
        *handle = INVALID_HANDLE_VALUE;
    }
    return (int)count;
}

void serialFlush(SerialHandle *handle)
{
    (void)handle;
    rxTail = rxHead;
}

void serialClose(SerialHandle *handle)
{
    if (linkState != LINK_CLOSED && rfcommCid != 0)
    {
        rfcomm_disconnect(rfcommCid);
        uint32_t deadline = nowMs() + 2000;
        while (linkState != LINK_CLOSED && nowMs() < deadline) pumpWhileWaiting();
    }
    *handle = INVALID_HANDLE_VALUE;
}

void sleepMilliseconds(uint32_t ms)
{
    uint32_t end = nowMs() + ms;
    while ((int32_t)(end - nowMs()) > 0) pumpWhileWaiting();
}

# endif // TBCI_PICO

