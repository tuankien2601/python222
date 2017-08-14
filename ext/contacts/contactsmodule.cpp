/*
 * ====================================================================
 * contactsmodule.cpp
 * 
 * Python API to Series 60 contacts database.
 *
 * Implements currently (21.04.2005) following Python methods and types:
 *
 * open()
 * - open the default contacts database.
 * open(<filename>)
 * - open the specified contacts database.
 * open(<filename>, 'c')
 * - open the specified contacts database. create if does not exist.
 * open(<filename>, 'n')
 * - create new, empty contacts database.
 *
 * ContactsDb
 *  close()
 *  - close the database.
 *  add_contact()
 *  - create new contact. note that the contact is not added and saved 
 *    into the database unless Contact.commit() is called.
 *  find(unicode [,tuple<int,..>])
 *  - return tuple that contains unique contact id:s found in the search.
 *    match is detected if search string is a substring of contact field data.
 *    if field type id:s are given in the optional tuple parameter the search 
 *    is reduced to fields of those types (however, this "reducing" cannot 
 *    be trusted a lot. see the c++ api documentation.) 
 *  export_vcards(tuple<int,..>)
 *  - returns string object that contains specified contacts as vcards. 
 *  import_vcards(string)
 *  - imports contacts that are in vcard format in the string object.
 *    returns tuple that contains unique id:s of imported contacts.
 *  field_types()
 *  - returns dictionary that contains some basic information of field types
 *    this contact database supports. key is the index of the field type 
 *    and value is a dictionary. 
 *  field_info(int)
 *  - returns detailed information of the specified field type. the parameter
 *    is the index of the field type (see field_types()). 
 *
 * ContactIterator
 *  next()
 *  - returns next contact id.
 *
 * Contact
 *  entry_data()
 *  - returns dictionary that contains information concerning the contact
 *    (uniqueid, title, lastmodified).
 *  begin()
 *  - sets the contact to read-write mode.
 *  commit()
 *  - saves the changes made to the contact and sets the contact to read-only 
 *    mode. 
 *  rollback()
 *  - discards the changes made to the contact and sets the contact to 
 *    read-only mode.
 *  add_field(int OR tuple<int,int>[, (value=)unicode][, (label=)unicode])
 *  - adds field to the contact. field type or field type and location (tuple)
 *    must be specified. value and label are optional parameters.
 *    returns the index of the field (index in the contact's field table).
 *  modify_field(int, [, (value=)unicode][, (label=)unicode])
 *  - modifies field value and/or label. field index must be specified
 *    (field's index in the contact's field table).
 *  field_info_index(int)
 *  - returns key/index of the field type (see field_types() and field_info())
 *    indicated by the given parameter (field's index in the contact's field table).
 *
 * FieldIterator
 *  next()
 *  - returns dictionary that contains field's data.
 *
 *
 *
 * FURTHER INFORMATION:
 *
 * About the values returned by field_info():
 *
 * fieldinfoindex
 *   -index/key of this field type in the dictionary returned by field_types().
 * fieldname
 *   -this field type's default label.
 * fieldid
 *   -field id of this field type.
 * fieldlocation
 *   -location information of the field.
 * storagetype
 *   -storage type of this field type. possible values:
 *     storage_type_text
 *       -text, phonenumbers, email addresses etc.
 *     storage_type_datetime
 *       -datetime values
 *     storage_type_store
 *       -binary data. reading or manipulating this type of content
 *        is not supported by this extension.
 *     storage_type_contact_item_id
 *       -used by agent fields. reading or manipulating this type of
 *        content is not supported by this extension.
 * multiplicity
 *   -multiplicity allowed for this field type. possible values: 
 *     field_type_multiplicity_one
 *     field_type_multiplicity_many
 * maxlength
 *   -maximum data length specified for this field type.
 * readonly
 *   -is this field read only.
 * namefield
 *   -is this is a name field type (like first name, last name).
 * usercanaddfield
 *   -can this field be added by user. 
 * editable
 *   -can the field be edited in Phonebook's contact editor. 
 * numericfield
 *   -is this a numeric field. 
 * phonenumberfield
 *   -is this a phone number field. 
 * mmsfield
 *   -is this is a MMS address field.
 * imagefield
 *   -is this an image field.
 * additemtext
 *   -the add item labeltext of the field.
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


#include "contactsmodule.h"


//////////////GENERAL FUNCTIONS///////////////


/*
 * Time handling.
 */


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


//////////////TYPE METHODS///////////////


/*
 * Module methods.
 */


/*
 * Opens the database and creates and returns a ContactsDb-object.
 *
 * open() - opens the default contact database file
 * open(u'filename') - opens file if it exists.
 * open(u'filename', 'c') - opens file, creates if the file does not exist.
 * open(u'filename', 'n') - creates new empty database file.
 */
extern "C" PyObject *
open_db(PyObject* /*self*/, PyObject *args)
{
  PyObject* filename = NULL;
  char* flag = NULL;
  TInt userError = KErrNone;
  CPbkContactEngine* contactEngine = NULL;
  RFs* fileServer = NULL;

  if (!PyArg_ParseTuple(args, "|Us", &filename, &flag)){ 
    return NULL;
  }
   
  TRAPD(serverError, { 
    fileServer = new (ELeave) RFs;
    User::LeaveIfError(fileServer->Connect());
  });
  if(serverError!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(serverError);
  }

  TRAPD(error, { 
    if(!flag){
      if(!filename){
        // Open default db file.
        contactEngine = CPbkContactEngine::NewL(fileServer);
      }else{
        // Open given db file, raise exception if the file does not exist.                   
        TBool fileExists = EFalse;
        RFs fileSession;
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), PyUnicode_GetSize(filename));
        User::LeaveIfError(fileSession.Connect());
        CleanupClosePushL(fileSession);
        fileExists = BaflUtils::FileExists(fileSession, filenamePtr);
        CleanupStack::PopAndDestroy(); // Close fileSession.

        if(fileExists){
          contactEngine = CPbkContactEngine::NewL(filenamePtr, EFalse, fileServer);
        }else{
          // File does not exist.
          userError = 1;
        }
      }
    }else{
      if(filename && flag[0] == 'c'){
        // Open, create if file doesn't exist.
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), PyUnicode_GetSize(filename));
        contactEngine = CPbkContactEngine::NewL(filenamePtr, EFalse, fileServer);
      }else if(filename && flag[0] == 'n'){
        // Create a new empty file.
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), PyUnicode_GetSize(filename));
        contactEngine = CPbkContactEngine::NewL(filenamePtr, ETrue, fileServer);
      }else{
        // Illegal parameter combination.
        userError = 2;
      }  
    }
  });

  if(error != KErrNone){
    fileServer->Close();
    delete fileServer;
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(userError == 1){
    fileServer->Close();
    delete fileServer;
    PyErr_SetString(PyExc_NameError, "file does not exist");
    return NULL;
  }
  if(userError == 2){
    fileServer->Close();
    delete fileServer;
    PyErr_SetString(PyExc_SyntaxError, "illegal parameter combination");
    return NULL;
  }
  return new_ContactsDb_object(contactEngine, fileServer);
}


/* 
 * ContactsDb methods.
 */


/*
 * Create new ContactsDb object.
 */
