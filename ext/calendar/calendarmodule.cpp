/*
 * ====================================================================
 * calendarmodule.cpp
 * 
 * Python API to Series 60 agenda database.
 *
 * Implements currently (30.05.2005) following Python methods and types:
 *
 * open()
 * - open the default calendar database.
 * open(<filename>)
 * - open the specified calendar database.
 * open(<filename>, 'c')
 * - open the specified calendar database. create if does not exist.
 * open(<filename>, 'n')
 * - create new, empty calendar database.
 *
 * CalendarDb
 *  add_entry()
 *  - create new entry. note that the entry is not added and saved 
 *    into the database unless entry.commit() is called.
 *  find_instances(float, float, unicode [, int])
 *  - parameters: start datetime, end datetime, search string, filter.
 *  - returns list that contains entry instances found in the search.
 *    however, note that all the returned instances occur in the same day
 *    (the first day between start and end datetime that contains instances).
 *    to search all instances between (initial) start and end datetime user 
 *    may have to execute several searches (and change the start datetime for
 *    each search). 
 *    match is detected if search string is a substring of entry's content.
 *    if filter is given in the optional int parameter the search 
 *    is reduced to entry types specified by the filter.
 *  monthly_instances(float)
 *  - parameters: datetime
 *  - returns list that contains entry instances occurring during the specified
 *    calendar month.
 *  daily_instances(float)
 *  - parameters: datetime
 *  - returns list that contains entry instances occurring during the specified
 *    day.
 *  todos_in_list(int)
 *  - parameters: todo list id
 *  - returns list that contains todo entries in the specified todo list.
 *  todo_lists() 
 *  - returns dictionary that contains todo lists (todo list id as a key
 *    and todo list name as a value)
 *  add_todo_list([unicode])
 *  - creates new todo list.
 *  - parameters: todo list name (optional)
 *  - returns id of the created todo list
 *  rename_todo_list(int, unicode)
 *  - renames specified todo list.
 *  - parameters: todo list id, new name of the todo list
 *  remove_todo_list(int)
 *  - removes specified todo list. note that the todo entries in the todo list
 *    are not deleted.
 *  - parameters: todo list id
 *  export_vcals(tuple<int,..>)
 *  - returns vcalendar string that contains specified entries.
 *  - parameters: tuple containing entry ids of exported entries
 *  import_vcals(string)
 *  - imports vcalendar entries (given in the string parameter) to the database. 
 *  - returns tuple that contains unique ids of the imported entries.
 *  replication()
 *  - returns entry's replication status.
 *  set_replication()
 *  - sets entry's replication status (EOpen, EPrivate, ERestricted).
 *  default_todo_list()
 *  -returns the id of the default todo list.
 *  
 *
 * EntryIterator
 *  next()
 *  - returns next entry id.
 *
 * Entry
 *  content()
 *  - returns entry's content text (unicode).
 *  set_content(unicode)
 *  - sets entry's content text (unicode).
 *  type()
 *  - returns entry's type (int): entry_type_appt, entry_type_event, entry_type_anniv or entry_type_todo
 *  commit()
 *  - saves the entry (or adds the entry into the database [in case of new entry]).
 *  - returns the id of the entry.
 *  set_repeat_data(dictionary)
 *  - sets the repeat data of the entry.
 *  - parameters: repeat data. contains all the repeat rules. see the further information below.
 *  repeat_data()
 *  - returns the repeat data of the entry.
 *  set_location(unicode)
 *  - sets entry's location data (the meeting room etc.)
 *  - parameters: the location.
 *  location()
 *  - returns entry's location data (unicode)
 *  set_start_and_end_datetime(float, float)
 *  - sets start datetime and end datetime of the entry.
 *  - parameters: start datetime, end datetime
 *  start_datetime()
 *  - returns start datetime (float) of the entry.
 *  end_datetime()
 *  - returns end datetime (float) of the entry.
 *  unique_id()
 *  - returns unique id of the entry
 *  last_modified()
 *  - returns datetime (float) of the entry's last modification (universal time).
 *  set_alarm(float)
 *  - sets alarm for the entry (or removes if the parameter equals to 0).
 *  - parameters: datetime of the alarm. see the further information below to check
 *    the restrictions of the alarm datetime. 
 *  has_alarm()
 *  - returns 0 if the entry is not alarmed, otherwise <> 0.
 *  alarm_datetime()
 *  - returns alarm datetime (float) of this entry.
 *  cancel_alarm()
 *  - cancels entry's alarm.
 *  set_priority(int)
 *  - sets the priority of the entry (1=high, 2=medium, 3=low)
 *  - parameters: the priority
 *  priority()
 *  - returns entry's priority.
 *  set_todo_list(int)
 *  - sets the todo list of the entry (todo entries only).
 *  - parameter: the todo list id
 *  todo_list_id()
 *  - returns the id of the todo list that contains this entry (todo entries only).
 *  set_crossed_out(float)
 *  - sets the crossed out value for the entry. if the entry is a todo then
 *    the parameter must represent a valid datetime value (cross out datetime) or
 *    equal to 0. in case of other entry types there is no restrictions for the value.
 *    if the value equals to 0 the entry is uncrossed out, otherwise it is crossed out.
 *  - parameters: crossed out datetime
 *  is_crossed_out()
 *  - returns 0 if the entry is not crossed out, otherwise <> 0
 *  crossed_out_date()
 *  - returns datetime (float) of the cross out (this is for todos only).
 *  make_undated()
 *  - makes todo entry undated (removes start and end dates and repeat rules)
 *  is_dated()
 *  - returns information whether the todo entry is dated (or undated).
 *
 * FURTHER INFORMATION:
 *
 *
 *
 * repeat rules:
 * - repeat rules specify entry's repeat status. this is much like the "recurrence" functionality
 *   in outlook's calendar appointment. 
 * - there are 6 repeat types: daily, weekly, monthly_by_dates, monthly_by_days, 
 *   yearly_by_date and yearly_by_day.
 *   - daily: repeated daily
 *   - weekly_by_days: repeat on the specified days of the week (like monday and wednesday etc.)
 *   - monthly_by_dates: repeat monthly on the specified dates (like 15th and 17th day of the month)
 *   - monthly_by_days: repeat monthly on the specified days (like fourth wednesday of the month,
 *     last monday of the month)
 *   - yearly_by_date: repeat yearly on the specified date (like 24th December)
 *   - yearly_by_day: repeat yearly on the specified day (like every third tuesday of may).
 * - there are exceptions e.g. you can specify datetime (float) such that there is no repeat of 
 *   the entry that day even if the repeat rule would specify that there is.
 * - you must set the start_date and end_date (floats) of the repeat, and you can set interval 
 *   (how often repeat occurs e.g. in case of daily repeat 1=every day, 2=every second day etc.), 
 *   and the "days"-specifier which lets you explicitely specify the repeat days (e.g in 
 *   case of weekly-repeat you can set "days":[0,2] which sets the repeat to occur these days
 *   (monday,wednesday). if you do not set the "days"-specifier it is calculated automatically
 *   using the start_date-value).
 * - note that you can modify repeat data by calling rep_data = entry.repeat_data(), then making
 *   changes to rep_data and then calling entry.set_repeat_data(rep_data).
 *
 * - repeat definition examples:
 *
 * repeat = {"type":"daily", #repeat type
 *           "exceptions":[exception_day, exception_day+2*24*60*60], #no appointment those days 
 *           "start":appt_start_date, #start of the repeat
 *           "end":appt_start_date+30*24*60*60, #end of the repeat
 *           "interval":1} #interval (1=every day, 2=every second day etc.)
 * 
 * repeat = {"type":"weekly", #repeat type
 *           "days":[0,1], #which days in a week (monday,tuesday)
 *           "exceptions":[exception_day], #no appointment that day 
 *           "start":appt_start_date, #start of the repeat
 *           "end":appt_start_date+30*24*60*60, #end of the repeat
 *           "interval":1} #interval (1=every week, 2=every second week etc.) 
 *
 * repeat = {"type":"monthly_by_days", #repeat type
 *           # appointments on second tuesday and last monday of the month
 *           "days":[{"week":1, "day":1},{"week":4, "day":0}],
 *           "exceptions":[exception_day], #no appointment that day 
 *           "start":appt_start_date, #start of the repeat
 *           "end":appt_start_date+30*24*60*60, #end of the repeat
 *           "interval":1} #interval (1=every month, 2=every second month etc.)
 *
 * repeat = {"type":"monthly_by_dates", #repeat type
 *           "days":[0,15], # appointments on first and 14th day of the month. 
 *           "exceptions":[exception_day], #no appointment that day 
 *           "start":appt_start_date, #start of the repeat
 *           "end":appt_start_date+30*24*60*60, #end of the repeat
 *           "interval":1} #interval (1=every month, 2=every second month etc.)
 *
 * repeat = {"type":"yearly_by_date", #repeat type
 *           "exceptions":[exception_day], #no appointment that day 
 *           "start":appt_start_date, #start of the repeat
 *           "end":appt_start_date+3*365*24*60*60, #end of the repeat
 *           "interval":1} #interval (1=every year, 2=every second year etc.)
 *
 * repeat = {"type":"yearly_by_day", #repeat type
 *           # appointments on second tuesday of february
 *           "days":{"day":1, "week":1, "month":1},
 *           "exceptions":[exception_day], #no appointment that day 
 *           "start":appt_start_date, #start of the repeat
 *           "end":appt_start_date+3*365*24*60*60, #end of the repeat
 *           "interval":1} #interval (1=every year, 2=every second year etc.)
 *
 *
 *
 * QUIRKS:
 *
 * -alarms can be set for all entry types but they work only with appointments and anniversaries
 *  (compare: with nokias native applications you cannot set an alarm for memos (=events) or todos).
 *
 * -if you press snooze when the phone alarms and then cancel the alarm it will still alarm after snoozing although
 *  the alarm has been cancelled (again, this is just how nokias native calendar application behaves).
 *
 * -if you use other than the default database alarms will not work (you can set them but they do nothing).
 *
 * -if you set an alarm after appoinment's start beware that the native calendar application tries
 *  to change the alarm time (to precede the appointment by 15 minutes).
 *
 * set_alarm:
 * - it seems that alarm can be set 1001 days before the entrys start day 
 *   (except with todo. with todo it is 1001 days before the entrys end day [due date])
 * - it seems the alarm can be set 23h 59 min after the entrys start datetime for appt
 *   (but if the date of the alarm is greater than entry's start date an "overflow" of the 
 *   alarm time occurs e.g. if the entry starts at 23.50 and alarm is set 20 minutes later
 *   the symbian api sets the alarm to occur at 00.10 on the same day as the entry starts!
 *   the alarm is therefore set 23h and 40min before(!) the entry's start!. To avoid this mess
 *   this extension checks that the alarm occurs earlier or on the same day as 
 *   the entry starts. otherwise an exception is raised.).
 *   for other entry types the the alarm can be set as much after the start datetime 
 *   (end/due datetime for todo's) as there is time left within that same day. this extension
 *   checks the alarm datetime similarly in case of any entry type. 
 *
 *
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
 *
 * ====================================================================
 */


#include "calendarmodule.h"
 


//////////////GENERAL FUNCTIONS///////////////


/*
 * (copied from appuifw).
 */
static TReal epoch_as_TReal()
{
  _LIT(KAppuifwEpoch, "19700000:");
  TTime epoch;
  epoch.Set(KAppuifwEpoch);
  return epoch.Int64().GetTReal();
}


/*
 * (copied from appuifw).
 */
static TReal time_as_UTC_TReal(const TTime& aTime)
{
  TLocale loc;
  return (((aTime.Int64().GetTReal()-epoch_as_TReal())/
          (1000.0*1000.0))-
          TInt64(loc.UniversalTimeOffset().Int()).GetTReal());
}


/*
 * Converts TReal time representation to TTime value.
 */
void pythonRealAsTTime(TReal timeValue, TTime& theTime)
{
  TLocale loc;
  TInt64 timeInt((timeValue + 
                 TInt64(loc.UniversalTimeOffset().Int()).GetTReal())
                 *(1000.0*1000.0)+epoch_as_TReal());
  theTime = timeInt;
}


/*
 * Converts TTime value to python float.
 */
extern PyObject *
ttimeAsPythonFloat(const TTime& timeValue)
{
  return Py_BuildValue("d",time_as_UTC_TReal(timeValue));
}


/*
 * Returns date value to corresponds the given datetime value.
 */
TTime truncToDate(const TTime& theTime)
{
  TDateTime aDateTime = theTime.DateTime();
  aDateTime.SetHour(0); 
  aDateTime.SetMinute(0); 
  aDateTime.SetSecond(0); 
  return TTime(aDateTime);
}


/*
 * Tests if the file exists.
 */
TBool testFileExistenceL(TDesC& filename){
  TBool fileExists = EFalse;
  RFs fileSession;
  User::LeaveIfError(fileSession.Connect());
  CleanupClosePushL(fileSession);
  fileExists = BaflUtils::FileExists(fileSession, filename);
  CleanupStack::PopAndDestroy(); // Close fileSession.
  return fileExists;
}


