#include <stdbool.h>
#ifdef _WIN32
# include <Windows.h>
#else
# include <unistd.h>
#endif
#include <trialfuncs.h>

#define TENTHS_TO_USECS(delay_tenths)	(delay_tenths * (100 ## 000))
#define TENTHS_TO_MILLIS(delay_tenths)	(delay_tenths * 100)

#define _CF_T(prefix, typestr)	prefix ## _ ## typestr ## _t
#define CF_T(prefix, typestr)	_CF_T(prefix, typestr)

struct _func_attrs_base {
	int  delay_tenths;
};
typedef struct _func_attrs_base func_attrs_base_t;

#define DEFINE_CASE_TYPES_FULL(typestr, typename)			\
	typedef struct {						\
		typename value;						\
	} _CF_T(boxed, typestr);					\
	typedef struct _func_attrs_##typestr {				\
		func_attrs_base_t delay;				\
		_CF_T(boxed, typestr) *result;				\
	} _CF_T(func_attrs, typestr);					\
	typedef struct _case_##typestr {				\
		_CF_T(func_attrs, typestr) *f_attrs, *g_attrs;		\
	} _CF_T(case, typestr)

#define DEFINE_CASE_TYPES(typestr)	DEFINE_CASE_TYPES_FULL(typestr, typestr) 

DEFINE_CASE_TYPES(bool);
DEFINE_CASE_TYPES(int);
DEFINE_CASE_TYPES(double);
DEFINE_CASE_TYPES_FULL(unsigned_int, unsigned int);

static inline void computational_delay(func_attrs_base_t *delay)
{
#ifdef _WIN32
	Sleep(delay? TENTHS_TO_MILLIS(delay->delay_tenths) : INFINITE);
#else
	if (! delay)
		pause();
	else
		usleep(TENTHS_TO_USECS(delay->delay_tenths));
#endif
}

bool index_inside_bounds(unsigned int index, unsigned int size) {
	return index < size;
}

#define DEFINE_COMP_FUNC(name, op) 								\
	compfunc_status_t trial_ ## name ## _ ## op(int x, TYPE(op) *valuep) {			\
		if (index_inside_bounds(x, sizeof cases_##op / sizeof cases_##op[0])) {		\
			computational_delay(& cases_##op[x].name##_attrs->delay);		\
			if (cases_##op[x].name##_attrs->result) {				\
				*valuep = cases_##op[x].name##_attrs->result->value;		\
				return COMPFUNC_SUCCESS;					\
			} 									\
		} 										\
		return COMPFUNC_HARD_FAIL;							\
	}										


#define _DEFINE_CASES_FULL(op, typestr) \
    	static _CF_T(case, typestr) cases_##op[]

#define DEFINE_CASES_FULL(op, typestr)	_DEFINE_CASES_FULL(op, typestr)  
#define DEFINE_CASES(op)		DEFINE_CASES_FULL(op, TYPESTR(op))

#define NUMERIC_CASES_INIT(typestr) 											\
	{ .f_attrs = & (_CF_T(func_attrs, typestr) ) { .delay.delay_tenths = 10, .result = & (_CF_T(boxed, typestr) ) { .value = 3 } },				\
	  .g_attrs = & (_CF_T(func_attrs, typestr) ) { .delay.delay_tenths = 30, .result = & (_CF_T(boxed, typestr) ) { .value = 5 } },				\
	}

DEFINE_CASES(and) = {
	    { .f_attrs = NULL,
	      .g_attrs = & (CF_T(func_attrs, bool) ) { .delay.delay_tenths = 50, .result = & (CF_T(boxed, bool) ) { .value = false } },
	    }
};

DEFINE_CASES(imul) = { NUMERIC_CASES_INIT(int) };
DEFINE_CASES(imin) = { NUMERIC_CASES_INIT(unsigned_int) };
DEFINE_CASES(fmul) = { NUMERIC_CASES_INIT(double) };

static case_double_t *cases_fmin = cases_fmul;

#define DEFINE_ALL_BUT_OR(name)	\
	DEFINE_COMP_FUNC(name, and)	\
	DEFINE_COMP_FUNC(name, imul)	\
	DEFINE_COMP_FUNC(name, fmul)	\
	DEFINE_COMP_FUNC(name, imin)

DEFINE_ALL_BUT_OR(f)
DEFINE_ALL_BUT_OR(g)

compfunc_status_t trial_f_or(int x, bool *valuep) {
	compfunc_status_t status = trial_f_and(x, valuep);
	if (status == COMPFUNC_SUCCESS)
		*valuep = !*valuep;

	return status;
}

compfunc_status_t trial_g_or(int x, bool *valuep) {
	compfunc_status_t status = trial_g_and(x, valuep);
	if (status == COMPFUNC_SUCCESS)
		*valuep = !*valuep;

	return status;
}

