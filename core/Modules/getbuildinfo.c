/* Portions Copyright (c) 2005 Nokia Corporation */
#include "Python.h"

#ifdef SYMBIAN
#include "python_globals.h"
#endif

#ifdef macintosh
#include "macbuildno.h"
#endif

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

#ifndef DATE
#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE "xx/xx/xx"
#endif
#endif

#ifndef TIME
#ifdef __TIME__
#define TIME __TIME__
#else
#define TIME "xx:xx:xx"
#endif
#endif

#ifndef BUILD
#define BUILD 0
#endif


DL_EXPORT(const char *)
Py_GetBuildInfo(void)
{
#ifndef SYMBIAN
        static char buildinfo[50];
#else
#define buildinfo (PYTHON_GLOBALS->buildinfo)
#endif
	PyOS_snprintf(buildinfo, sizeof(buildinfo),
		      "#%d, %.20s, %.9s", BUILD, DATE, TIME);
	return buildinfo;
}
