#ifndef _OS_LAB1_COMPFUNCS_H
#define _OS_LAB1_COMPFUNCS_H

#include <stdbool.h>

#define TYPENAME_and	bool
#define TYPENAME_or 	bool
#define TYPENAME_imul	int
#define TYPENAME_fmul	double
#define TYPENAME_imin	unsigned int
#define TYPENAME_fmin	double

#define TYPE(op)	TYPENAME_ ## op

enum _compfunc_status {
	COMPFUNC_SUCCESS = 0,
	COMPFUNC_SOFT_FAIL,
	COMPFUNC_HARD_FAIL,
	COMPFUNC_STATUS_MAX
};
typedef enum _compfunc_status compfunc_status_t;

static char *_symbolic_status[] = { [COMPFUNC_SUCCESS] = "value", 
				    [COMPFUNC_SOFT_FAIL] = "soft-fail", 
				    [COMPFUNC_HARD_FAIL] = "hard-fail",
				    [COMPFUNC_STATUS_MAX] = "undefined" };

static inline char *symbolic_status(compfunc_status_t status) 
{
	return _symbolic_status[(long )(unsigned )status >= COMPFUNC_STATUS_MAX ? COMPFUNC_STATUS_MAX : status];
}

#define DECLARE_COMPFUNC(op, name)			\
	extern compfunc_status_t  name ## _ ## op(int , TYPE(op) *)

#endif // _OS_LAB1_COMPFUNCS_H
