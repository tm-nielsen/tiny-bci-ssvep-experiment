# pragma once
# ifndef ENGINE_LINK_HUB
# define ENGINE_LINK_HUB

/*
 * One place to send and receive bci_link messages, whatever the transport.
 * Today: USB. The Wi-Fi TCP server plugs in here later without touching main.
 */

# include <stdbool.h>
# include <stdint.h>
# include "serial/bci_link.h"

void initializeLinkHub(void);
void updateLinkHub(void);                        // run transports; call often

void broadcastLinkMessage(BCILinkMessage message); // stamps seq, sends to all clients
bool tryReceiveLinkMessage(BCILinkMessage *message);

uint8_t getLinkClientCount(void);
uint32_t getLinkFramesDropped(void);

# endif