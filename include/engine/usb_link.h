# pragma once
# ifndef ENGINE_USB_LINK
# define ENGINE_USB_LINK

/*
 * USB for the Pico engine: two serial ports on one cable.
 *   port 0: bci_link frames to and from a client (presenter or collector)
 *   port 1: printf text, for a terminal
 * Opening either port at 1200 baud reboots into BOOTSEL (for flashing).
 */

# include <stdbool.h>
# include <stdint.h>
# include "serial/bci_link.h"

void initializeUsbLink(void);   // also routes printf to the debug port
void updateUsbLink(void);       // call often: runs TinyUSB, flushes output

bool isUsbLinkOpen(void);       // a client has the link port open
bool sendUsbLinkFrame(const uint8_t frame[BCI_LINK_FRAME_SIZE]); // false if no room
bool tryReceiveUsbLinkMessage(BCILinkMessage *message);

# endif