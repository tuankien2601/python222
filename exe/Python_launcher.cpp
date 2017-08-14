/*
* ====================================================================
*  Python_launcher.cpp
*  
*  Launchpad EXE for starting Python server scripts 
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

#include <e32std.h>
#include <utf.h>
#include "CSPyInterpreter.h"

static TInt AsyncRunCallbackL(TAny* aArg)
{
  char *argv[1];
  int argc;
  TBuf8<KMaxFileName> namebuf;

  CnvUtfConverter::ConvertFromUnicodeToUtf8(namebuf, *((TDesC*)aArg));
  argv[0] = (char*)namebuf.PtrZ();
  argc = 1;

  CSPyInterpreter* interp = CSPyInterpreter::NewInterpreterL();
  interp->RunScript(argc, argv);
  delete interp;
  
  CActiveScheduler::Stop();

  return KErrNone;
}

static void RunServerL(const TDesC& aScriptName)
{
  CActiveScheduler* as = new (ELeave) CActiveScheduler;
  CleanupStack::PushL(as);
  CActiveScheduler::Install(as);
  
  TCallBack cb(&AsyncRunCallbackL, (TAny*)&aScriptName);
  CAsyncCallBack* async_callback =
    new (ELeave) CAsyncCallBack(cb, CActive::EPriorityHigh);
  CleanupStack::PushL(async_callback);
  async_callback->CallBack();

  CActiveScheduler::Start();

  CleanupStack::PopAndDestroy(2);
}

#if defined(__WINS__)
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
EXPORT_C TInt WinsMain(TAny *aScriptName)
#else
GLDEF_C TInt E32Main()
#endif
{
  TInt error;
  TFileName script;
#if defined(__WINS__)
  script.Copy(*((TDesC*)aScriptName));
  delete (HBufC*)aScriptName;
#else
  RProcess().CommandLine(script);
#endif
  CTrapCleanup* cleanupStack = CTrapCleanup::New();

  TRAP(error, RunServerL(script));
  if (error != KErrNone)
    User::Panic(_L("Python server script"), error);

  delete cleanupStack;
  return KErrNone;
}
