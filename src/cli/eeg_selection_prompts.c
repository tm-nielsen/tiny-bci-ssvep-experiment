# include "cli/eeg_source_selection.h"
# include "cli/helpers.h"
# include "serial/port_enumeration.h"

EEGSourceType promptEEGSourceSelection()
{
    printf("Select EEG Source:\n");
    printf("\t%u - LSL Stream\n", LSL_SOURCE);
    printf("\t%u - Neuropawn over USB\n", NEUROPAWN_SOURCE);
    printf("\t%u - Unicorn over USB\n", UNICORN_SOURCE);
    printf("\t%u - DSI-7 over USB\n", DSI7_SOURCE);
    printf("\t%u - Synthetic test data\n", SYNTHETIC_SOURCE);

    return (EEGSourceType)getCLIIntegerSelection(SYNTHETIC_SOURCE);
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
        printf("Select one of options [0-%u]\t", deviceCount);
        uint32_t selection = getCLIIntegerSelection(deviceCount);

        if (selection == 0) continue;
        return getSerialPortName(selection - 1);
    }
}

bool promptEEGOutletUsageSelection()
{
    printf("Would you like to stream the raw EEG to LSL for recording?\n");
    return getCLIYesNo();
}