extern "C" PyObject *
new_ContactsDb_object(CPbkContactEngine* contactEngine, RFs* fileServer)
{
  if(!contactEngine){
    fileServer->Close();
    delete fileServer;
    PyErr_SetString(PyExc_RuntimeError, "contact engine is null");
    return NULL;
  }

  ContactsDb_object* contactsDb = 
    PyObject_New(ContactsDb_object, ContactsDb_type);
  if (contactsDb == NULL){
    delete contactEngine;
    fileServer->Close();
    delete fileServer;
    return PyErr_NoMemory();
  }

  contactsDb->contactEngine = contactEngine;
  contactsDb->fileServer = fileServer;

  return (PyObject*) contactsDb;
}


/*
 * Deallocate ContactsDb_object.
 */
extern "C" {
  static void ContactsDb_dealloc(ContactsDb_object *contactsDb)
  {
    delete contactsDb->contactEngine;
    contactsDb->fileServer->Close();
    delete contactsDb->fileServer;
    PyObject_Del(contactsDb);
  }
}


/*
 * Test whether contact entry indicated by uniqueId exists in the database
 * and creates python wrapper (Contact_object) object if it does.
 */
extern "C" PyObject *
ContactsDb_get_contact_by_id(ContactsDb_object* self, TContactItemId uniqueId)
{   
  ASSERT_DBOPEN
  
  if(uniqueId == KNullContactId){     
    PyErr_SetString(PyExc_ValueError, "illegal contact id");
    return NULL;
  }
  
  return new_Contact_object(self, uniqueId);
}


/*
 * Returns some information about supported field types 
 * (e.g. default labels, field id:s and location id:s of 
 * supported field types).
 */
extern "C" PyObject *
ContactsDb_field_types(ContactsDb_object* self, PyObject* /*args*/)
{
  ASSERT_DBOPEN

  TInt err = 0;

  const CPbkFieldsInfo& fieldsInfo = self->contactEngine->FieldsInfo();

  PyObject* fieldNameDict = PyDict_New(); 
  if (fieldNameDict == NULL){
    return PyErr_NoMemory();
  }  

  for(TInt i=0; i<fieldsInfo.Count();i++){ 
 
    PyObject* infoDict = 
      Py_BuildValue("{s:u#,s:i,s:i}",
        (const char*)(&KKeyStrFieldName)->Ptr(), fieldsInfo[i]->FieldName().Ptr(), 
                                                 fieldsInfo[i]->FieldName().Length(),
        (const char*)(&KKeyStrFieldId)->Ptr(), fieldsInfo[i]->FieldId(),
        (const char*)(&KKeyStrFieldLocation)->Ptr(), fieldsInfo[i]->Location());

    if (infoDict == NULL){
      PyObject_Del(fieldNameDict);
      return NULL;
    }

    PyObject* indexObj = Py_BuildValue("i", i);

    if (indexObj == NULL){
      PyObject_Del(infoDict);
      PyObject_Del(fieldNameDict);
      return NULL;
    }
  
    err = PyDict_SetItem(fieldNameDict, indexObj, infoDict);
    Py_DECREF(indexObj);
    Py_DECREF(infoDict);
    if(err<0){
      Py_DECREF(fieldNameDict);
      return NULL;
    }  
  }
  return fieldNameDict;
}


/*
 * Returns information about the field type indicated by the index.
 */
extern "C" PyObject *
ContactsDb_field_info(ContactsDb_object* self, PyObject *args)
{
  ASSERT_DBOPEN

  TInt index;
  if (!PyArg_ParseTuple(args, "i", &index)){ 
    return NULL;
  }

  const CPbkFieldsInfo& fieldsInfo = self->contactEngine->FieldsInfo();

  if (index < 0 || index >= fieldsInfo.Count()){
    PyErr_SetString(PyExc_IndexError, "illegal field type index");
    return NULL;
  } 
 
  return Py_BuildValue("{s:i,s:u#,s:i,s:i,s:i,s:i,s:i,\
                        s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:u#}",
      "fieldinfoindex", index,
      (const char*)(&KKeyStrFieldName)->Ptr(), fieldsInfo[index]->FieldName().Ptr(), 
                   fieldsInfo[index]->FieldName().Length(),
      (const char*)(&KKeyStrFieldId)->Ptr(), fieldsInfo[index]->FieldId(),
      (const char*)(&KKeyStrFieldLocation)->Ptr(), fieldsInfo[index]->Location(),
      "storagetype", fieldsInfo[index]->FieldStorageType(),
      "multiplicity", fieldsInfo[index]->Multiplicity(),
      "maxlength", fieldsInfo[index]->MaxLength(),           
      "readonly", fieldsInfo[index]->IsReadOnly(),
      "namefield", fieldsInfo[index]->NameField(),
      "usercanaddfield", fieldsInfo[index]->UserCanAddField(),
      "editable", fieldsInfo[index]->IsEditable(),
      "numericfield", fieldsInfo[index]->NumericField(),
      "phonenumberfield", fieldsInfo[index]->IsPhoneNumberField(),
      "mmsfield", fieldsInfo[index]->IsMmsField(),
      "imagefield", fieldsInfo[index]->IsImageField(), 
      "additemtext", fieldsInfo[index]->AddItemText().Ptr(),
                     fieldsInfo[index]->AddItemText().Length());                    
}              


/*
 * ContactsDb_object creates new contact.
 */
extern "C" PyObject *
ContactsDb_add_contact(ContactsDb_object* self, PyObject /**args*/)
{
  return new_Contact_object(self, KNullContactId);
}


/*
 * ContactsDb_object deletes given contact entry from the database.
 * Entry is identified using it's unique ID.
 */
extern "C" PyObject *
ContactsDb_delete_contact(ContactsDb_object* self, 
                          TContactItemId uniqueContactID)
{
  ASSERT_DBOPEN

  TInt error = KErrNone;
  TRAP(error, self->contactEngine->DeleteContactL(uniqueContactID));
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;   
}


/*
 * Converts tuple of field id:s to CPbkFieldIdArray.
 * (note that if fieldIdTuple==NULL CPbkFieldIdArray will contain all
 * field id:s [due to bug in c++ api's contact engine's FindLC method]).
 */
extern "C" CPbkFieldIdArray *
ContactsDb_getFieldIdArrayL(ContactsDb_object* self, PyObject *fieldIdTuple)
{
  CPbkFieldIdArray* fieldIdArray  = new (ELeave) CPbkFieldIdArray;
  CleanupStack::PushL(fieldIdArray);
  
  const CPbkFieldsInfo& fieldsInfo = self->contactEngine->FieldsInfo();
  for(TInt index = 0;index<fieldsInfo.Count();index++){
    fieldIdArray->AppendL(fieldsInfo[index]->FieldId());
  }

  if(fieldIdTuple){
    // Search only specified field types.
    CPbkFieldIdArray* selectedFieldIds = new (ELeave) CPbkFieldIdArray;
    CleanupStack::PushL(selectedFieldIds);

    for(TInt i=0;i<PyTuple_Size(fieldIdTuple);i++){
      PyObject* fieldIdItem = PyTuple_GetItem(fieldIdTuple, i);
      TInt fieldId = PyInt_AsLong(fieldIdItem);
      if(fieldIdArray->Find(fieldId)!=KErrNotFound){
        selectedFieldIds->AppendL(fieldId);
      }
    }
    CleanupStack::Pop(selectedFieldIds);
    CleanupStack::PopAndDestroy(fieldIdArray);
    return selectedFieldIds;
  }else{
    // Search all field types.
    CleanupStack::Pop(fieldIdArray);
    return fieldIdArray;
  }
}


/*
 * Execute the search.
 */
