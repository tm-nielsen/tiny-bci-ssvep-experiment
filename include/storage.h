# pragma once
# ifndef STORAGE

# include "tbci_context.h"
# include "synthetic_producer.h"
# include "nodes/preprocessing/tbci_bandpass_node.h"
# include "nodes/preprocessing/tbci_notch_node.h"
# include "nodes/decoder/tbci_cca_node.h"
# include "nodes/decoder/tbci_cca_model.h"

# define N_CHANNELS 8
# define SIG_CAPACITY 1024
# define TRIG_CAPACITY 32
# define EPOCH_CAPACITY 8
# define SRATE 256.0f
# define PRE_MS 200
# define POST_MS 800
# define TOTAL_FRAMES 256
# define TRIGGER_CODE 1u
# define TRIGGER_INTERVAL_MS 1000u

# define EPOCH_POOL_CAPACITY EPOCH_CAPACITY * TOTAL_FRAMES * N_CHANNELS


static float signalStorage [SIG_CAPACITY * N_CHANNELS];
static uint64_t signalTimestamps[SIG_CAPACITY];
static uint32_t signalIndices   [SIG_CAPACITY];

static float processedSignalStorage [SIG_CAPACITY * N_CHANNELS];
static uint64_t processedSignalTimestamps[SIG_CAPACITY];
static uint32_t processedSignalIndices [SIG_CAPACITY];

static TBCI_Trigger triggerStorage [TRIG_CAPACITY];

static TBCI_Epoch epochStorage [EPOCH_CAPACITY];
static float epochPool [EPOCH_POOL_CAPACITY];
static TBCI_Epoch featuresStorage [EPOCH_CAPACITY];
static float featuresPool [EPOCH_POOL_CAPACITY];
static TBCI_Epoch outputStorage [EPOCH_CAPACITY];
static float outputPool [EPOCH_POOL_CAPACITY];

static TBCI_SignalBuffer signalBuffer;
static TBCI_SignalBuffer processedSignalBuffer;

static TBCI_TriggerQueue triggerQueue;
static TBCI_EpochQueue epochQueue;
static TBCI_EpochQueue featuresQueue;
static TBCI_EpochQueue outputQueue;

static TBCI_Input tbciInputs;
static TBCI_Config tbciConfiguration;
static TBCI_Context tbciContext;

// ---

static SyntheticProducerConfig producerConfiguration;
static SyntheticProducer syntheticProducer;

static TBCI_TriggerGeneratorConfig triggerGeneratorConfiguration;
static TBCI_TriggerGeneratorState triggerGeneratorState;
static TBCI_TriggerGenerator triggerGenerator;

TBCI_Producer *producer = (TBCI_Producer *)&syntheticProducer;


// CCA constants
#define N_FREQS 6
#define N_HARMONICS 2
#define N_COMPONENTS (N_HARMONICS * 2)
#define REF_CAP (N_FREQS * N_COMPONENTS * TOTAL_FRAMES)
static float ref_signals[REF_CAP];

// Nodes
static TBCI_NotchNode notchNode;
static TBCI_NotchConfig notchConfiguration;
static TBCI_BandpassNode bandpassNode;
static TBCI_BandpassConfig bandpassConfiguration;
static TBCI_CCANode ccaNode;
static TBCI_CCAConfig ccaConfiguration;
static TBCI_CCAModel ccaModel;
static TBCI_CCAModelConfig ccaModelConfiguration;

static int initializePipeline(bool sliding);

# endif