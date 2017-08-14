/*
 * ====================================================================
 *  CSPyInterpreter.h
 *  
 *  An interface for creating/deleting a Python interpreter instance
 *  and some convenience functions for simple interaction with it.     
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <reent.h>
#include <estlib.h>
#include <eikenv.h>
#include <basched.h>
#include <e32std.h>
#include <utf.h>
#include "CSPyInterpreter.h"
#include "python.h"
#include "osdefs.h"
#include "python_globals.h"

const TInt KHeapSize = 16000000;
_LIT(KLibPath, "\\system\\libs");

static CSPyInterpreter* GetPythonInterpreter()
{
  return (CSPyInterpreter*)(PYTHON_GLOBALS->interpreter);
}

static int SPyInterpreter_read(void *cookie, char *buf, int n)
{
  return STATIC_CAST(CSPyInterpreter*, cookie)->read(buf, n);
}

static int SPyInterpreter_write(void *cookie, const char *buf, int n)
{
  return STATIC_CAST(CSPyInterpreter*, cookie)->write(buf, n);
}

static int SPyInterpreter_close(void* /*cookie*/)
{
  return 0;
}

extern "C" void PyOS_InitInterrupts() {}

extern "C" void PyOS_FiniInterrupts() {}

extern "C" int PyOS_InterruptOccurred()
{
  TInt r = (GetPythonInterpreter())->iInterruptOccurred;
  (GetPythonInterpreter())->iInterruptOccurred = 0;
  return r;
}

extern "C" char* SPy_get_path()
{
  RFs r;
  // Memory gets freed when the pool is destroyed
  char* path = (char*)PyMem_MALLOC(KMaxDrives*(TPtrC(KLibPath).Length()+2));
  
  if (path && (r.Connect() == KErrNone)) {
    TBuf8<KMaxFileName> buf;
    char* t = path;
    TFindFile ff(r);
    if (ff.FindByDir(KLibPath, _L("")) == KErrNone)
      do {
        CnvUtfConverter::ConvertFromUnicodeToUtf8(buf, ff.File());
        memcpy(t, buf.Ptr(), buf.Length());
        t += buf.Length();
        *t++ = ';';
      }
      while (ff.Find() == KErrNone);
    r.Close();
    --t;
    *t = '\0';
  }
  return path;
}

//
// Dynamic memory allocation from interpreter's own heap.
//

void* symport_malloc(size_t nbytes)
{
  return ((GetPythonInterpreter())->iPyheap)->Alloc(nbytes);
}

void* symport_realloc(void *p, size_t nbytes)
{
  return ((GetPythonInterpreter())->iPyheap)->ReAlloc(p, nbytes);
}

void symport_free(void *p)
{
  ((GetPythonInterpreter())->iPyheap)->Free(p);
}

//
// Connect std* file descriptors. If 'aStdioInitFunc' is given 
// to CSPyInterpreter::NewInterpreterL(), it is called to
// initialize the standard i/o streams. By default, InitStdio()
// below is used. 
//

static void InitStdio(void* ip)
{
  _REENT->_sf[0]._cookie = STATIC_CAST(void*, ip);
  _REENT->_sf[0]._read = SPyInterpreter_read;
  _REENT->_sf[0]._write = 0;
  _REENT->_sf[0]._seek = 0;
  _REENT->_sf[0]._close = SPyInterpreter_close;
  _REENT->_sf[0]._data = _REENT;

  _REENT->_sf[1]._cookie = STATIC_CAST(void*, ip);
  _REENT->_sf[1]._read = 0;
  _REENT->_sf[1]._write = SPyInterpreter_write;
  _REENT->_sf[1]._seek = 0;
  _REENT->_sf[1]._close = SPyInterpreter_close;
  _REENT->_sf[1]._data = _REENT;

  _REENT->_sf[2]._cookie = STATIC_CAST(void*, ip);
  _REENT->_sf[2]._read = 0;
  _REENT->_sf[2]._write = SPyInterpreter_write;
  _REENT->_sf[2]._seek = 0;
  _REENT->_sf[2]._close = SPyInterpreter_close;
  _REENT->_sf[2]._data = _REENT;
  
  _REENT->__sdidinit = 1;
}

