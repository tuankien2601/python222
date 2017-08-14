/*
* ====================================================================
*  thread_symbian.h
*  
*  Basic thread support on Symbian OS.
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

#include <e32std.h>
#include <e32base.h>

#define KStackSize 0x10000
#define KHeapSize 300000
#define NTHREADEXITFUNCS 32

extern "C" void SPy_InitStdio();   // in Symbian\CSPyInterpreter.cpp

/*
 * Initialization.
 */

static void
PyThread__init_thread(void)
{
}

/*
 * Thread support.
 */

struct launchpad_args {
  TThreadFunction f;
  TAny* arg;
  TAny* globals;
};

struct exitfuncs {
  void (*tab[NTHREADEXITFUNCS])();
  int n;
};

class CPyThreadRoot : public CActive
{
 public:
  CPyThreadRoot(TThreadFunction& aFunc, TAny* aArg):
    CActive(EPriorityStandard),iFunc(aFunc),iArg(aArg)
    {
      CActiveScheduler::Add(this);
    }

  void Start() {
    iStatus = KRequestPending;
    SetActive();
    iStatus = KErrNone;
    TRequestStatus* pstatus = &iStatus;
    RThread().RequestComplete(pstatus, 0);
    CActiveScheduler::Start();
  }

 private:
  void DoCancel() {;}
  void RunL() {
    struct exitfuncs xf;
    xf.n = 0;
    PYTHON_TLS->l = &xf;
    iFunc(iArg);
    CActiveScheduler::Stop();
  }

  TThreadFunction iFunc;
  TAny* iArg;
};

static TInt launchpad(TAny* p)
{
  TThreadFunction func = ((launchpad_args*)p)->f;
  TAny* arg = ((launchpad_args*)p)->arg;
  
  if (SPy_tls_initialize((SPy_Python_globals*)((launchpad_args*)p)->globals))
    User::Panic(_L("Python thread"), KErrNoMemory);

  PyMem_DEL(p);

  SPy_InitStdio();

  CTrapCleanup* cleanup_stack = CTrapCleanup::New();
  //  __UHEAP_MARK;

  TRAPD(error, {
    CActiveScheduler* as = new (ELeave) CActiveScheduler;
    CleanupStack::PushL(as);
    CActiveScheduler::Install(as);

    CPyThreadRoot* pytroot = new (ELeave) CPyThreadRoot(func, arg);
    CleanupStack::PushL(pytroot);

    pytroot->Start();

    CleanupStack::PopAndDestroy();
    CleanupStack::PopAndDestroy();
  });

  __ASSERT_ALWAYS(!error, User::Panic(_L("Python thread"), error));

  //  CloseSTDLIB();
  User::Free(ImpurePtr());
  delete cleanup_stack;
  SPy_tls_finalize(0);
  //  __UHEAP_MARKEND;
  RThread().Terminate(0);
  return 0; // dummy
}

extern "C" {
DL_EXPORT(long)
PyThread_start_new_thread(void (*func)(void *), void *arg)
{
  int success = 0;
  SPy_Python_globals* pg = PYTHON_GLOBALS;

  launchpad_args* lp_arg = (launchpad_args*)PyMem_NEW(struct launchpad_args, 1);
  if (!lp_arg)
    return (-1);
  lp_arg->f = (TThreadFunction)func;
  lp_arg->arg = (TAny*)arg;
  lp_arg->globals = (TAny*)pg;

  RThread t;
  success = t.Create(_L("PyThread"), launchpad, KStackSize,
                     KMinHeapSize, KHeapSize, (TAny*)lp_arg);
  if (success == 0) {
    t.SetPriority(EPriorityLess);
    t.Resume();
    success = t.Id();
  }
  else
    PyMem_DEL(lp_arg);
  
  t.Close();

  return ((success < 0) ? -1 : success);
}
} /* extern "C" */

class CPyThreadWait : public CActive
{
 public:
  CPyThreadWait(TInt aTid):CActive(EPriorityStandard),iTid(aTid) {
    CActiveScheduler::Add(this);
  }

  ~CPyThreadWait() {
    iThread.Close();
  }

  int Wait() {
    if (iThread.Open(iTid) == KErrNone) {
      iThread.Logon(iStatus);
      SetActive();
      Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT   
      iWait.Start();
#else
      CActiveScheduler::Start();
#endif
      Py_END_ALLOW_THREADS
      return ((iStatus.Int() != KErrNoMemory) ? 0 : (-1));
    }
    else
      return (-1);
  }

