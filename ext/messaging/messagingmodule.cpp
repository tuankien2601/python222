/*
* ====================================================================
*  messagingmodule.cpp  
*  
*  Python messaging services on Symbian OS
*
*  Implements currently (24.08.2004) following Python classes / methods:
*
*  sms_send(string, unicode_string)
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

#include <mtclreg.h>
#include <msvuids.h>
#include <txtrich.h>

#include <smut.h>
#include <smutset.h>
#include <smsclnt.h>
#include <smscmds.h>
#include <SMUTHDR.h>

class Ao_lock : public CActive {
public:
  Ao_lock();
  TInt Signal();
  void Wait();
private:
  void RunL();
  void DoCancel() {;}
  TThreadId iTid;
  TRequestStatus* iPst;
#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
};

Ao_lock::Ao_lock():CActive(0)
{
  iTid = RThread().Id();
  iStatus = KErrCancel;
  iPst = &iStatus;

  CActiveScheduler::Add(this);
}

void Ao_lock::Wait()
{
  if (iStatus != KErrNone) {
    iStatus = KRequestPending;
    iPst = &iStatus;
    SetActive();

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT   
    iWait.Start();
#else
    CActiveScheduler::Start();
#endif
    Py_END_ALLOW_THREADS
  }

  iStatus = KErrCancel;
  return;
}

TInt Ao_lock::Signal()
{
  TInt error = KErrNone;

  if (iStatus != KRequestPending) {
    iStatus = KErrNone;
    return error;
  }
  RThread t;
  error = t.Open(iTid);
  if (error == KErrNone) {
    iStatus = KErrNone;
    t.RequestComplete(iPst, 0);
    t.Close();
  }
  return error;
}

void Ao_lock::RunL()
{
#ifdef HAVE_ACTIVESCHEDULERWAIT
  iWait.AsyncStop();
#else
  CActiveScheduler::Stop();
#endif
}

/*
 *
 * Implementation of messaging.sms_send
 * (based on S60 SDK 2.0 example)
 *
 */

#define MaxMessageLength 160
#define MaxTelephoneNumberLength 30

class MMsvObserver
{
 public:
  enum TStatus {ECreated, EMovedToOutBox, EScheduledForSend, ESent, EDeleted};
  enum TError {EScheduleFailed, ESendFailed, ENoServiceCentre, EFatalServerError};

  virtual void HandleStatusChange(TStatus aStatus) = 0;
  virtual void HandleError(TError aError) = 0;
};


class CMsvHandler : public CActive, public MMsvSessionObserver
{
 public:
  ~CMsvHandler();
  virtual TBool IsIdle() {return (iMtmRegistry ? ETrue : EFalse);}

 protected: // from CActive
  CMsvHandler(MMsvObserver&);
 
  void DoCancel() { if (iOperation) iOperation->Cancel(); }
  void ConstructL() { iSession = CMsvSession::OpenAsyncL(*this); }
  virtual void CompleteConstructL() { iMtmRegistry = CClientMtmRegistry::NewL(*iSession); }

  virtual void SetMtmEntryL(TMsvId);
  virtual void DeleteEntryL(TMsvEntry&);
  
 protected:
  CMsvOperation*      iOperation;
  CMsvSession*        iSession;
  CBaseMtm*           iMtm;
  CClientMtmRegistry* iMtmRegistry;
  MMsvObserver&       iObserver;
};

CMsvHandler::CMsvHandler(MMsvObserver& aObserver) :
  CActive(EPriorityStandard), 
  iOperation(NULL), 
  iSession(NULL), 
  iMtm(NULL), 
  iMtmRegistry(NULL), 
  iObserver(aObserver) 
{
  CActiveScheduler::Add(this);
}


CMsvHandler::~CMsvHandler()
{
  Cancel(); 
  delete iOperation;
  iOperation = NULL;
  delete iMtm;
  iMtm = NULL;
  delete iMtmRegistry;
  iMtmRegistry = NULL;
  delete iSession;    // session must be deleted last (and constructed first)
  iSession = NULL;
}

void CMsvHandler::SetMtmEntryL(TMsvId aEntryId)
{
  CMsvEntry* entry = iSession->GetEntryL(aEntryId);
  CleanupStack::PushL(entry);
    
  if ((iMtm == NULL) || (entry->Entry().iMtm != (iMtm->Entry()).Entry().iMtm)) {
    delete iMtm;
    iMtm = NULL;
    iMtm = iMtmRegistry->NewMtmL(entry->Entry().iMtm);
  }

  iMtm->SetCurrentEntryL(entry);
  CleanupStack::Pop(entry); 
}

