/*
* ====================================================================
*  appuifwmodule.h
*
*  Python API to Series 60 application UI framework facilities
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

#ifndef __APPUIFWMODULE_H
#define __APPUIFWMODULE_H

#include "popup_field_hack.h" // Form

#include <eikenv.h>
#include <f32file.h>
#include <s32mem.h>
#include <apmstd.h>
#include <apparc.h>
#include <aknenv.h>
#include <aknappui.h>
#include <aknapp.h>
#include <eikon.hrh>          // Form
#include <avkon.hrh>
#include <avkon.rsg>
#include <avkon.mbg>	      // multiselectionlist
#include <barsread.h>
#include <eikedwin.h>         // Form
#include <eikcapc.h>          // Form
#include <txtfrmat.h>         // Text
#include <txtfmlyr.h>         // Text
#include <eikgted.h>          // Text
#include <eikrted.h>	      // RText
#include <txtrich.h>
#include <gdi.h>	      // RText
#include <aknlists.h>
#include <aknselectionlist.h>
#include <aknquerydialog.h>
#include <akntitle.h>
#include <aknnotewrappers.h>
#include <aknPopup.h>
#include <AknForm.h>
#include <aknglobalnote.h>  // for global note
#include <aknnotifystd.h>
#include <documenthandler.h>
#include <appuifwmodule.rsg>
#include <Python.rsg>	      //for submenus
#include "Python.h"
#include "structmember.h"     // Form
#include "Python_appui.h"
#include "symbian_python_ext_util.h"
#include "appuifwmodule.hrh"
#include "CAppuifwEventBindingArray.h"
#include "eikclbd.h"	      // for icons in Listbox

_LIT(KSeparatorTab, "\t");
_LIT(KEmptyString, "");
_LIT(KAppuiFwRscFile, "\\system\\data\\appuifwmodule.rsc");
_LIT8(KTextFieldType, "text");
_LIT(KUcTextFieldType, "text");
_LIT8(KNumberFieldType, "number");
_LIT(KUcNumberFieldType, "number");
_LIT8(KFloatFieldType, "float");
_LIT(KUcFloatFieldType, "float");
_LIT8(KDateFieldType, "date");
_LIT(KUcDateFieldType, "date");
_LIT8(KTimeFieldType, "time");
_LIT(KUcTimeFieldType, "time");
_LIT8(KCodeFieldType, "code");
_LIT(KUcCodeFieldType, "code");
_LIT8(KQueryFieldType, "query");
_LIT(KUcQueryFieldType, "query");
_LIT8(KComboFieldType, "combo");
_LIT8(KErrorNoteType, "error");
_LIT8(KInformationNoteType, "info");
_LIT8(KConfirmationNoteType, "conf");
_LIT8(KFlagAttrName, "flags");
_LIT(KAppuifwEpoch, "19700000:");
_LIT8(KNormal, "normal");
_LIT8(KAnnotation, "annotation");
_LIT8(KTitle, "title");
_LIT8(KLegend, "legend");
_LIT8(KSymbol, "symbol");
_LIT8(KDense, "dense");
_LIT8(KCheckboxStyle, "checkbox");
_LIT8(KCheckmarkStyle, "checkmark");


#define MY_APPUI ((get_app())->ob_data->appui)
enum ListboxType {ESingleListbox, EDoubleListbox, ESingleGraphicListbox, EDoubleGraphicListbox };

/*
 * Python types defined in this module
 */

#define Application_type ((PYTHON_GLOBALS->tobj).t_Application)
#define Listbox_type ((PYTHON_GLOBALS->tobj).t_Listbox)
#define Content_handler_type ((PYTHON_GLOBALS->tobj).t_Content_handler)
#define Text_type ((PYTHON_GLOBALS->tobj).t_Text)
#define Form_type ((PYTHON_GLOBALS->tobj).t_Form)
#define Canvas_type (PYTHON_GLOBALS->t_Canvas)
#define Icon_type (PYTHON_GLOBALS->t_Icon)

/*
 * Structure of Python objects that represent UI controls and
 * a check for control objects (hardcoded types -- bad)
 */

class CAppuifwEventBindingArray;

struct _control_object {
  PyObject_VAR_HEAD
  CCoeControl* ob_control;
  CAppuifwEventBindingArray* ob_event_bindings;
};

TInt AppuifwControl_Check(PyObject *obj);
struct _control_object *AppuifwControl_AsControl(PyObject *obj);

/*
 * appuifw.Application object type representation
 */

struct Application_data;

struct Application_object {
  PyObject_VAR_HEAD
  PyObject *ob_dict_attr;
  PyObject *ob_menu;
  PyObject *ob_body;
  PyObject *ob_title;
  PyObject *ob_screen;
  Application_data *ob_data;
};

/*
 * A helper function for the implementation of callbacks
 * from C/C++ code to Python callables
 */

TInt app_callback_handler(void* data);
TInt app_callback_handler(void* data, void* arg);

#endif /* __APPUIFWMODULE_H */