CContactIdArray*
ContactsDb_searchL(ContactsDb_object* self, 
                   TDesC& searchStr, 
                   CPbkFieldIdArray* selectedFieldIds)
{
  CContactIdArray* idArray = NULL;
  idArray = self->contactEngine->FindLC(searchStr, selectedFieldIds);
  CleanupStack::Pop(idArray);
  return idArray;
}


/*
 * Searches contacts by string (match is detected if the string is
 * a substring of some field value of the contact).
 * Returns tuple of unique contact ID:s representing contact entrys
 * matching the criteria.
 */
extern "C" PyObject *
ContactsDb_find(ContactsDb_object* self, PyObject *args)
{
  ASSERT_DBOPEN

  PyObject* searchString = NULL;
  CContactIdArray* idArray = NULL;
  PyObject* fieldIdTuple = NULL;

  if (!PyArg_ParseTuple(args, "U|O!", 
                        &searchString, &PyTuple_Type, &fieldIdTuple)){ 
    return NULL;
  }

  TPtrC searchStringPtr((TUint16*) PyUnicode_AsUnicode(searchString), 
                         PyUnicode_GetSize(searchString));
  
  TRAPD(error, {
    CPbkFieldIdArray* selectedFieldIds = 
      ContactsDb_getFieldIdArrayL(self, fieldIdTuple);
    CleanupStack::PushL(selectedFieldIds);
    idArray = ContactsDb_searchL(self, searchStringPtr, selectedFieldIds);
    CleanupStack::PopAndDestroy(selectedFieldIds);
  });

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
 
  PyObject* idArrayTuple = PyTuple_New(idArray->Count());
  if (idArrayTuple == NULL){
    delete idArray;
    return PyErr_NoMemory();
  }
  
  for(TInt i=0; i<idArray->Count(); i++){
    PyObject* entryId = Py_BuildValue("i", (*idArray)[i]);
    if (entryId == NULL){
      delete idArray;
      PyObject_Del(idArrayTuple);
      return NULL;
    }
    if(PyTuple_SetItem(idArrayTuple, i, entryId)<0){
      Py_DECREF(idArrayTuple);
      delete idArray;
      return NULL;
    }
  }
  delete idArray;
  return (PyObject*)idArrayTuple;
}


/*
 * Imports VCards (vcards are given in unicode string).
 * Returns tuple that containts unique id:s of imported
 * contact entries.
 */
extern "C" PyObject *
ContactsDb_import_vcards(ContactsDb_object* self, PyObject* args)
{
  ASSERT_DBOPEN

  char* vCardStr = NULL;
  TInt vCardStrLength = 0;
  TInt flags = CContactDatabase::EIncludeX 
              |CContactDatabase::ETTFormat;
  PyObject* idTuple = NULL;
  CArrayPtr<CContactItem>* imported = NULL;
   
  if (!PyArg_ParseTuple(args, "s#|i",
                        &vCardStr, &vCardStrLength, &flags)){ 

    return NULL;
  }

  TPtrC8 vCardStrPtr((TUint8*)vCardStr, vCardStrLength); 
  
  TUid uid;
  uid.iUid = KVersitEntityUidVCard;
  TBool success = EFalse;
 
  TRAPD(error, {
    RMemReadStream inputStream(vCardStrPtr.Ptr(), vCardStrPtr.Length());  
    CleanupClosePushL(inputStream);  
    imported = 
      self->contactEngine->Database().ImportContactsL(uid, inputStream, 
                                                      success, flags); 
    CleanupStack::PopAndDestroy(); // Close inputStream.
  });

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  idTuple = PyTuple_New(imported->Count());

  if(idTuple==NULL){
    imported->ResetAndDestroy(); 
    delete imported;
    return PyErr_NoMemory();
  }

  for(TInt i=0;i<imported->Count();i++){
    PyObject* idObj = Py_BuildValue("i", ((*imported)[i])->Id());
    if(idObj==NULL){
      Py_DECREF(idTuple);
      imported->ResetAndDestroy(); 
      delete imported;
      return NULL;
    }
    if(PyTuple_SetItem(idTuple, i, idObj)<0){
      Py_DECREF(idTuple);
      imported->ResetAndDestroy(); 
      delete imported;
      return NULL;
    }
  }

  imported->ResetAndDestroy(); 
  delete imported;

  return idTuple;
}


/*
 * Writes specified VCards into unicode string.
 */
extern "C" PyObject *
ContactsDb_export_vcards(ContactsDb_object* self, PyObject* args)
{
  ASSERT_DBOPEN

  TInt error = KErrNone;
  PyObject* idTuple;
  CContactIdArray* idArray = NULL;
  TInt flags = CContactDatabase::EIncludeX
              |CContactDatabase::ETTFormat;
  PyObject* ret = NULL;
  
  if (!PyArg_ParseTuple(args, "O!|i", 
                        &PyTuple_Type, &idTuple, &flags)){ 
    return NULL;
  }

  if(PyTuple_Size(idTuple)<1){
    PyErr_SetString(PyExc_SyntaxError, "no contact id:s given in the tuple");
    return NULL;
  }

  TRAP(error, { 
    idArray = CContactIdArray::NewL();
  });

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  

  // Put the unique contact id:s into the idArray.
  TInt idCount = PyTuple_Size(idTuple);
  for(TInt i=0;i<idCount;i++){
    PyObject* idItem = PyTuple_GetItem(idTuple, i);

    if(!PyInt_Check(idItem)){
      delete idArray;
      PyErr_SetString(PyExc_TypeError, "illegal argument in the tuple");
      return NULL;
    };

    TInt uniqueId = PyInt_AsLong(idItem);
    TRAPD(error, {
      idArray->AddL(uniqueId);
    });

    if(error != KErrNone){
      delete idArray;
      return SPyErr_SetFromSymbianOSErr(error);
    }
  }

  // Do the export.
  TRAP(error, {
    CleanupStack::PushL(idArray);

    CBufFlat* flatBuf = CBufFlat::NewL(4);
    CleanupStack::PushL(flatBuf);
    RBufWriteStream outputStream(*flatBuf);
    CleanupClosePushL(outputStream);

    TUid uid;
    uid.iUid = KVersitEntityUidVCard;
   
    // Export contacts into the stream.
    self->contactEngine->Database().ExportSelectedContactsL(uid,
                                                            *idArray, 
                                                            outputStream,
                                                            flags);  
    outputStream.CommitL();        
    CleanupStack::PopAndDestroy(); // Close outputStream.  
    
    ret = Py_BuildValue("s#", (char*)flatBuf->Ptr(0).Ptr(), flatBuf->Size());
    
    CleanupStack::PopAndDestroy(); // flatBuf.
    CleanupStack::PopAndDestroy(); // idArray.
  });

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }    
  return ret;
}


/*
 * Returns contact object (indicated by unique id given)
 */ 
static PyObject *
ContactsDb_getitem(ContactsDb_object *self, PyObject *key)
{
  ASSERT_DBOPEN

  if(!PyInt_Check(key)){
      PyErr_SetString(PyExc_TypeError, "illegal argument");
      return NULL;
  };
  return ContactsDb_get_contact_by_id(self, PyInt_AsLong(key));
}


/*
 * Deletes specified contact.
 */
