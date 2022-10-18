#ifndef __SHARED_DATA_INC__
#define __SHARED_DATA_INC__

#include <stdbool.h>

enum _computation_node
{
    F_NODE,
    G_NODE,
    NODES_COUNT
};


union _value
{
    bool b_val;
    double d_val;
    int i_val;
    unsigned int ui_val;
};

/// @brief Value type for data interchange
typedef union _value value_t;

typedef enum _computation_node computation_node;

extern const char *node_pipe[NODES_COUNT];

extern const char *calc_task;

#endif // __SHARED_DATA_INC__
