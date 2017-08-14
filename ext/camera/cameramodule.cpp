/**
 * ====================================================================
 *  cameramodule.cpp
 *
 *
 *  Python API to camera, slightly modified from "miso" (HIIT) and "CameraApp" 
 *  application (available from Forum Nokia). 
 *
 *  Implements currently following Python type and function:
 *
 *  Camera
 *   string take_photo([mode=value, size=valuer zoom=value, flash=value, 
 *                      exp=value, white=value, position=value])
 *      takes a photo, returns the image AND ownership in CFbsBitmap. 
 *      This operation fails with "SymbianError: KErrInUse" if some other 
 *      application is using the camera.
 *
 *      size
 *        image size (resolution)
 *      mode
 *        number of colors, defaults to EFormatFbsBitmapColor64K 
 *      zoom
 *        digital zoom factor, defaults to 0
 *      flash
 *        defaults to EFlashNone
 *      exp
 *        the exposure adjustment of the device, defaults to EExposureAuto
 *      white
 *        white balance, default EWBAuto
 *      position
 *        camera position, defaults to 0
 *
 *   image_modes()
 *   max_image_size()
 *   max_zoom()
 *   flash_modes()
 *   exp_modes()
 *   white_modes()
 *   cameras_available()
 *
 * Copyright 2005 Helsinki Institute for Information Technology (HIIT)
 * and the authors.  All rights reserved.
 * Authors: Tero Hasu <tero.hasu@hut.fi>
 *
 * Portions Copyright (c) 2005 Nokia Corporation 
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * ====================================================================
 */

#include <e32std.h>

#include "cameramodule.h"

//////////////TYPE METHODS

/*
 * Deallocate cam.
 */
extern "C" {
  static void cam_dealloc(CAM_object *camo)
  {
    if (camo->camera) {
      delete camo->camera;
      camo->camera = NULL;
    }
    PyObject_Del(camo);
  }
}

/*
 * Allocate rec.
 */
extern "C" PyObject *
new_cam_object(PyObject* /*self*/, PyObject /**args*/)
{
  CAM_object *camo = PyObject_New(CAM_object, CAM_type);
  if (camo == NULL)
    return PyErr_NoMemory();
  
  if (camo->camera == NULL) {
    PyObject_Del(camo);
    return PyErr_NoMemory();
  }  

  TRAPD(error, camo->camera = CMisoPhotoTaker::NewL(0));
  if (error != KErrNone){
    PyObject_Del(camo);
    return SPyErr_SetFromSymbianOSErr(error);    
  }
  
  camo->cameraUsed = ETrue;
  
  return (PyObject*) camo;
}

/*
 * Take photo.
 */
extern "C" PyObject *
cam_take_photo(CAM_object* self, PyObject *args, PyObject *keywds)
{
  // defaults:
  TInt mode = CCamera::EFormatFbsBitmapColor64K;
  TInt size = 0; 
  TInt zoom = 0;
  TInt flash = CCamera::EFlashNone;
  TInt exp = CCamera::EExposureAuto;
  TInt white = CCamera::EWBAuto;
  TInt position = 0; 
  
  static const char *const kwlist[] = 
    {"mode", "size", "zoom", "flash", "exp", "white", "position", NULL};
 
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|iiiiiii", (char**)kwlist,
                                   &mode, 
                                   &size,
                                   &zoom,
                                   &flash,
                                   &exp, 
                                   &white,
                                   &position)){ 
    return NULL;
  }

  // re-initialization of camera if needed
  // this is in-efficient but works
  if(self->cameraUsed) 
  {
   	delete self->camera;
   	self->camera = NULL;
    TRAPD(error, self->camera = CMisoPhotoTaker::NewL(position));
    if (error != KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);    
    }   
  }
  self->cameraUsed = ETrue;

  CFbsBitmap* bmp = NULL;
  
  TInt error = KErrNone;
  
  Py_BEGIN_ALLOW_THREADS
  TRAP(error, bmp = self->camera->TakePhotoL(mode, size, zoom, flash, exp, white));
  Py_END_ALLOW_THREADS
  
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  PyObject* arg = PyCObject_FromVoidPtr(bmp, NULL);
  if (!arg)
    // should have set an exception
    return NULL;
  return arg;
}

/*
 * Returns the image modes.
 */
extern "C" PyObject *
cam_image_modes(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetImageModes());
}

/*
 * Returns the maximum image size (camera internal).
 */
extern "C" PyObject *
cam_max_image_size(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetImageSizeMax());
}

/*
 * Returns the image size possible with specified color mode and index.
 */
extern "C" PyObject *
cam_image_size(CAM_object* self, PyObject *args)
{
  TInt mode;
  TInt index;

  if (!PyArg_ParseTuple(args, "ii", &mode, &index))
    return NULL;

  TSize size(0, 0);
  self->camera->GetImageSize(size, index, static_cast<CCamera::TFormat>(mode));
  return Py_BuildValue("(ii)", size.iWidth, size.iHeight);
}

/*
 * Returns the maximum digital zoom supported in the device.
 */
extern "C" PyObject *
cam_max_zoom(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetMaxZoom());
}

/*
 * Returns the flash modes (in bitfield) supported in the device.
 */
