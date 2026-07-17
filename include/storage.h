# pragma once
# ifndef STORAGE
# define STORAGE

# include "tbci_context.h"
# include "nodes/preprocessing/tbci_bandpass_node.h"
# include "nodes/preprocessing/tbci_notch_node.h"
# include "nodes/features/tbci_cca_node.h"
# include "nodes/decoder/tbci_cca_model.h"
# include "nodes/decoder/tbci_label_encoder_node.h"

# include "data/eeg_source.h"

# define SIG_CAPACITY 1024
# define TRIG_CAPACITY 32
# define EPOCH_CAPACITY 8

# define WINDOW_LENGTH_MS 4000
# define WINDOW_OVERLAP_MS (WINDOW_LENGTH_MS / 2)
# define TOTAL_FRAMES (size_t)(SAMPLE_RATE * WINDOW_LENGTH_MS / 1000)

# define EPOCH_POOL_CAPACITY EPOCH_CAPACITY * TOTAL_FRAMES * CHANNEL_COUNT

float signalStorage[SIG_CAPACITY * CHANNEL_COUNT];
uint64_t signalTimestamps[SIG_CAPACITY];
uint32_t signalIndices[SIG_CAPACITY];

float processedSignalStorage[SIG_CAPACITY * CHANNEL_COUNT];
uint64_t processedSignalTimestamps[SIG_CAPACITY];
uint32_t processedSignalIndices[SIG_CAPACITY];

TBCI_Trigger triggerStorage[TRIG_CAPACITY];

TBCI_Epoch epochStorage[EPOCH_CAPACITY];
float epochPool[EPOCH_POOL_CAPACITY];
TBCI_Epoch featuresStorage[EPOCH_CAPACITY];
float featuresPool[EPOCH_POOL_CAPACITY];
TBCI_Epoch outputStorage[EPOCH_CAPACITY];
float outputPool[EPOCH_POOL_CAPACITY];


TBCI_SignalBuffer signalBuffer;
TBCI_SignalBuffer processedSignalBuffer;

TBCI_TriggerQueue triggerQueue;
TBCI_EpochQueue epochQueue;
TBCI_EpochQueue featuresQueue;
TBCI_EpochQueue outputQueue;

TBCI_Input tbciInputs;
TBCI_Config tbciConfiguration;
TBCI_Context tbciContext;

// CCA constants
# define N_FREQS 6
# define N_HARMONICS 3
# define N_COMPONENTS (N_HARMONICS * 2)
# define REF_CAP (N_FREQS * N_COMPONENTS * TOTAL_FRAMES)
float ref_signals[REF_CAP];

// Nodes
TBCI_NotchNode notchNode;
TBCI_NotchConfig notchConfiguration;
TBCI_BandpassNode bandpassNode;
TBCI_BandpassConfig bandpassConfiguration;
TBCI_CCANode ccaNode;
TBCI_CCAConfig ccaConfiguration;
TBCI_CCAModel ccaModel;
TBCI_CCAModelConfig ccaModelConfiguration;
TBCI_LabelEncoderNode labelEncoderNode;
TBCI_LabelEncoderConfig labelEncoderConfiguration;

#endif