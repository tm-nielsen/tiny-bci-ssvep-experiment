# include "lsl/eeg_outlet.h"
# include "lsl/helpers.h"
# include "lsl_c.h"
# include "pipeline/storage.h"

static lsl_outlet outlet = NULL;

static void onFrame(
    const float *samples, size_t n_samples,
    const TBCI_Frame *frame, uint16_t trigger_val,
    void *user_data
) {
    if (outlet == NULL)
    {
        fprintf(stderr, "Error: EEG outlet is NULL\n");
        return;
    }
    lsl_push_sample_f(outlet, samples);
}

void createAndConnectPipelineEEGOutlet()
{
    lsl_streaminfo streamInfo = lsl_create_streaminfo(
        LSL_EEG_PIPE_STREAM_NAME, LSL_EEG_PIPE_STREAM_TYPE,
        (int32_t)tbciContext.config.n_channels,
        tbciContext.config.nominal_srate,
        cft_float32, LSL_EEG_PIPE_SOURCE_ID
    );
    outlet = lsl_create_outlet(streamInfo, 0, 360);
    lsl_destroy_streaminfo(streamInfo);

    if (outlet == NULL)
    {
        fprintf(stderr, "Failed to open LSL outlet ");
        fprintf(stderr, "'%s'\n", LSL_EEG_PIPE_STREAM_NAME);
        exit(EXIT_SUCCESS);
    }

    tbciContext.core_node.raw_out.on_frame = &onFrame;
}

void closePipelineEEGOutlet()
{
    tbciContext.core_node.raw_out.on_frame = NULL;
    closeLslOutlet(&outlet);
}