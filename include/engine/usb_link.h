# pragma once
# ifndef ENGINE_USB_LINK
# define ENGINE_USB_LINK

/*
 * USB for the Pico engine: three serial ports on one cable.
 *   port 0 "TinyBCI Link A": bci_link frames (e.g. the presenter)
 *   port 1 "TinyBCI Debug" : printf text, for a terminal
 *   port 2 "TinyBCI Link B": bci_link frames (e.g. the collector)
 * Both link ports are full clients: they receive every message and may send
 * triggers. Opening any port at 1200 baud reboots into BOOTSEL (for flashing).
 */

# include <stdbool.h>
# include <stdint.h>
# include "serial/bci_link.h"

# define USB_LINK_CLIENTS 2

void initializeUsbLink(void);   // also routes printf to the debug port
void updateUsbLink(void);       // call often: runs TinyUSB, flushes output

bool isUsbLinkOpen(uint8_t client);   // client 0 = Link A, 1 = Link B
bool sendUsbLinkFrame(uint8_t client, const uint8_t frame[BCI_LINK_FRAME_SIZE]); // false if no room
bool tryReceiveUsbLinkMessage(uint8_t client, BCILinkMessage *message);

# endif