void CMsvHandler::DeleteEntryL(TMsvEntry& aMsvEntry)
{
  CMsvEntry* parentEntry = CMsvEntry::NewL(*iSession, aMsvEntry.Parent(), TMsvSelectionOrdering());
  CleanupStack::PushL(parentEntry);

  iOperation = parentEntry->DeleteL(aMsvEntry.Id(), iStatus);
  CleanupStack::PopAndDestroy(parentEntry);
  SetActive();
}


class CSmsSendHandler : public CMsvHandler
{
 public:
  static CSmsSendHandler* NewL(MMsvObserver&, const TDesC&, const TDesC&);

 public: // from CMsvHandler
  TBool IsIdle();

 public: // from MMsvSessionObserver
  void HandleSessionEventL(TMsvSessionEvent, TAny*, TAny*, TAny*);

 protected:
  void RunL();
  CSmsSendHandler(MMsvObserver& aObserver, const TDesC& aTelNum, const TDesC& aMsgBody) :
    CMsvHandler(aObserver), iPhase(EIdle), iTelNum(aTelNum), iMessageText(aMsgBody) {;}

 private:
  void ConstructL() {CMsvHandler::ConstructL();}
  void CreateNewMessageL();
  TBool SetupSmsHeaderL();
  void PopulateMessageL(TMsvEntry&);
  TBool InitializeMessageL();
  TBool MoveMessageEntryL(TMsvId);
  void SetScheduledSendingStateL(CMsvEntrySelection&);
  void HandleChangedEntryL(TMsvId);
  void SendToL() { CreateNewMessageL(); };

 private:
  enum TPhase {EIdle, EWaitingForCreate, EWaitingForMove,
               EWaitingForScheduled, EWaitingForSent, EWaitingForDeleted};

  TPhase                           iPhase;
  TBuf<MaxTelephoneNumberLength>   iTelNum;
  TBuf<MaxMessageLength>           iMessageText;
};

CSmsSendHandler* CSmsSendHandler::NewL(MMsvObserver& aObserver, const TDesC& aTelNum, const TDesC& aMsgBody)
{
  CSmsSendHandler* self = new (ELeave) CSmsSendHandler(aObserver, aTelNum, aMsgBody);
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop();
  return self;
}

TBool CSmsSendHandler::IsIdle()
{
  if (CMsvHandler::IsIdle() && (iPhase == EIdle))
    return ETrue;
  else
    return EFalse;
}

void CSmsSendHandler::RunL()
{
  ASSERT(iOperation);
  User::LeaveIfError(iStatus.Int());
  
  TMsvLocalOperationProgress progress;
  
  TUid mtmUid = iOperation->Mtm();

  if (mtmUid == KUidMsvLocalServiceMtm) {
    progress = McliUtils::GetLocalProgressL(*iOperation);
    User::LeaveIfError(progress.iError);
  }
  else {
    if (iPhase != EWaitingForScheduled)
      /* User::Panic(KSms, ESmsStateError); */
      User::Leave(KErrUnknown); 
  }

  delete iOperation;
  iOperation = NULL;
  
  switch (iPhase) {
  case EWaitingForCreate:
    iObserver.HandleStatusChange(MMsvObserver::ECreated);
    SetMtmEntryL(progress.iId);
#ifdef __WINS__
    if (InitializeMessageL() && MoveMessageEntryL(KMsvDraftEntryId))
#else
    if (InitializeMessageL() && MoveMessageEntryL(KMsvGlobalOutBoxIndexEntryId))
#endif
      iPhase = EWaitingForMove;
    else
      iPhase = EIdle;
    break;
  case EWaitingForMove:
    {
      iObserver.HandleStatusChange(MMsvObserver::EMovedToOutBox);
      CMsvEntrySelection* selection = new (ELeave) CMsvEntrySelection;
      CleanupStack::PushL(selection);
      
      selection->AppendL(progress.iId);
      SetScheduledSendingStateL(*selection);
      CleanupStack::PopAndDestroy(selection);
      iPhase = EWaitingForScheduled;
    }
    break;
  case EWaitingForScheduled:
    {
      const TMsvEntry& msvEntry = iMtm->Entry().Entry();
      TInt state = msvEntry.SendingState();
      if (state != KMsvSendStateScheduled) {
        iObserver.HandleError(MMsvObserver::EScheduleFailed);
        iPhase = EIdle;
      }
      else {
        iObserver.HandleStatusChange(MMsvObserver::EScheduledForSend);
        iPhase = EWaitingForSent;
      }
    }
    break;
  case EWaitingForDeleted:
    iObserver.HandleStatusChange(MMsvObserver::EDeleted);
    iPhase = EIdle;
    break;
  case EWaitingForSent:  // We handle this in HandleSessionEventL
  case EIdle:            // Shouldn't get triggered in this state
  default:
    ASSERT(EFalse);
  }
}

