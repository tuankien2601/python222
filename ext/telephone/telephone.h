/**
 * ====================================================================
 * telephone.h
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

#include <e32std.h> 
#include <etel.h>

_LIT(KTsyName,"phonetsy.tsy");

#define MaxTelephoneNumberLength 30

/*class TPyPhoneCallBack
  {
  public:
    TInt StateChange(TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);
  public:
    PyObject* iCb; // Not owned.
  };*/

//Safe to use also from emulator.
class CPhoneCall : public CActive
  {
public:
  static CPhoneCall* NewL();
  static CPhoneCall* NewLC();
  ~CPhoneCall();
public: 
  CPhoneCall();
  void ConstructL();
  
  TInt Initialise();
  TInt UnInitialise();
  TInt Dial();
  void SetNumber(TDes& aNumber);
  TInt HangUp();
  //void SetCallBack(TPyPhoneCallBack &aCb);
  /*
  ENotCalling The initial (un-initialised) state
  EInitialised After Initialise() call, ready to Dial()
  ECalling A dial request has been issued
  ECallInProgress A call is ongoing
  */
	enum TCallState 
	  {
		ENotCalling,
		EInitialised,
    ECalling,
    ECallInProgress
	  };  
	//These are returned to avoid ETel server panics:
	enum TPhoneCallErrors
	  {
		ENumberNotSet = 0x7D0,
		ENotInitialised,
		ENotCallInProgress,
		EInitialiseCalledAlready,
		EAlreadyCalling,
	  };  
public: //from CActive:
  void RunL();
  void DoCancel();
private:
  RTelServer iServer;
  RTelServer::TPhoneInfo iInfo;
  RPhone iPhone;
  RPhone::TLineInfo iLineInfo;
  RLine iLine;
  TBuf<100> iNewCallName;
  RCall iCall;
  TBuf<MaxTelephoneNumberLength> iNumber;
  TCallState iCallState;
  TBool iNumberSet;
  //TPyPhoneCallBack iCallMe;
  //TBool iCallBackSet;
  };
  
//////////////TYPE DEFINITION

#define PHO_type ((PyTypeObject*)SPyGetGlobalString("PHOType"))

struct PHO_object {
  PyObject_VAR_HEAD
  CPhoneCall* phone;
  //TPyPhoneCallBack myCallBack;
  //TBool callBackSet;
};
