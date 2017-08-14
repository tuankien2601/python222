/*
* ====================================================================
*  dynload_symbian.h
*  
*  Support for dynamic loading of extension modules on Symbian OS
*
* Copyright (c) 2005 Nokia Corporation
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
* ====================================================================
*/

#include <eikdll.h>
#include <utf.h>

#include "Python.h"
#include "importdl.h"
#include "symbian_python_ext_util.h"

const struct filedescr _PyImport_DynLoadFiletab[] = {
        {".pyd", "rb", C_EXTENSION},
        /*      {".dll", "rb", C_EXTENSION}, */
        {0, 0}
};

struct Symbian_lib_handle {
  RLibrary lib;
  Symbian_lib_handle* next;
};

extern "C" {
  static void SPy_dynload_finalize()
  {
    Symbian_lib_handle* h =
      (Symbian_lib_handle*) PYTHON_GLOBALS->lib_handles;
    Symbian_lib_handle* n;
        
    while (h) {
      dl_funcptr p = (dl_funcptr) (h->lib).Lookup(2);
      if (p) p();
      (h->lib).Close();
      n = h->next;
      PyMem_Free((void*)h);
      h = n;
    }
  }
  
  dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname,
                                      const char *shortname,
                                      const char *pathname,
                                      FILE *fp)
  {
    TInt error;
    dl_funcptr p;
    Symbian_lib_handle* h = NULL;
    TBuf16<KMaxFileName> u_pathname;

    if (fp)
      fclose(fp);

    error = CnvUtfConverter::
      ConvertToUnicodeFromUtf8(u_pathname,
                               TPtrC8((const TUint8*)pathname));

    if (error == KErrNone) {
      for (TInt i=0; i < u_pathname.Length(); i++)
        if (u_pathname[i] == TChar('/'))
          u_pathname.Replace(i, 1, _L("\\"));
    }

    if (error == KErrNone) {
      void* raw_py_mem = PyMem_Malloc(sizeof(Symbian_lib_handle));
      if (raw_py_mem)
        h = new (raw_py_mem) Symbian_lib_handle();
      else
        error = KErrNoMemory;
    }

    if (error == KErrNone) {
      error = (h->lib).Load(u_pathname);
      if ((error == KErrNone) && PYTHON_GLOBALS->main_thread &&
          (PYTHON_GLOBALS->main_thread != PyThread_get_thread_ident())) {
        // trick to promote to process-wide handle
        RLibrary temp = h->lib;  
        error = (h->lib).Duplicate(RThread());
        if (error == KErrNone)
          temp.Close();
      }
    }

    if (error == KErrNone) {
      p = (dl_funcptr) (h->lib).Lookup(1);
      if (!p) {
        (h->lib).Close();
        error = KErrBadLibraryEntryPoint;
      }
    }
    
    if (error == KErrNone) {
      void** hs = &(PYTHON_GLOBALS->lib_handles);
      if (*hs == NULL) {
        h->next = NULL;
        *hs = h;
        Py_AtExit(&SPy_dynload_finalize);
      } else {
        h->next = (Symbian_lib_handle*)*hs;
        *hs = h;
      }
      return p;
    } else {
      delete h;
      return (dl_funcptr)SPyErr_SetFromSymbianOSErr(error);
    }
  }
} /* extern "C" */
