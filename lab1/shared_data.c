#include <string.h>

#include "shared_data.h"

const char *node_pipe[NODES_COUNT][2] = {
    "/tmp/lab_1_25_f_pipe", "/tmp/lab_1_25_f_pipe_res",
    "/tmp/lab_1_25_g_pipe", "/tmp/lab_1_25_g_pipe_res",
};

const char *calc_task = "calculon";

// NOTE: order must match enum _trial_functions
const char *trial_functions[TF_COUNT] = {
    "imul",
    "imin",
    "fmul",
    "and",
    "or",
};

// NOTE: order must match enum _trial_functions
tf_result_t tf_results[TF_COUNT] = {
    TFR_INT,
    TFR_UINT,
    TFR_FLOAT,
    TFR_BOOL,
    TFR_BOOL,
};

trial_function_t function_from_name(const char *tf)
{
    trial_function_t result = TF_UNKNOWN;

    for (trial_function_t i = TF_IMUL; i < TF_COUNT; i++)
    {
        if (strcmp(tf, trial_functions[i]) == 0)
        {
            result = i;
            break;
        }
    }

    return result;
}

tf_result_t trial_result_type(trial_function_t tf)
{
    if (tf != TF_UNKNOWN)
    {
        return tf_results[tf];
    }

    return TFR_UNKNOWN;
}

const char *tf_name(trial_function_t tf)
{
    if (tf != TF_UNKNOWN)
    {
        return trial_functions[tf];
    }

    return NULL;
}
