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
    // and the STATUS counter both show it.
    if (isUsbLinkOpen() && !sendUsbLinkFrame(frame)) framesDropped++;
}

bool tryReceiveLinkMessage(BCILinkMessage *message)
{
    return tryReceiveUsbLinkMessage(message);
}

uint8_t getLinkClientCount(void) { return isUsbLinkOpen() ? 1 : 0; }
uint32_t getLinkFramesDropped(void) { return framesDropped; }