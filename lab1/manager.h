#ifndef __MANAGER_INC__
#define __MANAGER_INC__

#include <stdbool.h>

/// @brief Encapsulate manager data in this structure.
///
/// Responsibility:
///   - create named pipes
///   - spawn processes
///   - get x
///   - send x to f
///   - send x to g
///   - get response from f
///   - get response from g
///   - calculate f*g, send result
typedef struct _manager_state manager_state_t;

/// @brief Initialize Inter-Process-Communication, spawn children
/// @param input_fd    File descriptor for reading input values
/// @param buffer_size Size of input and output buffers
/// @param f_func      Specify f(x)
/// @param g_func      Specify g(x)
/// @param final_func  Specify g(x)
manager_state_t *construct_manager(int input_fd, int buffer_size, const char *f_func, const char *g_func, const char *final_func);

/// @brief Close resources, kill children
/// @param mgr Manager allocated by construct_manager()
void destruct_manager(manager_state_t *mgr);

/// @brief Send and receive data
/// @param mgr Manager instance
/// @return True, if no failures in inter process communication
bool communicate(manager_state_t *mgr);

/// @brief Perform final computation
/// @param mgr Manager instance
/// @return True, on success
bool final_calculation(manager_state_t *mgr);

/// @brief Shutdown connection
/// @param mgr Manager instance
void shutdown(manager_state_t *mgr);

#endif // __MANAGER_INC__
