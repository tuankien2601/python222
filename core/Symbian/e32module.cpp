/*
* ====================================================================
*  e32module.cpp  
*  
*  Basic EPOC facilities for Python
*
*  Implements currently (23.11.2004) following Python classes / methods:
*
*  Ao_lock -- Symbian active object -based synchronization
*    Ao_lock()
*    wait()
*    signal()
*
*  Ao_timer -- Symbian active object -based timer
*    Ao_timer()
*    after()
*    cancel()
*
*  ao_yield()
*
*  ao_sleep(float [,callable])
*
*  callable ao_callgate(callable)
*
*  file_copy(unicode_string OR string, unicode_string OR string)
*
*  start_server(unicode_string OR string)
*
*  start_exe(unicode_string OR string, unicode_string OR string [,int])
*
*  [unicode_string] drive_list()
*
*  bool is_ui_thread()
*
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
#include <e32uid.h>   // e32_uidcrc_app()
#include <eikapp.h>
#include <f32file.h>
#include <bautils.h>  // e32_file_copy()
#include <apmstd.h>
#include <hal.h>      // e32_clock()
#include "Python.h"
#include "symbian_python_ext_util.h"
#include "CSPyInterpreter.h" // e32_stdo(), e32_mem_info()

/*
 *
 * Utilities for e32.start_server() and e32.start_exe()
 *
 */

