/** 
 * ====================================================================
 * takephoto.cpp
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

#include "cameramodule.h"

CMisoPhotoTaker* CMisoPhotoTaker::NewL(TInt aPosition)
{
  CMisoPhotoTaker* taker = new (ELeave) CMisoPhotoTaker;
  CleanupStack::PushL(taker);
  taker->iPosition = aPosition;
  taker->ConstructL();
  CleanupStack::Pop();
  return taker;
}

CMisoPhotoTaker::CMisoPhotoTaker()
{
  iStatus = KErrNone;
  iTakingPhoto = EFalse;
}

void CMisoPhotoTaker::ConstructL()
{
  if (!CCamera::CamerasAvailable())
    User::Leave(KErrHardwareNotAvailable);
  
  iLoop = new (ELeave) CActiveSchedulerWait();
  iCamera = CCamera::NewL(*this, iPosition);

  iCamera->CameraInfo(iInfo);
}

CMisoPhotoTaker::~CMisoPhotoTaker()
{
  if (iCamera) {
    // assuming this is safe even if not on
    iCamera->PowerOff();
    // assuming safe even if not reserved
    iCamera->Release();
    delete iCamera;
  }
  delete iLoop;
}

CFbsBitmap* CMisoPhotoTaker::TakePhotoL(TInt aMode, TInt aSize, 
                                        TInt aZoom, TInt aFlash, 
                                        TInt aExp, TInt aWhite)
{
  iTakingPhoto = ETrue;
  iMode = static_cast<CCamera::TFormat>(aMode);
  iSize = aSize;
  iZoom = aZoom;
  iFlash = static_cast<CCamera::TFlash>(aFlash);
  iExp = static_cast<CCamera::TExposure>(aExp);
  iWhite = static_cast<CCamera::TWhiteBalance>(aWhite);

  MakeTakePhotoRequest();
  // Start the wait for reserve complete, this ends after the completion
  // of ImageReady
  iLoop->Start(); 
  iTakingPhoto = EFalse;
  User::LeaveIfError(iStatus);

  // pass result ownership to caller
  CFbsBitmap* ret = iBitMap;
  iBitMap = NULL;
  return ret;
}

TBool CMisoPhotoTaker::TakingPhoto() const
{
  return iTakingPhoto;
}

TInt CMisoPhotoTaker::GetImageModes() const
{
  return iInfo.iImageFormatsSupported;
}

TInt CMisoPhotoTaker::GetImageSizeMax() const
{
  return iInfo.iNumImageSizesSupported;
}

void CMisoPhotoTaker::GetImageSize(TSize& aSize, TInt aSizeIndex, CCamera::TFormat aFormat) const
{
  iCamera->EnumerateCaptureSizes(aSize, aSizeIndex, aFormat);
}

TInt CMisoPhotoTaker::GetMaxZoom() const
{
  return iInfo.iMaxDigitalZoom;
}

TInt CMisoPhotoTaker::GetFlashModes() const
{
  return iInfo.iFlashModesSupported;
}

TInt CMisoPhotoTaker::GetExpModes() const
{
  return iInfo.iExposureModesSupported;
}

TInt CMisoPhotoTaker::GetWhiteModes() const
{
  return iInfo.iWhiteBalanceModesSupported;
}

void CMisoPhotoTaker::MakeTakePhotoRequest()
{
  iCamera->Reserve();
}

/*
// Returns highest color mode supported by HW
CCamera::TFormat CMisoPhotoTaker::ImageFormatMax() const
{
  if ( iInfo.iImageFormatsSupported & CCamera::EFormatFbsBitmapColor16M )
    {
    return CCamera::EFormatFbsBitmapColor16M;
    }
  else if ( iInfo.iImageFormatsSupported & CCamera::EFormatFbsBitmapColor64K )
    {
    return CCamera::EFormatFbsBitmapColor64K;
    }
  else 
    {
    return CCamera::EFormatFbsBitmapColor4K;
    }
}
*/

void CMisoPhotoTaker::SetSettingsL()
{
  // The parameters need to be checked prior invoking the 
  // methods since otherwise a panic may occur (and not leave)
  if (iZoom < 0 || iZoom > iInfo.iMaxDigitalZoom)
    User::Leave(KErrNotSupported);
  else
    iCamera->SetDigitalZoomFactorL(iZoom);
  
  if (iFlash & iInfo.iFlashModesSupported || iFlash == CCamera::EFlashNone)
    iCamera->SetFlashL(iFlash);
  else
    User::Leave(KErrNotSupported);
  
  if (iExp & iInfo.iExposureModesSupported || iExp == CCamera::EExposureAuto)
    iCamera->SetExposureL(iExp);
  else
    User::Leave(KErrNotSupported);
  
  if (iWhite & iInfo.iWhiteBalanceModesSupported || iWhite == CCamera::EWBAuto)
    iCamera->SetWhiteBalanceL(iWhite);
  else
    User::Leave(KErrNotSupported);
}

void CMisoPhotoTaker::ReserveComplete(TInt aError) 
{
  if (aError) {
    iStatus = aError;
    iLoop->AsyncStop();
  } else {
    iCamera->PowerOn();
  }
}

void CMisoPhotoTaker::PowerOnComplete(TInt aError) 
{
  if (aError) {
    iStatus = aError;
    iLoop->AsyncStop();
  } else {
    // the settings, notice that these can be only set here,
    // i.e. in camera state MCameraObserver::PowerOnComplete()
    TRAP(aError, SetSettingsL());
    if (aError) {
      iStatus = aError;
      iLoop->AsyncStop();
    } else  {
      TRAP(aError, iCamera->PrepareImageCaptureL(
	      iMode, iSize));    
      if (aError) {
        iStatus = aError;
        iLoop->AsyncStop();
      } else {
        // completes with ImageReady
        iCamera->CaptureImage();
      }
    }
  }
}

void CMisoPhotoTaker::ViewFinderFrameReady(CFbsBitmap& /*aFrame*/) {}

void CMisoPhotoTaker::ImageReady(CFbsBitmap* aBitmap, HBufC8* /*aData*/, 
				 TInt aError) 
{
  if (aError) {
    iStatus = aError;
    iLoop->AsyncStop();
  } else {
    delete iBitMap;
    iBitMap = aBitmap;
    iLoop->AsyncStop();
  }
}

void CMisoPhotoTaker::FrameBufferReady(MFrameBuffer* /*aFrameBuffer*/, 
          TInt /*aError*/) {}

