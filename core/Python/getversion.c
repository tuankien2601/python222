/* Portions Copyright (c) 2005 Nokia Corporation */

/* Return the full version string. */

#include "Python.h"

#include "patchlevel.h"

DL_EXPORT(const char *)
Py_GetVersion(void)
{
#ifndef SYMBIAN
 	static char version[250];
#else
#define version (PYTHON_GLOBALS->version)
#endif
	PyOS_snprintf(version, sizeof(version), "%.80s (%.80s) %.80s", 
		      PY_VERSION, Py_GetBuildInfo(), Py_GetCompiler());
	return version;
}