static int
ContactsDb_ass_sub(ContactsDb_object *self, PyObject *key, PyObject *value)
{
  ASSERT_DBOPEN_RET_INT

  PyObject* result = NULL;
 
  if(value!=NULL){
    PyErr_SetString(PyExc_NotImplementedError, "illegal usage");
    return -1; 
  }

  if(!PyInt_Check(key)){
      PyErr_SetString(PyExc_TypeError, "illegal argument");
      return -1;
  }

  result = ContactsDb_delete_contact(self, PyInt_AsLong(key));

  if(!result){
    return -1;
  }else{
    Py_DECREF(result);
  }
  return 0;  
} 


/*
 * Returns number of contacts.
 */
static int
ContactsDb_len(ContactsDb_object *self)
{
  ASSERT_DBOPEN_RET_INT

  TInt length = -1;

  TRAPD(error, {
    length = self->contactEngine->Database().CountL();
  });

  if(error != KErrNone){
    SPyErr_SetFromSymbianOSErr(error);
  }

  return length;
}


/*
 * Tests whether a compress is recommended.
 */
extern "C" PyObject *
ContactsDb_compact_recommended(ContactsDb_object* self, PyObject* args)
{
  return Py_BuildValue("i", self->contactEngine->Database().CompressRequired());
}


/*
 * Compresses the database.
 */
extern "C" PyObject *
ContactsDb_compact(ContactsDb_object* self, PyObject* args)
{
  ASSERT_DBOPEN

  TInt error = KErrNone;

  Py_BEGIN_ALLOW_THREADS
  TRAP(error, {
    self->contactEngine->Database().CompactL();
  });
  Py_END_ALLOW_THREADS

  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Contact methods.
 */


/*
 * Create new Contact object.
 * if (uniqueId == KNullContactId) completely new contact entry is created.
 * Else only new (python) contact object is created (wrapper object to 
 * existing contact entry that the uniqueId identifies).
 */
extern "C" PyObject *
new_Contact_object(ContactsDb_object* self, TContactItemId uniqueId)
{
  ASSERT_DBOPEN

  Contact_object* contact = PyObject_New(Contact_object, Contact_type);
  if (contact == NULL){
    return PyErr_NoMemory();
  }

  // Initialize the contact struct.
  contact->mode = CONTACT_NOT_OPEN;
  contact->contactItem = NULL;
  contact->contactsDb = NULL;
  contact->uniqueID = KNullContactId;
  
  if(uniqueId == KNullContactId){        
    // a new contact entry must be created into the database.
    TRAPD(error, {
      CPbkContactItem* newContact = 
        self->contactEngine->CreateEmptyContactL(); 
      contact->contactItem = newContact;
      contact->mode = CONTACT_READ_WRITE;
    });

    if(error != KErrNone){  
      PyObject_Del(contact);
      return SPyErr_SetFromSymbianOSErr(error);
    } 
  }else{
    // contact entry that corresponds to given unique id exists.
    TRAPD(error, {
      CPbkContactItem* contactItem = 
        self->contactEngine->ReadContactL(uniqueId); 
      contact->contactItem = contactItem;
      contact->mode = CONTACT_READ_ONLY;
    });
    if(error != KErrNone){  
      PyObject_Del(contact);
      return SPyErr_SetFromSymbianOSErr(error);
    } 
  }

  contact->contactsDb = self;
  contact->uniqueID = uniqueId;
  Py_INCREF(contact->contactsDb);
  return (PyObject*)contact;
}


/*
 * Deallocate Contact_object.
 */
extern "C" {
  static void Contact_dealloc(Contact_object *contact)
  {
    Contact_delete_entry(contact);
    Py_DECREF(contact->contactsDb); 
    PyObject_Del(contact);    
  }
}


/*
 * Sets value and/or label to given field.
 */
extern "C" PyObject *
Contact_modify_field_value_or_labelL(TPbkContactItemField & theField,
                                     char* value,
                                     TInt valueLength,
                                     PyObject* label,
                                     TReal timeValue)
{    
  if(value && theField.StorageType() == KStorageTypeText){
    // Text type.
    TPtrC valuePtr((TUint16*) value, valueLength);  
    theField.TextStorage()->SetTextL(valuePtr);
  }else if((timeValue>0) && theField.StorageType() == KStorageTypeDateTime){
    // DateTime type.
    TTime time;
    pythonRealAsTTime(timeValue, time);     
    theField.DateTimeStorage()->SetTime(time.DateTime());
  }else if(value || (timeValue>0)){
    PyErr_SetString(PyExc_ValueError, "illegal field value and type combination");
    return NULL; 
  }
  
  if(label){
    TPtrC labelPtr((TUint16*) PyUnicode_AsUnicode(label), 
                              PyUnicode_GetSize(label)); 
    theField.SetLabelL(labelPtr);
  }
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Modifies fields value and/or label.
 */
extern "C" PyObject *
Contact_modify_field(Contact_object* self, PyObject *args, PyObject *keywds)
{
  ASSERT_CONTACT_READ_WRITE

  TInt fieldIndex;
  PyObject* valueObj = NULL;
  char* value = NULL; 
  TInt valueLength = 0;
  PyObject* label = NULL;
  PyObject* ret = NULL;
  TReal dateVal = 0;
 
  static const char *const kwlist[] = 
    {"fieldindex", "value", "label", NULL};
 
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|OU", (char**)kwlist,
                                   &fieldIndex, &valueObj,
                                   &label)){ 
    return NULL;
  }

  if(!valueObj && !label){
    PyErr_SetString(PyExc_SyntaxError, "value and/or label must be given");
    return NULL;
  }

  if(valueObj){
    if(PyFloat_Check(valueObj)){
      // DateTime type.
      dateVal = PyFloat_AsDouble(valueObj);
    } else if(PyUnicode_Check(valueObj)){
      // Text type.
      value = (char*)PyUnicode_AsUnicode(valueObj);
      valueLength = PyUnicode_GetSize(valueObj);
    } else {
      PyErr_SetString(PyExc_TypeError, "illegal value parameter");
      return NULL;
    }
  }

  CPbkFieldArray& fieldArray = self->contactItem->CardFields();

  if (fieldIndex < 0 || fieldIndex >= fieldArray.Count()){
    PyErr_SetString(PyExc_IndexError, "illegal field index");
    return NULL; 
  }  

  TPbkContactItemField& theField = fieldArray[fieldIndex];

  TRAPD(error, {    
    ret = Contact_modify_field_value_or_labelL(theField,
                                               value,
                                               valueLength,
                                               label,
                                               dateVal);   
  });
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return ret;
}


/*
 * Adds field to the contact.
 */
