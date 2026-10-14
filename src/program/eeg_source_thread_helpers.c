# include "program/helpers.h"
# include "cli/eeg_source_selection.h"
# include "pipeline.h"
# include "tinycthread.h"

static thrd_t thread;
static bool exitThread = false;
static bool threadStarted = false;

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
    threadStarted = thrd_create(&thread, updateSelectedEEGSourceInThread, NULL) == thrd_success;
}

void cleanUpEEGSourceUpdateThread(void)
{
    if (threadStarted)
    {
        exitThread = true;
        int threadResult = 0;
        thrd_join(thread, &threadResult);
        threadStarted = false;
    }
    cleanUpSignalBufferMutex();
}