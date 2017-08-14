/* Copyright (c) 2005 Nokia Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Python.h"

extern void inite32(void);
extern void inite32posix(void);

#ifdef WITH_THREAD
extern void initthread(void);
#endif
extern void PyMarshal_Init(void);
extern void initimp(void);

extern void init_codecs(void);
extern void initbinascii(void);
extern void initerrno(void);
extern void initcStringIO(void);
extern void initoperator (void);
extern void initmath (void);
extern void initstruct (void);
extern void inittime (void);
extern void initmd5(void);
extern void init_sre(void);
extern void initxreadlines(void);
#ifdef WITH_CYCLE_GC
extern void initgc(void);
#endif

const struct _inittab _PyImport_Inittab[] = {
  {"e32", inite32},                    /* e32module.cpp */
  {"e32posix", inite32posix},          /* posixmodule.cpp */
#ifdef WITH_THREAD
  {"thread", initthread},              /* threadmodule.c */
#endif
  {"marshal", PyMarshal_Init},         /* marshal.c */
  {"imp", initimp},                    /* import.c */
  {"binascii", initbinascii},          /* binascii.c */
  {"errno", initerrno},                /* errnomodule.c */
  {"cStringIO", initcStringIO},        /* cStringIO.c */
  {"math", initmath},                  /* mathmodule.c */
  {"struct", initstruct},              /* structmodule.c */
  {"time", inittime},                  /* timemodule.c */
  {"operator", initoperator},          /* operator.c */
  {"md5", initmd5},                    /* md5module.c, md5c.c */
  {"_sre", init_sre},                  /* _sre.c */
  {"_codecs", init_codecs},            /* _codecsmodule.c */
  {"xreadlines", initxreadlines},      /* xreadlinesmodule.c */

  /* These entries are here for sys.builtin_module_names */
  {"__main__", NULL},
  {"__builtin__", NULL},
  {"sys", NULL},
  {"exceptions", NULL},

#ifdef WITH_CYCLE_GC
  {"gc", initgc},                      /* gcmodule.c */
#endif

  /* Sentinel */
  {0, 0}
};
