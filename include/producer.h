# pragma once
# include "storage.h"
# include "synthetic_producer.h"

SyntheticProducerConfig producerConfiguration;
SyntheticProducer syntheticProducer;

TBCI_TriggerGeneratorConfig triggerGeneratorConfiguration;
TBCI_TriggerGeneratorState triggerGeneratorState;
TBCI_TriggerGenerator triggerGenerator;


int initializeTinyBCIProducer(bool);
int updateTinyBCIProducer();
int closeTinyBCIProducer();