extern "C" PyObject *
Contact_add_field(Contact_object* self, PyObject *args, PyObject *keywds)
{
  ASSERT_CONTACT_READ_WRITE
  
  PyObject* fieldTypeObj = NULL;
  PyObject* valueObj = NULL;
  char* value = NULL; 
  TInt valueLength = 0;
  PyObject* label = NULL;
  PyObject* ret = NULL;
  CPbkFieldInfo* fieldInfo = NULL;
  TReal dateVal = 0;
  TInt error = KErrNone;
  TPbkContactItemField* theField = NULL;
 
  static const char *const kwlist[] = 
    {"fieldtype", "value", "label", NULL};
 
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OU", (char**)kwlist,
                                   &fieldTypeObj, 
                                   &valueObj, 
                                   &label)){ 
    return NULL;
  }

  if(valueObj){
    if(PyFloat_Check(valueObj)){
      // DateTime type.
      dateVal = PyFloat_AsDouble(valueObj);
    } else if(PyUnicode_Check(valueObj)){
      // Text type.
      value = (char*)PyUnicode_AsUnicode(valueObj);
      valueLength = PyUnicode_GetSize(valueObj);
    } else {
      PyErr_SetString(PyExc_TypeError, "illegal value parameter");
      return NULL;
    }
  }

  if(PyInt_Check(fieldTypeObj)){
    // Only field type id given.
    const CPbkFieldsInfo& fieldsInfo = 
      self->contactsDb->contactEngine->FieldsInfo();

    fieldInfo = fieldsInfo.Find(PyInt_AsLong(fieldTypeObj));
    if (!fieldInfo){
      PyErr_SetString(PyExc_ValueError, "unsupported field type");
      return NULL; 
    }
  }else if(PyTuple_Check(fieldTypeObj)){
    if(PyTuple_Size(fieldTypeObj)!=2){
      PyErr_SetString(PyExc_ValueError, "tuple must contain field and location id:s");
      return NULL; 
    }
    // field type id and field location given.
    PyObject* fieldIdObj = PyTuple_GetItem(fieldTypeObj, 0);
    PyObject* locationIdObj = PyTuple_GetItem(fieldTypeObj, 1);

    if(!(PyInt_Check(fieldIdObj) && PyInt_Check(locationIdObj))){
      PyErr_SetString(PyExc_TypeError, "illegal parameter in the tuple");
      return NULL; 
    }

    const CPbkFieldsInfo& fieldsInfo = 
      self->contactsDb->contactEngine->FieldsInfo();

    fieldInfo = 
      fieldsInfo.Find((TInt)PyInt_AsLong(fieldIdObj), 
                      static_cast<TPbkFieldLocation>(PyInt_AsLong(locationIdObj)));
    if (!fieldInfo){
      PyErr_SetString(PyExc_ValueError, "unsupported field or location type");
      return NULL; 
    }    
  }else{
    PyErr_SetString(PyExc_TypeError, "illegal fieldtype parameter");
    return NULL; 
  }
  

  TRAP(error, theField = &(self->contactItem->AddFieldL(*fieldInfo)));

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  TRAP(error, 
    ret = Contact_modify_field_value_or_labelL(*theField, value, valueLength,
                                               label, dateVal));

  if(error != KErrNone){
    self->contactItem->RemoveField(self->contactItem->FindFieldIndex(*theField));
    return SPyErr_SetFromSymbianOSErr(error);
  }

  if(!ret){
    self->contactItem->RemoveField(self->contactItem->FindFieldIndex(*theField));
    return NULL;
  }

  Py_DECREF(ret);
  return Py_BuildValue("i", self->contactItem->FindFieldIndex(*theField)); 
}


/*
 * Removes specified field of the contact.
 */
extern "C" PyObject *
Contact_remove_field(Contact_object* self, TInt fieldIndex)
{  
  ASSERT_CONTACT_READ_WRITE

  CPbkFieldArray& fieldArray = self->contactItem->CardFields();
  if (fieldIndex < 0 || fieldIndex >= fieldArray.Count()){
    PyErr_SetString(PyExc_IndexError, "illegal field index");
    return NULL; 
  }

  self->contactItem->RemoveField(fieldIndex);

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns contact data (attributes of contact, not it's field data).
 */
extern "C" PyObject *
Contact_entry_data(Contact_object* self, PyObject* /*args*/)
{
  ASSERT_CONTACTOPEN

  HBufC* titleBuf = NULL;
  TInt error = KErrNone;

  TRAP(error, titleBuf = self->contactItem->GetContactTitleL());

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);  
  }

  PyObject* entryData = 
    Py_BuildValue("{s:i,s:u#,s:d}", 
                   (const char*)(&KKeyStrUniqueId)->Ptr(), self->uniqueID,
                   (const char*)(&KKeyStrTitle)->Ptr(), 
                     titleBuf->Ptr(), titleBuf->Length(),
                   (const char*)(&KKeyStrLastModified)->Ptr(), 
                     time_as_UTC_TReal(self->contactItem->ContactItem().LastModified()));
  delete titleBuf;

  return  entryData;
}


/*
 * Sets the contact to the read-only mode.
 */
extern "C" PyObject *
Contact_open_ro(Contact_object* self)
{
  ASSERT_DBOPEN_CONTACT

  if(self->mode == CONTACT_READ_ONLY){
    // Already in read-only mode.
    Py_INCREF(Py_None);
    return Py_None;
  }

  TRAPD(error, {
    CPbkContactItem* contactItem = 
      self->contactsDb->contactEngine->ReadContactL(self->uniqueID);
    self->contactItem = contactItem;
    self->mode = CONTACT_READ_ONLY;
  });  

  RETURN_ERROR_OR_PYNONE(error);
}


/*
 * Sets the contact to the read-write mode.
 */
extern "C" PyObject *
Contact_open_rw(Contact_object* self)
{
  ASSERT_DBOPEN_CONTACT

  if(self->mode == CONTACT_READ_WRITE){
    // Already in read-write mode.
    Py_INCREF(Py_None);
    return Py_None;
  }

  TRAPD(error, {
    self->contactItem = 
      self->contactsDb->contactEngine->OpenContactL(self->uniqueID);
    self->mode = CONTACT_READ_WRITE;
  });

  RETURN_ERROR_OR_PYNONE(error);
}


/*
 * Deletes the CPbkContactItem inside the contact object.
 */
void Contact_delete_entry(Contact_object* self)
{
  delete self->contactItem;
  self->contactItem = NULL;
  self->mode = CONTACT_NOT_OPEN;
}


/*
 * Opens contact for writing.
 */
