# include "storage.h"

float signalStorageArray[SIG_CAPACITY * CHANNEL_COUNT];
uint64_t signalTimestampsArray[SIG_CAPACITY];
uint32_t signalIndicesArray[SIG_CAPACITY];

float processedSignalStorageArray[SIG_CAPACITY * CHANNEL_COUNT];
uint64_t processedSignalTimestampsArray[SIG_CAPACITY];
uint32_t processedSignalIndicesArray[SIG_CAPACITY];

TBCI_Trigger triggerStorageArray[TRIG_CAPACITY];

TBCI_Epoch epochStorageArray[EPOCH_CAPACITY];
float epochPoolArray[EPOCH_POOL_CAPACITY];
TBCI_Epoch featuresStorageArray[EPOCH_CAPACITY];
float featuresPoolArray[EPOCH_POOL_CAPACITY];
TBCI_Epoch outputStorageArray[EPOCH_CAPACITY];
float outputPoolArray[EPOCH_POOL_CAPACITY];

float refSignalsArray[REF_CAP];


float *signalStorage = signalStorageArray;
uint64_t *signalTimestamps = signalTimestampsArray;
uint32_t *signalIndices = signalIndicesArray;

float *processedSignalStorage = processedSignalStorageArray;
uint64_t *processedSignalTimestamps = processedSignalTimestampsArray;
uint32_t *processedSignalIndices = processedSignalIndicesArray;

TBCI_Trigger *triggerStorage = triggerStorageArray;

TBCI_Epoch *epochStorage = epochStorageArray;
float *epochPool = epochPoolArray;
TBCI_Epoch *featuresStorage = featuresStorageArray;
float *featuresPool = featuresPoolArray;
TBCI_Epoch *outputStorage = outputStorageArray;
float *outputPool = outputPoolArray;

TBCI_SignalBuffer signalBuffer;
TBCI_SignalBuffer processedSignalBuffer;

TBCI_TriggerQueue triggerQueue;
TBCI_EpochQueue epochQueue;
TBCI_EpochQueue featuresQueue;
TBCI_EpochQueue outputQueue;

TBCI_Input tbciInputs;
TBCI_Config tbciConfiguration;
TBCI_Context tbciContext;

float *refSignals = refSignalsArray;


TBCI_NotchNode notchNode;
TBCI_NotchConfig notchConfiguration;
TBCI_BandpassNode bandpassNode;
TBCI_BandpassConfig bandpassConfiguration;
TBCI_CCANode ccaNode;
TBCI_CCAConfig ccaConfiguration;
TBCI_CCAModel ccaModel;
TBCI_CCAModelConfig ccaModelConfiguration;