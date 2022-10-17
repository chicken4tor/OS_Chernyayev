#ifndef __MANAGER_INC__
#define __MANAGER_INC__

/// @brief Encapsulate manager data in this structure
typedef struct _manager_state manager_state_t;

/// @brief Initialize Inter-Process-Communication, spawn children
/// @param input_fd    File descriptor for reading input values
/// @param buffer_size Size of input and output buffers
manager_state_t *construct_manager(int input_fd, int buffer_size);

/// @brief Close resources, kill children
/// @param mgr Manager allocated by construct_manager()
void destruct_manager(manager_state_t *mgr);

/// @brief Send and receive data
/// @param mgr Manager instance
void communicate(manager_state_t *mgr);

#endif // __MANAGER_INC__
