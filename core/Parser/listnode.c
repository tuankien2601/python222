/* Portions Copyright (c) 2005 Nokia Corporation */

/* List a node on a file */

#include "python_globals.h"

#include "pgenheaders.h"
#include "token.h"
#include "node.h"

/* Forward */
static void list1node(FILE *, node *);
static void listnode(FILE *, node *);

DL_EXPORT(void)
PyNode_ListTree(node *n)
{
	listnode(stdout, n);
}

#ifndef SYMBIAN
static int level, atbol;
#else
#define level (pyglobals->listnode_level)
#define atbol (pyglobals->listnode_atbol)
#endif

static void
listnode(FILE *fp, node *n)
{
#ifdef SYMBIAN
        SPy_Python_globals* pyglobals = PYTHON_GLOBALS; //avoid TLS calls
#endif
	level = 0;
	atbol = 1;
	list1node(fp, n);
}

static void
list1node(FILE *fp, node *n)
{
#ifdef SYMBIAN
        SPy_Python_globals* pyglobals; //avoid TLS calls
#endif

	if (n == 0)
		return;
	if (ISNONTERMINAL(TYPE(n))) {
		int i;
		for (i = 0; i < NCH(n); i++)
			list1node(fp, CHILD(n, i));
	}
	else if (ISTERMINAL(TYPE(n))) {
#ifdef SYMBIAN
                pyglobals = PYTHON_GLOBALS;
#endif
		switch (TYPE(n)) {
		case INDENT:
			++level;
			break;
		case DEDENT:
			--level;
			break;
		default:
			if (atbol) {
				int i;
				for (i = 0; i < level; ++i)
					fprintf(fp, "\t");
				atbol = 0;
			}
			if (TYPE(n) == NEWLINE) {
				if (STR(n) != NULL)
					fprintf(fp, "%s", STR(n));
				fprintf(fp, "\n");
				atbol = 1;
			}
			else
				fprintf(fp, "%s ", STR(n));
			break;
		}
	}
	else
		fprintf(fp, "? ");
}
