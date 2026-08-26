#ifndef DSI_EEG_SOURCE_H
#define DSI_EEG_SOURCE_H
 
# include <stdint.h>
# include <stdbool.h>

# define DSI_7_MONTAGE "F4,C4,S3,S1,S2,C3,F3"

/* port:    serial port to connect on (e.g. "/dev/ttyACM0", "COM4"),
 *          or NULL to use the DSISerialPort environment variable.
 * montage: comma-separated electrode list, e.g. "P3,Pz,P4,O1,O2".
 *          Determines channelCount reported below. */
void connectDsiEEGSource(const char *port, const char *montage);
void updateDsiEEGSource();
void disconnectDsiEEGSource();

bool isDsiEEGSourceConnected();
uint8_t getDsiEEGSourceChannelCount();
uint32_t getDsiEEGSourceSampleRate();
 
#endif
