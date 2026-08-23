# include "lsl/helpers.h"
# include "lsl/constants.h"

lsl_inlet connectLslInlet(const char* predicate)
{
    lsl_streaminfo scanResults[2];

    int resultCount = lsl_resolve_bypred
    (
        scanResults, 2, predicate,
        1, LSL_SCAN_TIMEOUT
    );

    if (resultCount < 1)
    {
        fprintf(stderr, "Failed to locate LSL Source ");
        fprintf(stderr, "matching '%s'\n", predicate);
        return NULL;
    }
    else if (resultCount > 1)
    {
        fprintf(stderr, "Cannot choose between 2 or more ");
        fprintf(stderr, "LSL streams matching '%s'\n", predicate);
        return NULL;
    }

    lsl_streaminfo targetStream = scanResults[0];
    lsl_inlet inlet = lsl_create_inlet(targetStream, 360, LSL_NO_PREFERENCE, 1);

    for (int i = 0; i < resultCount; i++)
    {
        lsl_destroy_streaminfo(scanResults[i]);
    }

    if (inlet == NULL)
    {
        fprintf(stderr, "Failed to create LSL inlet\n");
        return NULL;
    }

    int32_t openError = 0;
    lsl_open_stream(inlet, LSL_CONNECT_TIMEOUT, &openError);

    if (openError != lsl_no_error)
    {
        lsl_destroy_inlet(inlet);
        fprintf(stderr, "Failed to connect to LSL stream\n");
        return NULL;
    }

    return inlet;
}

void closeLslInlet(lsl_inlet *inlet)
{
    if (*inlet == NULL) return;

    lsl_close_stream(*inlet);
    lsl_destroy_inlet(*inlet);
    *inlet = NULL;
}