# pragma once
# ifndef PROGRAM_CONSTANTS
# define PROGRAM_CONSTANTS

# ifndef FREQUENCIES
#   define FREQUENCIES { 4.5f, 5.0f, 5.5f, 6.0f, 6.5f, 7.0f }
# endif
# define FILTER_STABILIZATION_DELAY 5.0f

# define TRIAL_COUNT 4
# define STIMULUS_DURATION 6.0f
# define BREAK_DURATION 3.0f

# define SELECTION_DISPLAY_THRESHOLD 0.50f

# define CONNECTION_ATTEMPT_INTERVAL 0.5f

# define ERP_MODEL    "erp.onnx"

# endif