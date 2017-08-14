/**
 * ====================================================================
 *  sysinfomodule.cpp
 *
 *  Python API to system information, partly modified from SysInfo example 
 *  available from Forum Nokia and 
 *  http://www.newlc.com/article.php3?id_article=155
 *
 *  Implements currently following Python functions:
 * 
 *  tuple<int, int, int> os_version()
 *    major, minor, build
 * 
 *  unicode_string sw_version()
 *    hardcoded string "emulator" returned if in emulator
 *
 *  unicode_string imei()
 *    hardcoded string "000000000000000" returned if in emulator
 * 
 *  int battery()
 *    current battery level (0-7), returns always 0 if in emulator
 *
 *  int signal()
 *    current signal strength (0-7), returns always 0 if in emulator
 *
 *  int total_ram()
 *
 *  int total_rom()
 *
 *  int max_ramdrive_size()
 *  
 *  tuple<int_x, int_y> display_twips()
 *
 *  tuple<int_x, int_y> display_pixels()
 *
 *  int free_ram()
 *
 *  int ring_type()
 *    current ringing type. Possible values in release 2.0: 0 (normal) 
 *    1 (ascending) 2 (ring once) 3 (beep) 4 (silent) 
 *
 *  dict<unicode_string:int> free_drivespace()
 *    keys in dictionary are the drive letters followed by ':', values
 *    are the amount of free space left on the drive in bytes e.g. 
 *    {u'C:' 100} 
 *      
 * TODO
 *    - Only works from one process at time.
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

#include "Python.h"

#include "symbian_python_ext_util.h"

#include <sysutil.h>    // OS, SW info
#include <plpvariant.h> // IMEI
#include <saclient.h>   // Battery, network, see also sacls.h
#include <hal.h>        // HAL info
#if SERIES60_VERSION>12
#include <settinginfo.h> // Ringing volume, SDK 2.0 onwards
#endif
#include <f32file.h> 

const TInt KPhoneSwVersionLineFeed = '\n';

#ifdef __WINS__
_LIT(KEmulatorIMEI, "000000000000000");
_LIT(KEmulator, "emulator");
#endif

// UID for network signal strength
const TInt KUidNetworkBarsValue = 0x100052D4;
const TUid KUidNetworkBars ={KUidNetworkBarsValue};

// UID for battery level
const TInt KUidBatteryBarsValue = 0x100052D3;
const TUid KUidBatteryBars ={KUidBatteryBarsValue};

/*
 * Returns the operating system version.
 */
extern "C" PyObject *
sysinfo_osversion(PyObject* /*self*/)
{
  TVersion version;
  version = User::Version();

  return Py_BuildValue("(iii)", version.iMajor,
            version.iMinor, version.iBuild);
}

/*
 * Returns the software version of the device.
 */
extern "C" PyObject *
sysinfo_swversion(PyObject* /*self*/)
{
#ifdef __WINS__
  return Py_BuildValue("u#", ((TDesC&)KEmulator).Ptr(), ((TDesC&)KEmulator).Length());
#else
	TBufC<KSysUtilVersionTextLength> version;
  TPtr ptr(version.Des());
  TInt error = KErrNone;

	error = SysUtil::GetSWVersion(ptr);
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }  

	TInt index = 0;

	for (; index < ptr.Length(); index++)
		{
		if (ptr[index] == KPhoneSwVersionLineFeed)
			{
			ptr[index] = ' ';
			}
		}

  return Py_BuildValue("u#", version.Ptr(), version.Length());
#endif
}

/*
 * Returns the imei code.
 */
extern "C" PyObject *
sysinfo_imei(PyObject* /*self*/)
{
#ifdef __WINS__
  // Return a fake IMEI when on emulator
  return Py_BuildValue("u#", ((TDesC&)KEmulatorIMEI).Ptr(), ((TDesC&)KEmulatorIMEI).Length());
#else
  // This only works on target machine
  TPlpVariantMachineId imei;
  TRAPD(error,(PlpVariant::GetMachineIdL(imei)));
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }

  return Py_BuildValue("u#", imei.Ptr(), imei.Length());
#endif
}

/*
 * Returns the battery level left in the device.
 */
