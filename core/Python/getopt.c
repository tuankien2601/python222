/* Portions Copyright (c) 2005 Nokia Corporation */
/*---------------------------------------------------------------------------*
 * <RCS keywords>
 *
 * C++ Library
 *
 * Copyright 1992-1994, David Gottner
 *
 *                    All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice, this permission notice and
 * the following disclaimer notice appear unmodified in all copies.
 *
 * I DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL I
 * BE LIABLE FOR ANY SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Nevertheless, I would like to know about bugs in this library or
 * suggestions for improvment.  Send bug reports and feedback to
 * davegottner@delphi.com.
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "python_globals.h"

#ifndef SYMBIAN
int _PyOS_opterr = 1;          /* generate error messages */
int _PyOS_optind = 1;          /* index into argv array   */
char *_PyOS_optarg = NULL;     /* optional argument       */
#else
#define _PyOS_opterr (PYTHON_GLOBALS->_PyOS_opterr)
#define _PyOS_optind (PYTHON_GLOBALS->_PyOS_optind)
#define _PyOS_optarg (PYTHON_GLOBALS->_PyOS_optarg)
#endif

int _PyOS_GetOpt(int argc, char **argv, char *optstring)
{
#ifndef SYMBIAN
  	static char *opt_ptr = "";
#else
#define opt_ptr (PYTHON_GLOBALS->opt_ptr)
#endif
	char *ptr;
	int option;

	if (!_PyOS_opterr)
	  _PyOS_opterr = 1;

	if (!_PyOS_optind)
	  _PyOS_optind = 1;

	if (!opt_ptr)
	  opt_ptr = "";

	if (*opt_ptr == '\0') {

		if (_PyOS_optind >= argc ||
		    argv[_PyOS_optind][0] != '-' ||
		    argv[_PyOS_optind][1] == '\0' /* lone dash */ )
			return -1;

		else if (strcmp(argv[_PyOS_optind], "--") == 0) {
			++_PyOS_optind;
			return -1;
		}

		opt_ptr = &argv[_PyOS_optind++][1]; 
	}

	if ( (option = *opt_ptr++) == '\0')
		return -1;
	
	if ((ptr = strchr(optstring, option)) == NULL) {
		if (_PyOS_opterr)
			fprintf(stderr, "Unknown option: -%c\n", option);

		return '?';
	}

	if (*(ptr + 1) == ':') {
		if (*opt_ptr != '\0') {
			_PyOS_optarg  = opt_ptr;
			opt_ptr = "";
		}

		else {
			if (_PyOS_optind >= argc) {
				if (_PyOS_opterr)
					fprintf(stderr,
			    "Argument expected for the -%c option\n", option);
				return '?';
			}

			_PyOS_optarg = argv[_PyOS_optind++];
		}
	}

	return option;
}