/*
 * Creates new agenda file.
 */
void createNewAgendaFileL(TDesC& filename){
  RFs fileSession;
  User::LeaveIfError(fileSession.Connect());
  CleanupClosePushL(fileSession);
  TFileName aFilename(filename);
  CParaFormatLayer* paraFormatLayer = CParaFormatLayer::NewL();
  CleanupStack::PushL(paraFormatLayer);
  CCharFormatLayer* charFormatLayer = CCharFormatLayer::NewL();
  CleanupStack::PushL(charFormatLayer);

  CAgnModel* agendaModel = CAgnModel::NewL(NULL);
  CleanupStack::PushL(agendaModel);
  agendaModel->CreateL(fileSession, 
               aFilename,
               KDefaultTodoListName, 
               paraFormatLayer, 
               charFormatLayer);

  CleanupStack::PopAndDestroy(); // agendaModel. 
  CleanupStack::PopAndDestroy(); // charFormatLayer. 
  CleanupStack::PopAndDestroy(); // paraFormatLayer. 
  CleanupStack::PopAndDestroy(); // Close fileSession.  
}


/*
 * Checks if the value represents a weekday.
 */
TBool isWeekday(TInt value)
{
  TBool retVal = ETrue;
  switch(value){
    case EMonday:
    break;
    case ETuesday:
    break;
    case EWednesday:
    break; 
    case EThursday:
    break;
    case EFriday:
    break;
    case ESaturday:
    break;
    case ESunday:
    break;
    default:
      retVal = EFalse;
    break;
  }
  return retVal;
}


/*
 * Checks if the value represents a weeknumber (in month).
 */
TBool isWeekInMonth(TInt value)
{
  TBool retVal = ETrue;
  switch(value){
    case TAgnRpt::EFirst:
    break;
    case TAgnRpt::ESecond:
    break;
    case TAgnRpt::EThird:
    break;
    case TAgnRpt::EFourth:
    break;
    case TAgnRpt::ELast:
    break;
    default:
      retVal = EFalse;
    break;
  }
  return retVal;
}


/*
 * Checks if the value represents a month.
 */
TBool isMonth(TInt value)
{
  TBool retVal = ETrue;
  switch(value){
    case EJanuary:
    break;
    case EFebruary:
    break;
    case EMarch:
    break;
    case EApril:
    break;
    case EMay:
    break;
    case EJune:
    break;
    case EJuly:
    break;
    case EAugust:
    break;
    case ESeptember:
    break;
    case EOctober:
    break;
    case ENovember:
    break;
    case EDecember:
    break;
    default:
      retVal = EFalse;
    break;
  }
  return retVal;
}


/*
 * Checks if the time value is in the limits of agenda model (and not null).
 */
TBool Check_time_validity(const TTime& theTime)
{
  if(AgnDateTime::MaxDateAsTTime()<theTime || 
    AgnDateTime::MinDateAsTTime()>theTime ||
    AgnDateTime::TTimeToAgnDate(theTime)==AgnDateTime::NullDate()){
    return EFalse; // Illegal time value.
  }else{
    return ETrue;
  }
}


/*
 * Checks if the time value represents agenda model's null time.
 */
TBool Is_null_time(const TTime& theTime)
{
  if(AgnDateTime::TTimeToAgnDate(theTime)==AgnDateTime::NullDate()){
    return ETrue;
  }else{
    return EFalse;
  }
}


/*
 * Debug printing.
 */
void
pyprintf(const char *format, ...)
{
  va_list args;
  va_start(args,format);
  char buf[128];
  // Unfortunately Symbian doesn't seem to support the vsnprintf
  // function, which would allow us to specify the length of the
  // buffer we are writing to. As a result using this function is
  // unsafe and will lead to a buffer overflow if given an argument
  // that is too big. Do not use this function in production code.
  vsprintf(buf, format, args);    
  char buf2[128];
  sprintf(buf2, "print '%s'", buf);
  PyRun_SimpleString(buf2);
}


//////////////TYPE METHODS///////////////


/*
 * Module methods.
 */


/*
 * Opens the database and creates and returns a CalendarDb-object.
 *
 * open() - opens the default agenda database file
 * open(u'filename') - opens file if it exists.
 * open(u'filename', 'c') - opens file, creates if the file does not exist.
 * open(u'filename', 'n') - creates new empty database file.
 */
extern "C" PyObject *
open_db(PyObject* /*self*/, PyObject *args)
{  
  PyObject* filename = NULL;
  char* flag = NULL;
  TInt userError = 0;
  HBufC* filenameBuf = NULL;
  RAgendaServ* agendaServer = NULL;
  CAgnModel* agendaModel = NULL;
  if (!PyArg_ParseTuple(args, "|Us", &filename, &flag)){ 
    return NULL;
  }
  
  // Check filename length.
  if(filename && PyUnicode_GetSize(filename)>KMaxFileName){
    PyErr_SetString(PyExc_NameError, "filename too long");
    return NULL;
  }

  // Check the parameters and create new agenda file if needed.
  TRAPD(error1, {     
    if(!flag){
      if(!filename){
        filenameBuf = HBufC::NewL((&KDefaultAgendaFile)->Length());
        filenameBuf->Des().Append(KDefaultAgendaFile);
        if(!testFileExistenceL(*filenameBuf)){
          // Default file does not exist. Create it.       
          createNewAgendaFileL(*filenameBuf);
        }
      }else{
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), 
                          PyUnicode_GetSize(filename));
        filenameBuf = HBufC::NewL(filenamePtr.Length());
        filenameBuf->Des().Append(filenamePtr);
      }
      if(!testFileExistenceL(*filenameBuf)){
        // File does not exist.       
        userError = 1;
      }
    }else{
      if(filename && flag[0] == 'c'){
        // Open, create if file doesn't exist.
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), 
                          PyUnicode_GetSize(filename));
        filenameBuf = HBufC::NewL(filenamePtr.Length());
        filenameBuf->Des().Append(filenamePtr);
        if(!testFileExistenceL(*filenameBuf)){
          createNewAgendaFileL(*filenameBuf);
        }
      }else if(filename && flag[0] == 'n'){
        // Create a new empty file.
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), 
                          PyUnicode_GetSize(filename));
        filenameBuf = HBufC::NewL(filenamePtr.Length());
        filenameBuf->Des().Append(filenamePtr);
        createNewAgendaFileL(*filenameBuf);
      }else{
        // Illegal parameter combination.
        userError = 2;
      }  
    }
  });

  if(error1 != KErrNone){
    delete filenameBuf;
    return SPyErr_SetFromSymbianOSErr(error1);
  }
  if(userError == 1){
    delete filenameBuf;
    PyErr_SetString(PyExc_NameError, "file does not exist");
    return NULL;
  }
  if(userError == 2){
    delete filenameBuf;
    PyErr_SetString(PyExc_SyntaxError, "illegal parameter combination");
    return NULL;
  }

  // Open the db file.
  TRAPD(error2, {      
    agendaServer = RAgendaServ::NewL();  
    CleanupStack::PushL(agendaServer);
    agendaServer->Connect();
    CleanupClosePushL(*agendaServer); 

    agendaModel = CAgnModel::NewL(NULL);
    CleanupStack::PushL(agendaModel);
    agendaModel->SetServer(agendaServer);
    agendaModel->SetMode(CAgnEntryModel::EClient); 

    agendaModel->OpenL(filenameBuf->Des(), 
                       KDefaultTimeForEvents,
                       KDefaultTimeForAnnivs, 
                       KDefaultTimeForDayNote);

    agendaServer->WaitUntilLoaded();

    CleanupStack::Pop(); // agendaModel.
    CleanupStack::Pop(); // Close agendaServer.
    CleanupStack::Pop(); // agendaServer.
  });

  delete filenameBuf;

  if(error2 != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error2);
  }

  return new_CalendarDb_object(agendaServer, agendaModel);
}


/* 
 * CalendarDb methods.
 */


/*
 * Create new CalendarDb object.
 */
extern "C" PyObject *
new_CalendarDb_object(RAgendaServ* agendaServer, CAgnModel* agendaModel)
{
  CalendarDb_object* calendarDb = 
    PyObject_New(CalendarDb_object, CalendarDb_type);
  if (calendarDb == NULL){
    delete agendaModel;
    if(agendaServer->FileLoaded()){
      agendaServer->CloseAgenda(); 
    }
    agendaServer->Close();
    delete agendaServer;
    return PyErr_NoMemory();
  }
 
  calendarDb->agendaServer = agendaServer;
  calendarDb->agendaModel = agendaModel;
  
  return (PyObject*)calendarDb;
}


/*
 * Returns calendar entry object (indicated by given unique id).
 */ 
static PyObject *
CalendarDb_getitem(CalendarDb_object *self, PyObject *key)
{
  CHECK_AGENDA_STATE_DB

  if(!PyInt_Check(key)){
    PyErr_SetString(PyExc_TypeError, "entry id must be integer");
    return NULL;
  };

  TAgnUniqueId uniqueIdObj(PyInt_AsLong(key));

  if(uniqueIdObj.IsNullId()){
    PyErr_SetString(PyExc_ValueError, "illegal entry id");
    return NULL;
  }
  return new_Entry_object(self, uniqueIdObj);  
}


/*
 * Creates new entry (but does not commit it into the database).
 */ 
static PyObject *
CalendarDb_add_entry(CalendarDb_object *self, PyObject *args)
{
  CHECK_AGENDA_STATE_DB

  TInt entryType = 0;
  if (!PyArg_ParseTuple(args, "i", &entryType)){ 
    return NULL;
  }
  
  TAgnUniqueId id;
  id.SetNullId();

  return new_Entry_object(self, id, static_cast<CAgnEntry::TType>(entryType));
}


/*
 * Removes the entry from the database.
 */ 
