#ifndef __SHARED_DATA_INC__
#define __SHARED_DATA_INC__

#include <stdbool.h>

#include <compfuncs.h>

enum _computation_node
{
    F_NODE,
    G_NODE,
    NODES_COUNT
};

struct _value
{
    compfunc_status_t status; // Calculation status

    /// @brief Shared memory for specific value types
    union
    {
        bool b_val;
        double d_val;
        int i_val;
        unsigned int ui_val;
    };
};

/// @brief Value type for data interchange
typedef struct _value value_t;

typedef enum _computation_node computation_node;

extern const char *node_pipe[NODES_COUNT][2];

extern const char *calc_task;

enum _trial_functions
{
    TF_UNKNOWN = -1,
    TF_IMUL,
    TF_IMIN,
    TF_FMUL,
    TF_AND,
    TF_OR,
    TF_COUNT
};

typedef enum _trial_functions trial_function_t;

enum _tf_result
{
    TFR_UNKNOWN = -1,
    TFR_INT,
    TFR_UINT,
    TFR_FLOAT,
    TFR_BOOL
};

typedef enum _tf_result tf_result_t;

/// @brief Get trial function id from name
/// @param tf Trial function name
/// @return Trial function numerical id
trial_function_t function_from_name(const char *tf);

/// @brief Get trial function name from id
/// @param tf Trial function id
/// @return Trial function name
const char *tf_name(trial_function_t tf);

/// @brief Get result type from function id
/// @param tf Trial function numerical id
/// @return Trial function result type id
tf_result_t trial_result_type(trial_function_t tf);

#endif // __SHARED_DATA_INC__
