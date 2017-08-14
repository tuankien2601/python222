/*
 * ====================================================================
 *  Container.h
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

#ifndef __CONTAINER_H
#define __CONTAINER_H

#include <coecntrl.h>
#include <eikdef.h>
#include <eiklbx.h>
#include <eiklbo.h>
#include <aknnavi.h>
#include <aknnavide.h>
#include <akntabgrp.h>
#include "Python_appui.h"

class CAmarettoContainer : public CCoeControl, MCoeControlObserver
{
public:
  static CAmarettoContainer *NewL(const TRect& aRect);
  virtual ~CAmarettoContainer();
  
  TInt SetComponentL(CCoeControl* aComponent, 
		     CAmarettoCallback* aEventHandler=NULL);
  void Refresh() {SizeChanged(); DrawNow();}
  void EnableTabsL(const CDesCArray* aTabTexts, CAmarettoCallback* aFunc);
  void SetActiveTab(TInt aIndex);

protected:
  void ConstructL(const TRect&);

private:
  void SizeChanged() {if (iTop) iTop->SetRect(Rect());}
  TInt CountComponentControls() const {return (iTop ? 1 : 0);}
  CCoeControl* ComponentControl(TInt aIx) const {return ((aIx == 0) ? iTop : NULL);}
  void Draw(const TRect& aRect) const;
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);
  void HandleControlEventL(CCoeControl*, TCoeEvent) {;}
  
private:
  CAknNavigationControlContainer* GetNaviPane() const;
  CAknNavigationDecorator* iDecoratedTabGroup;
  CAknTabGroup* iTabGroup;
  CCoeControl* iTop;
  CAmarettoCallback* iTabCallback;
  CAmarettoCallback* iEventCallback;
};

#endif /* __CONTAINER_H */
