# include "cli/recording_options.h"
# include "cli/helpers.h"
# include "pipeline/storage.h"

static bool dataStreamingIsDesired;

static bool promptDataStreamingSelection(void)
{
    printf("Would you like to record EEG and Triggers from LSL streams?\n");
    return getCLIYesNo();
}

void runRecordingOptionSelection(void)
{
    printf("Enter subject identifier [P08]: ");
    scanf("%s", tbciConfiguration.log_subject);
    printf("Enter session identifier [S04]: ");
    scanf("%s", tbciConfiguration.log_session);

    dataStreamingIsDesired = promptDataStreamingSelection();
    if (dataStreamingIsDesired)
    {
        printf(
            "EEG and Trigger streams will be created"
            " if they do not already exist.\n"
        );
    }
}

bool shouldStreamEEGAndTriggers(void)
{
    return dataStreamingIsDesired;
}