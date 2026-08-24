# include "cli/eeg_source_selection.h"
# include "data/serial_port_enumeration.h"

EEGSourceType promptEEGSourceSelection()
{
    printf("Select EEG Source\n");
    printf("\t%u - LSL Stream\n", LSLSource);
    printf("\t%u - Neuropawn over USB\n", NeuropawnSource);
    printf("\t%u - Unicorn over USB\n", UnicornSource);
    printf("\t%u - DSI-7 over USB\n", DSI7Source);
    printf("\t%u - Synthetic test data\n", SyntheticSource);

    unsigned int selection = 0;
    scanf("%u", &selection);

    while (selection > SyntheticSource)
    {
        printf("invalid selection\n");
        scanf("%u", &selection);
    }

    return (EEGSourceType)selection;
}

const char* promptSerialPortSelection()
{
    while (true)
    {
        uint32_t deviceCount = enumerateSerialPorts();
        printf("%u Available Ports:\n", deviceCount);
        printf("\t0 : rescan\n");

        for (uint32_t i = 0; i < deviceCount; i++)
        {
            printf("\t%u : %s\n", i + 1, getSerialPortName(i));
        }
        printf("Selection one of options [0-%u]\t", deviceCount + 1);

        uint32_t selection;
        scanf("%u", &selection);

        if (selection == 0) continue;
        if (selection > deviceCount)
        {
            printf("Invalid selection\n Try Again\n");
            continue;
        }
        return getSerialPortName(selection - 1);
    }
}