extern "C" PyObject *
sysinfo_battery(PyObject* /*self*/)
{
#ifdef __WINS__
  return Py_BuildValue("i", 0);
#else
  RSystemAgent systemAgent;
  TInt error = KErrNone;
  if ((error = systemAgent.Connect()) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

	// Get battery value:
	TInt batteryValue = systemAgent.GetState(KUidBatteryBars); 
  systemAgent.Close();
  return Py_BuildValue("i", batteryValue);
#endif
}

/*
 * Returns the signal strength currently.
 */
extern "C" PyObject *
sysinfo_signal(PyObject* /*self*/)
{
#ifdef __WINS__
  return Py_BuildValue("i", 0);
#else
  RSystemAgent systemAgent;
  TInt error = KErrNone;
  if ((error = systemAgent.Connect()) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

	// Get network value:
	TInt networkValue = systemAgent.GetState(KUidNetworkBars); 
  systemAgent.Close();
  return Py_BuildValue("i", networkValue);
#endif
}

/*
 * Returns the total amount of RAM in the device.
 */
extern "C" PyObject *
sysinfo_memoryram(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt eValue;
  
  if ((error = HAL::Get(HALData::EMemoryRAM, eValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", eValue);
}

/*
 * Returns the total amount of ROM in the device.
 */
extern "C" PyObject *
sysinfo_memoryrom(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt eValue;
  
  if ((error = HAL::Get(HALData::EMemoryROM, eValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", eValue);
}

/*
 * Returns the size of RAM drive in the device (normally D:-drive).
 */
extern "C" PyObject *
sysinfo_maxramdrivesize(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt eValue;
  
  if ((error = HAL::Get(HALData::EMaxRAMDriveSize, eValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", eValue);
}

/*
 * Returns the twips of the device's display.
 */
extern "C" PyObject *
sysinfo_displaytwips(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt xValue;
  TInt yValue;
  
  if ((error = HAL::Get(HALData::EDisplayXTwips, xValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  if ((error = HAL::Get(HALData::EDisplayYTwips, yValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("(ii)", xValue, yValue);
}

/*
 * Returns the pixels in the device's display.
 */
extern "C" PyObject *
sysinfo_displaypixels(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt xValue;
  TInt yValue;
  
  if ((error = HAL::Get(HALData::EDisplayXPixels, xValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  if ((error = HAL::Get(HALData::EDisplayYPixels, yValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("(ii)", xValue, yValue);
}

/*
 * Returns the free ram available.
 */
extern "C" PyObject *
sysinfo_memoryramfree(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt eValue;
  
  if ((error = HAL::Get(HALData::EMemoryRAMFree, eValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", eValue);
}

/*
 * Returns the ringing type set.
 */
#if SERIES60_VERSION>12
extern "C" PyObject *
sysinfo_ringtype(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt ret;
  
  CSettingInfo* settings = NULL;

  // NULL given since notifications are not needed
  TRAP(error,(settings = CSettingInfo::NewL(NULL)));
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }

  if((error = settings->Get(SettingInfo::ERingingType, ret)) != KErrNone) {
    delete settings;
    settings = NULL;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  delete settings;
  settings = NULL;

  return Py_BuildValue("i", ret);
}
#endif

/*
 * Returns the drives and the amount of free space in the device.
 */
extern "C" PyObject *
sysinfo_drive_free(PyObject* /*self*/)
{
  TInt error = KErrNone;
  RFs fsSession;
  if ((error = fsSession.Connect()) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  PyObject* ret = PyDict_New();
  if (ret == NULL)
    return PyErr_NoMemory();

  TVolumeInfo volumeInfo; 
  TInt driveNumber;

  for (driveNumber=EDriveA; driveNumber<=EDriveZ; driveNumber++) {
    TInt err = fsSession.Volume(volumeInfo,driveNumber); 
    if (err != KErrNone)
      continue; 

    char d[2];
    d[0] = 'A'+driveNumber; d[1] = ':';

    TInt freeSpace;
    freeSpace = (volumeInfo.iFree).GetTInt();

    PyObject* f = Py_BuildValue("i", freeSpace);
    PyObject* v = PyUnicode_Decode(d, 2, NULL, NULL);
    if (v == NULL || f == NULL) {
      Py_XDECREF(v);
      Py_XDECREF(f);
      Py_DECREF(ret);
      ret = NULL;
      break;     
    }

    if ((PyDict_SetItem(ret, (v), f) != 0)) {
      Py_DECREF(v);
      Py_DECREF(f);
      Py_DECREF(ret);
      ret = NULL;
      break; 
    }

    Py_DECREF(v);
    Py_DECREF(f);
  }

  fsSession.Close();

  return ret;
}

extern "C" {

  static const PyMethodDef sysinfo_methods[] = {
    {"os_version", (PyCFunction)sysinfo_osversion, METH_NOARGS, NULL},
    {"sw_version", (PyCFunction)sysinfo_swversion, METH_NOARGS, NULL},
    {"imei", (PyCFunction)sysinfo_imei, METH_NOARGS, NULL},
    {"battery", (PyCFunction)sysinfo_battery, METH_NOARGS, NULL},
    {"signal", (PyCFunction)sysinfo_signal, METH_NOARGS, NULL},
    {"total_ram", (PyCFunction)sysinfo_memoryram, METH_NOARGS, NULL},
    {"total_rom", (PyCFunction)sysinfo_memoryrom, METH_NOARGS, NULL},
    {"max_ramdrive_size", (PyCFunction)sysinfo_maxramdrivesize, METH_NOARGS, NULL},
    {"display_twips", (PyCFunction)sysinfo_displaytwips, METH_NOARGS, NULL},
    {"display_pixels", (PyCFunction)sysinfo_displaypixels, METH_NOARGS, NULL},
    {"free_ram", (PyCFunction)sysinfo_memoryramfree, METH_NOARGS, NULL},
#if SERIES60_VERSION>12
    {"ring_type", (PyCFunction)sysinfo_ringtype, METH_NOARGS, NULL},
#endif
    {"free_drivespace", (PyCFunction)sysinfo_drive_free, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initsysinfo(void)
  {
    PyObject *m;

    m = Py_InitModule("_sysinfo", (PyMethodDef*)sysinfo_methods);
  }
} /* extern "C" */

GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