extern "C" PyObject *
Contact_begin(Contact_object* self, PyObject* /*args*/)
{
  ASSERT_DBOPEN_CONTACT

  if(self->mode != CONTACT_READ_WRITE){
    Contact_delete_entry(self);
    return Contact_open_rw(self);
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Commits the contact.
 */
extern "C" PyObject *
Contact_commit(Contact_object* self, PyObject* /*args*/)
{
  ASSERT_CONTACT_READ_WRITE

  TInt error = KErrNone;
  
  if(self->uniqueID == KNullContactId){
    // New contact.
    TRAP(error, 
      self->uniqueID = 
        self->contactsDb->contactEngine->AddNewContactL(*self->contactItem));
  } else {
    // Old contact (begin() called).
    TRAP(error, 
      self->contactsDb->contactEngine->CommitContactL(*self->contactItem));
  }  
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Contact_delete_entry(self);

  return Contact_open_ro(self);
}


/*
 * Closes (but does not commit) the contact.
 */
extern "C" PyObject *
Contact_rollback(Contact_object* self, PyObject* /*args*/)
{
  ASSERT_DBOPEN_CONTACT

  if(self->mode != CONTACT_READ_WRITE || self->uniqueID == KNullContactId){
    PyErr_SetString(PyExc_RuntimeError, "begin not called");
    return NULL; 
  }

  TInt error = KErrNone;

  TRAP(error, 
    self->contactsDb->contactEngine->CloseContactL(self->uniqueID));
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Contact_delete_entry(self);

  return Contact_open_ro(self);
}


/*
 * Gets field type (e.g. fieldinfo) index in the tuple of supported
 * field types that corresponds to type of the field indicated
 * by given field index (field index in the contact entry's field array).
 */
extern "C" PyObject *
Contact_field_info_index(Contact_object* self, PyObject *args)
{
  ASSERT_CONTACTOPEN

  TInt fieldIndex;

  if (!PyArg_ParseTuple(args, "i", &fieldIndex)){ 
    return NULL;
  }

  CPbkFieldArray& fieldArray = self->contactItem->CardFields();
  if (fieldIndex < 0 || fieldIndex >= fieldArray.Count()){
    PyErr_SetString(PyExc_IndexError, "illegal field index");
    return NULL; 
  }

  TPbkContactItemField& field = fieldArray[fieldIndex];
  const CPbkFieldsInfo& fieldsInfo = 
    self->contactsDb->contactEngine->FieldsInfo();
  for(TInt i = 0; i< fieldsInfo.Count(); i++){
    if (fieldsInfo[i]->IsSame(field.FieldInfo())){      
      return Py_BuildValue("i", i); // Match found
    }
  }
  
  return Py_BuildValue("i", -1);  
}


/*
 * Returns field data of the field indicated by the fieldIndex.
 */
extern "C" PyObject *
Contact_build_field_data_object(Contact_object* self, TInt fieldIndex)
{
  ASSERT_CONTACTOPEN
  
  CPbkFieldArray& fieldArray = self->contactItem->CardFields();

  if (fieldIndex < 0 || fieldIndex >= fieldArray.Count()){
    PyErr_SetString(PyExc_IndexError, "illegal field index");
    return NULL; 
  }  

  TPbkContactItemField& field = fieldArray[fieldIndex];
  
  PyObject* fieldValue = NULL;

  if(KStorageTypeText == field.FieldInfo().FieldStorageType()){
    fieldValue = 
      Py_BuildValue("u#", field.Text().Ptr(), field.Text().Length()); 
  }else if(KStorageTypeDateTime == field.FieldInfo().FieldStorageType()){
    fieldValue = ttimeAsPythonFloat(field.Time());
  }else{
    fieldValue = Py_BuildValue("u", ""); // binary support ???
  }

  if(fieldValue == NULL){      
    return NULL;
  }

  PyObject* ret =  
    Py_BuildValue("{s:u#,s:u#,s:O,s:i,s:i,s:i,s:i}",
                  (const char*)(&KKeyStrFieldName)->Ptr(), 
                    field.FieldInfo().FieldName().Ptr(), 
                    field.FieldInfo().FieldName().Length(),
                  "label",field.Label().Ptr(), field.Label().Length(),
                  "value", fieldValue,
                  "fieldindex", fieldIndex,
                  (const char*)(&KKeyStrFieldId)->Ptr(), 
                    field.FieldInfo().FieldId(),
                  "storagetype", field.FieldInfo().FieldStorageType(),
                  "maxlength", field.FieldInfo().MaxLength());

  Py_DECREF(fieldValue);

  return ret;
}


/*
 * Returns indexes of the fields that represent given fieldtype 
 * and location (optional) values.
 */
extern "C" PyObject *
Contact_find_field_indexes(Contact_object* self, PyObject* args)
{
  ASSERT_CONTACTOPEN

  CArrayVarFlat<TInt>* indexArray = NULL;
  TInt fieldIndex=0;
  TInt fieldId = -1;
  TInt location = -1;
  TPbkContactItemField* field = NULL;
     
  if (!PyArg_ParseTuple(args, "i|i", &fieldId, &location)){ 
    return NULL;
  }

  TRAPD(error, {
    indexArray = new (ELeave) CArrayVarFlat<TInt>(2); // 2 is the granularity (not the size..)
    do{
      field = self->contactItem->FindField(fieldId, fieldIndex);
      if(field && (field->FieldInfo().Location()==location || location==-1)){
        indexArray->AppendL(fieldIndex, sizeof(fieldIndex));  
      }
      fieldIndex++;
    }while(field);
  });
  if(error!=KErrNone){
    delete indexArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  PyObject* indexList = PyList_New(indexArray->Count());
  if(indexList==NULL){
    delete indexArray;
    return PyErr_NoMemory();
  }
  for(TInt i=0;i<indexArray->Count();i++){
    PyObject* theIndex = Py_BuildValue("i", indexArray->At(i));
    if(theIndex==NULL){
      delete indexArray;
      Py_DECREF(indexList);
      return NULL;
    }
    if(0>PyList_SetItem(indexList,i,theIndex)){
      delete indexArray;
      Py_DECREF(indexList);
      return NULL;
    }
  }

  delete indexArray;
  return indexList;
}  


/*
 * Returns number of contact's fields.
 */
static int
Contact_len(Contact_object *self)
{
  ASSERT_CONTACTOPEN_RET_INT

  return self->contactItem->CardFields().Count();
}


/*
 * Returns contact object's field data (indicated by field index)
 */ 
static PyObject *
Contact_getitem(Contact_object *self, PyObject *key)
{
  ASSERT_CONTACTOPEN

  if(!PyInt_Check(key)){
      PyErr_SetString(PyExc_TypeError, "illegal argument");
      return NULL;
  };
  return Contact_build_field_data_object(self, PyInt_AsLong(key));
}
 

/*
 * Deletes specified field.
 */
static int
Contact_ass_sub(Contact_object *self, PyObject *key, PyObject *value)
{
  
  ASSERT_CONTACT_READ_WRITE_RET_INT

  PyObject* result = NULL;

  if(value!=NULL){
    PyErr_SetString(PyExc_NotImplementedError, "cannot set field value this way");
    return -1; 
  }

  if(!PyInt_Check(key)){
      PyErr_SetString(PyExc_TypeError, "illegal argument");
      return -1;
  }

   result = Contact_remove_field(self, PyInt_AsLong(key));

   if(!result){
     return -1;
   }else{
     Py_DECREF(result);
   }  

   return 0;  
} 


/*
 * String representation of Contact object.
 */
static PyObject *
Contact_repr_str(Contact_object* self)
{
  ASSERT_CONTACTOPEN

  HBufC* titleBuf = NULL;
  TInt error = KErrNone;

  TRAP(error, titleBuf = self->contactItem->GetContactTitleL());

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);  
  }

  PyObject* ret = Py_BuildValue("u#", titleBuf->Ptr(), titleBuf->Length()); 

  delete titleBuf;
 
  return ret;
}


/*
 * Contact iterator methods.
 */



/*
 * Creates CPbkContactIter object.
 */
CPbkContactIter*
ContactIterator_create_CPbkContactIterL(ContactsDb_object* self)
{
  CPbkContactIter* iter = NULL;
  iter = self->contactEngine->CreateContactIteratorLC();
  CleanupStack::Pop(iter);
  return iter;
}


/*
 * Create new ContactIterator object.
 */
extern "C" PyObject *
new_ContactIterator_object(ContactsDb_object* self, PyObject /**args*/)
{
  ASSERT_DBOPEN

  ContactIterator_object* ci = 
    PyObject_New(ContactIterator_object, ContactIterator_type);

  if (ci == NULL){
    return PyErr_NoMemory();
  }

  // Initialize the struct.
  ci->iterator = NULL;
  ci->initialized = false;
  ci->contactsDb = NULL;
   
  TRAPD(error, {
    ci->iterator = ContactIterator_create_CPbkContactIterL(self);
  });

  if(error != KErrNone){
    PyObject_Del(ci);   
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  ci->contactsDb = self;
  Py_INCREF(ci->contactsDb);
  return (PyObject*)ci;
}


/*
 * Get the uniqueId of the next contact entry object.
 */
extern "C" PyObject *
ContactIterator_next(ContactIterator_object* self, PyObject /**args*/)
{
  ASSERT_DBOPEN_FI

  TContactItemId uniqueId = KNullContactId;
  TInt error = KErrNone;

  if ( self->initialized ){
    TRAP(error, uniqueId = self->iterator->NextL());   
  }else{
    TRAP(error, uniqueId = self->iterator->FirstL());
    self->initialized = true;
  }

  if(error != KErrNone){   
    return SPyErr_SetFromSymbianOSErr(error);
  }

  // Check if the iteration ended.
  if(uniqueId == KNullContactId){
    PyErr_SetObject(PyExc_StopIteration, Py_None);
    return NULL;
  }
  return Py_BuildValue("i", uniqueId);
}


/*
 * Deallocate ContactIterator_object.
 */
extern "C" {
  static void ContactIterator_dealloc(ContactIterator_object *contactIterator)
  {  
    delete contactIterator->iterator;
    contactIterator->iterator = NULL;
  
    Py_DECREF(contactIterator->contactsDb);
    PyObject_Del(contactIterator);
  }
}


/*
 * Field iterator methods.
 */


/*
 * Create new FieldIterator object.
 */
extern "C" PyObject *
new_FieldIterator_object(Contact_object* self, PyObject /**args*/)
{ 
  ASSERT_CONTACTOPEN

  FieldIterator_object* fi = 
    PyObject_New(FieldIterator_object, FieldIterator_type);
  if (fi == NULL){
    return PyErr_NoMemory();
  }

  fi->contact = self;
  fi->initialized = false;
  fi->iterationIndex = 0;
  Py_INCREF(fi->contact);
  return (PyObject*)fi; 
}


/*
 * Deallocate FieldIterator_object.
 */
extern "C" {
  static void FieldIterator_dealloc(FieldIterator_object *fieldIterator)
  {
    Py_DECREF(fieldIterator->contact);
    fieldIterator->contact = NULL;
    PyObject_Del(fieldIterator);  
  }
}


/*
 * Returns data of the next field of the contact.
 */
extern "C" PyObject *
FieldIterator_next_field(FieldIterator_object* self, PyObject /**args*/)
{  
  ASSERT_CONTACTOPEN_FI

  if (!self->initialized) {
    self->iterationIndex = 0;
    self->initialized = true;   
  }

  CPbkFieldArray& fieldArray = self->contact->contactItem->CardFields();

  if(self->iterationIndex < fieldArray.Count()){
    PyObject* ret = 
      Contact_build_field_data_object(self->contact, self->iterationIndex);
    self->iterationIndex++;
    return ret;
  }

  PyErr_SetObject(PyExc_StopIteration, Py_None);
  return NULL;
}



//////////////TYPE SET//////////////


extern "C" {


/*
 * Just return self. Used as a helper function in the iterator
 * protocol.
 */

  PyObject *
  return_self(PyObject* self, PyObject /* *args */)
  {
    Py_INCREF(self);
    return self;
  }

  const static PyMethodDef ContactsDb_methods[] = {
    {"field_types", (PyCFunction)ContactsDb_field_types, METH_NOARGS},
    {"field_info", (PyCFunction)ContactsDb_field_info, METH_VARARGS},
    {"add_contact", (PyCFunction)ContactsDb_add_contact, METH_NOARGS},
    {"find", (PyCFunction)ContactsDb_find, METH_VARARGS},
    {"export_vcards", (PyCFunction)ContactsDb_export_vcards, METH_VARARGS},
    {"import_vcards", (PyCFunction)ContactsDb_import_vcards, METH_VARARGS},
    {"compact", (PyCFunction)ContactsDb_compact, METH_NOARGS},
    {"compact_recommended", (PyCFunction)ContactsDb_compact_recommended, METH_NOARGS},
    {NULL, NULL}  
  };


  const static PyMethodDef ContactIterator_methods[] = {
    {"next", (PyCFunction)ContactIterator_next, METH_NOARGS},
    {NULL, NULL}  
  };


  const static PyMethodDef Contact_methods[] = {
    {"begin", (PyCFunction)Contact_begin, METH_NOARGS, NULL}, 
    {"commit", (PyCFunction)Contact_commit, METH_NOARGS, NULL}, 
    {"rollback", (PyCFunction)Contact_rollback, METH_NOARGS, NULL}, 
    {"add_field", (PyCFunction)Contact_add_field, METH_VARARGS | METH_KEYWORDS, NULL},
    {"modify_field", (PyCFunction)Contact_modify_field, METH_VARARGS | METH_KEYWORDS, NULL}, 
    {"field_info_index", (PyCFunction)Contact_field_info_index, METH_VARARGS, NULL}, 
    {"entry_data", (PyCFunction)Contact_entry_data, METH_NOARGS, NULL},
    {"find_field_indexes", (PyCFunction)Contact_find_field_indexes, METH_VARARGS, NULL},
    {NULL, NULL, 0 , NULL}  
  };


  const static PyMethodDef FieldIterator_methods[] = {
    {"next", (PyCFunction)FieldIterator_next_field, METH_NOARGS},
    {NULL, NULL}  
  };


  static PyObject *
  ContactsDb_getattr(ContactsDb_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)ContactsDb_methods, (PyObject *)op, name);
  }


  static PyObject *
  ContactIterator_getattr(ContactIterator_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)ContactIterator_methods, (PyObject *)op, name);
  }


  static PyObject *
  Contact_getattr(Contact_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)Contact_methods, (PyObject *)op, name);
  }


  static PyObject *
  FieldIterator_getattr(FieldIterator_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)FieldIterator_methods, (PyObject *)op, name);
  }


  #ifdef __WINS__
  static PyMappingMethods contactsDb_as_mapping = {
  #else
  static const PyMappingMethods contactsDb_as_mapping = {
  #endif  
    (inquiry)ContactsDb_len,                  /*mp_length*/
    (binaryfunc)ContactsDb_getitem,           /*mp_subscript*/
    (objobjargproc)ContactsDb_ass_sub,        /*mp_ass_subscript*/
  };


  #ifndef SYMBIAN
  PyTypeObject c_ContactsDb_type = {
    PyObject_HEAD_INIT(&PyType_Type)
  #else
  const PyTypeObject c_ContactsDb_type = {
    PyObject_HEAD_INIT(NULL)
  #endif
    0,                                        /*ob_size*/
    "_contacts.ContactsDb",                   /*tp_name*/
    sizeof(ContactsDb_object),                /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)ContactsDb_dealloc,           /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)ContactsDb_getattr,          /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    &contactsDb_as_mapping,                   /*tp_as_mapping*/
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
    (getiterfunc)new_ContactIterator_object,  /*tp_iter */
  };


  static const PyTypeObject c_ContactIterator_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_contacts.ContactIterator",               /*tp_name*/
    sizeof(ContactIterator_object),           /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)ContactIterator_dealloc,      /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)ContactIterator_getattr,     /*tp_getattr*/
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
    (getiterfunc)return_self,                 /*tp_iter */
    (iternextfunc)ContactIterator_next,       /*tp_iternext */
  };


  #ifdef __WINS__
  static PyMappingMethods contact_as_mapping = {
  #else
  static const PyMappingMethods contact_as_mapping = {
  #endif
    (inquiry)Contact_len,                     /*mp_length*/
    (binaryfunc)Contact_getitem,              /*mp_subscript*/
    (objobjargproc)Contact_ass_sub,           /*mp_ass_subscript*/
  };


  #ifndef SYMBIAN
  PyTypeObject c_Contact_type = {
    PyObject_HEAD_INIT(&PyType_Type)
  #else
  const PyTypeObject c_Contact_type = {
    PyObject_HEAD_INIT(NULL)
  #endif
    0,                                        /*ob_size*/
    "_contacts.Contact",                       /*tp_name*/
    sizeof(Contact_object),                   /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Contact_dealloc,              /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)Contact_getattr,             /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    &contact_as_mapping,                      /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    (reprfunc)Contact_repr_str,               /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    (getiterfunc)new_FieldIterator_object,    /*tp_iter */
  };


  static const PyTypeObject c_FieldIterator_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_contacts.FieldIterator",                 /*tp_name*/
    sizeof(FieldIterator_object),             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)FieldIterator_dealloc,        /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)FieldIterator_getattr,       /*tp_getattr*/
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
    (getiterfunc)return_self,                 /*tp_iter */
    (iternextfunc)FieldIterator_next_field,   /*tp_iternext */
  };


} //extern C


