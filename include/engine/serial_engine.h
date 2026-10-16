# pragma once
# ifndef SERIAL_ENGINE
# define SERIAL_ENGINE

/*
 * Presenter-side client for a TinyBCI engine on a serial port (the Pico over
 * USB). Mirrors what the LSL streams do in PRESENTATION_ONLY mode: send
 * triggers, receive inferences, know whether the engine is connected and ready.
 */

# include <stdbool.h>
# include <stdint.h>
# include "pipeline.h"
# include "serial/bci_link.h"

void runSerialEnginePortSelection(void);   // CLI prompt, before the window opens

bool openSerialEngine(void);               // opens the selected port, sends HELLO
void updateSerialEngine(void);             // read incoming frames; call every frame
void closeSerialEngine(void);

bool isSerialEngineConnected(void);        // port open and engine heard recently
bool isSerialEngineReady(void);            // engine reports READY
BCIEngineState getSerialEngineState(void);
const char *getSerialEngineStateName(void);

void pushSerialEngineTrigger(uint16_t code);
bool pollSerialEngineInference(TinyBCIInference *inference, uint64_t *engineTimestampUs);

# endif