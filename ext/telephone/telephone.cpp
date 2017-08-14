/**
 * ====================================================================
 * telephone.cpp
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

//A helper function for the implementation of callbacks
//from C/C++ code to Python callables (modified from appuifwmodule.cpp)
/*TInt TPyPhoneCallBack::StateChange(TInt aPreviousState, TInt aCurrentState, TInt aErrorCode)
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
    }
  }
  else
    Py_DECREF(rval);

  PyEval_SaveThread();
  return error;
  }*/

//////////////

CPhoneCall* CPhoneCall::NewL()
  {
  CPhoneCall* self = NewLC();
  CleanupStack::Pop(self); 
  return self;
  }

CPhoneCall* CPhoneCall::NewLC()
  {
  CPhoneCall* self = new (ELeave) CPhoneCall();
  CleanupStack::PushL(self);
  self->ConstructL();
  return self;
  }

CPhoneCall::CPhoneCall()
  : CActive(CActive::EPriorityStandard) 
  {
  CActiveScheduler::Add(this);
  }
  
void CPhoneCall::ConstructL()
  {
  iCallState = ENotCalling;
  iNumberSet = EFalse;
  }

CPhoneCall::~CPhoneCall()
  {
  Cancel();

  //This is the state where we should be. If not, then we need to close the
  //servers
  if (iCallState != ENotCalling) 
    {
#ifndef __WINS__
	  //Close the phone, line and call connections
	  //NOTE: This does not hang up the call
    iPhone.Close();
    iLine.Close();
    iCall.Close();

    //XXX error code
    //Unload the phone device driver
    /*error =*/ iServer.UnloadPhoneModule(KTsyName);
    /*if (error != KErrNone)
      return error;*/

	  //Close the connection to the tel server
    iServer.Close();
#endif /* __WINS__ */
    }
  }

void CPhoneCall::RunL()
  {
  switch (iCallState) 
    {
    case ECalling:
      iCallState = ECallInProgress;
      break;
    default:
      break;
    }
  }
   
void CPhoneCall::DoCancel()
  {
  iCall.DialCancel();
  iCallState = EInitialised;
  }
  
TInt CPhoneCall::Initialise()
  {
  TInt error = KErrNone;
  if (iCallState != ENotCalling)
    return EInitialiseCalledAlready;
#ifndef __WINS__

	//Create a connection to the tel server
  error = iServer.Connect();
  if (error != KErrNone)
    return error;

	//Load in the phone device driver
  error = iServer.LoadPhoneModule(KTsyName);
  if (error != KErrNone)
    return error;
  
	//Find the number of phones available from the tel server
	TInt numberPhones;
  error = iServer.EnumeratePhones(numberPhones);
  if (error != KErrNone)
    return error;

	//Check there are available phones
	if (numberPhones < 1)
		{
		return KErrNotFound;
    }

	//Get info about the first available phone
  error = iServer.GetPhoneInfo(0, iInfo);
  if (error != KErrNone)
    return error;

	//Use this info to open a connection to the phone, the phone is identified by its name
  error = iPhone.Open(iServer, iInfo.iName);
  if (error != KErrNone)
    return error;
    
  //"The phone hardware is usually automatically initialised before 
  //the first command is sent" (from SDK), no need for Initialise()

	//Get info about the first line from the phone
  error = iPhone.GetLineInfo(0, iLineInfo);
  if (error != KErrNone)
    return error;

	//Use this to open a line
  error = iLine.Open(iPhone, iLineInfo.iName);
  if (error != KErrNone)
    return error;

	//Open a new call on this line
  error = iCall.OpenNewCall(iLine, iNewCallName);
  if (error != KErrNone)
    return error;
#endif /* __WINS__ */
  iCallState = EInitialised;
    
  return error;
  }
  
TInt CPhoneCall::UnInitialise()
  {
  TInt error = KErrNone;
  
  if (IsActive())
    return EAlreadyCalling;
    
  if (iCallState == ENotCalling)
    return ENotInitialised;

#ifndef __WINS__
	//Close the phone, line and call connections
	//NOTE: This does not hang up the call
  iPhone.Close();
  iLine.Close();
  iCall.Close();

  //Unload the phone device driver
  error = iServer.UnloadPhoneModule(KTsyName);
  if (error != KErrNone)
    return error;

	//Close the connection to the tel server
  iServer.Close();
#endif /* __WINS__ */
  iCallState = ENotCalling;
  
  return error;
  }

TInt CPhoneCall::Dial()
  {
  TInt error = KErrNone;
  
  if (!iNumberSet)
    return ENumberNotSet;
    
  if (iCallState == ENotCalling)
    return ENotInitialised;

  if (IsActive())
    return EAlreadyCalling;
    
#ifndef __WINS__
  iCall.Dial(iStatus, iNumber);  
  SetActive();
#endif /* __WINS__ */  
  iCallState = ECalling;
  return error;
  }
  
void CPhoneCall::SetNumber(TDes& aNumber) 
  {
  iNumber = aNumber;
  iNumberSet = ETrue;
  }

TInt CPhoneCall::HangUp()
  {
  TInt error = KErrNone;
 
  if (iCallState == ECallInProgress || iCallState == ECalling) 
    {
#ifndef __WINS__
    error = iCall.HangUp(); //synchronous
#endif /* __WINS__ */  
    iCallState = EInitialised;
    return error;
    }
    
  return ENotCallInProgress;
  }

/*void CPhoneCall::SetCallBack(TPyPhoneCallBack& aCb) 
  {
  iCallMe = aCb;
  iCallBackSet = ETrue;
  }*/