//////////////INIT//////////////


extern "C" {
  static const PyMethodDef contacts_methods[] = {
    {"open", (PyCFunction)open_db, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initcontacts(void)
  {
    PyTypeObject* contacts_db_type = PyObject_New(PyTypeObject, &PyType_Type);
    *contacts_db_type = c_ContactsDb_type;
    contacts_db_type->ob_type = &PyType_Type;
    SPyAddGlobalString("ContactsDbType", (PyObject*)contacts_db_type);

    PyTypeObject* contact_iterator_type = PyObject_New(PyTypeObject, &PyType_Type);
    *contact_iterator_type = c_ContactIterator_type;
    contact_iterator_type->ob_type = &PyType_Type;
    SPyAddGlobalString("ContactIteratorType", (PyObject*)contact_iterator_type);

    PyTypeObject* contact_type = PyObject_New(PyTypeObject, &PyType_Type);
    *contact_type = c_Contact_type;
    contact_type->ob_type = &PyType_Type;
    SPyAddGlobalString("ContactType", (PyObject*)contact_type);

    PyTypeObject* field_iterator_type = PyObject_New(PyTypeObject, &PyType_Type);
    *field_iterator_type = c_FieldIterator_type;
    field_iterator_type->ob_type = &PyType_Type;
    SPyAddGlobalString("FieldIteratorType", (PyObject*)field_iterator_type);

    PyObject *m, *d;

    m = Py_InitModule("_contacts", (PyMethodDef*)contacts_methods);
    d = PyModule_GetDict(m);
    
    d = PyModule_GetDict(m);

    // Field id:s.
    PyDict_SetItemString(d,"none", PyInt_FromLong(EPbkFieldIdNone));
    PyDict_SetItemString(d,"last_name", PyInt_FromLong(EPbkFieldIdLastName));
    PyDict_SetItemString(d,"first_name", PyInt_FromLong(EPbkFieldIdFirstName));
    PyDict_SetItemString(d,"phone_number_general", PyInt_FromLong(EPbkFieldIdPhoneNumberGeneral));
    PyDict_SetItemString(d,"phone_number_standard", PyInt_FromLong(EPbkFieldIdPhoneNumberStandard));
    PyDict_SetItemString(d,"phone_number_home", PyInt_FromLong(EPbkFieldIdPhoneNumberHome));
    PyDict_SetItemString(d,"phone_number_work", PyInt_FromLong(EPbkFieldIdPhoneNumberWork));
    PyDict_SetItemString(d,"phone_number_mobile", PyInt_FromLong(EPbkFieldIdPhoneNumberMobile));
    PyDict_SetItemString(d,"fax_number", PyInt_FromLong(EPbkFieldIdFaxNumber));
    PyDict_SetItemString(d,"pager_number", PyInt_FromLong(EPbkFieldIdPagerNumber));
    PyDict_SetItemString(d,"email_address", PyInt_FromLong(EPbkFieldIdEmailAddress));
    PyDict_SetItemString(d,"postal_address", PyInt_FromLong(EPbkFieldIdPostalAddress));
    PyDict_SetItemString(d,"url", PyInt_FromLong(EPbkFieldIdURL));
    PyDict_SetItemString(d,"job_title", PyInt_FromLong(EPbkFieldIdJobTitle));
    PyDict_SetItemString(d,"company_name", PyInt_FromLong(EPbkFieldIdCompanyName));
    PyDict_SetItemString(d,"company_address", PyInt_FromLong(EPbkFieldIdCompanyAddress));
    PyDict_SetItemString(d,"dtmf_string", PyInt_FromLong(EPbkFieldIdDTMFString));
    PyDict_SetItemString(d,"date", PyInt_FromLong(EPbkFieldIdDate));
    PyDict_SetItemString(d,"note", PyInt_FromLong(EPbkFieldIdNote));
    PyDict_SetItemString(d,"po_box", PyInt_FromLong(EPbkFieldIdPOBox));
    PyDict_SetItemString(d,"extended_address", PyInt_FromLong(EPbkFieldIdExtendedAddress));
    PyDict_SetItemString(d,"street_address", PyInt_FromLong(EPbkFieldIdStreetAddress));
    PyDict_SetItemString(d,"postal_code", PyInt_FromLong(EPbkFieldIdPostalCode));
    PyDict_SetItemString(d,"city", PyInt_FromLong(EPbkFieldIdCity));
    PyDict_SetItemString(d,"state", PyInt_FromLong(EPbkFieldIdState));
    PyDict_SetItemString(d,"country", PyInt_FromLong(EPbkFieldIdCountry));
    PyDict_SetItemString(d,"wvid", PyInt_FromLong(EPbkFieldIdWVID));

    // Location id:s.
    PyDict_SetItemString(d,"location_none", PyInt_FromLong(EPbkFieldLocationNone));
    PyDict_SetItemString(d,"location_home", PyInt_FromLong(EPbkFieldLocationHome));
    PyDict_SetItemString(d,"location_work", PyInt_FromLong(EPbkFieldLocationWork));    
   
    // vcard options.
    PyDict_SetItemString(d,"vcard_include_x", PyInt_FromLong(CContactDatabase::EIncludeX));
    PyDict_SetItemString(d,"vcard_ett_format", PyInt_FromLong(CContactDatabase::ETTFormat));
    PyDict_SetItemString(d,"vcard_exclude_uid", PyInt_FromLong(CContactDatabase::EExcludeUid));
    PyDict_SetItemString(d,"vcard_dec_access_count", PyInt_FromLong(CContactDatabase::EDecreaseAccessCount));
    PyDict_SetItemString(d,"vcard_import_single_contact", PyInt_FromLong(CContactDatabase::EImportSingleContact));
    PyDict_SetItemString(d,"vcard_inc_access_count", PyInt_FromLong(CContactDatabase::EIncreaseAccessCount));

    // Storage types.
    PyDict_SetItemString(d,"storage_type_text", PyInt_FromLong(KStorageTypeText));
    PyDict_SetItemString(d,"storage_type_store", PyInt_FromLong(KStorageTypeStore));
    PyDict_SetItemString(d,"storage_type_contact_item_id", PyInt_FromLong(KStorageTypeContactItemId));
    PyDict_SetItemString(d,"storage_type_datetime", PyInt_FromLong(KStorageTypeDateTime));
   
    // Field type multiplicity.
    PyDict_SetItemString(d,"field_type_multiplicity_one", PyInt_FromLong(EPbkFieldMultiplicityOne));
    PyDict_SetItemString(d,"field_type_multiplicity_many", PyInt_FromLong(EPbkFieldMultiplicityMany));

    return;
  }
} /* extern "C" */


GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
