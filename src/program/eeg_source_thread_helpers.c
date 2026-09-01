# include "program/helpers.h"
# include "cli/eeg_source_selection.h"
# include "tinycthread.h"

static mtx_t mutex;
static thrd_t thread;
static bool exitThread = false;

static int updateSelectedEEGSourceInThread(void *data)
{
    while (!exitThread)
    {
        mtx_lock(&mutex);
        updateSelectedEEGSource();
        mtx_unlock(&mutex);
    }
    return 0;
}

void startUpdateThreadForSelectedEEGSource(void)
{
    mtx_init(&mutex, mtx_plain);
    thrd_create(&thread, updateSelectedEEGSourceInThread, NULL);
}

void cleanUpEEGSourceUpdateThread(void)
{
    exitThread = true;
    int threadResult = 0;
    thrd_join(thread, &threadResult);
    mtx_destroy(&mutex);
}

void lockEEGSourceMutex(void) { mtx_lock(&mutex); }
void unlockEEGSourceMutex(void) { mtx_unlock(&mutex); }