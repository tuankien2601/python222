/** 
 * ====================================================================
 * cameramodule.cpp
 *
 * Modified from "miso" (HIIT)  
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
 *
 * ====================================================================
 */
 
#include <ecam.h>
#include <fbs.h>
#include <e32std.h>

#include "Python.h"
#include "symbian_python_ext_util.h"

class CMisoPhotoTaker : public CBase, public MCameraObserver
{
public:
  static CMisoPhotoTaker* NewL(TInt aPosition);
  void ConstructL();
  ~CMisoPhotoTaker();
private:
  CMisoPhotoTaker();
public:
  CFbsBitmap* TakePhotoL(TInt aMode, TInt aSize, 
                         TInt aZoom, TInt aFlash, 
                         TInt aExp, TInt aWhite); // synchronous
  TBool TakingPhoto() const;
  // accessors:
  TInt GetImageModes() const;
  TInt GetImageSizeMax() const;
  void GetImageSize(TSize& aSize, 
                    TInt aSizeIndex, 
                    CCamera::TFormat aFormat) const;
  TInt GetMaxZoom() const;
  TInt GetFlashModes() const;
  TInt GetExpModes() const;
  TInt GetWhiteModes() const;
private:
  CActiveSchedulerWait* iLoop;
private:
  void MakeTakePhotoRequest();
  //CCamera::TFormat ImageFormatMax() const;
  void SetSettingsL();
private:
  CCamera* iCamera;
  CFbsBitmap* iBitMap;
  TInt iStatus;
  TBool iTakingPhoto;
  TCameraInfo iInfo;
  CCamera::TFormat iMode;
  TInt iSize;
  TInt iZoom;
  CCamera::TFlash iFlash;
  CCamera::TExposure iExp;
  CCamera::TWhiteBalance iWhite;
  TInt iPosition;
private: // MCameraObserver
  void ReserveComplete(TInt aError);
  void PowerOnComplete(TInt aError);
  void ViewFinderFrameReady(CFbsBitmap& aFrame);
  void ImageReady(CFbsBitmap* aBitmap, HBufC8* aData, TInt aError);
  void FrameBufferReady(MFrameBuffer* aFrameBuffer, TInt aError);
};

//////////////TYPE DEFINITION

#define CAM_type ((PyTypeObject*)SPyGetGlobalString("CAMType"))

struct CAM_object {
  PyObject_VAR_HEAD
  CMisoPhotoTaker* camera;
  TBool cameraUsed;
};
