/*
 * ====================================================================
 *  Container.cpp
 *
 *  Window-owning container control class CAmarettoContainer
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

#include "Container.h"

CAmarettoContainer* CAmarettoContainer::NewL(const TRect& aRect)
{
  CAmarettoContainer *self = new (ELeave) CAmarettoContainer;
  CleanupStack::PushL(self);
  self->ConstructL(aRect);
  CleanupStack::Pop();
  return self;
} 

void CAmarettoContainer::ConstructL(const TRect& aRect)
{
  CreateWindowL();
  iTop = NULL;
  SetRect(aRect);
  ActivateL();
}

CAmarettoContainer::~CAmarettoContainer()
{
  if (iDecoratedTabGroup) {
    GetNaviPane()->Pop(iDecoratedTabGroup);
    delete iDecoratedTabGroup;
  }
}

CAknNavigationControlContainer* CAmarettoContainer::GetNaviPane() const
{
  CEikStatusPane* sp = 
    (STATIC_CAST(CAknAppUi*, iEikonEnv->EikAppUi()))->StatusPane();
  return (STATIC_CAST(CAknNavigationControlContainer*,
                      sp->ControlL(TUid::Uid(EEikStatusPaneUidNavi))));
}

void CAmarettoContainer::EnableTabsL(const CDesCArray* aTabTexts,
                                     CAmarettoCallback* aFunc)
{
  if (iDecoratedTabGroup) {
    GetNaviPane()->Pop(iDecoratedTabGroup);
    delete iDecoratedTabGroup;
  }
  if (aTabTexts) {
    iDecoratedTabGroup = GetNaviPane()->CreateTabGroupL();
    iTabGroup = STATIC_CAST(CAknTabGroup*,
                            iDecoratedTabGroup->DecoratedControl());
  
    for (int i = 0; i < aTabTexts->Count(); i++)
      iTabGroup->AddTabL(i, (*aTabTexts)[i]);
  
    GetNaviPane()->PushL(*iDecoratedTabGroup);
    iTabCallback = aFunc;
    iTabGroup->SetActiveTabByIndex(0);
    
    if (iTop)
      iTop->SetFocus(EFalse);
    iTabGroup->SetFocus(ETrue);

    //  Refresh();
  }
  else {
    iDecoratedTabGroup = NULL;
    iTabGroup = NULL;
    iTabCallback = NULL;
    if (iTop)
      iTop->SetFocus(ETrue);
  }
}

void CAmarettoContainer::SetActiveTab(TInt aIndex)
{
  if (iTabGroup && (aIndex >= 0) && (aIndex < iTabGroup->TabCount()))
    iTabGroup->SetActiveTabByIndex(aIndex);
}

TInt CAmarettoContainer::SetComponentL(CCoeControl* aComponent,
                                       CAmarettoCallback* aEventHandler)
{
  if (iTop)
    iTop->MakeVisible(EFalse);
  iTop = aComponent;
  if (iTop)
    iTop->MakeVisible(ETrue);
  iEventCallback = aEventHandler;

  if (aComponent && !iTabGroup)
    aComponent->SetFocus(ETrue);

  Refresh();

  return KErrNone;
}

void CAmarettoContainer::Draw(const TRect& aRect) const
{
  CWindowGc& gc = SystemGc();
  gc.SetPenStyle(CGraphicsContext::ENullPen);
  gc.SetBrushColor(KRgbWhite);
  gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
  gc.DrawRect(aRect);
}

TKeyResponse
CAmarettoContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
  if (iTabGroup && ((aKeyEvent.iCode == EKeyLeftArrow) ||
                    (aKeyEvent.iCode == EKeyRightArrow))) {
    
    TInt active = iTabGroup->ActiveTabIndex();
    TInt count = iTabGroup->TabCount();
    
    if ((aKeyEvent.iCode == EKeyLeftArrow) && (active > 0))
      active--;
    else if ((aKeyEvent.iCode == EKeyRightArrow) && ((active + 1) < count))
      active++;
    else
      return EKeyWasConsumed;
    
    iTabGroup->SetActiveTabByIndex(active);
    iTabCallback->Call((void*)active);

    Refresh();
    
    return EKeyWasConsumed;
  } 
  
  if (iTop) {
    if ((aType == EEventKey) && iEventCallback) {
      SAmarettoEventInfo event_info;
      event_info.iType = SAmarettoEventInfo::EKey;
      event_info.iKeyEvent = aKeyEvent;
      iEventCallback->Call((void*)&event_info);
    }
    return iTop->OfferKeyEventL(aKeyEvent, aType);
  }
  else
    return EKeyWasNotConsumed;
}
