# pragma once
# ifndef STORAGE
# define STORAGE

# include "tbci_context.h"
# include "nodes/preprocessing/tbci_bandpass_node.h"
# include "nodes/preprocessing/tbci_notch_node.h"
# include "nodes/features/tbci_cca_node.h"
# include "nodes/decoder/tbci_cca_model.h"

# include "data/eeg_source.h"

# define SIG_CAPACITY 1024
# define TRIG_CAPACITY 32
# define EPOCH_CAPACITY 8
# define PRE_MS 0
# define POST_MS 1000
# define TOTAL_FRAMES 256

# define EPOCH_POOL_CAPACITY EPOCH_CAPACITY * TOTAL_FRAMES * CHANNEL_COUNT

float signalStorage[];
uint64_t signalTimestamps[];
uint32_t signalIndices[];

float processedSignalStorage[];
uint64_t processedSignalTimestamps[];
uint32_t processedSignalIndices[];

TBCI_Trigger triggerStorage[];

TBCI_Epoch epochStorage[];
float epochPool[];
TBCI_Epoch featuresStorage[];
float featuresPool[];
TBCI_Epoch outputStorage[];
float outputPool[];

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
# define N_HARMONICS 2
# define N_COMPONENTS (N_HARMONICS * 2)
# define REF_CAP (N_FREQS * N_COMPONENTS * TOTAL_FRAMES)
float ref_signals[];

// Nodes
TBCI_NotchNode notchNode;
TBCI_NotchConfig notchConfiguration;
TBCI_BandpassNode bandpassNode;
TBCI_BandpassConfig bandpassConfiguration;
TBCI_CCANode ccaNode;
TBCI_CCAConfig ccaConfiguration;
TBCI_CCAModel ccaModel;
TBCI_CCAModelConfig ccaModelConfiguration;

#endif