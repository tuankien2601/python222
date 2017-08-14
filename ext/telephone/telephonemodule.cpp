/**
 * ====================================================================
 * telephonemodule.cpp
 *
 * Python API to telephone, modified from the "dialer" example 
 * available from the Series 60 SDK
 *
 * Implements currently following Python type:
 * 
 * Phone
 *
 *    open()
 *      opens the line. MUST be called prior dial().  
 *    
 *    close()
 * 
 *    set_number(str_number)
 *      sets the number to call.
 *
 *    dial()
 *      dials the number set.
 *      
 *    hang_up()
 *      hangs up if call in process. "SymbianError: KErrNotReady" returned
 *      if this call is already finished.
 *
 * If this extension is used in emulator nothing happens. Notice that since
 * the user of the device can also hang-up the phone explicitly s/he might 
 * affect the current status of the call.
 *
 * Calling close() should be safe even if there is a call ongoing.
 *
 * If there is a phone call already going on prior to calling dial() from
 * Python then the earlier call is put on on hold and the new call established.
 *
 * Calling multiple times dial() where e.g. the first call is answered and a
 * line is established results in subsequent calls doing nothing.
 *
 * TODO continue with the call back (it is commented out currently)
 * TODO focus does not return to Python automatically when hangup occurs
 *      in the middle of phone call
 * TODO speaker on/off could be given as call parameters
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

#include "telephone.h"

//////////////TYPE METHODS

/*
 * Deallocate phone.
 */
extern "C" {

  static void phone_dealloc(PHO_object *phoo)
  {
    if (phoo->phone) {
      delete phoo->phone;
      phoo->phone = NULL;
    }
    /*if (phoo->callBackSet) {
      //XXX is this ok:
      Py_XDECREF(phoo->myCallBack.iCb);
    }*/
    PyObject_Del(phoo);
  }
  
}

/*
 * Allocate phone.
 */
extern "C" PyObject *
new_phone_object(PyObject* /*self*/, PyObject /**args*/)
{
  PHO_object *phoo = PyObject_New(PHO_object, PHO_type);
  if (phoo == NULL)
    return PyErr_NoMemory();

  TRAPD(error, phoo->phone = CPhoneCall::NewL());
  if (phoo->phone == NULL) {
    PyObject_Del(phoo);
    return PyErr_NoMemory();
  }

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  //phoo->callBackSet = EFalse;

  return (PyObject*) phoo;
}

/*
 * Dial.
 */
extern "C" PyObject *
phone_dial(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;
  
  Py_BEGIN_ALLOW_THREADS
  error = self->phone->Dial();
  Py_END_ALLOW_THREADS

  if(error != KErrNone){
    char* err_string;
    switch (error) {
      case CPhoneCall::ENumberNotSet:
        err_string = "number not set"; 
        break;
      case CPhoneCall::ENotInitialised:
        err_string = "open() not called";
        break;
      case CPhoneCall::EAlreadyCalling:
        err_string = "call in progress, hang up first";
        break;
      default:
        return SPyErr_SetFromSymbianOSErr(error);
    }  
    PyErr_SetString(PyExc_RuntimeError, err_string);
    return NULL;  
  }
    
  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Set number.
 */
extern "C" PyObject *
phone_set_number(PHO_object* self, PyObject *args)
{
  //Parse the passed telephone number (modified from "messagingmodule.cpp"):
  char* number;
  int number_l;

  if (!PyArg_ParseTuple(args, "s#", &number, &number_l))  
    return NULL;
  
  if ((number_l > MaxTelephoneNumberLength)) {
    PyErr_BadArgument();
    return NULL;
  }

  TBuf<MaxTelephoneNumberLength> tel_number;
  tel_number.FillZ(MaxTelephoneNumberLength);
  tel_number.Copy(TPtrC8((TUint8*)number, number_l));

  self->phone->SetNumber(tel_number);

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Initialise.
 */
extern "C" PyObject *
phone_open(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;
  
  Py_BEGIN_ALLOW_THREADS
  error = self->phone->Initialise();
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    if (error == CPhoneCall::EInitialiseCalledAlready) {
      PyErr_SetString(PyExc_RuntimeError, "open() already called");
      return NULL;  
    }
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Close.
 */
extern "C" PyObject *
phone_close(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;

  Py_BEGIN_ALLOW_THREADS
  error = self->phone->UnInitialise();
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    char* err_string;
    switch (error) {
      case CPhoneCall::ENotInitialised:
        err_string = "open() not called"; 
        break;
      case CPhoneCall::EAlreadyCalling:
        err_string = "call in progress, hang up first";
        break;
      default:
        return SPyErr_SetFromSymbianOSErr(error);
    }  
    PyErr_SetString(PyExc_RuntimeError, err_string);
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Hang up.
 */
extern "C" PyObject *
phone_hang_up(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;

  Py_BEGIN_ALLOW_THREADS
  error = self->phone->HangUp();
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    char* err_string;
    switch (error) {
      case CPhoneCall::ENotCallInProgress:
        err_string = "no call to hang up"; 
        break;
      default:
        return SPyErr_SetFromSymbianOSErr(error);
    }  
    PyErr_SetString(PyExc_RuntimeError, err_string);
    return NULL;  
  }

  Py_INCREF(Py_None);
  return Py_None;
}

//////////////TYPE SET

extern "C" {

  const static PyMethodDef phone_methods[] = {
    {"dial", (PyCFunction)phone_dial, METH_NOARGS},
    {"set_number", (PyCFunction)phone_set_number, METH_VARARGS},
    {"open", (PyCFunction)phone_open, METH_NOARGS},
    {"close", (PyCFunction)phone_close, METH_NOARGS},
    {"hang_up", (PyCFunction)phone_hang_up, METH_NOARGS},
    {NULL, NULL}  
  };

  static PyObject *
  phone_getattr(PHO_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)phone_methods, (PyObject *)op, name);
  }

  static const PyTypeObject c_pho_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_telephone.Phone",                        /*tp_name*/
    sizeof(PHO_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)phone_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)phone_getattr,               /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };
} /* extern "C" */ 

//////////////INIT

extern "C" {
  
  static const PyMethodDef telephone_methods[] = {
    {"Phone", (PyCFunction)new_phone_object, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) inittelephone(void)
  {
    PyTypeObject* pho_type = PyObject_New(PyTypeObject, &PyType_Type);
    *pho_type = c_pho_type;
    pho_type->ob_type = &PyType_Type;

    SPyAddGlobalString("PHOType", (PyObject*)pho_type);

    PyObject *m/*, *d*/;

    m = Py_InitModule("_telephone", (PyMethodDef*)telephone_methods);
    //d = PyModule_GetDict(m);
  }
} /* extern "C" */  
  

GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