void CSmsSendHandler::CreateNewMessageL()
{
  TMsvEntry newEntry;              // This represents an entry in the Message Server index
  newEntry.iMtm = KUidMsgTypeSMS;                         // message type is SMS
  newEntry.iType = KUidMsvMessageEntry;                   // this defines the type of the entry: message 
  newEntry.iServiceId = KMsvLocalServiceIndexEntryId;     // ID of local service (containing the standard folders)
  newEntry.iDate.HomeTime();                              // set the date of the entry to home time
  newEntry.SetInPreparation(ETrue);                       // a flag that this message is in preparation

  CMsvEntry* entry = CMsvEntry::NewL(*iSession, KMsvDraftEntryIdValue, TMsvSelectionOrdering());
  CleanupStack::PushL(entry);

  iOperation = entry->CreateL(newEntry, iStatus);
  CleanupStack::PopAndDestroy(entry);

  SetActive();
  iPhase = EWaitingForCreate;
}


TBool CSmsSendHandler::SetupSmsHeaderL()
{
  if (iMtm) {
    CSmsClientMtm* smsMtm = static_cast<CSmsClientMtm*>(iMtm);
    smsMtm->RestoreServiceAndSettingsL();
    
    CSmsHeader&   header          = smsMtm->SmsHeader();
    CSmsSettings& serviceSettings = smsMtm->ServiceSettings();
    
    CSmsSettings* sendOptions = CSmsSettings::NewL();
    CleanupStack::PushL(sendOptions);
    sendOptions->CopyL(serviceSettings); // restore existing settings
    
    sendOptions->SetDelivery(ESmsDeliveryImmediately);      // set to be delivered immediately
    header.SetSmsSettingsL(*sendOptions);
    
    CleanupStack::PopAndDestroy(sendOptions);
    
    if (header.Message().ServiceCenterAddress().Length() == 0) {
      if (serviceSettings.NumSCAddresses() != 0) {
        CSmsNumber& sc = serviceSettings.SCAddress(serviceSettings.DefaultSC());
        header.Message().SetServiceCenterAddressL(sc.Address());
      }
      else {
        // here there could be a dialog in which user can add sc number
	iObserver.HandleError(MMsvObserver::ENoServiceCentre);
#ifdef __WINS__
	return ETrue; 
#else
	return EFalse;
#endif
      }
    }
    return ETrue;
  }
  else
    return EFalse;
}

void CSmsSendHandler::PopulateMessageL(TMsvEntry& aMsvEntry)
{
  ASSERT(iMtm);

  CRichText& mtmBody = iMtm->Body();
  mtmBody.Reset();
  mtmBody.InsertL(0, iMessageText);   

  aMsvEntry.iDetails.Set(iTelNum);  
  aMsvEntry.SetInPreparation(EFalse);         

  aMsvEntry.SetSendingState(KMsvSendStateWaiting);   
  aMsvEntry.iDate.HomeTime();                       
}

TBool CSmsSendHandler::InitializeMessageL()
{
  ASSERT(iMtm);

  TMsvEntry msvEntry = (iMtm->Entry()).Entry();

  PopulateMessageL(msvEntry);

  if (!SetupSmsHeaderL())
    return EFalse;
  
  iMtm->AddAddresseeL(iTelNum, msvEntry.iDetails);
    
  CMsvEntry& entry = iMtm->Entry();
  entry.ChangeL(msvEntry);              
  iMtm->SaveMessageL();                 

  return ETrue;
}

TBool CSmsSendHandler::MoveMessageEntryL(TMsvId aTarget)
{
  ASSERT(iMtm);

  TMsvEntry msvEntry((iMtm->Entry()).Entry());

  if (msvEntry.Parent() != aTarget) {
    TMsvSelectionOrdering sort;
    sort.SetShowInvisibleEntries(ETrue);    
    CMsvEntry* parentEntry = CMsvEntry::NewL(iMtm->Session(), msvEntry.Parent(), sort);
    CleanupStack::PushL(parentEntry);
    
    iOperation = parentEntry->MoveL(msvEntry.Id(), aTarget, iStatus);
    
    CleanupStack::PopAndDestroy(parentEntry);
    SetActive();

    return ETrue;
  }
  return EFalse;
}

void CSmsSendHandler::SetScheduledSendingStateL(CMsvEntrySelection& aSelection)
{
  ASSERT(iMtm);

  TBuf8<1> dummyParams;
  iOperation = iMtm->InvokeAsyncFunctionL(ESmsMtmCommandScheduleCopy,
					  aSelection, dummyParams, iStatus);
  SetActive();
}