class CE32ProcessWait : public CActive
{
public:
  CE32ProcessWait():CActive(EPriorityStandard) {
    CActiveScheduler::Add(this);
  }
#if defined(__WINS__)  
  TInt Wait(RThread& aProcess) {
#else
  TInt Wait(RProcess& aProcess) {
#endif
    aProcess.Logon(iStatus);
    aProcess.Resume();
    SetActive();
#ifdef HAVE_ACTIVESCHEDULERWAIT   
    iWait.Start();
#else
    CActiveScheduler::Start();
#endif
    return iStatus.Int();
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
#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
};

static ProcessLaunch(const TDesC& aFileName, const TDesC& aCommand,
                     TInt aWaitFlag=0)
{
  TInt error;
  Py_BEGIN_ALLOW_THREADS
#if defined(__WINS__)
  RThread proc;
  RLibrary lib;
  HBufC* pcommand = aCommand.Alloc();
  error = lib.Load(aFileName);
  if (error == KErrNone) {
    TThreadFunction func = (TThreadFunction)(lib.Lookup(1));
    error = proc.Create(_L(""), func, 0x1000, (TAny*) pcommand, &lib,
                        RThread().Heap(), 0x1000, 0x100000, EOwnerProcess);
    lib.Close();
  }
  else
    delete pcommand;
#else
  RProcess proc;
  error = proc.Create(aFileName, aCommand);
#endif
  if (error == KErrNone)
    if (aWaitFlag) {
      CE32ProcessWait* w = new CE32ProcessWait();
      if (w) {
        error = w->Wait(proc);
        delete w;
      }
      else
        error = KErrNoMemory;
    }
    else
      proc.Resume();
  proc.Close();
  Py_END_ALLOW_THREADS
  return error;
}

/*
 *
 * Implementation of e32.start_server()
 *
 */

extern "C" PyObject *
e32_start_server(PyObject* /*self*/, PyObject* args)
{
  PyObject* it;

  if (!PyArg_ParseTuple(args, "O", &it))
    return NULL;

  PyObject* fn = PyUnicode_FromObject(it);
  if (!fn)
    return NULL;

  TPtrC name(PyUnicode_AsUnicode(fn), PyUnicode_GetSize(fn));
  TParse p;
  p.Set(name, NULL, NULL);
  
  if (!(p.Ext().CompareF(_L(".py")) == 0)) {
    Py_DECREF(fn);
    PyErr_SetString(PyExc_TypeError, "Python script name expected");
    return NULL;
  }
  
  TInt error;
  error =
#if defined(__WINS__)
    ProcessLaunch(_L("\\system\\programs\\Python_launcher.dll"), name);
#else
    ProcessLaunch(_L("python_launcher.exe"), name);
#endif

  Py_DECREF(fn);

  RETURN_ERROR_OR_PYNONE(error);
}

/*
 *
 * Implementation of e32.start_exe()
 *
 */

extern "C" PyObject *
e32_start_exe(PyObject* /*self*/, PyObject* args)
{
  PyObject *it0, *it1;
  int wait_flag = 0;

  if (!PyArg_ParseTuple(args, "OO|i", &it0, &it1, &wait_flag))
    return NULL;

  PyObject* n = PyUnicode_FromObject(it0);
  if (!n)
    return NULL;
  
  TPtrC name(PyUnicode_AsUnicode(n), PyUnicode_GetSize(n));
  TParse p;
  p.Set(name, NULL, NULL);
  
#if defined(__WINS__)
  if (!(p.Ext().CompareF(_L(".dll")) == 0)) {
#else
  if (!(p.Ext().CompareF(_L(".exe")) == 0)) {
#endif
    Py_DECREF(n);
    PyErr_SetString(PyExc_TypeError, "Executable expected");
    return NULL;
  }
  
  PyObject* a = PyUnicode_FromObject(it1);
  if (!a) {
    Py_DECREF(n);
    return NULL;
  }

  TInt error = ProcessLaunch(name,
                             TPtrC(PyUnicode_AsUnicode(a),
                                   PyUnicode_GetSize(a)),
                             wait_flag);

  Py_DECREF(n);
  Py_DECREF(a);
  
  if (wait_flag && (error >= 0))
    return Py_BuildValue("i", error);
  else
    RETURN_ERROR_OR_PYNONE(error);
}

/*
 *
 * Implementation of e32.drive_list()
 *
 */

extern "C" PyObject *
e32_drive_list(PyObject* /*self*/)
{
  TInt error;
  RFs rfs;
  
  if ((error = rfs.Connect()) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  PyObject* r;
  TDriveList l;

  error = rfs.DriveList(l);
  
  if (error == KErrNone) {
    if (r = PyList_New(0)) {
      for (int i = 0; i < KMaxDrives; i++) {
        if (l[i]) {
          char d[2];
          d[0] = 'A'+i; d[1] = ':';
          
          PyObject* v = PyUnicode_Decode(d, 2, NULL, NULL); 
          if ((v == NULL) || (PyList_Append(r, v) != 0)) {
            Py_XDECREF(v);
            Py_DECREF(r);
            r = NULL;
            break;
          }
          Py_DECREF(v);
        }
      }
    }
  }
  else
    r = SPyErr_SetFromSymbianOSErr(error);

  rfs.Close();

  return r;
}

/*
 *
 * Implementation of e32.file_copy()
 *
 */

extern "C" PyObject *
e32_file_copy(PyObject* /*self*/, PyObject* args)
{
  PyObject *it0, *it1;

  if (!PyArg_ParseTuple(args, "OO", &it0, &it1))
    return NULL;

  PyObject *t, *s;

  if (!((t = PyUnicode_FromObject(it0)) &&
        (s = PyUnicode_FromObject(it1)))) {
    Py_XDECREF(t);
    return NULL;
  }

  TPtrC target(PyUnicode_AsUnicode(t), PyUnicode_GetSize(t));
  TPtrC source(PyUnicode_AsUnicode(s), PyUnicode_GetSize(s));
  
  TInt error;
  RFs rfs;
  
  if ((error = rfs.Connect()) == KErrNone) {
    error = BaflUtils::CopyFile(rfs, source, target);
    rfs.Close();
  }

  Py_DECREF(t); Py_DECREF(s);

  RETURN_ERROR_OR_PYNONE(error);
}

/*
 *
 * Implementation of e32.Ao_lock
 *
 */

#define Ao_lock_type ((PYTHON_GLOBALS->tobj).t_Ao)

class Ao_lock : public CActive {
public:
  Ao_lock();
  TInt Signal(TUint aTid);
  void Wait();
private:
  void RunL();
  void DoCancel() {;}
  TRequestStatus* iPst;
#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
};

Ao_lock::Ao_lock():CActive(0)
{
  iStatus = KErrCancel;
  iPst = &iStatus;
  CActiveScheduler::Add(this);
}

void Ao_lock::Wait()
{
  if (iStatus != KErrNone) {
    iStatus = KRequestPending;
    iPst = &iStatus;
    SetActive();

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT   
    iWait.Start();
#else
    CActiveScheduler::Start();
#endif
    Py_END_ALLOW_THREADS
  }
  iStatus = KErrCancel;
  return;
}

TInt Ao_lock::Signal(TUint aTid)
{
  TInt error = KErrNone;

  if (iStatus != KRequestPending) {
    iStatus = KErrNone;
    return error;
  }
  RThread t;
  error = t.Open(aTid);
  if (error == KErrNone) {
    iStatus = KErrNone;
    t.RequestComplete(iPst, 0);
    t.Close();
  }
  return error;
}

void Ao_lock::RunL()
{
#ifdef HAVE_ACTIVESCHEDULERWAIT
  iWait.AsyncStop();
#else
  CActiveScheduler::Stop();
#endif
}

struct Ao_lock_object {
  PyObject_VAR_HEAD
  Ao_lock* ob_data;
  unsigned int ob_tid;
};

extern "C" PyObject *
new_e32_ao_object(PyObject* /*self*/)
{
  if (!CActiveScheduler::Current()) {
    PyErr_SetString(PyExc_AssertionError, "no ao scheduler");
    return NULL;
  }

  Ao_lock_object *op = PyObject_New(Ao_lock_object, &Ao_lock_type);
  if (op == NULL)
    return PyErr_NoMemory();

  op->ob_data = new Ao_lock();
  if (op->ob_data == NULL) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }
  op->ob_tid = RThread().Id();
  return (PyObject *) op;
}

extern "C" PyObject *
ao_wait(Ao_lock_object *self, PyObject* /*args*/)
{
  if ((TUint)RThread().Id() != self->ob_tid) {
    PyErr_SetString(PyExc_AssertionError,
                    "Ao_lock.wait must be called from lock creator thread");
    return NULL;
  }
  if (self->ob_data->iStatus == KRequestPending) {
    PyErr_SetString(PyExc_AssertionError, "wait() called on Ao_lock while another wait() on the same lock is in progress");
    return NULL;
  }
  self->ob_data->Wait(); 
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
ao_signal(Ao_lock_object *self, PyObject* /*args*/)
{
  TInt error = self->ob_data->Signal(self->ob_tid);
  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" {

  static void
  ao_dealloc(Ao_lock_object *op)
  {
    if ((TUint)RThread().Id() == op->ob_tid) {
      delete op->ob_data;
      op->ob_data = NULL;
    }
    PyObject_Del(op);
  }
  
  static const PyMethodDef ao_methods[] = {
    {"wait", (PyCFunction)ao_wait, METH_NOARGS},
    {"signal", (PyCFunction)ao_signal, METH_NOARGS},
    {NULL,              NULL}           /* sentinel */
  };

  static PyObject *
  ao_getattr(Ao_lock_object *p, char *name)
  {
    return Py_FindMethod((PyMethodDef*)ao_methods,
                         (PyObject *)p, name);
  }

  static const PyTypeObject c_Ao_lock_type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "e32.Ao_lock",
    sizeof(Ao_lock_object),
    0,
    /* methods */
    (destructor)ao_dealloc,             /* tp_dealloc */
    0,                                  /* tp_print */
    (getattrfunc)ao_getattr,            /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as _number*/
    0,                                  /* tp_as _sequence*/
    0,                                  /* tp_as _mapping*/
    0,                                  /* tp_hash */
  };
} /* extern "C" */

/*
 *
 * Implementation of e32.ao_yield
 *
 */

class CE32AoYield : public CActive
{
 public:
  CE32AoYield():CActive(EPriorityStandard) {
    CActiveScheduler::Add(this);
  }
  
  void DoYield() {
    iStatus = KRequestPending;
    SetActive();
    iStatus = KErrNone;
    TRequestStatus* pstatus = &iStatus;
    RThread().RequestComplete(pstatus, 0);
#ifdef HAVE_ACTIVESCHEDULERWAIT   
    iWait.Start();
#else
    CActiveScheduler::Start();
#endif
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

#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
};

extern "C" PyObject *
e32_ao_yield(PyObject* /*self*/)
{
  CE32AoYield* y = new CE32AoYield();
  if (!y)
    return PyErr_NoMemory();
  
  Py_BEGIN_ALLOW_THREADS
  y->DoYield();
  delete y;
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 *
 * Implementation of e32.ao_sleep
 *
 */

class CE32AoSleep : public CActive
{
 public:
  CE32AoSleep(PyObject* aCb=0):CActive(EPriorityStandard) {
    iCb = aCb;
    Py_XINCREF(iCb);
  }
  ~CE32AoSleep() {
    Cancel();
    iTimer.Close();
    Py_XDECREF(iCb);
  }

  TInt Construct() {
    TInt error = iTimer.CreateLocal();
    if (error == KErrNone)
      CActiveScheduler::Add(this);
    return error;
  }

  void DoSleep(TTimeIntervalMicroSeconds32 aDelay) {
    iTimer.After(iStatus, aDelay);
    SetActive();
    if (!iCb) {
      Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT   
      iWait.Start();
#else
      CActiveScheduler::Start();
#endif
      Py_END_ALLOW_THREADS
      delete this;
    }
  }

 private:
  void DoCancel() {
    iTimer.Cancel();
  }
  void RunL() {
    if (!iCb) {
#ifdef HAVE_ACTIVESCHEDULERWAIT
      iWait.AsyncStop();
#else
      CActiveScheduler::Stop();
#endif
    }
    else {
      PyEval_RestoreThread(PYTHON_TLS->thread_state);
      PyObject* tmp_r = NULL;
      tmp_r = PyEval_CallObject(iCb, NULL);
      Py_XDECREF(tmp_r);
      if (PyErr_Occurred())
	PyErr_Print();
      PyEval_SaveThread();
      delete this;
    }
  }

  RTimer iTimer;
#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
  PyObject* iCb;
};

extern "C" PyObject *
e32_ao_sleep(PyObject* /*self*/, PyObject* args)
{
  TReal d;
  PyObject* c=NULL;

  if (!PyArg_ParseTuple(args, "d|O", &d, &c))
    return NULL;

  if (c && !PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected for 2nd argument");
    return NULL;
  }

  if (d < 0) {
    PyErr_SetString(PyExc_RuntimeError, "negative number not allowed");
    return NULL;
  }

  CE32AoSleep* s = new CE32AoSleep(c);
  if (!s)
    return PyErr_NoMemory();

  TInt error = s->Construct();
  if (error != KErrNone) {
    delete s;
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  s->DoSleep(TTimeIntervalMicroSeconds32(TInt64(d*1000*1000).GetTInt()));

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 *
 * Implementation of e32.Ao_timer
 *
 */

#define Ao_timer_type (PYTHON_GLOBALS->t_Ao_timer)

class Ao_timer : public CActive {
public:
  Ao_timer();
  void After(TTimeIntervalMicroSeconds32 aSleep, PyObject* aCb);
  void Cancel();
  TInt Construct();
  ~Ao_timer();
private:
  void RunL();
  void DoCancel();
  RTimer iTimer;
  PyObject* iCb;
#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
};

Ao_timer::Ao_timer():CActive(EPriorityStandard) {;}

void Ao_timer::After(TTimeIntervalMicroSeconds32 aDelay, PyObject* aCb=0)
{
  iCb = aCb;
  Py_XINCREF(iCb);
  
  iTimer.After(iStatus, aDelay);
  SetActive();
  if (!iCb) {
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT   
    iWait.Start();
#else
    CActiveScheduler::Start();
#endif
    Py_END_ALLOW_THREADS
  }
}

void Ao_timer::Cancel()
{
  if (IsActive()) {
    iTimer.Cancel();
  }
}

TInt Ao_timer::Construct() 
{
  TInt error = iTimer.CreateLocal();
  if (error == KErrNone)
    CActiveScheduler::Add(this);
  return error;
}

Ao_timer::~Ao_timer()
{
  Cancel();
  iTimer.Close();
  Py_XDECREF(iCb);  
}

void Ao_timer::RunL()
{
    if (!iCb) {
#ifdef HAVE_ACTIVESCHEDULERWAIT
      iWait.AsyncStop();
#else
      CActiveScheduler::Stop();
#endif
    }
    else {
    PyEval_RestoreThread(PYTHON_TLS->thread_state);
    PyObject* tmp_r = NULL;
    tmp_r = PyEval_CallObject(iCb, NULL);
    Py_XDECREF(tmp_r);
    if (PyErr_Occurred())
	    PyErr_Print();
    PyEval_SaveThread();
  }
}

void Ao_timer::DoCancel() 
{
  iTimer.Cancel();
}

struct Ao_timer_object {
  PyObject_VAR_HEAD
  Ao_timer* ob_data;
};

extern "C" PyObject *
new_e32_ao_timer_object(PyObject* /*self*/, PyObject /**args*/)
{
  if (!CActiveScheduler::Current()) {
    PyErr_SetString(PyExc_AssertionError, "no ao scheduler");
    return NULL;
  }

  Ao_timer_object *op = PyObject_New(Ao_timer_object, &Ao_timer_type);
  if (op == NULL)
    return PyErr_NoMemory();

  op->ob_data = new Ao_timer();
  if (op->ob_data == NULL) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }

  TInt error = op->ob_data->Construct();
  if (error != KErrNone) {
    PyObject_Del(op);
    return SPyErr_SetFromSymbianOSErr(error);
  }  
  
  return (PyObject *) op;
}

extern "C" PyObject *
ao_timer_after(Ao_timer_object *self, PyObject* args)
{
  TReal d;
  PyObject* c=NULL;

  if (!PyArg_ParseTuple(args, "d|O", &d, &c))
    return NULL;

  if (c && !PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected for 2nd argument");
    return NULL;
  }
  
  if(self->ob_data->IsActive()) {
    PyErr_SetString(PyExc_RuntimeError, "Timer pending - cancel first");
    return NULL;  
  }
  
  if (d < 0) {
    PyErr_SetString(PyExc_RuntimeError, "negative number not allowed");
    return NULL;
  }
  
  self->ob_data->After(TTimeIntervalMicroSeconds32(TInt64(d*1000*1000).GetTInt()), c);

  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
ao_timer_cancel(Ao_timer_object *self, PyObject* /*args*/)
{
  self->ob_data->Cancel();
  
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" {

  static void
  ao_timer_dealloc(Ao_timer_object *op)
  {
    delete op->ob_data;
    op->ob_data = NULL;

    PyObject_Del(op);
  }
  
  static const PyMethodDef ao_timer_methods[] = {
    {"after", (PyCFunction)ao_timer_after, METH_VARARGS},
    {"cancel", (PyCFunction)ao_timer_cancel, METH_NOARGS},
    {NULL,              NULL}           /* sentinel */
  };

  static PyObject *
  ao_timer_getattr(Ao_timer_object *p, char *name)
  {
    return Py_FindMethod((PyMethodDef*)ao_timer_methods,
                         (PyObject *)p, name);
  }

  static const PyTypeObject c_Ao_timer_type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "e32.Ao_timer",
    sizeof(Ao_timer_object),
    0,
    /* methods */
    (destructor)ao_timer_dealloc,       /* tp_dealloc */
    0,                                  /* tp_print */
    (getattrfunc)ao_timer_getattr,      /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as _number*/
    0,                                  /* tp_as _sequence*/
    0,                                  /* tp_as _mapping*/
    0,                                  /* tp_hash */
  };
} /* extern "C" */


/*
 *
 * Implementation of e32.ao_callgate
 *
 */

#define Ao_callgate_type ((PYTHON_GLOBALS->tobj).t_Ao_callgate)


struct Ao_callgate_call_item {
  PyObject* arg;
  Ao_callgate_call_item* next;
};

class Ao_callgate;

struct Ao_callgate_object {
  PyObject_VAR_HEAD
  Ao_callgate* ob_data;
  unsigned int ob_tid;
  Ao_callgate_call_item ob_args;
};

class Ao_callgate : public CActive {
public:
  Ao_callgate(Ao_callgate_object* aOb, PyObject* aCb, TInt aTid);
  ~Ao_callgate() {
    Cancel();
    Py_DECREF(iCb);
  }
  TInt Signal(TInt aStatus=KErrNone);
private:
  void RunL();
  void DoCancel() {
    Signal(KErrCancel);
  }
  TRequestStatus* iPst;
  PyObject* iCb;
  Ao_callgate_object* iOb;
  TInt iTid;
};

Ao_callgate::Ao_callgate(Ao_callgate_object* aOb, PyObject* aCb, TInt aTid):
  CActive(0),iTid(aTid),iOb(aOb)
{
  iCb = aCb;
  Py_INCREF(iCb);
  CActiveScheduler::Add(this);
  iStatus = KRequestPending;
  iPst = &iStatus;
  SetActive();
}

TInt Ao_callgate::Signal(TInt aStatus)
{
  if (iStatus != KRequestPending)
    return KErrNone;

  RThread t;
  TInt error = t.Open(iTid);
  if (error == KErrNone) {
    iStatus = aStatus;
    t.RequestComplete(iPst, 0);
    t.Close();
  }
  return error;
}

void Ao_callgate::RunL()
{
  PyEval_RestoreThread(PYTHON_TLS->thread_state);
  Ao_callgate_call_item* p = iOb->ob_args.next;
  iOb->ob_args.next = NULL;
  while (p) {
    PyObject* tmp_r = NULL;
    tmp_r = PyEval_CallObject(iCb, p->arg);
    Py_XDECREF(tmp_r);
    if (PyErr_Occurred())
      PyErr_Print();
    Ao_callgate_call_item* tmp = p->next;
    Py_XDECREF(p->arg);
    PyMem_Free(p);
    p = tmp;
  }
  PyEval_SaveThread();
  iStatus = KRequestPending;
  iPst = &iStatus;
  SetActive();
}

extern "C" PyObject *
e32_ao_callgate(PyObject* /*self*/, PyObject* args)
{
  if (!CActiveScheduler::Current()) {
    PyErr_SetString(PyExc_AssertionError, "no ao scheduler");
    return NULL;
  }

  PyObject* c;
  if (!PyArg_ParseTuple(args, "O", &c) || !PyCallable_Check(c))
    return NULL;

  Ao_callgate_object *op = PyObject_New(Ao_callgate_object,
                                        &Ao_callgate_type);
  if (op == NULL)
    return PyErr_NoMemory();

  op->ob_args.arg = NULL;
  op->ob_args.next = NULL;
  op->ob_tid = RThread().Id();
  if (!(op->ob_data = new Ao_callgate(op, c, op->ob_tid))) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }
  return (PyObject *) op;
}

extern "C" PyObject *
ao_cg_call(PyObject* self, PyObject* arg, PyObject* /*kw*/)
{
  Ao_callgate_object *op = (Ao_callgate_object*)self;
  Ao_callgate_call_item* n =
    (Ao_callgate_call_item*)PyMem_Malloc(sizeof(Ao_callgate_call_item));
  if (!n)
    return PyErr_NoMemory();
  n->arg = arg;
  Py_XINCREF(arg);
  n->next = NULL;

  Ao_callgate_call_item* p = &(op->ob_args);
  while (p->next) p = p->next;
  p->next = n;
  
  TInt error = op->ob_data->Signal();
  if (error != KErrNone) {
    p->next = NULL;
    Py_XDECREF(n->arg);
    PyMem_Free(n);
  }
  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" {

  static void
  ao_cg_dealloc(Ao_callgate_object *op)
  {
    if ((TUint)RThread().Id() == op->ob_tid) {
      delete op->ob_data;
      op->ob_data = NULL;
    }
    Ao_callgate_call_item* p = op->ob_args.next;
    while (p) {
      Ao_callgate_call_item* tmp = p->next;
      Py_XDECREF(p->arg);
      PyMem_Free(p);
      p = tmp;
    }
    PyObject_Del(op);
  }

  static const PyTypeObject c_Ao_callgate_type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "e32.Ao_callgate",
    sizeof(Ao_callgate_object),
    0,
    /* methods */
    (destructor)ao_cg_dealloc,          /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as _number*/
    0,                                  /* tp_as _sequence*/
    0,                                  /* tp_as _mapping*/
    0,                                  /* tp_hash */
    ao_cg_call,                         /* tp_call */
  };
} /* extern "C" */

/*
 *
 * Implementation of e32._as_level
 *
 */

class CMyAccessor : public CActiveScheduler
{
public:
  TInt MyLevel() {return Level();}
};

extern "C" PyObject *
e32__as_level(PyObject* /*self*/, PyObject* /*args*/)
{
  TInt l =
    STATIC_CAST(CMyAccessor*, CActiveScheduler::Current())->MyLevel();
  return Py_BuildValue("i", l);
}

/*
 * This does not work on target (!?)
 *
 * extern "C" double
 * e32_clock()
 * {
 *   TTimeIntervalMicroSeconds cpu_time(0.0);
 *
 *   RThread().GetCpuTime(cpu_time);
 *
 *   return ((cpu_time.Int64().GetTReal()) / (1000.0 * 1000.0));
 * }
 */

extern "C" double
e32_clock()
{
  TInt period = 1000*1000;
  
  HAL::Get(HALData::ESystemTickPeriod, period);
  
  return (((double)(((double)User::TickCount())*
                    (((double)period)/1000.0)))/1000.0);
}

extern "C" double
e32_UTC_offset()
{
  TLocale loc;
  return TInt64(loc.UniversalTimeOffset().Int()).GetTReal();
}

extern "C" int
e32_check_stack()
{
  TInt heap_sz, stack_sz;
  
  if (RThread().GetRamSizes(heap_sz, stack_sz) == KErrNone) {
    if (((TUint)&heap_sz-(TUint)((RThread().Heap())->Base()-stack_sz)) > 0x800)
      return 0;
  }
  else
    return 0;
  
  return (-1);
}

/*
 *
 * Implementation of e32.is_ui_thread
 *
 */

static TBool is_main_thread()
{
  return ((PYTHON_GLOBALS->main_thread == 0) ||
          (PYTHON_GLOBALS->main_thread == PyThread_get_thread_ident()));
}

extern "C" PyObject *
e32_is_ui_thread(PyObject* /*self*/, PyObject* /*args*/)
{
  PyObject* rval = (is_main_thread() ? Py_True : Py_False);
  Py_INCREF(rval);
  return rval;
}

/*
 *
 * Implementation of e32.in_emulator
 *
 */

extern "C" PyObject *
e32_in_emulator(PyObject* /*self*/, PyObject* /*args*/)
{
#ifdef __WINS__
  PyObject* rval = Py_True;
#else
  PyObject* rval = Py_False;
#endif
  Py_INCREF(rval);
  return rval;
}

/*
 *
 * Implementation of e32.set_home_time
 *
 */

/*
 * (copied from appuifw).
 */
static TReal epoch_as_TReal()
{
  _LIT(KAppuifwEpoch, "19700000:");
  TTime epoch;
  epoch.Set(KAppuifwEpoch);
  return epoch.Int64().GetTReal();
}

/*
 * Converts TReal time representation to TTime value.
 */
void pythonRealAsTTime(TReal timeValue, TTime& theTime)
{
  TLocale loc;
  TInt64 timeInt((timeValue + 
                 TInt64(loc.UniversalTimeOffset().Int()).GetTReal())
                 *(1000.0*1000.0)+epoch_as_TReal());
  theTime = timeInt;
}

extern "C" PyObject *
e32_set_home_time(PyObject* /*self*/, PyObject* args)
{
  TInt error = KErrNone;
  TReal time = 0;
  if (!PyArg_ParseTuple(args, "d", &time)){ 
    return NULL;
  }

  TTime conv_time;
  pythonRealAsTTime(time, conv_time);

  error = User::SetHomeTime(conv_time);

  RETURN_ERROR_OR_PYNONE(error);
}


#ifdef USE_GLOBAL_DATA_HACK
extern "C" PyObject *
e32_globcnt(PyObject* /*self*/, PyObject* /*args*/)
{
  PyObject* rval = Py_BuildValue("i", *(PYTHON_GLOBALS->globptr));
  return rval;
}
#endif

/*
 *
 * Implementation of e32._uidcrc_app
 *
 */

class TMyCheckedAppUid : public TCheckedUid
{
public:
  TMyCheckedAppUid(TUint uid):
    TCheckedUid(TUidType(TUid::Uid(KDynamicLibraryUidValue),
                         TUid::Uid(KAppUidValue16),
                         TUid::Uid(uid))) {;}
  TUint CheckSum() {return Check();}
};

extern "C" PyObject *
e32_uidcrc_app(PyObject* /*self*/, PyObject *args)
{
  TUint uid;
  
  if (!PyArg_ParseTuple(args, "i", &uid))
    return NULL;
  
  TMyCheckedAppUid chk(uid);

  return Py_BuildValue("l", chk.CheckSum());
}

static int call_stdo(const char *buf, int n)
{
  PyObject* args = Py_BuildValue("(N)",
                                 PyUnicode_Decode(buf, n, NULL, NULL));
  PyObject* rval =
    PyEval_CallObject(SPyGetGlobalString("e32_stdo"), args);
  Py_XDECREF(args);
  Py_XDECREF(rval);
  return n;
}

static int print_stdo(const char *buf, int n)
{
  RFs rfs;
  
  if (rfs.Connect() == KErrNone) {
    RFile f;
    SPy_Python_globals* pg = PYTHON_GLOBALS;
    TPtrC fn((TUint16*)pg->stdo_buf, pg->stdo_buf_len);

    TInt error = f.Open(rfs, fn, EFileWrite);
    if (error == KErrNotFound)
      error = f.Create(rfs, fn, EFileWrite);
    
    TInt dummy;
    if ((error == KErrNone) && (f.Seek(ESeekEnd, dummy) == KErrNone))
      f.Write(TPtrC8((const TUint8*)buf, n));
    
    f.Close();
    rfs.Close();
  }

  return n;
}

extern "C" PyObject *
e32_stdo(PyObject* /*self*/, PyObject* args)
{
  PyObject* c;

  if (!PyArg_ParseTuple(args, "O", &c))
    return NULL;

  SPy_Python_globals* pg = PYTHON_GLOBALS;

  if (PyCallable_Check(c)) {
    if (SPyAddGlobalString("e32_stdo", c))
      return NULL;
    ((CSPyInterpreter*)pg->interpreter)->iStdO = &call_stdo;
  }
  else if (PyUnicode_Check(c)) {
    TPtr buf((TUint16*)pg->stdo_buf, 0x100);
    buf.Copy(PyUnicode_AsUnicode(c), PyUnicode_GetSize(c));
    pg->stdo_buf_len = PyUnicode_GetSize(c);
    ((CSPyInterpreter*)pg->interpreter)->iStdO = &print_stdo;
  }
  else if (c == Py_None) {
    ((CSPyInterpreter*)pg->interpreter)->iStdO = NULL;
  }
  else {
    PyErr_SetString(PyExc_TypeError, "callable or unicode expected");
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
e32_mem_info(PyObject* /*self*/)
{
  TInt error;
  TInt heap_sz, stack_sz, current_heap_usage,
    max_stack_usage, current_pyheap_usage;
  
  error = RThread().GetRamSizes(heap_sz, stack_sz);
  
  if (error == KErrNone) {
    (RThread().Heap())->AllocSize(current_heap_usage);
    ((CSPyInterpreter*)PYTHON_GLOBALS->interpreter)->
      iPyheap->AllocSize(current_pyheap_usage);
  }

  if (error == KErrNone) {
    TUint8* pbase = (RThread().Heap())->Base();
    TUint8* pstacklast = pbase - stack_sz;
    while (*pstacklast == 0x29)
      pstacklast++;
    max_stack_usage = pbase - pstacklast;
  }
  
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
  else
    return Py_BuildValue("(iiiii)", heap_sz, stack_sz,
                         current_heap_usage, max_stack_usage,
                         current_pyheap_usage);
}

extern "C" PyObject *
e32_strerror(PyObject* /*self*/, PyObject* args)
{
  int errcode;

  if (!PyArg_ParseTuple(args, "i", &errcode))
    return NULL;

  return SPyErr_SymbianOSErrAsString(errcode);
}

extern "C" {

  static const PyMethodDef e32_methods[] = {
    {"Ao_lock", (PyCFunction)new_e32_ao_object, METH_NOARGS, NULL},
    {"Ao_timer", (PyCFunction)new_e32_ao_timer_object, METH_NOARGS, NULL},
    {"ao_yield", (PyCFunction)e32_ao_yield, METH_NOARGS, NULL},
    {"ao_sleep", (PyCFunction)e32_ao_sleep, METH_VARARGS, NULL},
    {"ao_callgate", (PyCFunction)e32_ao_callgate, METH_VARARGS, NULL},
    {"_as_level", (PyCFunction)e32__as_level, METH_NOARGS, NULL},
    {"start_server", (PyCFunction)e32_start_server, METH_VARARGS, NULL},
    {"start_exe", (PyCFunction)e32_start_exe, METH_VARARGS, NULL},
    {"drive_list", (PyCFunction)e32_drive_list, METH_NOARGS, NULL},
    {"file_copy", (PyCFunction)e32_file_copy, METH_VARARGS, NULL},
    {"is_ui_thread", (PyCFunction)e32_is_ui_thread, METH_NOARGS, NULL},
    {"in_emulator", (PyCFunction)e32_in_emulator, METH_NOARGS, NULL},
    {"set_home_time", (PyCFunction)e32_set_home_time, METH_VARARGS, NULL},
    {"_uidcrc_app", (PyCFunction)e32_uidcrc_app, METH_VARARGS, NULL},
    {"_stdo", (PyCFunction)e32_stdo, METH_VARARGS, NULL},
    {"_mem_info", (PyCFunction)e32_mem_info, METH_NOARGS, NULL},
    {"strerror", (PyCFunction)e32_strerror, METH_VARARGS, NULL},
#ifdef USE_GLOBAL_DATA_HACK
    {"_globcnt", (PyCFunction)e32_globcnt, METH_NOARGS, NULL},
#endif
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) inite32(void)
  {
    PyObject *m, *d;

    Ao_lock_type = c_Ao_lock_type;
    Ao_lock_type.ob_type = &PyType_Type;
    
    Ao_timer_type = c_Ao_timer_type;
    Ao_timer_type.ob_type = &PyType_Type;
    
    Ao_callgate_type = c_Ao_callgate_type;
    Ao_callgate_type.ob_type = &PyType_Type;

    m = Py_InitModule("e32", (PyMethodDef*)e32_methods);
    d = PyModule_GetDict(m);
    
    PyDict_SetItemString(d,"pys60_version_info", 
      Py_BuildValue("(iiisi)", PYS60_VERSION_MAJOR, PYS60_VERSION_MINOR, PYS60_VERSION_MICRO, 
                               PYS60_VERSION_TAG, PYS60_VERSION_SERIAL));
    PyDict_SetItemString(d,"pys60_version", Py_BuildValue("s", PYS60_VERSION_STRING));
    PyDict_SetItemString(d,"s60_version_info", Py_BuildValue("(ii)", 
#if SERIES60_VERSION==12
    1, 2
#endif
#if SERIES60_VERSION==20
    2, 0
#endif
#if SERIES60_VERSION==26
    2, 6
#endif
#if SERIES60_VERSION==28
    2, 8
#endif
    ));    
  }
} /* extern "C" */
