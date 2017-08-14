/**
 * ====================================================================
 * recorder.h
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
#include <MdaAudioSampleEditor.h>
#include <e32std.h>
#include <Mda\Common\resource.h>

class TPyRecCallBack
  {
  public:
    TInt StateChange(TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);
  public:
    PyObject* iCb; // Not owned.
  };

class CRecorderAdapter : public CBase, public MMdaObjectStateChangeObserver
  {
public:
  static CRecorderAdapter* NewL();
  static CRecorderAdapter* NewLC();
  ~CRecorderAdapter();
public: 
  void PlayL(TInt aTimes, const TTimeIntervalMicroSeconds& aDuration);
  void Stop();
  void RecordL();
  void OpenFileL(const TDesC& aFileName);
  void CloseFile();
  TInt State();
  // Calling with larger than GetMaxVolume() or negative panics e.g. 6630
  // Value is checked in Python wrapper
  void SetVolume(TInt aVolume);
#if SERIES60_VERSION>12
  TInt GetCurrentVolume(TInt &aVolume);
#endif /* SERIES60_VERSION */
  TInt GetMaxVolume();
  void Duration(TTimeIntervalMicroSeconds& aDuration);
  void SetPosition(const TTimeIntervalMicroSeconds& aPosition);
  void GetCurrentPosition(TTimeIntervalMicroSeconds& aPosition);
  void SetCallBack(TPyRecCallBack &aCb);
public: // from MMdaObjectStateChangeObserver
  void MoscoStateChangeEvent(CBase* aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);
private:
  void ConstructL();
private:
  CMdaAudioRecorderUtility* iMdaAudioRecorderUtility;
  TPyRecCallBack iCallMe;
  TInt iErrorState;
  TBool iCallBackSet;
  };
  
//////////////TYPE DEFINITION

#define REC_type ((PyTypeObject*)SPyGetGlobalString("RECType"))

struct REC_object {
  PyObject_VAR_HEAD
  CRecorderAdapter* recorder;
  TPyRecCallBack myCallBack;
  TBool callBackSet;
};

//////////////ASSERTS

/*#define ASSERT_FILEOPEN						\
  if (self->recorder->State() == CMdaAudioRecorderUtility::ENotReady) {					\
    PyErr_SetString(PyExc_RuntimeError, "File not open");	\
    return NULL;						\
  }*/

/*#define READY_TO_RECORD_OR_PLAY						\
  if (self->recorder->State() != CMdaAudioRecorderUtility::EOpen) {					\
    PyErr_SetString(PyExc_RuntimeError, "file not opened or recording/playing/not ready");	\
    return NULL;						\
  }*/
