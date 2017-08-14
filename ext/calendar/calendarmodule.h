/*
 * ====================================================================
 *  calendarmodule.h
 *
 *  Python API to Series 60 agenda database.
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

#if SERIES60_VERSION>12
#include <vrecur.h>
#endif
#include <agclient.h>
#include <agmalarm.h>
#include <bautils.h>
#include <agmrepli.h>

#define NOT_REPEATED 0
#define DAILY_REPEAT 1
#define WEEKLY_REPEAT 2
#define MONTHLY_BY_DATES_REPEAT 3
#define MONTHLY_BY_DAYS_REPEAT 4
#define YEARLY_BY_DATE_REPEAT 5
#define YEARLY_BY_DAY_REPEAT 6

#define DAYS_IN_WEEK 7
#define DAYS_IN_MONTH 31
#define WEEKS_IN_MONTH 5
#define MONTHS_IN_YEAR 12

#define ENTRY_PTR_ARR_SIZE 100

#define EARLIEST_ALARM_DAY_INTERVAL  1001
#define LATEST_APPT_ALARM_MINUTE_INTERVAL 23*60+59

// Filters.
#define APPTS_INC_FILTER   0x01
#define EVENTS_INC_FILTER  0x02
#define ANNIVS_INC_FILTER  0x04
#define TODOS_INC_FILTER   0x08

// Keys of repeat dictionary
_LIT8(KRepeatType, "type");
_LIT8(KStartDate, "start");
_LIT8(KEndDate, "end");
_LIT8(KRepeatDays, "days");
_LIT8(KExceptions, "exceptions");
_LIT8(KInterval, "interval");

_LIT8(KWeek, "week");
_LIT8(KMonth, "month");
_LIT8(KDay, "day");

// Repeat types
_LIT8(KDaily, "daily");
_LIT8(KWeekly, "weekly");
_LIT8(KMonthlyByDates, "monthly_by_dates");
_LIT8(KMonthlyByDays, "monthly_by_days");
_LIT8(KYearlyByDate, "yearly_by_date");
_LIT8(KYearlyByDay, "yearly_by_day");
_LIT8(KRepeatNone, "no_repeat");

_LIT(KDefaultAgendaFile, "C:\\system\\data\\Calendar");
_LIT(KDefaultTodoListName, "Todo-list");

_LIT8(KUniqueId, "id");
_LIT8(KDateTime, "datetime");


//////////////TYPE DEFINITION


#define CalendarDb_type ((PyTypeObject*)SPyGetGlobalString("CalendarDbType"))
struct CalendarDb_object {
  PyObject_VAR_HEAD
  RAgendaServ* agendaServer;  
  CAgnModel* agendaModel;
};

#define Entry_type ((PyTypeObject*)SPyGetGlobalString("EntryType"))
struct Entry_object {
  PyObject_VAR_HEAD
  CalendarDb_object* calendarDb;
  CAgnEntry* entryItem;
  CParaFormatLayer* paraFormatLayer;
  CCharFormatLayer* charFormatLayer;
  TAgnUniqueId uniqueId;
  TAgnDate instanceDate;
#if SERIES60_VERSION==12
  CAgnAlarm* alarm;
#endif
};

#define EntryIterator_type ((PyTypeObject*)SPyGetGlobalString("EntryIteratorType"))
struct EntryIterator_object {
  PyObject_VAR_HEAD
  CalendarDb_object* calendarDb;
  TBool hasMoreEntries;
};

const TInt KMinsPerHour = 60;

#define KDefaultTimeForEvents TTimeIntervalMinutes(13 * KMinsPerHour)
#define KDefaultTimeForAnnivs TTimeIntervalMinutes(13 * KMinsPerHour)
#define KDefaultTimeForDayNote  TTimeIntervalMinutes(13 * KMinsPerHour)


//////////////MACRO DEFINITION///////////////


#define CHECK_AGENDA_STATE_DB \
  if(self->agendaModel->State()!=CAgnEntryModel::EOk){ \
    PyErr_SetString(PyExc_RuntimeError, "agenda state does not allow the operation"); \
    return NULL; \
  } \

#define CHECK_AGENDA_STATE_ENTRY \
  if(self->calendarDb->agendaModel->State()!=CAgnEntryModel::EOk){ \
    PyErr_SetString(PyExc_RuntimeError, "agenda state does not allow the operation"); \
    return NULL; \
  } \

#define CHECK_AGENDA_STATE_ITERATOR \
  if(self->calendarDb->agendaModel->State()!=CAgnEntryModel::EOk){ \
    PyErr_SetString(PyExc_RuntimeError, "agenda state does not allow the operation"); \
    return NULL; \
  } \

#define GET_VALUE_FROM_DICT(str_key, dict) \
  key = Py_BuildValue("s", (&str_key)->Ptr());\
  if(NULL==key){ \
    return NULL; \
  } \
  value = PyDict_GetItem(dict, key); \
  Py_DECREF(key); \

#define GET_REP_START_AND_END \
  pythonRealAsTTime(startDate, startTime);\
  pythonRealAsTTime(endDate, endTime); \
  startTime=truncToDate(startTime); \
  endTime=truncToDate(endTime); \
  if(eternalRepeat!=EFalse){ \
    endTime=startTime; \
    endTime+=TTimeIntervalDays(1); \
  } \
  if(startDate==0 || EFalse==Check_time_validity(startTime)){ \
    PyErr_SetString(PyExc_TypeError, "start date illegal or missing"); \
    return NULL; \
  } \
  if((eternalRepeat==EFalse && endDate==0) || EFalse==Check_time_validity(endTime)){ \
    PyErr_SetString(PyExc_TypeError, "end date illegal or missing"); \
    return NULL; \
  } \
  if(endTime<=startTime){ \
    PyErr_SetString(PyExc_TypeError, "end date must be greater than start date"); \
    return NULL; \
  } \

#define ADD_REPEAT \
  if(exceptionArray){ \
    for(TInt i=0;i<exceptionArray->Count();i++){ \
     TTime exceptionTime; \
     pythonRealAsTTime(exceptionArray->At(i), exceptionTime); \
     if(EFalse==Check_time_validity(exceptionTime)){\
       delete rpt; \
       PyErr_SetString(PyExc_TypeError, "illegal exception date"); \
       return NULL; \
     }\
     TAgnException exception; \
     exception.SetDate(TAgnDateTime(exceptionTime).Date()); \
     TRAPD(error1, rpt->AddExceptionL(exception)) \
     if(error1!=KErrNone){ \
       delete rpt; \
       return SPyErr_SetFromSymbianOSErr(error1); \
     }; \
    } \
  } \
  TRAPD(error2, self->entryItem->SetRptDefL(rpt)); \
  delete rpt; \
  if(error2!=KErrNone){ \
    return SPyErr_SetFromSymbianOSErr(error2); \
  } \

#define SET_REPEAT_DATES \
  rpt->SetStartDate(startTime); \
  rpt->SetEndDate(endTime); \
  if(eternalRepeat!=EFalse){ \
    rpt->SetRepeatForever(ETrue); \
  } \
  rpt->SetInterval(interval); \

#define SET_REPEAT_DATES_NO_START_DAY \
  rpt->SetEndDate(endTime); \
  if(eternalRepeat!=EFalse){ \
    rpt->SetRepeatForever(ETrue); \
  } \
  rpt->SetInterval(interval); \

#define CREATE_RPT \
  TRAPD(error0, { \
   rpt = CAgnRptDef::NewL(); \
  }); \
  if(error0!=KErrNone){ \
   return SPyErr_SetFromSymbianOSErr(error0); \
  } \

#define ADD_ITEM_TO_REP_DICT(key, value) \
  if(!(key && value)){ \
    Py_XDECREF(key); \
    Py_XDECREF(value); \
    Py_DECREF(repeatDataDict); \
    return NULL; \
  } \
  err = PyDict_SetItem(repeatDataDict, key, value); \
  Py_DECREF(key); \
  Py_DECREF(value); \
  if(err){ \
    Py_DECREF(repeatDataDict); \
    return NULL; \
  } \
      
#define SET_ITEM_TO_DAYLIST \
  if(dayNum==NULL){ \
    Py_DECREF(dayList); \
      return NULL; \
  } \
  if(PyList_SetItem(dayList, listIndex++, dayNum)){ \
    Py_DECREF(dayList); \
    return NULL; \
  } \

#define CHECK_DAYLIST_CREATION \
  if(dayList==NULL){ \
    return PyErr_NoMemory(); \
  } \

#define START_END_DATE_CHECK \
  startTTime=truncToDate(startTTime); \
  endTTime=truncToDate(endTTime); \
  if(startTTime>endTTime){ \
  PyErr_SetString(PyExc_ValueError, \
                  "start date cannot be greater than end date"); \
    return NULL; \
  } \


//////////////METHODS///////////////


/*
 * Create new CalendarDb object.
 */
extern "C" PyObject *
new_CalendarDb_object(RAgendaServ* agendaServer, CAgnModel* agendaModel);


/*
 * Creates new entry object "wrapper".
 * -fetches the entry identified by uniqueIdObj from the database and wraps that into the python object.
 * -if uniqueIdObj is null-id a new entry is created (but not added to the database until entry.commit() is called).
 * in this case entry's unique id will remain as a null-id until the entry is added (by committing) into the database. 
 */
extern "C" PyObject *
new_Entry_object(CalendarDb_object* self, TAgnUniqueId uniqueIdObj, CAgnEntry::TType entryType = CAgnEntry::EAppt);



