/*
 * ====================================================================
 *  AppMgr.cpp
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

#include <f32file.h>
#include <eikenv.h>
#include <eikappui.h>
#include <AppMgr.rsg>
#include "Python_appui.h"
#include "AppMgr.h"

const TUid KUidAppMgrApp = {0x10201512};
_LIT(KInboxSubPath, "\\System\\Mail\\");
_LIT(KDefaultScript, "default.py");

CEikAppUi* CAppMgrDocument::CreateAppUiL() 
{
  CEikAppUi* appui = CreateAmarettoAppUi(R_APPMGR_EXTENSION_MENU);
  if (!appui)
    User::Leave(KErrNoMemory);
  return appui;
}

CFileStore* CAppMgrDocument::OpenFileL(TBool /*aDoOpen*/, const TDesC& aFileName, RFs& /*aFs*/)
{
  //  if (aFileName.Find(KInboxSubPath) != KErrNotFound) {
  TFileName appName = Application()->AppFullName();
  TParse p;
  p.Set(KDefaultScript, &appName, NULL);
  
  (STATIC_CAST(CAmarettoAppUi*, CEikonEnv::Static()->EikAppUi()))->RunScriptL(p.FullName(), &aFileName);
  // This would be insecure! Make sure execution always passes through our default.py.
  /*  }
      else
      (STATIC_CAST(CAmarettoAppUi*, CEikonEnv::Static()->EikAppUi()))->RunScriptL(aFileName);
  */
  return NULL;
}

TUid CAppMgrApplication::AppDllUid() const 
{
  return KUidAppMgrApp;
}

CApaDocument* CAppMgrApplication::CreateDocumentL() 
{
  CAppMgrDocument* document = new (ELeave) CAppMgrDocument(*this);
  return document;
}

EXPORT_C CApaApplication* NewApplication() 
{
  return new CAppMgrApplication;
}

GLDEF_C TInt E32Dll(TDllReason) 
{
  return KErrNone;
}