extern "C" void SPy_InitStdio()
{
  CSPyInterpreter* ip = GetPythonInterpreter();

  ip->iStdioInitFunc(ip->iStdioInitCookie);
}

EXPORT_C CSPyInterpreter*
CSPyInterpreter::NewInterpreterL(TBool aCloseStdlib,
                                 void(*aStdioInitFunc)(void*),
                                 void* aStdioInitCookie)
{
  CSPyInterpreter* self = new (ELeave) CSPyInterpreter(aCloseStdlib);
  
  self->iStdioInitFunc = (aStdioInitFunc ? aStdioInitFunc : &InitStdio);
  self->iStdioInitCookie = (aStdioInitCookie ? aStdioInitCookie : self);

  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop();

  return self;
}

void CSPyInterpreter::ConstructL()
{
  iStdioInitFunc(iStdioInitCookie);
  
  iPyheap = UserHeap::ChunkHeap(NULL, KMinHeapSize, KHeapSize);
  if (iPyheap == NULL)
    User::Leave(KErrNoMemory);
  
  if (SPy_globals_initialize((void*)this))
    User::Leave(KErrNoMemory);
  
  Py_VerboseFlag = EFalse;
  Py_NoSiteFlag = EFalse;
  Py_TabcheckFlag = EFalse;
  Py_DebugFlag = EFalse;
  
  Py_Initialize();

  PYTHON_TLS->thread_state = _PyThreadState_Current;
  PyEval_SaveThread();
}

EXPORT_C TInt CSPyInterpreter::RunScript(int argc, char** argv)
{
  if (argc < 1)
    return KErrArgument;
  char* filename = argv[0];
  rewind(stdin);
  FILE *fp = fopen(filename, "r");
  if (!fp)
    return KErrNotFound;

  __ASSERT_DEBUG(_PyThreadState_Current != PYTHON_TLS->thread_state,
                 User::Panic(_L("CSPyInterpreter"), 1));

  PyEval_RestoreThread(PYTHON_TLS->thread_state);

  PySys_SetArgv(argc, argv);
  int err = PyRun_SimpleFile(fp, filename);
  fclose(fp);
  
  __ASSERT_DEBUG(_PyThreadState_Current == PYTHON_TLS->thread_state,
                 User::Panic(_L("CSPyInterpreter"), 2));

  PyEval_SaveThread();

  return (err ? KErrGeneral : KErrNone);
}

EXPORT_C void CSPyInterpreter::PrintError()
{
  __ASSERT_DEBUG(_PyThreadState_Current != PYTHON_TLS->thread_state,
                 User::Panic(_L("CSPyInterpreter"), 3));

  PyEval_RestoreThread(PYTHON_TLS->thread_state);
  
  if (PyErr_Occurred())
    PyErr_Print();
  
  __ASSERT_DEBUG(_PyThreadState_Current == PYTHON_TLS->thread_state,
                 User::Panic(_L("CSPyInterpreter"), 4));
  
  PyEval_SaveThread();
}

EXPORT_C CSPyInterpreter::~CSPyInterpreter()
{
  __ASSERT_DEBUG(_PyThreadState_Current != PYTHON_TLS->thread_state,
                 User::Panic(_L("CSPyInterpreter"), 5));

  PyEval_RestoreThread(PYTHON_TLS->thread_state);
  
  Py_Finalize();
  SPy_globals_finalize();
  if (iCloseStdlib) CloseSTDLIB();
  iPyheap->Close();
}

GLDEF_C TInt E32Dll(TDllReason aReason)
{
  /*  // This doesn't work for some reason. Doesn't this get called?
      if (aReason==EDllProcessAttach)
      return Dll::InitialiseData();
      else if (aReason==EDllProcessDetach)
      Dll::FreeData();

  */
  return (KErrNone);
}