static PyObject *
CalendarDb_delete_entry(CalendarDb_object *self, const TAgnUniqueId& uniqueId)
{
  // pyprintf("delete %d", uniqueId); // ??? debug

  CHECK_AGENDA_STATE_DB

  TRAPD(error, {
    self->agendaModel->DeleteEntryL(self->agendaServer->GetEntryId(uniqueId));
  });

  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Deletes specified entry.
 */
static int
CalendarDb_ass_sub(CalendarDb_object *self, PyObject *key, PyObject *value)
{
  CHECK_AGENDA_STATE_DB

  PyObject* result = NULL;
 
  if(value!=NULL){
    PyErr_SetString(PyExc_NotImplementedError, "illegal usage");
    return -1; 
  }

  if(!PyInt_Check(key)){
      PyErr_SetString(PyExc_TypeError, "entry id must be integer");
      return -1;
  }

  result = CalendarDb_delete_entry(self, TAgnUniqueId(PyInt_AsLong(key)));

  if(!result){
    return -1;
  }else{
    Py_DECREF(result);
  }
  return 0;  
}


/*
 * Returns number of entries in the database.
 */
static int
CalendarDb_len(CalendarDb_object *self)
{
  CHECK_AGENDA_STATE_DB

  TInt length = -1;

  TRAPD(error, {
    length = self->agendaModel->EntryCount();
  });

  if(error != KErrNone){
    SPyErr_SetFromSymbianOSErr(error);
  }

  return length;
}


/*
 * Deallocate the calendarDb object.
 */
extern "C" {
  static void CalendarDb_dealloc(CalendarDb_object *calendarDb)
  {
    if (calendarDb->agendaModel) {
      // pyprintf("DB dealloc %d", calendarDb); // ??? debug
      delete calendarDb->agendaModel;
      calendarDb->agendaModel = NULL;
    }
    if (calendarDb->agendaServer) {
      if(calendarDb->agendaServer->FileLoaded()){
        calendarDb->agendaServer->CloseAgenda();
        
      }
      calendarDb->agendaServer->Close();
      delete calendarDb->agendaServer;
      calendarDb->agendaServer = NULL;
    }
    
    PyObject_Del(calendarDb);
  }
}


/*
 * Sets the filter to be used in the entry search.
 */
void setFilterData(TAgnFilter& filter, TInt filterData)
{
  if(filterData==0){
    return; // Use the default.
  }

  // Set the filter data.

  if(filterData & APPTS_INC_FILTER){
    filter.SetIncludeTimedAppts(ETrue);
  }else{
    filter.SetIncludeTimedAppts(EFalse);
  }

  if(filterData & EVENTS_INC_FILTER){
     filter.SetIncludeEvents(ETrue);
  }else{
     filter.SetIncludeEvents(EFalse);
  }

  if(filterData & ANNIVS_INC_FILTER){
    filter.SetIncludeAnnivs(ETrue);
  }else{
    filter.SetIncludeAnnivs(EFalse);
  }

  if(filterData & TODOS_INC_FILTER){
    filter.SetIncludeTodos(ETrue);
  }else{
    filter.SetIncludeTodos(EFalse);
  }
}


/*
 * Returns all instances (instance = entry's unique id + 
 * datetime of the specific occurrence of the entry)
 * of the given month.
 * -the month is the month (of the calendar:january, ..) 
 * the given datetime parameter belongs to (it has no 
 * matter which day of this month the datetime specifies).
 * -returns list of instances like [{"unique_id":543245, 
 * "datetime":14565645.0}, {"unique_id":653456, "datetime":14565655.0}].
 */
extern "C" PyObject *
CalendarDb_monthly_instances(CalendarDb_object* self, PyObject *args)
{
  CHECK_AGENDA_STATE_DB

  TReal month = 0;
  CAgnMonthInstanceList* monthList = NULL;
  TInt filterInt = 0;
  if (!PyArg_ParseTuple(args, "d|i", &month, &filterInt)){ 
    return NULL;
  }

  TTime monthTTime;
  pythonRealAsTTime(month, monthTTime);
  if(Check_time_validity(monthTTime)==EFalse){
    PyErr_SetString(PyExc_ValueError, "illegal date value");
    return NULL;
  }

  TTime today;
  today.HomeTime();
  TAgnFilter filter;
  setFilterData(filter, filterInt);
  TRAPD(error, {
    monthList = 
      CAgnMonthInstanceList::NewL(TTimeIntervalYears(monthTTime.DateTime().Year()), 
                                  monthTTime.DateTime().Month());
    self->agendaModel->PopulateMonthInstanceListL(monthList, filter, today);
  });

  if(error != KErrNone){
    if(monthList){
      monthList->Reset();
      delete monthList;
    }
    return SPyErr_SetFromSymbianOSErr(error);
  }

  PyObject* idList = PyList_New(monthList->Count()); 
  if(idList==NULL){
    monthList->Reset();
    delete monthList;
    return PyErr_NoMemory();
  }

  for(TInt i=0;i<monthList->Count();i++){
    TAgnInstanceId id = (*monthList)[i];
    PyObject* instanceInfo =
      Py_BuildValue("{s:i,s:d}", (const char*)(&KUniqueId)->Ptr(), 
                                 self->agendaServer->GetUniqueId(id), 
                                 (const char*)(&KDateTime)->Ptr(), 
                                 time_as_UTC_TReal(AgnDateTime::AgnDateToTTime(id.Date())));
    if(instanceInfo==NULL){
      Py_DECREF(idList);
      monthList->Reset();
      delete monthList;
      return NULL;
    }
    if(PyList_SetItem(idList, i, instanceInfo)<0){
      Py_DECREF(idList);
      monthList->Reset();
      delete monthList;
      return NULL;
    }
  }

  monthList->Reset();
  delete monthList;

  return idList;
}


/*
 * Returns all instances (instance = entry's unique id + 
 * datetime of the specific occurrence of the entry)
 * of the given day.
 * -the day is the day (of the calendar like "11/02/2005") 
 * the given datetime parameter belongs to (it has no 
 * matter which moment of this day the datetime specifies).
 * -returns list of instances like [{"unique_id":543245, 
 * "datetime":14565645.0}, {"unique_id":653456, "datetime":14565655.0}].
 */
extern "C" PyObject *
CalendarDb_daily_instances(CalendarDb_object* self, PyObject *args)
{
  CHECK_AGENDA_STATE_DB

  TReal day = 0;
  CAgnDayList<TAgnInstanceId>* dayList = NULL;
  TInt filterInt = 0;
  if (!PyArg_ParseTuple(args, "d|i", &day, &filterInt)){ 
    return NULL;
  }
 
  TTime dayTTime;
  pythonRealAsTTime(day, dayTTime);
  if(Check_time_validity(dayTTime)==EFalse){
    PyErr_SetString(PyExc_ValueError, "illegal date value");
    return NULL;
  }

  TTime today;
  today.HomeTime();
	TAgnFilter filter;
  setFilterData(filter, filterInt);

  // Create a day list.
  TRAPD(error, {
	  dayList = CAgnDayList<TAgnInstanceId>::NewL(dayTTime.DateTime()); 
    self->agendaModel->PopulateDayInstanceListL(dayList, filter, today);
  });

  if(error!=KErrNone){
    if(dayList){
      dayList->Reset();
      delete dayList;
    }
    return SPyErr_SetFromSymbianOSErr(error);
  }

  PyObject* idList = PyList_New(dayList->Count()); 
  if(idList==NULL){
    dayList->Reset();
    delete dayList;
    return PyErr_NoMemory();
  }

  for(TInt i=0;i<dayList->Count();i++){
    TAgnInstanceId id = (*dayList)[i];
    PyObject* instanceInfo =
      Py_BuildValue("{s:i,s:d}", (const char*)(&KUniqueId)->Ptr(), 
                                 self->agendaServer->GetUniqueId(id), 
                                 (const char*)(&KDateTime)->Ptr(),
                                 time_as_UTC_TReal(AgnDateTime::AgnDateToTTime(id.Date())));
    if(instanceInfo==NULL){
      Py_DECREF(idList);
      dayList->Reset();
      delete dayList;
      return NULL;
    }

    if(PyList_SetItem(idList, i, instanceInfo)<0){
      Py_DECREF(idList);
      dayList->Reset();
      delete dayList;
      return NULL;
    }
  }
 
  dayList->Reset();
  delete dayList;
  return idList;
}


/*
 * Finds instances (that occur between the specified time interval) by search string.
 * -Only returns instances of one day (the first day that has instance that
 * meets the criteria).
 */
extern "C" PyObject *
CalendarDb_find_instances(CalendarDb_object* self, PyObject *args)
{
  CHECK_AGENDA_STATE_DB

  TReal startDate = 0;
  TReal endDate = 0;
  PyObject* searchText = NULL;
  TInt filterInt = 0;
  CAgnDayList<TAgnInstanceId>* dayList = NULL;
  
  if (!PyArg_ParseTuple(args, "ddU|i", &startDate, &endDate, &searchText, &filterInt)){ 
    return NULL;
  }

  TPtrC searchTextPtr((TUint16*) PyUnicode_AsUnicode(searchText), 
                    PyUnicode_GetSize(searchText));
 
  TTime startTTime;
  TTime endTTime;
  pythonRealAsTTime(startDate, startTTime);
  pythonRealAsTTime(endDate, endTTime);
  startTTime=truncToDate(startTTime);
  endTTime=truncToDate(endTTime);

  if(Check_time_validity(startTTime)==EFalse || Check_time_validity(endTTime)==EFalse){
    PyErr_SetString(PyExc_ValueError, "illegal date value");
    return NULL;
  }

  if(startTTime>endTTime){
    PyErr_SetString(PyExc_ValueError, "start date greater than end date");
    return NULL;
  }

  TTime today;
  today.HomeTime();
	TAgnFilter filter;
  setFilterData(filter, filterInt);

  // Create a day list.
  TRAPD(error, {
	  dayList = CAgnDayList<TAgnInstanceId>::NewL(startTTime.DateTime()); 
    self->agendaModel->FindNextInstanceL(dayList, searchTextPtr, startTTime, endTTime, filter, today); 
  });
  if(error!=KErrNone){
    if(dayList){
      dayList->Reset();
      delete dayList;
    }
    return SPyErr_SetFromSymbianOSErr(error);
  }

  PyObject* idList = PyList_New(dayList->Count()); 
  if(idList==NULL){
    dayList->Reset();
    delete dayList;
    return PyErr_NoMemory();
  }

  for(TInt i=0;i<dayList->Count();i++){
    TAgnInstanceId id = (*dayList)[i];
    PyObject* instanceInfo =
      Py_BuildValue("{s:i,s:d}", (const char*)(&KUniqueId)->Ptr(), 
                                 self->agendaServer->GetUniqueId(id), 
                                 (const char*)(&KDateTime)->Ptr(),
                                 time_as_UTC_TReal(AgnDateTime::AgnDateToTTime(id.Date())));
    if(instanceInfo==NULL){
      Py_DECREF(idList);
      dayList->Reset();
      delete dayList;
      return NULL;
    }

    if(PyList_SetItem(idList, i, instanceInfo)<0){
      Py_DECREF(idList);
      dayList->Reset();
      delete dayList;
      return NULL;
    }
  }
 
  dayList->Reset();
  delete dayList;
  return idList;
}


/*
 * Returns todos in the specified todo list.
 */
extern "C" PyObject *
CalendarDb_todos_in_list(CalendarDb_object* self, PyObject* args)
{
  CHECK_AGENDA_STATE_DB

  CAgnTodoInstanceList* todoList = NULL; 
  TInt32 todoListId = 0;
  if (!PyArg_ParseTuple(args, "i", &todoListId)){ 
    return NULL;
  }

  // Get the entries in the list.
  TTime today;
  today.HomeTime();

  TRAPD(error, {
    todoList = CAgnTodoInstanceList::NewL(TAgnTodoListId(todoListId));
    self->agendaModel->PopulateTodoInstanceListL(todoList, today);
  });
  if(error!=KErrNone){
    if(todoList){
      todoList->Reset();
      delete todoList;
    }
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  PyObject* idList = PyList_New(todoList->Count()); 
  if(idList==NULL){
    todoList->Reset();
    delete todoList;
    return PyErr_NoMemory();
  }

  for(TInt i=0;i<todoList->Count();i++){
    TAgnInstanceId id = (*todoList)[i];
    PyObject* instanceId=
      Py_BuildValue("i", self->agendaServer->GetUniqueId(id));
    if(instanceId==NULL){
      Py_DECREF(idList);
      todoList->Reset();
      delete todoList;
      return NULL;
    }

    if(PyList_SetItem(idList, i, instanceId)<0){
      Py_DECREF(idList);
      todoList->Reset();
      delete todoList;
      return NULL;
    }
  }
 
  todoList->Reset();
  delete todoList;
  return idList;
}


/*
 * Returns information (id and name) of existing todo lists.
 * -returned data is like {11:"To-do list", 6543456:"my own todo list"}
 */
extern "C" PyObject *
CalendarDb_todo_lists(CalendarDb_object* self, PyObject* /*args*/)
{ 
  CHECK_AGENDA_STATE_DB

  CAgnTodoListNames* todoListNames = NULL;

  TRAPD(error, { 
    todoListNames = CAgnTodoListNames::NewL();
    self->agendaModel->PopulateTodoListNamesL(todoListNames);
  });  
  if(error!=KErrNone){
    delete todoListNames;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  PyObject* listInfo = PyDict_New();
  if(listInfo==NULL){
    delete todoListNames;
    return PyErr_NoMemory();
  }

  for(TInt i=0;i<todoListNames->Count();i++){ 
    PyObject* listId = Py_BuildValue("i", todoListNames->TodoListId(i).Id());
    if(listId==NULL){ 
      Py_DECREF(listInfo);
      delete todoListNames;
      return NULL;
    }

    PyObject* listName = 
      Py_BuildValue("u#", todoListNames->TodoListName(i).Ptr(), 
                          todoListNames->TodoListName(i).Length());
    if(listName==NULL){ 
      Py_DECREF(listInfo);
      Py_DECREF(listId);
      delete todoListNames;
      return NULL;
    }

    TInt err = PyDict_SetItem(listInfo, listId, listName);
    Py_DECREF(listId);
    Py_DECREF(listName);
    if(err<0){
      Py_DECREF(listInfo);
      delete todoListNames;
      return NULL;
    }
  }

  delete todoListNames;

  return listInfo;  
}


/*
 * Returns id of the default todo list.
 */
extern "C" PyObject *
CalendarDb_default_todo_list(CalendarDb_object* self, PyObject* /*args*/)
{
  CHECK_AGENDA_STATE_DB

  CAgnTodoListNames* todoListNames = NULL;

  TRAPD(error, { 
    todoListNames = CAgnTodoListNames::NewL();
    self->agendaModel->PopulateTodoListNamesL(todoListNames);
  });  
  if(error!=KErrNone){
    delete todoListNames;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  if(todoListNames->Count()<1){
    delete todoListNames;
    PyErr_SetString(PyExc_RuntimeError, "there are no todo lists");
  }

  PyObject* listId = Py_BuildValue("i", todoListNames->TodoListId(0).Id());

  delete todoListNames;
  
  return listId;
}


/*
 * Creates new todo list.
 * -name can be (optionally) given as a parameter.
 * -returns the id of the created todo list.
 */
extern "C" PyObject *
CalendarDb_add_todo_list(CalendarDb_object* self, PyObject* args)
{  
  CHECK_AGENDA_STATE_DB

  PyObject* todoListName = NULL;
  CAgnTodoList* todoList = NULL;
  TAgnTodoListId listId;
  if (!PyArg_ParseTuple(args, "|U", &todoListName)){ 
    return NULL;
  }

  TRAPD(error, {
    todoList = CAgnTodoList::NewL();
    CleanupStack::PushL(todoList);
    if(todoListName){
      TPtrC todoListNamePtr((TUint16*) PyUnicode_AsUnicode(todoListName), 
                            PyUnicode_GetSize(todoListName));
      todoList->SetName(todoListNamePtr);
    }
    listId = self->agendaModel->AddTodoListL(todoList);
    CleanupStack::PopAndDestroy(); // todoList
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  return Py_BuildValue("i", listId.Id());
}


/*
 * Removes specified (specified by todo list id) todo list.
 */
extern "C" PyObject *
CalendarDb_remove_todo_list(CalendarDb_object* self, PyObject* args)
{  
  CHECK_AGENDA_STATE_DB

  TInt32 todoListId = 0; 
  if (!PyArg_ParseTuple(args, "i", &todoListId)){ 
    return NULL;
  }
 
  TRAPD(error, {
    self->agendaModel->DeleteTodoListL(TAgnTodoListId(todoListId));
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
 
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Renames specified (specified by todo list id) todo list 
 * according to given string parameter.
 */
extern "C" PyObject *
CalendarDb_rename_todo_list(CalendarDb_object* self, PyObject* args)
{  
  CHECK_AGENDA_STATE_DB

  TInt32 todoListId = 0; 
  PyObject* todoListName = NULL; 
  CAgnTodoList* todoList = NULL; 
  if (!PyArg_ParseTuple(args, "iU", &todoListId, &todoListName)){ 
    return NULL;
  }
 
  TRAPD(error, {
    todoList = self->agendaModel->FetchTodoListL(TAgnTodoListId(todoListId));
    TPtrC todoListNamePtr((TUint16*) PyUnicode_AsUnicode(todoListName), 
                          PyUnicode_GetSize(todoListName));
    todoList->SetName(todoListNamePtr);
    self->agendaModel->UpdateTodoListL(todoList);
  });
 
  delete todoList;

  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Exports vcalendar entries from the database. 
 * -returned string contains entries indicated by content (unique ids)
 * of the tuple given as the parameter.
 */
extern "C" PyObject *
CalendarDb_export_vcals(CalendarDb_object* self, PyObject *args)
{
  CHECK_AGENDA_STATE_DB

  CArrayFixFlat<TAgnEntryId>* idArray = NULL;
  PyObject* idTuple = NULL;
  PyObject* ret = NULL;
  TInt error = KErrNone;

  if (!PyArg_ParseTuple(args, "O!", 
                        &PyTuple_Type, &idTuple)){ 
    return NULL;
  }
  
  if(PyTuple_Size(idTuple)<1){
    PyErr_SetString(PyExc_SyntaxError, 
                    "no calendar entry id:s given in the tuple");
    return NULL;
  }

  TRAP(error, 
    idArray = new (ELeave) CArrayFixFlat<TAgnEntryId>(PyTuple_Size(idTuple)));
  if(error!=KErrNone){
    delete idArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  // Put the calendar entry id:s into the idArray.
  TInt idCount = PyTuple_Size(idTuple);
  for(TInt i=0;i<idCount;i++){
    PyObject* idItem = PyTuple_GetItem(idTuple, i);

    if(!PyInt_Check(idItem)){
      delete idArray;
      PyErr_SetString(PyExc_TypeError, "illegal argument in the tuple");
      return NULL;
    };

    TRAP(error, 
      idArray->AppendL(self->agendaServer->GetEntryId(TAgnUniqueId(PyInt_AsLong(idItem)))));
    if(error != KErrNone){
      delete idArray;
      return SPyErr_SetFromSymbianOSErr(error);
    }
  }

  TRAP(error, {
    CBufFlat* flatBuf = CBufFlat::NewL(4);
    CleanupStack::PushL(flatBuf);
    RBufWriteStream outputStream(*flatBuf);
    CleanupClosePushL(outputStream);

    self->agendaModel->ExportVCalL(outputStream, idArray);

    outputStream.CommitL();        
    CleanupStack::PopAndDestroy(); // Close outputStream.  

    ret = Py_BuildValue("s#", (char*)flatBuf->Ptr(0).Ptr(), flatBuf->Size()); 

    CleanupStack::PopAndDestroy(); // flatBuf.  
  });
  delete idArray;
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  return ret;
}


/*
 * Imports vcalendar entries (given in the string parameter) to the database. 
 * -returns tuple that contains unique ids of the imported entries.
 */
extern "C" PyObject *
CalendarDb_import_vcals(CalendarDb_object* self, PyObject *args)
{
  CHECK_AGENDA_STATE_DB

  CArrayPtrFlat<CAgnEntry>* entryArray = NULL;
  char* vCalStr = NULL;
  TInt32 vCalStrLength = 0;
  PyObject* idTuple = NULL;
  TInt error = KErrNone;

  if (!PyArg_ParseTuple(args, "s#",
                        &vCalStr, &vCalStrLength)){ 
    return NULL;
  }

  TPtrC8 vCalStrPtr((TUint8*)vCalStr, vCalStrLength); 
  
  RMemReadStream inputStream(vCalStrPtr.Ptr(), vCalStrPtr.Length()); 

  TRAP(error, 
    entryArray = new (ELeave) CArrayPtrFlat<CAgnEntry>(ENTRY_PTR_ARR_SIZE));
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
    
  TRAP(error, {
    CleanupClosePushL(inputStream);
    self->agendaModel->ImportVCalL(inputStream, entryArray);
    CleanupStack::PopAndDestroy(); // Close inputStream.
  });
  if(error!=KErrNone){
    entryArray->ResetAndDestroy();
    delete entryArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }  

  idTuple = PyTuple_New(entryArray->Count());
  if(NULL==idTuple){
    entryArray->ResetAndDestroy();
    delete entryArray;
    return PyErr_NoMemory();
  }

  TAgnUniqueId id;
  for(TInt i=0;i<entryArray->Count();i++){
    CAgnEntry* entryObj = (*entryArray)[i];
    TRAP(error, 
      id = self->agendaServer->GetUniqueId(self->agendaModel->AddEntryL(entryObj)));
    if(error!=KErrNone){
      entryArray->ResetAndDestroy();
      delete entryArray;
      Py_DECREF(idTuple);
      return SPyErr_SetFromSymbianOSErr(error);
    } 
    PyObject* idObj = Py_BuildValue("i", id.Id());
    if(NULL==idObj){
      entryArray->ResetAndDestroy();
      delete entryArray;
      Py_DECREF(idTuple);
      return NULL;
    } 
    if(PyTuple_SetItem(idTuple, i, idObj)){
      entryArray->ResetAndDestroy();
      delete entryArray;
      Py_DECREF(idTuple);
      return NULL;
    };
  } 
 
  entryArray->ResetAndDestroy();
  delete entryArray;
  return idTuple;
}


/*
 * Compresses the Calendar db file. Returns success status.
 */
extern "C" PyObject *
CalendarDb_compact(CalendarDb_object* self, PyObject *args)
{
  CHECK_AGENDA_STATE_DB
  
  return Py_BuildValue("i", self->agendaServer->CompactFile());
}


/*
 * Entry methods.
 */


/*
 * Creates new calendar entry "wrapper" object.
 * -fetches the entry identified by uniqueIdObj from the 
 * database and wraps that into the python object.
 * -if uniqueIdObj is null-id a new entry is created (but 
 * not added to the database until entry.commit() is called).
 * in this case entry's unique id will remain as a null-id 
 * until the entry is added (by committing) into the database. 
 */
extern "C" PyObject *
new_Entry_object(CalendarDb_object* self, 
                 TAgnUniqueId uniqueIdObj, 
                 CAgnEntry::TType entryType)
{  
  CHECK_AGENDA_STATE_DB

  Entry_object* entryObj = NULL;
  TInt userError = 0;

  if(uniqueIdObj.IsNullId()){

    // New entry must be created

    CAgnEntry* entry = NULL;
    CParaFormatLayer* paraFormatLayer = NULL;
	  CCharFormatLayer* charFormatLayer = NULL;
   
    TRAPD(error, {     
      paraFormatLayer = CParaFormatLayer::NewL();
      CleanupStack::PushL(paraFormatLayer);
      charFormatLayer = CCharFormatLayer::NewL();
      CleanupStack::PushL(charFormatLayer);

      switch(entryType){
        case CAgnEntry::EAppt:       
          entry = CAgnAppt::NewL(paraFormatLayer, charFormatLayer);
        break;
        case CAgnEntry::ETodo:        
          entry = CAgnTodo::NewL(paraFormatLayer, charFormatLayer);
        break;
        case CAgnEntry::EEvent:        
          entry = CAgnEvent::NewL(paraFormatLayer, charFormatLayer);
        break;
        case CAgnEntry::EAnniv:
          entry = CAgnAnniv::NewL(paraFormatLayer, charFormatLayer);
        break;
        default:
          userError = 1;
        break;
      }
      CleanupStack::Pop(); // charFormatLayer
      CleanupStack::Pop(); // paraFormatLayer
    });
  
    if(error!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);
    }

    if(userError==1){
      delete paraFormatLayer;
      delete charFormatLayer;
      PyErr_SetString(PyExc_ValueError, "illegal entrytype parameter");
      return NULL; 
    }

    entryObj = PyObject_New(Entry_object, Entry_type); 
    if(entryObj == NULL){
      return PyErr_NoMemory();
    }

    entryObj->charFormatLayer = charFormatLayer;
    entryObj->paraFormatLayer = paraFormatLayer;
    entryObj->calendarDb = self;
    entryObj->entryItem = entry;
    entryObj->uniqueId.SetNullId();
#if SERIES60_VERSION<20
    entryObj->alarm=NULL;
#endif

  }else{

    // Wrap an existing entry. 

    CAgnEntry* entry = NULL;   
    TRAPD(error, {
      entry = self->agendaModel->FetchEntryL(self->agendaServer->GetEntryId(uniqueIdObj));
    });
    if(error!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);
    }

    entryObj = PyObject_New(Entry_object, Entry_type); 
    if(entryObj == NULL){
      delete entry;
      return PyErr_NoMemory();
    }

    entryObj->charFormatLayer = NULL;
    entryObj->paraFormatLayer = NULL;
    entryObj->calendarDb = self;
    entryObj->uniqueId = uniqueIdObj;
    entryObj->entryItem = entry;
#if SERIES60_VERSION<20
    entryObj->alarm=NULL;
#endif
  }
  
  Py_INCREF(entryObj->calendarDb);

  return (PyObject*)entryObj;
}


/*
 * Returns entry's location information (text). 
 */
extern "C" PyObject *
Entry_location(Entry_object* self, PyObject* /*args*/)
{
  return Py_BuildValue("u#", self->entryItem->Location().Ptr(), 
                             self->entryItem->Location().Length()); 
}


/*
 * Sets entry's location information (text). 
 */
extern "C" PyObject *
Entry_set_location(Entry_object* self, PyObject* args)
{
  PyObject* location = NULL;
  if (!PyArg_ParseTuple(args, "U", &location)){ 
    return NULL;
  }
  TPtrC locationPtr((TUint16*) PyUnicode_AsUnicode(location), 
                    PyUnicode_GetSize(location));
  TRAPD(error, {
    self->entryItem->SetLocationL(locationPtr);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Sets entry's start and end datetime.
 */
extern "C" PyObject *
Entry_set_start_and_end_datetime(Entry_object* self, PyObject* args)
{
  TReal startTime = 0;
  TReal endTime = 0;
  if (!PyArg_ParseTuple(args, "dd", &startTime, &endTime)){ 
    return NULL;
  }

  TTime startTTime;
  TTime endTTime;
  pythonRealAsTTime(startTime, startTTime);
  pythonRealAsTTime(endTime, endTTime);

  if(Check_time_validity(startTTime)==EFalse){
    PyErr_SetString(PyExc_ValueError, "illegal start datetime");
    return NULL;
  }
  if(Check_time_validity(endTTime)==EFalse){
    PyErr_SetString(PyExc_ValueError, "illegal end datetime");
    return NULL;
  }

  switch(self->entryItem->Type()){
    case CAgnEntry::EAppt:
      {  
        if(startTTime>endTTime){ 
          PyErr_SetString(PyExc_ValueError, 
                          "start datetime cannot be greater than end datetime");
          return NULL;
        }
        self->entryItem->CastToAppt()->SetStartAndEndDateTime(startTTime, endTTime);
      }
    break;
    case CAgnEntry::EEvent:
      {
        START_END_DATE_CHECK
        self->entryItem->CastToEvent()->SetStartAndEndDate(startTTime, endTTime);
      }
    break;
    case CAgnEntry::EAnniv:
      {
        START_END_DATE_CHECK
        self->entryItem->CastToAnniv()->SetStartAndEndDate(startTTime, endTTime);
      }
    break;
    case CAgnEntry::ETodo:
      {
        START_END_DATE_CHECK
        self->entryItem->CastToTodo()->SetDueDate(endTTime);
        self->entryItem->CastToTodo()->SetDuration(endTTime.DaysFrom(startTTime));
      }
    break; 
  }  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns information whether entry is dated (todos only).
 */
extern "C" PyObject *
Entry_is_dated(Entry_object* self, PyObject* /*args*/)
{
  if(self->entryItem->Type()!=CAgnEntry::ETodo){
    PyErr_SetString(PyExc_RuntimeError, "this method is for todos only");
    return NULL;
  }
  return Py_BuildValue("i", self->entryItem->CastToTodo()->IsDated());
}


/*
 * Makes entry undated (todos only).
 */
extern "C" PyObject *
Entry_make_undated(Entry_object* self, PyObject* /*args*/)
{
  if(self->entryItem->Type()!=CAgnEntry::ETodo){
    PyErr_SetString(PyExc_RuntimeError, "this method is for todos only");
    return NULL;
  }
  self->entryItem->CastToTodo()->MakeUndated();
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns entry's start datetime.
 */
extern "C" PyObject *
Entry_start_datetime(Entry_object* self, PyObject* /*args*/)
{
  TTime startTTime;
  switch(self->entryItem->Type()){
    case CAgnEntry::EAppt:
      {
        startTTime = self->entryItem->CastToAppt()->StartDateTime();
      }
    break;
    case CAgnEntry::EEvent:
      {
        startTTime = self->entryItem->CastToEvent()->StartDate();
      }
    break;
    case CAgnEntry::EAnniv:
      {
        startTTime = self->entryItem->CastToAnniv()->StartDate();
      }
    break;
    case CAgnEntry::ETodo:
      if(self->entryItem->CastToTodo()->IsDated()){
        startTTime = self->entryItem->CastToTodo()->InstanceStartDate();
      }else{
        startTTime=AgnDateTime::AgnDateToTTime(AgnDateTime::NullDate());
      }
    break; 
  }  

  if(EFalse!=Is_null_time(startTTime)){
    Py_INCREF(Py_None);
    return Py_None;
  }

  return ttimeAsPythonFloat(startTTime);
}


/*
 * Returns entry's end datetime.
 */
extern "C" PyObject *
Entry_end_datetime(Entry_object* self, PyObject* /*args*/)
{
  TTime endTTime;
  switch(self->entryItem->Type()){
    case CAgnEntry::EAppt:
      {
        endTTime = self->entryItem->CastToAppt()->EndDateTime();
      }
    break;
    case CAgnEntry::EEvent:
      {
        endTTime = self->entryItem->CastToEvent()->EndDate();
      }
    break;
    case CAgnEntry::EAnniv:
      {
        endTTime = self->entryItem->CastToAnniv()->EndDate();
      }
    break;
    case CAgnEntry::ETodo:
      {
        endTTime = self->entryItem->CastToTodo()->DueDate();
      }
    break;
  } 

  if(EFalse!=Is_null_time(endTTime)){
    Py_INCREF(Py_None);
    return Py_None;
  }
 
  return ttimeAsPythonFloat(endTTime);
}


/*
 * Returns the content (text) of the entry.
 */
extern "C" PyObject *
Entry_content(Entry_object* self, PyObject* /*args*/)
{
  PyObject* ret = NULL;
  HBufC* buf = NULL;
  TRAPD(error, {
    buf = HBufC::NewL(self->entryItem->RichTextL()->DocumentLength());
    CleanupStack::PushL(buf);
    self->entryItem->RichTextL()->Extract((TDes&)buf->Des(), 0);
    CleanupStack::Pop(); // buf.
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  ret = Py_BuildValue("u#", buf->Ptr(), buf->Length()); 
  delete buf;

  return ret;
}


/*
 * Sets the content (text) of the entry.
 */
extern "C" PyObject *
Entry_set_content(Entry_object* self, PyObject* args)
{
  PyObject* content = NULL;
  if (!PyArg_ParseTuple(args, "U", &content)){ 
    return NULL;
  }

  TPtrC contentPtr((TUint16*) PyUnicode_AsUnicode(content), 
                   PyUnicode_GetSize(content));

  TRAPD(error, {
    self->entryItem->RichTextL()->Reset();
    self->entryItem->RichTextL()->InsertL(0, contentPtr);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Puts the given repeat information into the entry.
 */
extern "C" PyObject *
Entry_install_repeat_data(Entry_object* self, 
                          TReal startDate,
                          TReal endDate,
                          TInt interval,
                          TInt repeatIndicator,
                          CArrayFixSeg<TReal>* exceptionArray,
                          PyObject* repeatDays,
                          TBool eternalRepeat)
{
  CAgnRptDef* rpt = NULL;
  TTime startTime;
  TTime endTime;
  
  switch(repeatIndicator){
    case NOT_REPEATED:
      {
        self->entryItem->ClearRepeat();
      }
    break;
    case DAILY_REPEAT:
      {
         GET_REP_START_AND_END    
         CREATE_RPT
         TAgnDailyRpt dailyRpt;
         rpt->SetDaily(dailyRpt);
         SET_REPEAT_DATES
         ADD_REPEAT
      }
    break;
    case WEEKLY_REPEAT:
      { 
         GET_REP_START_AND_END 
         TAgnWeeklyRpt weeklyRpt;

         // Add repeat days.
         if(repeatDays){
           if(!PyList_Check(repeatDays)){
             PyErr_SetString(PyExc_SyntaxError, 
                             "weekly repeat days must be given in a list");
             return NULL;
           }
           for(TInt i=0;i<PyList_Size(repeatDays);i++){
             PyObject* day = PyList_GetItem(repeatDays, i);
             if(!PyInt_Check(day) || 
                0>PyInt_AsLong(day) || 
                EFalse==isWeekday(PyInt_AsLong(day))){
                PyErr_SetString(PyExc_ValueError, 
                                "bad value for weekly repeat day");
                return NULL;
             }
             weeklyRpt.SetDay(static_cast<TDay>(PyInt_AsLong(day)));
           }
         }
         if(weeklyRpt.NumDaysSet()==0){
           // default repeat day.
           weeklyRpt.SetDay(startTime.DayNoInWeek());
         }       
         CREATE_RPT
         rpt->SetWeekly(weeklyRpt);
         SET_REPEAT_DATES
         ADD_REPEAT
      }
    break;
    case MONTHLY_BY_DATES_REPEAT:
      {
         GET_REP_START_AND_END         
         TAgnMonthlyByDatesRpt monthlyRpt;

          // Add repeat dates.
         if(repeatDays){
           if(!PyList_Check(repeatDays)){
             PyErr_SetString(PyExc_SyntaxError, 
                             "monthly repeat dates must be given in a list");
             return NULL;
           }
           for(TInt i=0;i<PyList_Size(repeatDays);i++){
             PyObject* day = PyList_GetItem(repeatDays, i);
             if(!PyInt_Check(day) || 
                0>PyInt_AsLong(day) || 
                PyInt_AsLong(day)>=DAYS_IN_MONTH){
                PyErr_SetString(PyExc_ValueError, 
                                "monthly repeat dates must be integers (0-30)");
               return NULL;
             }
             monthlyRpt.SetDate(PyInt_AsLong(day));
           }
         }
         if(monthlyRpt.NumDatesSet()==0){
           // default repeat date.
           monthlyRpt.SetDate(startTime.DayNoInMonth());
         }
         CREATE_RPT
         rpt->SetMonthlyByDates(monthlyRpt);
         SET_REPEAT_DATES
         ADD_REPEAT
      }
    break;
    case MONTHLY_BY_DAYS_REPEAT:
      {
         GET_REP_START_AND_END 
         TAgnMonthlyByDaysRpt monthlyRpt;

          // Add repeat days.
         if(repeatDays){
           if(!PyList_Check(repeatDays)){
             PyErr_SetString(PyExc_SyntaxError, 
                             "monthly repeat days must be given in a list");
             return NULL;
           }
           PyObject* weekKey = Py_BuildValue("s", (const char*)(&KWeek)->Ptr());
           PyObject* dayKey = Py_BuildValue("s", (const char*)(&KDay)->Ptr());
           if(!(weekKey && dayKey)){
             Py_XDECREF(weekKey);
             Py_XDECREF(dayKey);
             return NULL;
           } 
           for(TInt i=0;i<PyList_Size(repeatDays);i++){
             PyObject* day = PyList_GetItem(repeatDays, i);
             if(!PyDict_Check(day)){
               Py_DECREF(weekKey);
               Py_DECREF(dayKey);
               PyErr_SetString(PyExc_TypeError, 
                               "repeat day must be dictionary"); 
               return NULL;
             }             
             PyObject* weekNum = PyDict_GetItem(day, weekKey);
             PyObject* dayNum = PyDict_GetItem(day, dayKey);
             if(!(weekNum && dayNum && 
                PyInt_Check(weekNum) && PyInt_Check(dayNum))){
               Py_DECREF(weekKey);
               Py_DECREF(dayKey);
               PyErr_SetString(PyExc_SyntaxError, 
                               "week and day must be given and they must be integers");
               return NULL;
             }
             TInt weekNumInt = PyInt_AsLong(weekNum);
             TInt dayNumInt = PyInt_AsLong(dayNum);
             if(isWeekInMonth(weekNumInt)==EFalse || 
                isWeekday(dayNumInt)==EFalse){
               Py_DECREF(weekKey);
               Py_DECREF(dayKey);
               PyErr_SetString(PyExc_ValueError, "bad value for week or day");
               return NULL;
             }
             monthlyRpt.SetDay(static_cast<TDay>(dayNumInt), 
                               static_cast<TAgnRpt::TWeekInMonth>(weekNumInt));
           }
           Py_DECREF(weekKey);
           Py_DECREF(dayKey);
         }
         if(monthlyRpt.NumDaysSet()==0){
           // default repeat day.
           monthlyRpt.SetDay(startTime.DayNoInWeek(), 
                             static_cast<TAgnRpt::TWeekInMonth>(startTime.DayNoInMonth()/DAYS_IN_WEEK));
         }         
         CREATE_RPT
         rpt->SetMonthlyByDays(monthlyRpt);
         SET_REPEAT_DATES
         ADD_REPEAT
      }
    break;
    case YEARLY_BY_DATE_REPEAT:
      {
         GET_REP_START_AND_END
         CREATE_RPT
         TAgnYearlyByDateRpt yearlyRpt;
         rpt->SetYearlyByDate(yearlyRpt);
         SET_REPEAT_DATES;
         ADD_REPEAT
      }
    break;
    case YEARLY_BY_DAY_REPEAT:
      {
         GET_REP_START_AND_END        
         TAgnYearlyByDayRpt yearlyRpt;

         // Add repeat day.
         if(repeatDays){           
           if(!PyDict_Check(repeatDays)){
             PyErr_SetString(PyExc_SyntaxError, 
               "yearly repeat day must be given in a dictionary");
             return NULL;
           }
           PyObject* weekKey = Py_BuildValue("s", (const char*)(&KWeek)->Ptr());
           PyObject* dayKey = Py_BuildValue("s", (const char*)(&KDay)->Ptr());
           PyObject* monthKey = Py_BuildValue("s", (const char*)(&KMonth)->Ptr());
           if(!(weekKey && dayKey && monthKey)){
             Py_XDECREF(weekKey);
             Py_XDECREF(dayKey);
             Py_XDECREF(monthKey);
             return NULL;
           }        
           PyObject* day = PyDict_GetItem(repeatDays, dayKey);
           PyObject* week = PyDict_GetItem(repeatDays, weekKey);
           PyObject* month = PyDict_GetItem(repeatDays, monthKey);
           Py_DECREF(weekKey);
           Py_DECREF(dayKey);
           Py_DECREF(monthKey);
         
           if(!day || !week || !month){
             PyErr_SetString(PyExc_SyntaxError, 
               "day, week and month must be given");
             return NULL;
           } 

           if(!(PyInt_Check(day) && PyInt_Check(week) && 
              PyInt_Check(month))){
             PyErr_SetString(PyExc_TypeError, 
                             "day, week and month must be integers");
             return NULL;
           } 

           TInt dayInt = PyInt_AsLong(day);
           TInt weekInt = PyInt_AsLong(week);
           TInt monthInt = PyInt_AsLong(month);
           
           if(isWeekday(dayInt)==EFalse || 
              isWeekInMonth(weekInt)==EFalse ||
              isMonth(monthInt)==EFalse){
             PyErr_SetString(PyExc_ValueError, "bad value for day, week or month");
             return NULL;
           }

           yearlyRpt.SetStartDay(static_cast<TDay>(dayInt), 
                                 static_cast<TAgnRpt::TWeekInMonth>(weekInt), 
                                 static_cast<TMonth>(monthInt), 
                                 startTime.DateTime().Year());

           // Ensure that the specified day is between start date and end date.

           if(yearlyRpt.StartDate()<startTime){
             yearlyRpt.SetStartDay(static_cast<TDay>(dayInt), 
                                   static_cast<TAgnRpt::TWeekInMonth>(weekInt), 
                                   static_cast<TMonth>(monthInt), 
                                   startTime.DateTime().Year()+1);

             if(yearlyRpt.StartDate()>endTime-TTimeIntervalDays(1)){ // must be endTime-1 day or panic occurs!!!
               PyErr_SetString(PyExc_ValueError, "yearly day of repeat must be between (repeat start date) and (repeat end date - 1 day)");
               return NULL;
             }
           }

           if(yearlyRpt.StartDate()>endTime-TTimeIntervalDays(1)){
             yearlyRpt.SetStartDay(static_cast<TDay>(dayInt), 
                                   static_cast<TAgnRpt::TWeekInMonth>(weekInt), 
                                   static_cast<TMonth>(monthInt), 
                                   endTime.DateTime().Year()-1);

             if(yearlyRpt.StartDate()<startTime){
               PyErr_SetString(PyExc_ValueError, "yearly day of repeat must be between (repeat start date) and (repeat end date - 1 day)");
               return NULL;
             }
           }

         }else{
           // default repeat date.
           yearlyRpt.SetStartDay(startTime.DayNoInWeek(), 
                                 static_cast<TAgnRpt::TWeekInMonth>(startTime.DayNoInMonth()/DAYS_IN_WEEK),
                                 startTime.DateTime().Month(), 
                                 startTime.DateTime().Year());
         }

         CREATE_RPT       
         rpt->SetYearlyByDay(yearlyRpt);
         SET_REPEAT_DATES_NO_START_DAY
         ADD_REPEAT
      }
    break;
    default:
      PyErr_SetString(PyExc_SyntaxError, "illegal repeat definition");
      return NULL; 
    break;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Parses and installs given repeat data into the entry.
 */
extern "C" PyObject *
Entry_set_repeat_data(Entry_object* self, PyObject* args)
{
  PyObject* retVal = NULL;
  PyObject* repeatDict = NULL;
  PyObject* key = NULL;
  PyObject* value = NULL;
  PyObject* repeatDays = NULL;
  TReal startDate = 0;
  TReal endDate = 0;
  TInt interval = 1;
  TInt repeatIndicator = -1;
  CArrayFixSeg<TReal>* exceptionArray = NULL;
  TBool eternalRepeat = EFalse;

  if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &repeatDict)){ 
    return NULL;
  }
    
  // Start date
  GET_VALUE_FROM_DICT(KStartDate, repeatDict)
  if(value){
    if(!PyFloat_Check(value)){
      PyErr_SetString(PyExc_TypeError, "start date must be float");
      return NULL; 
    }
    startDate = PyFloat_AsDouble(value);
  }
  
  // End date
  GET_VALUE_FROM_DICT(KEndDate, repeatDict)
  if(value){
    if(PyFloat_Check(value)){
      endDate = PyFloat_AsDouble(value);
    }else{
      if(value==Py_None){
        // None given as end date.
        eternalRepeat=ETrue;
      }else{
        PyErr_SetString(PyExc_TypeError, "end date must be float (or None)");
        return NULL; 
      }
    }
  }
  
  // Interval
  GET_VALUE_FROM_DICT(KInterval, repeatDict)
  if(value){
    if(!PyInt_Check(value) || PyInt_AsLong(value)<=0){
      PyErr_SetString(PyExc_TypeError, "interval must be int and >0");
      return NULL; 
    }
    interval = PyInt_AsLong(value);
  }
  
  // Repeat days
  GET_VALUE_FROM_DICT(KRepeatDays, repeatDict)
  repeatDays = value;

  // Repeat type
  GET_VALUE_FROM_DICT(KRepeatType, repeatDict)
  if(value){
    if(!PyString_Check(value)){
      PyErr_SetString(PyExc_TypeError, "repeat type must be string");
      return NULL; 
    }
    TPtrC8 valuePtr((TUint8*)PyString_AsString(value), PyString_Size(value));
    if(0 == valuePtr.Compare(KDaily)){
      repeatIndicator = DAILY_REPEAT;
    }else if(0 == valuePtr.Compare(KWeekly)){
      repeatIndicator = WEEKLY_REPEAT;
    }else if(0 == valuePtr.Compare(KMonthlyByDates)){
      repeatIndicator = MONTHLY_BY_DATES_REPEAT;
    }else if(0 == valuePtr.Compare(KMonthlyByDays)){
      repeatIndicator = MONTHLY_BY_DAYS_REPEAT;
    }else if(0 == valuePtr.Compare(KYearlyByDate)){
      repeatIndicator = YEARLY_BY_DATE_REPEAT;
    }else if(0 == valuePtr.Compare(KYearlyByDay)){
      repeatIndicator = YEARLY_BY_DAY_REPEAT;
    }else if(0 == valuePtr.Compare(KRepeatNone)){
      repeatIndicator = NOT_REPEATED;
    }else{
      PyErr_SetString(PyExc_ValueError, "illegal repeat type");
      return NULL; 
    }
  }  

  // Exceptions
  GET_VALUE_FROM_DICT(KExceptions, repeatDict)
  if(value){
    if(!PyList_Check(value)){
      PyErr_SetString(PyExc_SyntaxError, "exceptions must be given in a list");
      return NULL; 
    }
    if(PyList_Size(value)>0){
      TRAPD(error, 
            exceptionArray = new (ELeave) CArrayFixSeg<TReal>(PyList_Size(value)));
      if(error!=KErrNone){
        return SPyErr_SetFromSymbianOSErr(error);
      }
    }
    for(TInt i=0;i<PyList_Size(value);i++){
      PyObject* listItem = PyList_GetItem(value, i);
      if(PyFloat_Check(listItem)){
        TRAPD(error, exceptionArray->AppendL(PyFloat_AsDouble(listItem)));
        if(error!=KErrNone){
          delete exceptionArray;
          return SPyErr_SetFromSymbianOSErr(error); 
        }
      }else{
        delete exceptionArray;
        PyErr_SetString(PyExc_TypeError, "exception dates must be floats");
        return NULL; 
      }
    }
  }

  retVal = Entry_install_repeat_data(self, startDate, endDate, interval, 
                                     repeatIndicator, exceptionArray, 
                                     repeatDays, eternalRepeat);
  delete exceptionArray;  

  return retVal;
}


/*
 * Returns entry's recurrence ("days") information.
 */
extern "C" PyObject *
Entry_recurrence_data(Entry_object* self)
{
  PyObject* dayList = NULL;

  // Recurrence.
  switch(self->entryItem->RptDef()->Type()){
    case CVersitRecurrence::EWeekly:
      {        
        dayList = PyList_New(self->entryItem->RptDef()->Weekly().NumDaysSet());
        CHECK_DAYLIST_CREATION
        TDay dayArr[DAYS_IN_WEEK] = 
          {EMonday, ETuesday, EWednesday, EThursday, EFriday, ESaturday, ESunday};
        TInt listIndex = 0;
        for(TInt i=0;i<DAYS_IN_WEEK;i++){
          if(self->entryItem->RptDef()->Weekly().IsDaySet(dayArr[i]) != EFalse){
            PyObject* dayNum = Py_BuildValue("i", dayArr[i]);
            SET_ITEM_TO_DAYLIST 
          }
        }
      }  
    break;
    case CVersitRecurrence::EMonthlyByPos:
      {        
        dayList = PyList_New(self->entryItem->RptDef()->MonthlyByDates().NumDatesSet());
        CHECK_DAYLIST_CREATION
        TInt listIndex = 0;
        for(TInt i=0;i<DAYS_IN_MONTH;i++){
          if(self->entryItem->RptDef()->MonthlyByDates().IsDateSet(i) != EFalse){
            PyObject* dayNum = Py_BuildValue("i", i);            
            SET_ITEM_TO_DAYLIST
          }
        }
      }
    break;
    case CVersitRecurrence::EMonthlyByDay:
      {        
        dayList = PyList_New(self->entryItem->RptDef()->MonthlyByDays().NumDaysSet());
        CHECK_DAYLIST_CREATION
        TInt listIndex = 0;
        TDay dayArr[DAYS_IN_WEEK] = 
          {EMonday, ETuesday, EWednesday, EThursday, EFriday, ESaturday, ESunday};
        for(TInt i=0;i<WEEKS_IN_MONTH;i++){
          for(TInt j=0;j<DAYS_IN_WEEK;j++){
            if(self->entryItem->RptDef()->MonthlyByDays().IsDaySet(dayArr[j], 
                                                                   static_cast<TAgnRpt::TWeekInMonth>(i)) != EFalse){
              PyObject* dayNum = Py_BuildValue("{s:i,s:i}", (const char*)(&KDay)->Ptr(), dayArr[j], (const char*)(&KWeek)->Ptr(), i);   
              SET_ITEM_TO_DAYLIST
            }
          }
        } 
      }
    break;
    case CVersitRecurrence::EYearlyByDay:
      {    
        TDay day;
        TAgnRpt::TWeekInMonth week;
        TMonth month;
        TInt year;
        self->entryItem->RptDef()->YearlyByDay().GetStartDay(day, week, month, year);

        dayList = 
          Py_BuildValue("{s:i,s:i,s:i}", 
          (const char*)(&KDay)->Ptr(), day, 
          (const char*)(&KWeek)->Ptr(), week,
          (const char*)(&KMonth)->Ptr(), month); 
      }
    break;
    default:
      Py_INCREF(Py_None);
      return Py_None;
    break;
  } 

  return dayList;
}


/*
 * Returns entry's repeat information.
 */
extern "C" PyObject *
Entry_repeat_data(Entry_object* self, PyObject* /*args*/)
{
  TInt err = 0;
  PyObject* value = NULL;
  PyObject* key = NULL;

  if(!self->entryItem->RptDef()){
    // Not repeated.
    return Py_BuildValue("{s:s}", (const char*)(&KRepeatType)->Ptr(), 
                                  (const char*)(&KRepeatNone)->Ptr());    
  };

  PyObject* repeatDataDict = PyDict_New();
  if (repeatDataDict == NULL){
    return PyErr_NoMemory();
  }   

  // Repeat type
  key = Py_BuildValue("s", (const char*)(&KRepeatType)->Ptr());
  if(key == NULL){
    Py_DECREF(repeatDataDict);
    return NULL;
  }  
  
  switch(self->entryItem->RptDef()->Type()){
    case CAgnRptDef::EDaily:
      value = Py_BuildValue("s", (const char*)(&KDaily)->Ptr());
    break;
    case CAgnRptDef::EWeekly:
      value = Py_BuildValue("s", (const char*)(&KWeekly)->Ptr());
    break;
    case CAgnRptDef::EMonthlyByDates:
      value = Py_BuildValue("s", (const char*)(&KMonthlyByDates)->Ptr());
    break;
    case CAgnRptDef::EMonthlyByDays:
      value = Py_BuildValue("s", (const char*)(&KMonthlyByDays)->Ptr());
    break;
    case CAgnRptDef::EYearlyByDate:
      value = Py_BuildValue("s", (const char*)(&KYearlyByDate)->Ptr());
    break;
    case CAgnRptDef::EYearlyByDay:
      value = Py_BuildValue("s", (const char*)(&KYearlyByDay)->Ptr());
    break;
    default:
      value = Py_BuildValue("s", "unknown");
    break;
  }

  ADD_ITEM_TO_REP_DICT(key, value)
  
  // Start date.
  key = Py_BuildValue("s", (const char*)(&KStartDate)->Ptr());
  value = 
    Py_BuildValue("d", time_as_UTC_TReal(self->entryItem->RptDef()->StartDate()));
  ADD_ITEM_TO_REP_DICT(key, value)

  // End date.
  key = Py_BuildValue("s", (const char*)(&KEndDate)->Ptr());
  if(self->entryItem->RptDef()->RepeatForever()!=EFalse){
    // Repeats forever. Set end date to None.
    Py_INCREF(Py_None);
    value = Py_None;
  }else{
    value = 
      Py_BuildValue("d", time_as_UTC_TReal(self->entryItem->RptDef()->EndDate()));
  }
  ADD_ITEM_TO_REP_DICT(key, value)

  // Interval.
  key = Py_BuildValue("s", (const char*)(&KInterval)->Ptr());
  value = Py_BuildValue("i", self->entryItem->RptDef()->Interval());
  ADD_ITEM_TO_REP_DICT(key, value)

  // Exceptions.
  const CAgnExceptionList* exceptionList = self->entryItem->RptDef()->Exceptions(); 
  if(exceptionList && exceptionList->Count()>0){
    PyObject* exceptionPyList = PyList_New(exceptionList->Count());
    if (exceptionPyList == NULL){
      Py_DECREF(repeatDataDict);
      return PyErr_NoMemory();
    } 
    for(TInt i=0;i<exceptionList->Count();i++){
      const TAgnException& aException = exceptionList->At(i);      
      PyObject* exceptionDate =
        Py_BuildValue("d", time_as_UTC_TReal(AgnDateTime::AgnDateToTTime(aException.Date())));
      if (exceptionDate == NULL){
        Py_DECREF(repeatDataDict);
        Py_DECREF(exceptionPyList);
        return NULL;
      } 
      if(PyList_SetItem(exceptionPyList, i, exceptionDate)){
        Py_DECREF(repeatDataDict);
        Py_DECREF(exceptionPyList);
        return NULL;
      }      
    }
    key = Py_BuildValue("s", (const char*)(&KExceptions)->Ptr());
    if(key==NULL){
      Py_DECREF(repeatDataDict);
      Py_DECREF(exceptionPyList);
      return NULL;
    }
    err = PyDict_SetItem(repeatDataDict, key, exceptionPyList);
    Py_DECREF(key);
    Py_DECREF(exceptionPyList);
    if(err){
      Py_DECREF(repeatDataDict);
      return NULL;
    }
  }else{
    PyObject* exceptionPyList = PyList_New(0);
    if (exceptionPyList == NULL){
      Py_DECREF(repeatDataDict);
      return PyErr_NoMemory();
    } 
    key = Py_BuildValue("s", (const char*)(&KExceptions)->Ptr());
    if(key==NULL){
      Py_DECREF(repeatDataDict);
      Py_DECREF(exceptionPyList);
      return NULL;
    }
    err = PyDict_SetItem(repeatDataDict, key, exceptionPyList);
    Py_DECREF(key);
    Py_DECREF(exceptionPyList);
    if(err){
      Py_DECREF(repeatDataDict);
      return NULL;
    }
  }

  // Recurrence.
  PyObject* recurrenceData = Entry_recurrence_data(self);
  if(recurrenceData==NULL){
    Py_DECREF(repeatDataDict);
    return NULL;
  }

  if(recurrenceData != Py_None){
    key = Py_BuildValue("s", (const char*)(&KRepeatDays)->Ptr());
    if(key==NULL){
      Py_DECREF(repeatDataDict);
      return NULL;
    }
    err = PyDict_SetItem(repeatDataDict, key, recurrenceData);
    Py_DECREF(key);
    Py_DECREF(recurrenceData);
    if(err){
      Py_DECREF(repeatDataDict);
      return NULL;
    }
  }else{
    Py_DECREF(recurrenceData);
  }

  return repeatDataDict;
}


/*
 * Sets entry's priority value.
 *
 * the native calendar application uses following priority values:
 * 1, high
 * 2, normal
 * 3, low
 * others, low (exception:in the calendar (the initial value) 0 = normal, but in the todo app it is not). 
 */
extern "C" PyObject *
Entry_set_priority(Entry_object* self, PyObject* args)
{
  TInt priority = 0;
  if (!PyArg_ParseTuple(args, "i", &priority)){ 
    return NULL;
  }

  switch(self->entryItem->Type()){
    case CAgnEntry::ETodo:
      {
        self->entryItem->CastToTodo()->SetPriority(priority);
      }
    break;  
    default:
      {
#if SERIES60_VERSION>12
  self->entryItem->SetEventPriority(priority);
#else
  PyErr_SetString(PyExc_RuntimeError, "only todos have priority");
  return NULL; 
#endif
      }
    break;
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns entry's priority value. 
 */
extern "C" PyObject *
Entry_priority(Entry_object* self, PyObject* /*args*/)
{
  TInt priority = 0;
  
  switch(self->entryItem->Type()){
    case CAgnEntry::ETodo:
      {
        priority = self->entryItem->CastToTodo()->Priority();
      }
    break;  
    default:
      {
#if SERIES60_VERSION>12
  priority = self->entryItem->EventPriority();
#else
  PyErr_SetString(PyExc_RuntimeError, "only todos have priority");
  return NULL; 
#endif
      }
    break;
  }

  return Py_BuildValue("i", priority);
}


/*
 * Sets entry's todo list id e.g. the todo list id the
 * entry belongs to. 
 * -this method is for todo-entries only.
 */
extern "C" PyObject *
Entry_set_todo_list(Entry_object* self, PyObject* args)
{
  CHECK_AGENDA_STATE_ENTRY

  TInt32 todoListId = 0;
  TBool idIsValid = EFalse;
  if (!PyArg_ParseTuple(args, "i", &todoListId)){ 
    return NULL;
  }

  if(self->entryItem->Type()!=CAgnEntry::ETodo){
    PyErr_SetString(PyExc_RuntimeError, 
                    "cannot set todo list id for this entry type");
    return NULL;
  }

  TAgnTodoListId suggestedId(todoListId);

  CAgnTodoListNames* todoListNames = NULL;
  TRAPD(error, {
    todoListNames = CAgnTodoListNames::NewL();
    CleanupStack::PushL(todoListNames);
    self->calendarDb->agendaModel->PopulateTodoListNamesL(todoListNames);
    for(TInt i=0;i<todoListNames->Count();i++){
      if(todoListNames->TodoListId(i)==suggestedId){
        idIsValid=ETrue;
        break;
      }
    }
    CleanupStack::PopAndDestroy(); // todoListNames 
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  if(idIsValid==EFalse){
    PyErr_SetString(PyExc_ValueError, 
                    "illegal todo list id");
    return NULL;
  }

  self->entryItem->CastToTodo()->SetTodoListId(suggestedId);

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns entry's todo list id e.g. the todo list id the
 * entry belongs to. 
 * -this method is for todo-entries only.
 */
extern "C" PyObject *
Entry_todo_list_id(Entry_object* self, PyObject* /*args*/)
{
  if(self->entryItem->Type()!=CAgnEntry::ETodo){
    PyErr_SetString(PyExc_RuntimeError, 
                    "only todo entry has a todo list");
    return NULL;
  }
  
  return Py_BuildValue("i", self->entryItem->CastToTodo()->TodoListId().Id());
}


/*
 * Sets the given crossed out datetime for the entry.
 * -if 0 is given as a datetime the entry is set to uncrossed state.
 */
extern "C" PyObject *
Entry_set_crossed_out(Entry_object* self, PyObject* args)
{
  TReal crossedOut = 0;
  if (!PyArg_ParseTuple(args, "d", &crossedOut)){ 
    return NULL;
  }
 
  if(0 != crossedOut){
    if(self->entryItem->Type()==CAgnEntry::ETodo){
      TTime theTime;
      pythonRealAsTTime(crossedOut, theTime);
      if(Check_time_validity(theTime)==EFalse){
        PyErr_SetString(PyExc_ValueError, 
                        "illegal datetime value");
        return NULL;
      }          
      self->entryItem->CastToTodo()->CrossOut(theTime);  
    }else{
      self->entryItem->SetIsCrossedOut(ETrue);
    }  
   }else{
     if(self->entryItem->Type()==CAgnEntry::ETodo){
       self->entryItem->CastToTodo()->UnCrossOut();  
     }else{
       self->entryItem->SetIsCrossedOut(EFalse);
     }
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns information whether the entry is crossed out.
 */
extern "C" PyObject *
Entry_is_crossed_out(Entry_object* self, PyObject* /*args*/)
{
  return Py_BuildValue("i", self->entryItem->IsCrossedOut());
}


/*
 * Returns crossed out datetime (only for todos).
 */
extern "C" PyObject *
Entry_crossed_out_date(Entry_object* self, PyObject* /*args*/)
{
  if(self->entryItem->Type()!=CAgnEntry::ETodo){
    PyErr_SetString(PyExc_RuntimeError, 
                    "only todos have crossed out date");
    return NULL;
  }
  
  if(EFalse!=Is_null_time(self->entryItem->CastToTodo()->CrossedOutDate())){
    Py_INCREF(Py_None);
    return Py_None;
  }

  return ttimeAsPythonFloat(self->entryItem->CastToTodo()->CrossedOutDate());
}


/*
 * Saves the entry into the database.
 * -new entries are added into the database, old entries are updated.
 */ 
extern "C" PyObject *
Entry_commit(Entry_object* self, PyObject* /*args*/)
{
  CHECK_AGENDA_STATE_ENTRY

  if(self->uniqueId.IsNullId()){
    // New entry (does not exist in the database yet).
    switch(self->entryItem->Type()){
      case CAgnEntry::ETodo:
        {
          // Check that at least one todo list exists.
          if(0==self->calendarDb->agendaModel->TodoListCount()){
            PyErr_SetString(PyExc_RuntimeError, "no todo-list available");
            return NULL;
          }  
        
          TRAPD(error, {
            CAgnTodo* todo = self->entryItem->CastToTodo();
            
            if(todo->TodoListId().IsNullId()){
              // Get the "default" list id (e.g. the first in the list).
              CAgnTodoListNames* todoListNames = NULL;
              todoListNames = CAgnTodoListNames::NewL();
              CleanupStack::PushL(todoListNames);
              self->calendarDb->agendaModel->PopulateTodoListNamesL(todoListNames);
              todo->SetTodoListId(todoListNames->TodoListId(0));
              CleanupStack::PopAndDestroy(); // todoListNames
            }
            
            self->uniqueId = 
              self->calendarDb->agendaServer->GetUniqueId(
                self->calendarDb->agendaModel->AddEntryL(self->entryItem));  
          });
          if(error!=KErrNone){
            return SPyErr_SetFromSymbianOSErr(error);
          }
        }
      break;
      case CAgnEntry::EEvent:
        {
          TRAPD(error, {
            self->uniqueId = 
              self->calendarDb->agendaServer->GetUniqueId(
                self->calendarDb->agendaModel->AddEntryL(self->entryItem->CastToEvent()));
          });
          if(error!=KErrNone){
            return SPyErr_SetFromSymbianOSErr(error);
          }
        }
      break;
      case CAgnEntry::EAnniv:
        {
          TRAPD(error, {
            self->uniqueId = 
              self->calendarDb->agendaServer->GetUniqueId(
                self->calendarDb->agendaModel->AddEntryL(self->entryItem->CastToAnniv()));
          });
          if(error!=KErrNone){
            return SPyErr_SetFromSymbianOSErr(error);
          }
        }
      break;
      case CAgnEntry::EAppt:
        {
          CAgnAppt* appt = self->entryItem->CastToAppt();
          if(appt->StartDateTime() == Time::NullTTime() || 
             appt->EndDateTime() == Time::NullTTime()){
            PyErr_SetString(PyExc_RuntimeError, "appointment must have start and end datetime");
            return NULL; 
          }
          TRAPD(error, {
            self->uniqueId = 
              self->calendarDb->agendaServer->GetUniqueId(
                self->calendarDb->agendaModel->AddEntryL(appt));
          });
          if(error!=KErrNone){
            return SPyErr_SetFromSymbianOSErr(error);
          }
        }
      break;
    }
  }else{
    // Old entry (already exists in the database).
    TRAPD(error, {
      self->calendarDb->agendaModel->UpdateEntryL(self->entryItem);
    });
    if(error!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);
    }
  }

  return Py_BuildValue("i", self->uniqueId.Id());
}


/*
 * Returns the type of the entry (appt, event, anniv, todo).
 */
extern "C" PyObject *
Entry_entry_type(Entry_object* self, PyObject* /*args*/)
{
  return Py_BuildValue("i", self->entryItem->Type());
}


/*
 * Cancels entry's alarm.
 */
extern "C" PyObject *
Entry_cancel_alarm(Entry_object* self, PyObject* /*args*/)
{
  self->entryItem->ClearAlarm();
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Sets an alarm to the entry.
 * -time of the alarm is given as a parameter
 */
extern "C" PyObject *
Entry_set_alarm(Entry_object* self, PyObject* args)
{
  CHECK_AGENDA_STATE_ENTRY

  TReal alarmTime = 0;
  if (!PyArg_ParseTuple(args, "d", &alarmTime)){ 
    return NULL;
  }

  TTime alarmTTime;
  pythonRealAsTTime(alarmTime, alarmTTime);
 
  TTime startTTime(TInt64(0));  

  switch(self->entryItem->Type()){
    case CAgnEntry::EAppt:
      {
        startTTime = self->entryItem->CastToAppt()->StartDateTime();
      }
    break;
    case CAgnEntry::ETodo:
      {
        startTTime = self->entryItem->CastToTodo()->DueDate();
      }
    break;
    default: // Anniv, Event
      {
        startTTime = self->entryItem->CastToEvent()->StartDate();
      }
  }

  if(EFalse==Check_time_validity(startTTime)){
    PyErr_SetString(PyExc_RuntimeError, 
                    "set start time for the entry before setting an alarm");
    return NULL;
  }

  if(EFalse==Check_time_validity(alarmTTime)){
    PyErr_SetString(PyExc_ValueError, 
                    "illegal alarm datetime value");
    return NULL;
  }

  if(startTTime.DaysFrom(alarmTTime)>=TTimeIntervalDays(EARLIEST_ALARM_DAY_INTERVAL)){
    PyErr_SetString(PyExc_ValueError, "alarm datetime too early for the entry");
    return NULL;
  }
 
  if(alarmTTime.DateTime().Year()>startTTime.DateTime().Year() ||
    (alarmTTime.DateTime().Year()==startTTime.DateTime().Year() && alarmTTime.DayNoInYear()>startTTime.DayNoInYear())){
    PyErr_SetString(PyExc_ValueError, "alarm datetime too late for the entry");
    return NULL;
  }
   
  self->entryItem->SetAlarm(startTTime.DaysFrom(alarmTTime), 
                            alarmTTime.DateTime().Hour()*60+alarmTTime.DateTime().Minute());
  

#if SERIES60_VERSION<20
  TRAPD(error, {
    if(!self->alarm){
      self->alarm=CAgnAlarm::NewL(self->calendarDb->agendaModel);
    }
    self->calendarDb->agendaModel->RegisterAlarm(self->alarm);
    self->alarm->FindAndQueueNextFewAlarmsL();
    self->alarm->OrphanAlarm();
  });
#else
  TRAPD(error, {
    CAgnAlarm* alarm = CAgnAlarm::NewL(self->calendarDb->agendaModel);
    CleanupStack::PushL(alarm);
    self->calendarDb->agendaModel->RegisterAlarm(alarm);
    alarm->FindAndQueueNextFewAlarmsL();
    alarm->OrphanAlarm();
    CleanupStack::PopAndDestroy(); // alarm.
  });
#endif
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns the alarm datetime.
 */
extern "C" PyObject *
Entry_alarm_datetime(Entry_object* self, PyObject* /*args*/)
{
  return ttimeAsPythonFloat(self->entryItem->AlarmInstanceDateTime());  
}


/*
 * Returns the information whether the entry has an alarm.
 */
extern "C" PyObject *
Entry_has_alarm(Entry_object* self, PyObject* /*args*/)
{
  return Py_BuildValue("i", self->entryItem->HasAlarm());  
}


/*
 * Returns unique id of this entry.
 */
extern "C" PyObject *
Entry_unique_id(Entry_object* self, PyObject* /*args*/)
{
  return Py_BuildValue("i", self->entryItem->UniqueId().Id());
}


/*
 * Returns the datetime this entry was last modified.
 */
extern "C" PyObject *
Entry_last_modified(Entry_object* self, PyObject* /*args*/)
{
  CHECK_AGENDA_STATE_ENTRY

  if(self->uniqueId.IsNullId()){
    PyErr_SetString(PyExc_RuntimeError, 
                    "the entry has not been committed to the database");
    return NULL;
  }
  TAgnDateTime dateTime = self->calendarDb->agendaServer->UniqueIdLastChangedDate(self->uniqueId);
  return ttimeAsPythonFloat(AgnDateTime::AgnDateTimeToTTime(dateTime));  
}


/*
 * Sets entry's replication status.
 */
extern "C" PyObject *
Entry_set_replication(Entry_object* self, PyObject* args)
{  
  TInt replicationStatus = 0;
  if (!PyArg_ParseTuple(args, "i", &replicationStatus)){ 
    return NULL;
  }

  if(replicationStatus!=TAgnReplicationData::EOpen && 
     replicationStatus!=TAgnReplicationData::EPrivate &&
     replicationStatus!=TAgnReplicationData::ERestricted){
     PyErr_SetString(PyExc_ValueError, 
                    "illegal replication status");
     return NULL;    
  }

  TAgnReplicationData replicationData;
  replicationData.SetStatus(static_cast<TAgnReplicationData::TStatus>(replicationStatus));
 
  self->entryItem->SetReplicationData(replicationData);
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns entry's replication status.
 */
extern "C" PyObject *
Entry_replication(Entry_object* self, PyObject* /*args*/)
{  
  return Py_BuildValue("i", self->entryItem->ReplicationData().Status());
}


/*
 * Deallocate the entry object.
 */
extern "C" {
  static void Entry_dealloc(Entry_object *entry)
  {
    delete entry->entryItem;
    delete entry->paraFormatLayer;
    delete entry->charFormatLayer;
#if SERIES60_VERSION<20
    delete entry->alarm;
#endif
    Py_DECREF(entry->calendarDb);
    PyObject_Del(entry);
  }
}


/*
 * EntryIterator methods.
 */


/*
 * Creates new entry iterator object.
 */
extern "C" PyObject *
new_entryIterator(CalendarDb_object* self, PyObject* /*args*/)
{ 
  CHECK_AGENDA_STATE_DB

  EntryIterator_object* ei =
    PyObject_New(EntryIterator_object, EntryIterator_type);
  
  if (ei == NULL){
    return PyErr_NoMemory();
  }

  ei->calendarDb = self;
  ei->hasMoreEntries = ei->calendarDb->agendaServer->CreateEntryIterator();
 
  Py_INCREF(ei->calendarDb);
  return (PyObject*)ei;  
}


/*
 * Returns next entry in the database (entry can be of 
 * any type [appt, event, anniv, todo]).
 */
extern "C" PyObject *
entryIterator_next(EntryIterator_object* self, PyObject* /*args*/)
{
  CHECK_AGENDA_STATE_ITERATOR

  TAgnEntryId entryId;

  if(EFalse == self->hasMoreEntries){
    PyErr_SetObject(PyExc_StopIteration, Py_None);
    return NULL;
  }

  entryId = self->calendarDb->agendaServer->EntryIteratorPosition();
 
  self->hasMoreEntries = self->calendarDb->agendaServer->EntryIteratorNext();
  
  return Py_BuildValue("i", 
                       self->calendarDb->agendaServer->GetUniqueId(entryId).Id());
}


/*
 * Deallocates entry iterator object.
 */
extern "C" {
  static void EntryIterator_dealloc(EntryIterator_object *entryIterator)
  {
    Py_DECREF(entryIterator->calendarDb);
    PyObject_Del(entryIterator);
  }
}


//////////////TYPE SET


extern "C" {

  const static PyMethodDef CalendarDb_methods[] = {
    {"add_entry", (PyCFunction)CalendarDb_add_entry, METH_VARARGS},
    {"monthly_instances", (PyCFunction)CalendarDb_monthly_instances, METH_VARARGS},
    {"daily_instances", (PyCFunction)CalendarDb_daily_instances, METH_VARARGS},
    {"export_vcals", (PyCFunction)CalendarDb_export_vcals, METH_VARARGS},
    {"import_vcals", (PyCFunction)CalendarDb_import_vcals, METH_VARARGS},
    {"todo_lists", (PyCFunction)CalendarDb_todo_lists, METH_NOARGS},
    {"add_todo_list", (PyCFunction)CalendarDb_add_todo_list, METH_VARARGS},
    {"remove_todo_list", (PyCFunction)CalendarDb_remove_todo_list, METH_VARARGS},
    {"rename_todo_list", (PyCFunction)CalendarDb_rename_todo_list, METH_VARARGS},
    {"find_instances", (PyCFunction)CalendarDb_find_instances, METH_VARARGS},
    {"todos_in_list", (PyCFunction)CalendarDb_todos_in_list, METH_VARARGS},
    {"default_todo_list", (PyCFunction)CalendarDb_default_todo_list, METH_NOARGS},
    {"compact", (PyCFunction)CalendarDb_compact, METH_NOARGS},
    {NULL, NULL}  
  };

  const static PyMethodDef EntryIterator_methods[] = {
    {NULL, NULL}  
  };

  const static PyMethodDef Entry_methods[] = {
    {"content", (PyCFunction)Entry_content, METH_NOARGS},
    {"set_content", (PyCFunction)Entry_set_content, METH_VARARGS},
    {"type", (PyCFunction)Entry_entry_type, METH_NOARGS},
    {"commit", (PyCFunction)Entry_commit, METH_NOARGS},
    {"set_repeat_data", (PyCFunction)Entry_set_repeat_data, METH_VARARGS},
    {"repeat_data", (PyCFunction)Entry_repeat_data, METH_NOARGS},
    {"location", (PyCFunction)Entry_location, METH_NOARGS},
    {"set_location", (PyCFunction)Entry_set_location, METH_VARARGS},
    {"set_start_and_end_datetime", (PyCFunction)Entry_set_start_and_end_datetime, METH_VARARGS},
    {"start_datetime", (PyCFunction)Entry_start_datetime, METH_NOARGS},
    {"end_datetime", (PyCFunction)Entry_end_datetime, METH_NOARGS},
    {"unique_id", (PyCFunction)Entry_unique_id, METH_NOARGS},
    {"last_modified", (PyCFunction)Entry_last_modified, METH_NOARGS},
    {"set_alarm", (PyCFunction)Entry_set_alarm, METH_VARARGS},
    {"set_priority", (PyCFunction)Entry_set_priority, METH_VARARGS},
    {"set_todo_list", (PyCFunction)Entry_set_todo_list, METH_VARARGS},
    {"todo_list_id", (PyCFunction)Entry_todo_list_id, METH_NOARGS},
    {"set_crossed_out", (PyCFunction)Entry_set_crossed_out, METH_VARARGS},
    {"is_crossed_out", (PyCFunction)Entry_is_crossed_out, METH_NOARGS},
    {"crossed_out_date", (PyCFunction)Entry_crossed_out_date, METH_NOARGS},
    {"alarm_datetime", (PyCFunction)Entry_alarm_datetime, METH_NOARGS},
    {"has_alarm", (PyCFunction)Entry_has_alarm, METH_NOARGS},
    {"priority", (PyCFunction)Entry_priority, METH_NOARGS},
    {"cancel_alarm", (PyCFunction)Entry_cancel_alarm, METH_NOARGS},
    {"set_replication", (PyCFunction)Entry_set_replication, METH_VARARGS},
    {"replication", (PyCFunction)Entry_replication, METH_NOARGS},
    {"make_undated", (PyCFunction)Entry_make_undated, METH_NOARGS},
    {"is_dated", (PyCFunction)Entry_is_dated, METH_NOARGS},
    {NULL, NULL}  
  };


  static PyObject *
  CalendarDb_getattr(CalendarDb_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)CalendarDb_methods, (PyObject *)op, name);
  }

  static PyObject *
  EntryIterator_getattr(EntryIterator_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)EntryIterator_methods, (PyObject *)op, name);
  }

  static PyObject *
  Entry_getattr(Entry_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)Entry_methods, (PyObject *)op, name);
  }


  #ifdef __WINS__
  static PyMappingMethods calendarDb_as_mapping = {
  #else
  static const PyMappingMethods calendarDb_as_mapping = {
  #endif  
    (inquiry)CalendarDb_len,                  /*mp_length*/
    (binaryfunc)CalendarDb_getitem,           /*mp_subscript*/
    (objobjargproc)CalendarDb_ass_sub,        /*mp_ass_subscript*/
  };

  static const PyTypeObject c_CalendarDb_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_calendar.CalendarDb",                   /*tp_name*/
    sizeof(CalendarDb_object),                /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)CalendarDb_dealloc,           /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)CalendarDb_getattr,          /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    &calendarDb_as_mapping,                   /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    (getiterfunc)new_entryIterator,           /*tp_iter */
  };

  static const PyTypeObject c_EntryIterator_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_calendar.EntryIterator",                /*tp_name*/
    sizeof(EntryIterator_object),             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)EntryIterator_dealloc,        /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)EntryIterator_getattr,       /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    0,                                        /*tp_iter */
    (iternextfunc)entryIterator_next,         /*tp_iternext */
  };

  static const PyTypeObject c_Entry_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_calendar.Entry",                        /*tp_name*/
    sizeof(Entry_object),                     /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)Entry_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)Entry_getattr,               /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    0,                                        /*tp_iter */
  };
   
} //extern C


//////////////INIT//////////////


extern "C" {
  static const PyMethodDef calendar_methods[] = {
    {"open", (PyCFunction)open_db, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initcalendar(void)
  {
    PyTypeObject* calendar_db_type = PyObject_New(PyTypeObject, &PyType_Type);
    *calendar_db_type = c_CalendarDb_type;
    calendar_db_type->ob_type = &PyType_Type;
    SPyAddGlobalString("CalendarDbType", (PyObject*)calendar_db_type);    

    PyTypeObject* entry_type = PyObject_New(PyTypeObject, &PyType_Type);
    *entry_type = c_Entry_type;
    entry_type->ob_type = &PyType_Type;
    SPyAddGlobalString("EntryType", (PyObject*)entry_type);    

    PyTypeObject* entry_iterator_type = PyObject_New(PyTypeObject, &PyType_Type);
    *entry_iterator_type = c_EntryIterator_type;
    entry_iterator_type->ob_type = &PyType_Type;
    SPyAddGlobalString("EntryIteratorType", (PyObject*)entry_iterator_type);    

    PyObject *m, *d;

    m = Py_InitModule("_calendar", (PyMethodDef*)calendar_methods);
    d = PyModule_GetDict(m);
  
    // Entry types. 
    PyDict_SetItemString(d,"entry_type_appt", PyInt_FromLong(CAgnEntry::EAppt));
    PyDict_SetItemString(d,"entry_type_todo", PyInt_FromLong(CAgnEntry::ETodo));
    PyDict_SetItemString(d,"entry_type_event", PyInt_FromLong(CAgnEntry::EEvent));
    PyDict_SetItemString(d,"entry_type_anniv", PyInt_FromLong(CAgnEntry::EAnniv));
  
    // Filters.
    PyDict_SetItemString(d,"appts_inc_filter", PyInt_FromLong(APPTS_INC_FILTER));
    PyDict_SetItemString(d,"events_inc_filter", PyInt_FromLong(EVENTS_INC_FILTER));
    PyDict_SetItemString(d,"annivs_inc_filter", PyInt_FromLong(ANNIVS_INC_FILTER));
    PyDict_SetItemString(d,"todos_inc_filter", PyInt_FromLong(TODOS_INC_FILTER));

    // Replication statuses.
    PyDict_SetItemString(d,"rep_open", PyInt_FromLong(TAgnReplicationData::EOpen));
    PyDict_SetItemString(d,"rep_private", PyInt_FromLong(TAgnReplicationData::EPrivate));
    PyDict_SetItemString(d,"rep_restricted", PyInt_FromLong(TAgnReplicationData::ERestricted));

    return;
  }
} /* extern "C" */


GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
