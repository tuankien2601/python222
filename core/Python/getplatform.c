/* Portions Copyright (c) 2005 Nokia Corporation */

#include "Python.h"

#ifndef PLATFORM
#define PLATFORM "unknown"
#endif

DL_EXPORT(const char *)
Py_GetPlatform(void)
{
	return PLATFORM;
}
