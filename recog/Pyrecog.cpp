/*
* ====================================================================
*  Pyrecog.cpp
*
*  Recognizer plug-in
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

#include <apmrec.h>
#include <apmstd.h>
#include <f32file.h>

const TUid KUidPyrecog = {0x10201513};

class CApaPyRecognizer : public CApaDataRecognizerType
{
public:
  CApaPyRecognizer():CApaDataRecognizerType(KUidPyrecog, EHigh) {
    iCountDataTypes = 1;
  }
  virtual TUint PreferredBufSize() {return 0x10;}
  virtual TDataType SupportedDataTypeL(TInt /*aIndex*/) const {
    return TDataType(_L8("x-application/x-python"));
  }
private:
  virtual void DoRecognizeL(const TDesC& aName, const TDesC8& aBuffer);
};

void CApaPyRecognizer::DoRecognizeL(const TDesC& aName, const TDesC8& /*aBuffer*/)
{
  TParsePtrC p(aName);
  
  if ((p.Ext().CompareF(_L(".py")) == 0) ||
      (p.Ext().CompareF(_L(".pyc")) == 0) || 
      (p.Ext().CompareF(_L(".pyo")) == 0) ||
      (p.Ext().CompareF(_L(".pyd")) == 0))
    {
      iConfidence = ECertain;
      iDataType = TDataType(_L8("x-application/x-python"));
    }
}

EXPORT_C CApaDataRecognizerType* CreateRecognizer()
{
  return new CApaPyRecognizer;
}

GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
{
  return KErrNone;
}
