#ifndef DSI_EEG_SOURCE_H
#define DSI_EEG_SOURCE_H

# include "DSI.h"

# define DSI_LIBRARY_PATH "./libDSI" DYLIB_EXTENSION
# define DSI_7_FLEX_MONTAGE "S2,S3,S4,S5,S6,S7"
# define DSI_7_MONTAGE "F4,C4,S3,S1,S2,C3,F3"

/* port:    serial port to connect on (e.g. "/dev/ttyACM0", "COM4"),
 *          or NULL to use the DSISerialPort environment variable.
 * montage: comma-separated electrode list, e.g. "P3,Pz,P4,O1,O2".
 *          Determines channelCount reported below. */
void connectDsiEEGSource(const char *port, const char *montage);
void updateDsiEEGSource(void);
void disconnectDsiEEGSource(void);

bool isDsiEEGSourceConnected(void);
uint8_t getDsiEEGSourceChannelCount(void);
uint32_t getDsiEEGSourceSampleRate(void);

bool isDsiLibraryAvailable(void);
 
#endif
