# pragma once
# ifndef LSL_EEG_PIPE
# define LSL_EEG_PIPE

# define LSL_EEG_PIPE_STREAM_NAME "Tiny_BCI_Experiment_EEG_Redirect"
# define LSL_EEG_PIPE_STREAM_TYPE "EEG"
# define LSL_EEG_PIPE_SOURCE_ID "tiny_bci_ssvep_experiment"

void createAndConnectPipelineEEGOutlet(void);
void closePipelineEEGOutlet(void);

# endif