void CSmsSendHandler::HandleChangedEntryL(TMsvId aEntryId)
{
  if (iMtm) {
    TMsvEntry msvEntry((iMtm->Entry()).Entry());
    if (msvEntry.Id() == aEntryId) {
      if (msvEntry.SendingState() == KMsvSendStateSent) {
        iPhase = EWaitingForDeleted;
        iObserver.HandleStatusChange(MMsvObserver::ESent);
        DeleteEntryL(msvEntry);
      }
      else if (msvEntry.SendingState() == KMsvSendStateFailed) {
        iPhase = EIdle;
        iObserver.HandleError(MMsvObserver::ESendFailed);
      }
    }
  }
}

void CSmsSendHandler::HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* /*aArg2*/, TAny* /*aArg3*/)
{
  switch (aEvent) {
  case EMsvEntriesChanged:
    {
      if (iPhase == EWaitingForSent) {
        CMsvEntrySelection* entries = static_cast<CMsvEntrySelection*>(aArg1);
        
        for(TInt i = 0; i < entries->Count(); i++)
          HandleChangedEntryL(entries->At(i));       
      }
    }
    break;
  case EMsvServerReady:
    CompleteConstructL();       
    SendToL();
    break;
  case EMsvCloseSession:
  case EMsvServerTerminated:
    iSession->CloseMessageServer();
    break;
  case EMsvGeneralError:
  case EMsvServerFailedToStart:
    iObserver.HandleError(MMsvObserver::EFatalServerError);
    break;
  default:
    break;
  }
}

class E32SyncSmsSender: public MMsvObserver
{
public:
  E32SyncSmsSender():iSmsLock(NULL),iStatus(KErrNone) {;}
  ~E32SyncSmsSender() {if (iSmsLock) delete iSmsLock;}

  static TInt SendSms(const TDesC&, const TDesC&);
  
  void HandleStatusChange(MMsvObserver::TStatus aStatus);
  void HandleError(MMsvObserver::TError aError);
  
private:
  Ao_lock* iSmsLock;
  TInt iStatus;
};

TInt E32SyncSmsSender::SendSms(const TDesC& aTelNum, const TDesC& aMsgBody)
{
  E32SyncSmsSender sender;
  sender.iSmsLock = new Ao_lock();
  if (!sender.iSmsLock)
    return KErrNoMemory;

  CMsvHandler* send_handler;
  TRAPD(error, send_handler = CSmsSendHandler::NewL(sender, aTelNum, aMsgBody));
  if (error != KErrNone)
    return error;

  sender.iSmsLock->Wait();
  delete send_handler;
  return sender.iStatus;
}

void E32SyncSmsSender::HandleStatusChange(MMsvObserver::TStatus aStatus)
{
  switch (aStatus) {
  case EDeleted:
    iSmsLock->Signal();
    break;
  case ECreated:
  case EMovedToOutBox:
  case EScheduledForSend:
  case ESent:
  default:
    break;
  }
}

void E32SyncSmsSender::HandleError(MMsvObserver::TError aError)
{
  switch (aError) {
  case ENoServiceCentre:
#ifdef __WINS__
    iStatus = KErrNone;
    iSmsLock->Signal();
    break;
#endif 
  case EScheduleFailed:
  case ESendFailed:
  case EFatalServerError:
  default:
    iStatus = KErrGeneral;
    iSmsLock->Signal();
    break;
  }
}

extern "C" PyObject *
messaging_sms_send(PyObject* /*self*/, PyObject* args)
{
  char* number;
  char* message;
  int number_l;
  int message_l;

  if (!PyArg_ParseTuple(args, "s#u#", &number, &number_l, &message, &message_l))  
    return NULL;
  
  if ((number_l > MaxTelephoneNumberLength) || (message_l > MaxMessageLength)) {
    PyErr_BadArgument();
    return NULL;
  }

  TBuf<MaxTelephoneNumberLength> tel_number;
  tel_number.FillZ(MaxTelephoneNumberLength);
  tel_number.Copy(TPtrC8((TUint8*)number, number_l));
  
  TPtrC msg_body((TUint16*)message, message_l);

  TInt error = E32SyncSmsSender::SendSms(tel_number, msg_body);

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" {

  static const PyMethodDef messaging_methods[] = {
    {"sms_send", (PyCFunction)messaging_sms_send, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initmessaging(void)
  {
    PyObject *m;

    m = Py_InitModule("messaging", (PyMethodDef*)messaging_methods);
  }
} /* extern "C" */

GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
