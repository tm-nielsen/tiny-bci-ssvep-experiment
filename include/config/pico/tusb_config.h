# ifndef TUSB_CONFIG_H
# define TUSB_CONFIG_H

// Three USB serial ports: 0 = link A, 1 = debug text (printf), 2 = link B.
// Link A and B are identical; two lets a presenter and a collector connect at once.

# ifndef CFG_TUSB_RHPORT0_MODE
# define CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE
# endif

# define CFG_TUD_ENABLED 1
# define CFG_TUD_ENDPOINT0_SIZE 64

# define CFG_TUD_CDC 3
# define CFG_TUD_MSC 0
# define CFG_TUD_HID 0
# define CFG_TUD_MIDI 0
# define CFG_TUD_VENDOR 0

# define CFG_TUD_CDC_RX_BUFSIZE 1024
# define CFG_TUD_CDC_TX_BUFSIZE 4096   // ~280 ms of EEG frames if the host stalls
# define CFG_TUD_CDC_EP_BUFSIZE 64

# endif