# pragma once
# include "storage.h"
# include "../thirdparty/tiny_bci/producer/unicorn_producer.h"

void initializeUnicornEEGSource(const char *port);
void resetUnicornEEGSource();
void updateUnicornEEGSource();
void closeUnicornEEGSource();