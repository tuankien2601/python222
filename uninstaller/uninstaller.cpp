/*
* ====================================================================
*  installer.cpp
*  
*  Python for Series 60 installation program.
*
*  Performs cleanup of Python -related files on removal of
*  Python for Series 60.
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
#include <f32file.h>

void RemoveLibs(RFs& aFs)
{
  TFindFile ff(aFs);
  CDir* ls;
  TInt error = ff.FindWildByDir(_L("*.py*"), _L("\\system\\libs\\"), ls);
  while (error == KErrNone) {
    TInt i;
    for (i = 0; i < ls->Count(); i++) {
      TParse p;
      p.Set((*ls)[i].iName, &(ff.File()), NULL);
      if (!(aFs.IsFileInRom(p.FullName())))
        aFs.Delete(p.FullName());
    }
    delete ls;
    error = ff.FindWild(ls);
  }
}

HBufC* AppDirsL(RFs& aFs)
{
  HBufC* hbuf = HBufC::NewL(0x2000);
  TPtr* buf = &(hbuf->Des());

  TFindFile ff(aFs);
  CDir* ls;
  TInt error = ff.FindWildByDir(_L("*"), _L("\\system\\apps\\"), ls);
  while (error == KErrNone) {
    TInt i;
    for (i = 0; i < ls->Count(); i++) {
      if ((*ls)[i].IsDir()) {
        TParse p;
        p.Set((*ls)[i].iName, &(ff.File()), NULL);
        if (!(aFs.IsFileInRom(p.FullName()))) {
          if ((p.FullName().Length()+buf->Length()+2) > buf->MaxLength()) {
            hbuf = hbuf->ReAllocL(hbuf->Length()+1024);
            buf = &(hbuf->Des());
          }
          buf->Append(p.FullName());
          buf->Append(TChar('\\'));
          buf->Append(TChar(';'));
        }
      }
    }
    delete ls;
    error = ff.FindWild(ls);
  }
  return hbuf;
}

void RemoveStandalones(RFs& aFs, CFileMan* aFm)
{
  TFindFile ff(aFs);
  CDir* ls;
  TDesC* app_dirs = AppDirsL(aFs);
  TInt error = ff.FindWildByPath(_L("default.py*"), app_dirs, ls);
  while (error == KErrNone) {
    aFm->RmDir(ff.File());
    delete ls;
    error = ff.FindWild(ls);
  }
  User::Free(app_dirs);
}

void DoCleanupL()
{
  RFs rfs;
  User::LeaveIfError(rfs.Connect());
  RemoveLibs(rfs);
  CFileMan* fm = CFileMan::NewL(rfs);
  RemoveStandalones(rfs, fm);
  User::Free(fm);
  rfs.Close();
}

#if defined(__WINS__)
EXPORT_C TInt WinsMain(TAny* /*aDocName*/)
#else
GLDEF_C TInt E32Main()
#endif
{
  CTrapCleanup* cleanup_stack = CTrapCleanup::New();
  //  CActiveScheduler* scheduler = new (ELeave) CActiveScheduler();
  //  CActiveScheduler::Install(scheduler);
  TRAPD(error, DoCleanupL());
  //  delete scheduler;
  delete cleanup_stack;
  return KErrNone;
}

#if defined(__WINS__)
GLDEF_C TInt E32Dll(TDllReason)
{
  return(KErrNone);
}
#endif
