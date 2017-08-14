/*
* ====================================================================
*  symbian_python_ext_util.cpp
*  
*  Utilities for Symbian OS specific Python extensions.
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

#include "symbian_python_ext_util.h"

extern "C" {

  PyObject *
  SPyErr_SymbianOSErrAsString(int err) 
  {
    char* err_string;
    char buffer[32];

    switch(err) {
    case 0:
      err_string = "KErrNone";
      break;
    case (-1):
      err_string = "KErrNotFound";
      break;
    case (-2):
      err_string = "KErrGeneral";
      break;
    case (-3):
      err_string = "KErrCancel";
      break;
    case (-4):
      err_string = "KErrNoMemory";
      break;
    case (-5):
      err_string = "KErrNotSupported";
      break;
    case (-6):
      err_string = "KErrArgument";
      break;
    case (-7):
      err_string = "KErrTotalLossOfPrecision";
      break;
    case (-8):
      err_string = "KErrBadHandle";
      break;
    case (-9):
      err_string = "KErrOverflow";
      break;
    case (-10):
      err_string = "KErrUnderflow";
      break;
    case (-11):
      err_string = "KErrAlreadyExists";
      break;
    case (-12):
      err_string = "KErrPathNotFound";
      break;
    case (-13):
      err_string = "KErrDied";
      break;
    case (-14):
      err_string = "KErrInUse";
      break;
    case (-15):
      err_string = "KErrServerTerminated";
      break;
    case (-16):
      err_string = "KErrServerBusy";
      break;
    case (-17):
      err_string = "KErrCompletion";
      break;
    case (-18):
      err_string = "KErrNotReady";
      break;
    case (-19):
      err_string = "KErrUnknown";
      break;
    case (-20):
      err_string = "KErrCorrupt";
      break;
    case (-21):
      err_string = "KErrAccessDenied";
      break;
    case (-22):
      err_string = "KErrLocked";
      break;
    case (-23):
      err_string = "KErrWrite";
      break;
    case (-24):
      err_string = "KErrDisMounted";
      break;
    case (-25):
      err_string = "KErrEof";
      break;
    case (-26):
      err_string = "KErrDiskFull";
      break;
    case (-27):
      err_string = "KErrBadDriver";
      break;
    case (-28):
      err_string = "KErrBadName";
      break;
    case (-29):
      err_string = "KErrCommsLineFail";
      break;
    case (-30):
      err_string = "KErrCommsFrame";
      break;
    case (-31):
      err_string = "KErrCommsOverrun";
      break;
    case (-32):
      err_string = "KErrCommsParity";
      break;
    case (-33):
      err_string = "KErrTimedOut";
      break;
    case (-34):
      err_string = "KErrCouldNotConnect";
      break;
    case (-35):
      err_string = "KErrCouldNotDisconnect";
      break;
    case (-36):
      err_string = "KErrDisconnected";
      break;
    case (-37):
      err_string = "KErrBadLibraryEntryPoint";
      break;
    case (-38):
      err_string = "KErrBadDescriptor";
      break;
    case (-39):
      err_string = "KErrAbort";
      break;
    case (-40):
      err_string = "KErrTooBig";
      break;
    case (-41):
      err_string = "KErrDivideByZero";
      break;
    case (-42):
      err_string = "KErrBadPower";
      break;
    case (-43):
      err_string = "KErrDirFull";
      break;
    case (-44):
      err_string = "KErrHardwareNotAvailable";
      break;
    case (-45):
      err_string = "KErrSessionClosed";
      break;
    case (-46):
      err_string = "KErrPermissionDenied";
      break;
    default:
      sprintf(buffer, "Error %d", err); 
      err_string = buffer;
      break;
    }
    return Py_BuildValue("s",err_string);
  }

  DL_EXPORT(PyObject *)
    SPyErr_SetFromSymbianOSErr(int err)
  {
    if (err == KErrPython) 
      return NULL;
    PyObject *err_string=SPyErr_SymbianOSErrAsString(err);
    if (!err_string)
      return NULL;
    PyObject *v=Py_BuildValue("(iO)", err, err_string);
    if (v != NULL) {
      PyErr_SetObject(PyExc_SymbianError, v);
      Py_DECREF(v);
      Py_DECREF(err_string);
    }
    return NULL;
  }

  /* rely on the separate Python dynamic memory pool for cleanup */

  DL_EXPORT(int) SPyAddGlobal(PyObject *key, PyObject *value)
  {
    if ((!(PYTHON_GLOBALS->global_dict)) &&
        (!(PYTHON_GLOBALS->global_dict = PyDict_New())))
      return (-1);
    
    return PyDict_SetItem(PYTHON_GLOBALS->global_dict, key, value);
  }

  DL_EXPORT(int) SPyAddGlobalString(char *key, PyObject *value)
  {
    if ((!(PYTHON_GLOBALS->global_dict)) &&
        (!(PYTHON_GLOBALS->global_dict = PyDict_New())))
      return (-1);
    
    return PyDict_SetItemString(PYTHON_GLOBALS->global_dict, key, value);
  }

  DL_EXPORT(PyObject *) SPyGetGlobal(PyObject *key)
  {
    return ((PYTHON_GLOBALS->global_dict) ?
            PyDict_GetItem(PYTHON_GLOBALS->global_dict, key) : NULL);
  }

  DL_EXPORT(PyObject *) SPyGetGlobalString(char *key)
  {
    return ((PYTHON_GLOBALS->global_dict) ?
            PyDict_GetItemString(PYTHON_GLOBALS->global_dict, key) : NULL);
  }

  DL_EXPORT(void) SPyRemoveGlobal(PyObject *key)
  {
    if (PYTHON_GLOBALS->global_dict)
      PyDict_DelItem(PYTHON_GLOBALS->global_dict, key);
  }

  DL_EXPORT(void) SPyRemoveGlobalString(char *key)
  {
    if (PYTHON_GLOBALS->global_dict)
      PyDict_DelItemString(PYTHON_GLOBALS->global_dict, key);
  }
}
