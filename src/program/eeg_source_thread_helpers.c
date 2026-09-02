# include "program/helpers.h"
# include "cli/eeg_source_selection.h"
# include "pipeline.h"
# include "tinycthread.h"

static thrd_t thread;
static bool exitThread = false;

static int updateSelectedEEGSourceInThread(void *data)
{
    while (!exitThread)
    {
        if (!isSelectedEEGSourceConnected()) return EXIT_FAILURE;

        updateSelectedEEGSource();
    }
    return EXIT_SUCCESS;
}

void startUpdateThreadForSelectedEEGSource(void)
{
    initializeSignalBufferMutex();
    thrd_create(&thread, updateSelectedEEGSourceInThread, NULL);
}

void cleanUpEEGSourceUpdateThread(void)
{
    exitThread = true;
    int threadResult = 0;
    thrd_join(thread, &threadResult);
    cleanUpSignalBufferMutex();
}