extern "C" PyObject *
cam_flash_modes(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetFlashModes());
}

/*
 * Returns the exposure modes (in bitfield) supported in the device.
 */
extern "C" PyObject *
cam_exposure_modes(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetExpModes());
}

/*
 * Returns the white balance modes (in bitfield) supported in the device.
 */
extern "C" PyObject *
cam_white_modes(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->GetWhiteModes());
}

/*
 * Returns the available cameras.
 */
extern "C" PyObject *
cam_cameras_available(CAM_object* self)
{
  return Py_BuildValue("i", CCamera::CamerasAvailable());
}

/*
 * Return whether there is a photo request ongoing.
 */
extern "C" PyObject *
cam_taking_photo(CAM_object* self)
{
  return Py_BuildValue("i", self->camera->TakingPhoto());
}

//////////////TYPE SET

extern "C" {

  static const PyMethodDef cam_methods[] = {
    {"take_photo", (PyCFunction)cam_take_photo, METH_VARARGS | METH_KEYWORDS, NULL},
    {"image_modes", (PyCFunction)cam_image_modes, METH_NOARGS, NULL},
    {"max_image_size", (PyCFunction)cam_max_image_size, METH_NOARGS, NULL},
    {"image_size", (PyCFunction)cam_image_size, METH_VARARGS, NULL},
    {"max_zoom", (PyCFunction)cam_max_zoom, METH_NOARGS, NULL},
    {"flash_modes", (PyCFunction)cam_flash_modes, METH_NOARGS, NULL},
    {"exposure_modes", (PyCFunction)cam_exposure_modes, METH_NOARGS, NULL},
    {"white_balance_modes", (PyCFunction)cam_white_modes, METH_NOARGS, NULL},
    {"cameras_available", (PyCFunction)cam_cameras_available, METH_NOARGS, NULL},
    {"taking_photo", (PyCFunction)cam_taking_photo, METH_NOARGS, NULL},
    {NULL, NULL, 0 , NULL}             /* sentinel */
  };
  
  static PyObject *
  cam_getattr(CAM_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)cam_methods, (PyObject *)op, name);
  }
  
  static const PyTypeObject c_cam_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_camera.Camera",                         /*tp_name*/
    sizeof(CAM_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)cam_dealloc,                  /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)cam_getattr,                 /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };
} //extern C

//////////////INIT

extern "C" {

  static const PyMethodDef camera_methods[] = {
    {"Camera", (PyCFunction)new_cam_object, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initcamera(void)
  {
    PyTypeObject* cam_type = PyObject_New(PyTypeObject, &PyType_Type);
    *cam_type = c_cam_type;
    cam_type->ob_type = &PyType_Type;

    SPyAddGlobalString("CAMType", (PyObject*)cam_type);

    PyObject *m, *d;

    m = Py_InitModule("_camera", (PyMethodDef*)camera_methods);
    d = PyModule_GetDict(m);
    
    PyDict_SetItemString(d,"EColor4K", PyInt_FromLong(CCamera::EFormatFbsBitmapColor4K));
    PyDict_SetItemString(d,"EColor64K", PyInt_FromLong(CCamera::EFormatFbsBitmapColor64K));
    PyDict_SetItemString(d,"EColor16M", PyInt_FromLong(CCamera::EFormatFbsBitmapColor16M));
    
    PyDict_SetItemString(d,"EFlashNone", PyInt_FromLong(CCamera::EFlashNone));
    PyDict_SetItemString(d,"EFlashAuto", PyInt_FromLong(CCamera::EFlashAuto));
    PyDict_SetItemString(d,"EFlashForced", PyInt_FromLong(CCamera::EFlashForced));
    PyDict_SetItemString(d,"EFlashFillIn", PyInt_FromLong(CCamera::EFlashFillIn));
    PyDict_SetItemString(d,"EFlashRedEyeReduce", PyInt_FromLong(CCamera::EFlashRedEyeReduce));
    
    PyDict_SetItemString(d,"EExposureAuto", PyInt_FromLong(CCamera::EExposureAuto));
    PyDict_SetItemString(d,"EExposureNight", PyInt_FromLong(CCamera::EExposureNight));
    PyDict_SetItemString(d,"EExposureBacklight", PyInt_FromLong(CCamera::EExposureBacklight));
    PyDict_SetItemString(d,"EExposureCenter", PyInt_FromLong(CCamera::EExposureCenter));
    
    PyDict_SetItemString(d,"EWBAuto", PyInt_FromLong(CCamera::EWBAuto));
    PyDict_SetItemString(d,"EWBDaylight", PyInt_FromLong(CCamera::EWBDaylight));
    PyDict_SetItemString(d,"EWBCloudy", PyInt_FromLong(CCamera::EWBCloudy));
    PyDict_SetItemString(d,"EWBTungsten", PyInt_FromLong(CCamera::EWBTungsten));
    PyDict_SetItemString(d,"EWBFluorescent", PyInt_FromLong(CCamera::EWBFluorescent));
    PyDict_SetItemString(d,"EWBFlash", PyInt_FromLong(CCamera::EWBFlash));
  }
  
} /* extern "C" */

GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
