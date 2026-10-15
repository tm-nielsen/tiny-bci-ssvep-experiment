# ifndef BTSTACK_CONFIG_H
# define BTSTACK_CONFIG_H

// Bluetooth Classic only: an RFCOMM client for one headset.

# ifndef ENABLE_CLASSIC   // pico_btstack_classic already defines it
# define ENABLE_CLASSIC
# endif
# define ENABLE_LOG_ERROR
# define ENABLE_PRINTF_HEXDUMP

// Buffers
# define HCI_OUTGOING_PRE_BUFFER_SIZE 4
# define HCI_ACL_PAYLOAD_SIZE (1691 + 4)
# define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4

// One connection, one serial channel
# define MAX_NR_HCI_CONNECTIONS 1
# define MAX_NR_L2CAP_SERVICES 2
# define MAX_NR_L2CAP_CHANNELS 4
# define MAX_NR_RFCOMM_MULTIPLEXERS 1
# define MAX_NR_RFCOMM_SERVICES 1
# define MAX_NR_RFCOMM_CHANNELS 1
# define MAX_NR_SERVICE_RECORD_ITEMS 1
# define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 2
# define MAX_NR_CONTROLLER_ACL_BUFFERS 3
# define MAX_NR_CONTROLLER_SCO_PACKETS 3

// Link keys are kept in flash by the SDK
# define NVM_NUM_LINK_KEYS 4

// Flow control between the radio chip and BTstack
# define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL
# define HCI_HOST_ACL_PACKET_LEN (1691 + 4)
# define HCI_HOST_ACL_PACKET_NUM 3
# define HCI_HOST_SCO_PACKET_LEN 120
# define HCI_HOST_SCO_PACKET_NUM 3

# define HAVE_EMBEDDED_TIME_MS
# define HAVE_ASSERT
# define HCI_RESET_RESEND_TIMEOUT_MS 1000

# endif