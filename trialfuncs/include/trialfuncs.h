#ifndef _OS_LAB1_TRIALFUNCS_H
#define _OS_LAB1_TRIALFUNCS_H
#include "compfuncs.h"
#ifdef _MSC_VER
# include "trialfuncs_exports.h"
#else
# define LAB1_EXPORTS
#endif
#include <stdio.h>

#define DECLARE_FUNCS(op)			\
	LAB1_EXPORTS DECLARE_COMPFUNC(op, trial_f);		\
	LAB1_EXPORTS DECLARE_COMPFUNC(op, trial_g)		

DECLARE_FUNCS(and);
DECLARE_FUNCS(or);
DECLARE_FUNCS(imul);
DECLARE_FUNCS(fmul);
DECLARE_FUNCS(imin);
DECLARE_FUNCS(fmin);

#define TYPESTR_and	bool
#define TYPESTR_or 	bool
#define TYPESTR_imul	int
#define TYPESTR_fmul	double
#define TYPESTR_imin	unsigned_int
#define TYPESTR_fmin	double

#define TYPESTR(op)	TYPESTR_ ## op

static inline void print_int_value(int value) {
	printf("%d", value);
}

static inline void print_unsigned_int_value(int value) {
	printf("%d", value);
}

static inline void print_double_value(double value) {
	printf("%lf", value);
}

static inline void print__Bool_value(bool value) {
	printf("%s", value ? "true" : "false");
}

#define _PRINT_VALUE(typestr, value)	print_ ## typestr ## _value(value)
#define PRINT_VALUE(typestr, value)	_PRINT_VALUE(typestr, value)
#define PROCESS_FUNC(key, op, x)								\
		do {										\
			TYPE(op) value;							\
			compfunc_status_t status = trial_##key##_##op(x, &value);		\
			printf("trial_" #key "_" #op "(%d): %s", x, symbolic_status(status));	\
			if (status == COMPFUNC_SUCCESS) {					\
				putchar('<');							\
				PRINT_VALUE(TYPESTR(op), value);				\
				putchar('>');							\
			}									\
			putchar('\n');								\
		} while (0)									\

#endif // _OS_LAB1_TRIALFUNCS_H
