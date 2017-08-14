/* Portions Copyright (c) 2005 Nokia Corporation */

/* Return the compiler identification, if possible. */

#include "Python.h"

#ifndef COMPILER

#ifdef __GNUC__
#define COMPILER "\n[GCC " __VERSION__ "]"
#endif

#endif /* !COMPILER */

#ifndef COMPILER

#ifdef __cplusplus
#define COMPILER "[C++]"
#else
#define COMPILER "[C]"
#endif

#endif /* !COMPILER */

DL_EXPORT(const char *)
Py_GetCompiler(void)
{
	return COMPILER;
}
