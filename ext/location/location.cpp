/**
 * ====================================================================
 *  location.cpp
 *
 *  Python API to location information.
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

#include "Python.h"
#include "symbian_python_ext_util.h"

#include <eikenv.h>
#include <e32std.h>

#include <etelbgsm.h>


_LIT (KTsyName, "phonetsy.tsy");

extern "C" PyObject *
get_location(PyObject* /*self*/)
{
  TInt error = KErrNone;

  TInt enumphone = 1;
  RTelServer	 server;
  RBasicGsmPhone phone;
  RTelServer::TPhoneInfo info;
  MBasicGsmPhoneNetwork::TCurrentNetworkInfo NetworkInfo;

  error = server.Connect();
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = server.LoadPhoneModule(KTsyName);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = server.EnumeratePhones(enumphone);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  if (enumphone < 1)
    return SPyErr_SetFromSymbianOSErr(KErrNotFound);

  error = server.GetPhoneInfo(0, info);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = phone.Open(server, info.iName);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = phone.GetCurrentNetworkInfo(NetworkInfo);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
		       
  phone.Close();
  server.Close();

  return Py_BuildValue("(iiii)", NetworkInfo.iNetworkInfo.iId.iMCC,
		       NetworkInfo.iNetworkInfo.iId.iMNC,
		       NetworkInfo.iLocationAreaCode,
		       NetworkInfo.iCellId);
}



extern "C" {

  static const PyMethodDef location_methods[] = {
    {"gsm_location", (PyCFunction)get_location, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initlocation(void)
  {
    PyObject *m;

    m = Py_InitModule("location", (PyMethodDef*)location_methods);
    return;
  }
} /* extern "C" */

GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
