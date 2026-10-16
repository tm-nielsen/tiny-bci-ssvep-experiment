# include "engine/link_hub.h"
# include "engine/usb_link.h"

static uint8_t nextSequence = 0;
static uint32_t framesDropped = 0;

void initializeLinkHub(void) { initializeUsbLink(); }
void updateLinkHub(void) { updateUsbLink(); }

void broadcastLinkMessage(BCILinkMessage message)
{
    message.sequence = nextSequence++;
    uint8_t frame[BCI_LINK_FRAME_SIZE];
    encodeBCILinkFrame(&message, frame);

    // A client that is connected but can't keep up loses frames; the seq gap
    // and the STATUS counter both show it. Wi-Fi clients join this loop later.
    for (uint8_t client = 0; client < USB_LINK_CLIENTS; client++)
    {
        if (isUsbLinkOpen(client) && !sendUsbLinkFrame(client, frame)) framesDropped++;
    }
}

bool tryReceiveLinkMessage(BCILinkMessage *message)
{
    for (uint8_t client = 0; client < USB_LINK_CLIENTS; client++)
    {
        if (tryReceiveUsbLinkMessage(client, message)) return true;
    }
    return false;
}

uint8_t getLinkClientCount(void)
{
    uint8_t count = 0;
    for (uint8_t client = 0; client < USB_LINK_CLIENTS; client++) count += isUsbLinkOpen(client);
    return count;
}

uint32_t getLinkFramesDropped(void) { return framesDropped; }