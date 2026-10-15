# pragma once
# ifndef ENGINE_LINK_OUTPUTS
# define ENGINE_LINK_OUTPUTS

/*
 * Turns pipeline output into bci_link messages: one EEG message per raw CSV
 * row, one INFERENCE message per inference CSV row. Same row contents as the
 * files the Pi writes, so receivers can produce identical CSVs.
 */

# include <stdint.h>
# include "pipeline.h"
# include "serial/bci_link.h"

// Call after initializeTinyBCIPipeline(): hooks the raw logger's frame tap.
void installEEGLinkTap(void);
uint32_t getEEGFramesSent(void);

void sendInferenceToLink(TinyBCIInference inference, uint64_t timestamp);
void sendTriggerEchoToLink(uint16_t code, uint64_t timestamp);
void sendStatusToLink(BCIEngineState state, uint32_t maxTickUs);

# endif