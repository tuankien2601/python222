/**
 * ====================================================================
 * recordadapter.cpp
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
 
#include "recorder.h"

// A helper function for the implementation of callbacks
// from C/C++ code to Python callables (modified from appuifwmodule.cpp)
TInt TPyRecCallBack::StateChange(TInt aPreviousState, TInt aCurrentState, TInt aErrorCode)
  {
  PyEval_RestoreThread(PYTHON_TLS->thread_state);
  TInt error = KErrNone;
  
  PyObject* arg = Py_BuildValue("(iii)", aPreviousState, aCurrentState, aErrorCode);
  PyObject* rval = PyEval_CallObject(iCb, arg);
  
  Py_DECREF(arg);
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      if (PyInt_Check(value))
        error = PyInt_AS_LONG(value);
      Py_XDECREF(type);
      Py_XDECREF(value);
      Py_XDECREF(traceback);
    } else {
      PyErr_Print();
    }
  }
  else
    Py_DECREF(rval);  

  PyEval_SaveThread();
  return error;
  }

//////////////

CRecorderAdapter* CRecorderAdapter::NewL()
  {
  CRecorderAdapter* self = NewLC();
  CleanupStack::Pop(self); 
  return self;
  }

CRecorderAdapter* CRecorderAdapter::NewLC()
  {
  CRecorderAdapter* self = new (ELeave) CRecorderAdapter();
  CleanupStack::PushL(self);
  self->ConstructL();
  return self;
  }

void CRecorderAdapter::ConstructL()
  {
  //The magic number 80 is suggested in document "Symbian OS: Creating Audio
  //Applications in C++"
  iMdaAudioRecorderUtility = CMdaAudioRecorderUtility::NewL(*this, 0, 80, EMdaPriorityPreferenceQuality);
  iCallBackSet = EFalse;
  }

CRecorderAdapter::~CRecorderAdapter()
  {
  CloseFile();
  delete iMdaAudioRecorderUtility;    
  iMdaAudioRecorderUtility = NULL;
  }

void CRecorderAdapter::PlayL(TInt aTimes, const TTimeIntervalMicroSeconds& aDuration)
  {
  // Set the repeats:
  iMdaAudioRecorderUtility->SetRepeats(aTimes, aDuration);
  
  // XXX this does not have effect at least in 6600, it goes always to speaker
  // Select the mode, e.g. through the device speaker
  iMdaAudioRecorderUtility->SetAudioDeviceMode(CMdaAudioRecorderUtility::EDefault);
  
  iMdaAudioRecorderUtility->PlayL();
  }

void CRecorderAdapter::Stop()
  {
  iMdaAudioRecorderUtility->Stop();
  }

void CRecorderAdapter::RecordL()
  { 
  iMdaAudioRecorderUtility->SetAudioDeviceMode(CMdaAudioRecorderUtility::EDefault);  
  // Set maximum gain for recording
  iMdaAudioRecorderUtility->SetGain(iMdaAudioRecorderUtility->MaxGain());
    
  // XXX: the next calls don't work atleast in 6630 and 6600:
  // Delete current audio sample from beginning of file
  //iMdaAudioRecorderUtility->SetPosition(TTimeIntervalMicroSeconds(0));
  //iMdaAudioRecorderUtility->CropL();
    
  iMdaAudioRecorderUtility->RecordL();
  }

void CRecorderAdapter::OpenFileL(const TDesC& aFileName)
  {
  iMdaAudioRecorderUtility->OpenFileL(aFileName);  
  }

void CRecorderAdapter::CloseFile()
  {
  // Just in case stop
  Stop();
  iMdaAudioRecorderUtility->Close();  
  }

TInt CRecorderAdapter::State()
  {
  return iMdaAudioRecorderUtility->State();
  }

void CRecorderAdapter::SetVolume(TInt aVolume)
  {
  iMdaAudioRecorderUtility->SetVolume(aVolume);
  }

#if SERIES60_VERSION>12
TInt CRecorderAdapter::GetCurrentVolume(TInt &aVolume)
  {
  return iMdaAudioRecorderUtility->GetVolume(aVolume);
  }
#endif /* SERIES60_VERSION */

TInt CRecorderAdapter::GetMaxVolume()
  {
  return iMdaAudioRecorderUtility->MaxVolume();
  }

void CRecorderAdapter::Duration(TTimeIntervalMicroSeconds& aDuration)
  {
  aDuration = iMdaAudioRecorderUtility->Duration();
  }

void CRecorderAdapter::SetPosition(const TTimeIntervalMicroSeconds& aPosition)
  {
  iMdaAudioRecorderUtility->SetPosition(aPosition);
  }
  
void CRecorderAdapter::GetCurrentPosition(TTimeIntervalMicroSeconds& aPosition)
  {
  aPosition = iMdaAudioRecorderUtility->Position();
  }

void CRecorderAdapter::MoscoStateChangeEvent(CBase* /*aObject*/, 
                                            TInt aPreviousState, 
                                            TInt aCurrentState, 
                                            TInt aErrorCode)
  {
   if (iCallBackSet) {
    iErrorState = iCallMe.StateChange(aPreviousState, aCurrentState, aErrorCode);
   }
  }
  
void CRecorderAdapter::SetCallBack(TPyRecCallBack &aCb) 
  {
  iCallMe = aCb;
  iCallBackSet = ETrue;
  }
