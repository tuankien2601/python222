/*
* ====================================================================
*  CAppuifwEventBindingArray.cpp
*
*  class CAppuifwEventBindingArray & related utilities
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

#include "CAppuifwEventBindingArray.h"

static TBool IsBoundTo(const TKeyEvent& aEv1, const TKeyEvent& aEv2)
{
  return (aEv1.iModifiers ?
          ((aEv2.iModifiers & aEv1.iModifiers) &&
           (aEv2.iCode == aEv1.iCode)) :
          (aEv2.iCode == aEv1.iCode));
}

CAppuifwEventBindingArray::~CAppuifwEventBindingArray()
{
  TInt i = 0;

  while (i < iKey.Count()) {
    Py_XDECREF(iKey[i].iCb);
    i++;
  } 
}

void CAppuifwEventBindingArray::InsertEventBindingL(SAppuifwEventBinding& aBindInfo)
{
  TInt i = 0;
  
  switch (aBindInfo.iType) {
  case SAmarettoEventInfo::EKey:
    while (i < iKey.Count()) {
      if (aBindInfo.iKeyEvent.iCode < iKey[i].iKeyEvent.iCode)
        break;
      if (IsBoundTo(aBindInfo.iKeyEvent, iKey[i].iKeyEvent)) {
        Py_XDECREF(iKey[i].iCb);
        iKey.Delete(i);
        if (aBindInfo.iCb == NULL)
          return;
        break;
      }
      i++;
    }
    iKey.InsertL(i, aBindInfo);
    break;
  default:
    User::Leave(KErrArgument);
    break;
  }
}

TInt CAppuifwEventBindingArray::Callback(SAmarettoEventInfo& aEv)
{
  TInt i = 0;

  switch (aEv.iType) {
  case SAmarettoEventInfo::EKey:
    while (i < iKey.Count()) {
      if (IsBoundTo(iKey[i].iKeyEvent, aEv.iKeyEvent))
        return app_callback_handler(iKey[i].iCb);
      if (aEv.iKeyEvent.iCode < iKey[i].iKeyEvent.iCode)
        break;
      i++;
    }
    break;
  default:
    break;
  }
  
  return KErrNone;
}
