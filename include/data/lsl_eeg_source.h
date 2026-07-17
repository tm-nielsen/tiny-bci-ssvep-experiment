# pragma once
# ifndef LSL_EEG_SOURCE
# define LSL_EEG_SOURCE

# define LSL_SCAN_TIMEOUT 1.0
# define LSL_CONNECT_TIMEOUT 2.0

void connectLslEEGSource();
void updateLslEEGSource();
void disconnectLslEEGSource();

# endif