 private:
  void DoCancel() {;}
  void RunL() {
#ifdef HAVE_ACTIVESCHEDULERWAIT
    iWait.AsyncStop();
#else
    CActiveScheduler::Stop();
#endif
  }
  
  TInt iTid;
  RThread iThread;
#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
};

extern "C" {
DL_EXPORT(int)
PyThread_ao_waittid(long tid)
{
  CPyThreadWait* w = new CPyThreadWait(tid);
  
  if (w) {
    int error = w->Wait();
    delete w;
    return error;
  }
  else
    return (-1);
}

DL_EXPORT(long)
PyThread_get_thread_ident(void)
{
  return (TUint)(RThread().Id());
}

static
void do_PyThread_exit_thread(int no_cleanup)
{
  if (!initialized)
    if (no_cleanup)
      _exit(0);
    else
      exit(0);

  struct exitfuncs *pxf = (struct exitfuncs *)(PYTHON_TLS->l);
  while (pxf->n > 0)
    (pxf->tab[--pxf->n])();
}

DL_EXPORT(void)
PyThread_exit_thread(void)
{
  do_PyThread_exit_thread(0);
}

DL_EXPORT(void)
PyThread__exit_thread(void)
{
  do_PyThread_exit_thread(1);
}

DL_EXPORT(int)
PyThread_AtExit(void (*func)(void))
{
  if ((PYTHON_GLOBALS->main_thread == 0) ||
      (PYTHON_GLOBALS->main_thread == PyThread_get_thread_ident()))
    return Py_AtExit(func);
  
  struct exitfuncs *pxf = (struct exitfuncs *)(PYTHON_TLS->l);
  if (pxf->n >= NEXITFUNCS)
    return -1;
  pxf->tab[pxf->n++] = func;
  return 0;
}
} /* extern "C" */

/*
 * Lock support.
 */

class Symbian_lock {
public:
  Symbian_lock():lock_value(0) {;}
  ~Symbian_lock() {cs.Close(); wait_q.Close();}
  TInt init();
  RCriticalSection cs;
  RSemaphore wait_q;
  TInt lock_value;
};

TInt Symbian_lock::init()
{
  TInt error = cs.CreateLocal();
  if (error == KErrNone) {
    error = wait_q.CreateLocal(0);
    if (error != KErrNone)
      cs.Close();
  }
  
  return error;
}

extern "C" {
DL_EXPORT(PyThread_type_lock)
PyThread_allocate_lock(void)
{
  Symbian_lock* lock = NULL;
  void* raw_mem_in_python_chunk = PyMem_Malloc(sizeof(Symbian_lock));
  if (raw_mem_in_python_chunk) {
    lock = new (raw_mem_in_python_chunk) Symbian_lock();
    if (lock->init() != KErrNone) {
      lock->~Symbian_lock();
      PyMem_Free(raw_mem_in_python_chunk);
      lock = NULL;
    }
  } 
  return (PyThread_type_lock) lock;
}

DL_EXPORT(void)
PyThread_free_lock(PyThread_type_lock lock)
{
  ((Symbian_lock*)lock)->~Symbian_lock();
  PyMem_Free((void*)lock);
}

DL_EXPORT(int)
PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)
{
  int success;

  ((Symbian_lock*)lock)->cs.Wait();
  success = (((Symbian_lock*)lock)->lock_value == 0);
  if (success || waitflag)
    ((Symbian_lock*)lock)->lock_value++;
  ((Symbian_lock*)lock)->cs.Signal();

  if (!success && waitflag) {
    ((Symbian_lock*)lock)->wait_q.Wait();
    success = 1;
  }

  return success;
}

DL_EXPORT(void)
PyThread_release_lock(PyThread_type_lock lock)
{
  ((Symbian_lock*)lock)->cs.Wait();
  
  if (((Symbian_lock*)lock)->lock_value > 0) {
    ((Symbian_lock*)lock)->lock_value--;
    if (((Symbian_lock*)lock)->lock_value)
      ((Symbian_lock*)lock)->wait_q.Signal();
  }

  ((Symbian_lock*)lock)->cs.Signal();
}
} /* extern "C" */
