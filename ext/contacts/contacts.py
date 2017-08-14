#
# contacts.py
#
# Copyright (c) 2005 Nokia Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import _contacts
 
def revdict(d):
    return dict([(d[k],k) for k in d.keys()])
 
fieldtype_names=["none",
                 "last_name",
                 "first_name",
#                 "phone_number_general",
#                 "phone_number_home",
#                 "phone_number_work",
                 "phone_number_mobile",
                 "fax_number",
                 "pager_number",
                 "email_address",
                 "postal_address",
                 "url",
                 "job_title",
                 "company_name",
                 #"company_address",                 # same as postal_address
                 "dtmf_string",
                 "date",
                 "note",
                 "po_box",
                 "extended_address",
                 "street_address",
                 "postal_code",
                 "city",
                 "state",
                 "country",
                 "wvid"]

fieldtypemap=dict([(k,getattr(_contacts,k)) for k in fieldtype_names])
fieldtypemap.update({'video_number': 32,
                     'picture': 0x12,
                     'thumbnail_image': 0x13,
                     'voice_tag': 0x14,
                     'speed_dial': 0x15,
                     'personal_ringtone': 0x16,
                     'second_name': 0x1f,
                     'last_name_reading': 0x21,
                     'first_name_reading': 0x22,
                     'locationid_indication': 0x23})
fieldtypemap['mobile_number']=fieldtypemap['phone_number_mobile']
del fieldtypemap['phone_number_mobile']
fieldtypereversemap=revdict(fieldtypemap)

locationmap={None: -1,
             'none': _contacts.location_none,
             'home': _contacts.location_home,
             'work': _contacts.location_work}
locationreversemap=revdict(locationmap)

_phonenumber_location_map={
    'none': _contacts.phone_number_general,
    'home': _contacts.phone_number_home,
    'work': _contacts.phone_number_work
    }
_phonenumber_location_reversemap=revdict(_phonenumber_location_map)

storagetypemap={"text":_contacts.storage_type_text,
                "binary":_contacts.storage_type_store,
                "item_id":_contacts.storage_type_contact_item_id,
                "datetime":_contacts.storage_type_datetime}
_storagetypereversemap=revdict(storagetypemap)

   
def nativefieldtype_from_pythontype_and_location(type,location=None):
    if isinstance(type,int):
        return type
    if location is None:
        location='none'
    if type=='phone_number':
        return (_phonenumber_location_map[location],locationmap[location])
    return (fieldtypemap[type],locationmap[location])

_pythontype_from_nativefieldtype_map=dict(fieldtypereversemap)
_pythontype_from_nativefieldtype_map.update(
    {_contacts.phone_number_general: 'phone_number',
     _contacts.phone_number_home: 'phone_number',
     _contacts.phone_number_work: 'phone_number'})
     
class ContactBusy(RuntimeError):
    pass

class ContactsDb(object):
    def __init__(self,dbfile=None,mode=None):
        if dbfile is None:
            self._db=_contacts.open()
        else:
            if mode is None:
                self._db=_contacts.open(unicode(dbfile))
            else:
                self._db=_contacts.open(unicode(dbfile),mode)
                if mode=='n':
                    for id in self._db:
                        del self._db[id]
        self._field_types=[self._field_schema(k) for k in self._db.field_types()]
    def _field_schema(self,schemaid):
        schema=self._db.field_info(schemaid)
        del schema['fieldinfoindex']
        schema['storagetype']=_storagetypereversemap[schema['storagetype']]
        if fieldtypereversemap.has_key(schema['fieldid']):
            schema['type']=fieldtypereversemap[schema['fieldid']]
            schema['location']=locationreversemap[schema['fieldlocation']]
            del schema['fieldid']
            del schema['fieldlocation']
        elif _phonenumber_location_reversemap.has_key(schema['fieldid']):
            schema['location']=_phonenumber_location_reversemap[schema['fieldid']]
            schema['type']="phone_number"
            del schema['fieldid']
            del schema['fieldlocation']
        return schema
    def field_types(self):
        return self._field_types
    def field_schema(self,schemaid):
        return self._field_types[schemaid]
    def __iter__(self):
        return iter(self._db)
    def __getitem__(self,key):
        return ContactsDb.Contact(self._db[key],self)
    def __delitem__(self,key):
        del self._db[key]
    def __len__(self):
        return len(self._db)
    def add_contact(self):
        return ContactsDb.Contact(self._db.add_contact(),self,locked='as_new_contact')
    def keys(self):
        return list(self._db)
    def values(self):
        return [self[k] for k in self]
    def items(self):
        return [(k,self[k]) for k in self]
    def _build_vcard_flags(self,include_x=0,ett_format=0,exclude_uid=0,decrease_access_count=0,increase_access_count=0,import_single_contact=0):
        vcard_flags=0
        if include_x:
            vcard_flags|=_contacts.vcard_include_x
        if ett_format:
            vcard_flags|=_contacts.vcard_ett_format
        if exclude_uid:
            vcard_flags|=_contacts.vcard_exclude_uid
        if decrease_access_count:
            vcard_flags|=_contacts.vcard_dec_access_count
        if increase_access_count:
            vcard_flags|=_contacts.vcard_inc_access_count
        if import_single_contact:
            vcard_flags|=_contacts.vcard_import_single_contact
        return vcard_flags
    def import_vcards(self,vcards,include_x=1,ett_format=1,import_single_contact=0):
        vcard_flags=self._build_vcard_flags(include_x,ett_format,0,0,0,import_single_contact)
        return [self[x] for x in self._db.import_vcards(unicode(vcards),vcard_flags)]
    def export_vcards(self,vcard_ids,include_x=1,ett_format=1,exclude_uid=0):
        vcard_flags=self._build_vcard_flags(include_x,ett_format,exclude_uid)
        return self._db.export_vcards(tuple(vcard_ids),vcard_flags)
    def find(self,searchterm):
        return [self[x] for x in self._db.find(unicode(searchterm))]
    def compact_required(self):
        return self._db.compact_recommended()
    def compact(self):
        return self._db.compact()
    class ContactsIterator(object):
        def __init__(self,db,_iter):
            self.db=db
            self._iter=_iter
        def next(self):
            return self.db[self._iter.next()]
        def __iter__(self):
            return self
    class Contact(object):
        def __init__(self,_contact,db,locked=0):
            self._contact=_contact
            self.db=db
            self._locked=locked
        def _data(self,key):
            return self._contact.entry_data()[key]
        id=property(lambda self:self._data('uniqueid'))
        last_modified=property(lambda self:self._data('lastmodified'))
        title=property(lambda self:self._data('title'))
        def add_field(self,type,value=None,location=None,label=None):
            fieldtype=nativefieldtype_from_pythontype_and_location(type,location)
            kw={}
            if value is not None:
                if fieldtype[0] != _contacts.date:
                    kw['value']=unicode(value)
                else:
                    kw['value']=value
            if label is not None: kw['label']=unicode(label)
            if not self._locked:
                self._begin()
            self._contact.add_field(fieldtype,**kw)
            if not self._locked:
                self._contact.commit()
        def __len__(self):
            return len(self._contact)
        def __getitem__(self,key):
            #print "getitem",self,index
            if isinstance(key,int):
                if key >= len(self._contact):
                    raise IndexError
                return ContactsDb.ContactField(self,key)
            raise TypeError('field indices must be integers')
        def __delitem__(self,index):
            self[index] # Check validity of index
            ### NOTE: After this all ContactFields after this will have incorrect indices!
            if not self._locked:
                self._begin()
            del self._contact[index]
            if not self._locked:
                self._contact.commit()  
            
        def find(self,type=None,location=None):
            if type:
                if type == 'phone_number':
                    if not location:
                        return (self.find(type,'none')+
                                self.find(type,'home')+
                                self.find(type,'work'))
                    typecode=_phonenumber_location_map[location]
                else:
                    typecode=fieldtypemap[type]
                return [ContactsDb.ContactField(self,x)
                        for x in self._contact.find_field_indexes(typecode,locationmap[location])]
            else:
                if location: # this is slow, but this should be a rare case
                    return [x for x in self if x.location==location]
                else: # no search terms, return all fields
                    return list(self)
        def keys(self):
            return [x['fieldindex'] for x in self._contact]
        def __str__(self):
            return '<Contact #%d: "%s">'%(self.id,self.title)
        __repr__=__str__
        def _set(self,index,value=None,label=None):
            if not self._locked:
                self._begin()
            kw={}
            if value is not None: kw['value']=unicode(value)
            if label is not None: kw['label']=unicode(label)
            self._contact.modify_field(index,**kw)
            if not self._locked:
                self._contact.commit()            
        def _begin(self):
            try:
                self._contact.begin()
            except SymbianError:
                raise ContactBusy
        def begin(self):
            if self._locked:
                raise RuntimeError('contact already open')
            self._begin()
            self._locked=1
        def commit(self):
            if not self._locked:
                raise RuntimeError('contact not open')
            self._contact.commit()
            self._locked=0
        def rollback(self):
            if not self._locked:
                raise RuntimeError('contact not open')
            if self._locked == 'as_new_contact':
                # clear the content of new uncommited _contact by creating a new _contact.
                self._contact=self.db._db.add_contact()
            else:
                # clear the content of old committed _contact by fetching the last committed data from the database.
                self._contact.rollback()
            self._locked=0
        def __del__(self):
            if self._locked:
                import warnings
                warnings.warn("contact still locked in destructor", RuntimeWarning)
        def as_vcard(self):
            return self.db.export_vcards((self.id,))
            

    class ContactField(object):
        def __init__(self,contact,index):
            #print "Create field",contact,index 
            self.contact=contact
            self.index=index
            
        schema=property(lambda self: self.contact.db.field_schema(self.contact._contact.field_info_index(self.index)))
        type=property(lambda self:_pythontype_from_nativefieldtype_map[self.contact._contact[self.index]['fieldid']])
        label=property(lambda self: self.contact._contact[self.index]['label'],
                       lambda self,x: self.contact._set(self.index,label=x))
        value=property(lambda self: self.contact._contact[self.index]['value'],
                       lambda self,x: self.contact._set(self.index,value=x))
        location=property(lambda self:self.schema['location'])
        
        def __str__(self):
            return '<field #%d of %s: type=%s value=%s location=%s label=%s>'%(self.index, 
                                                                               self.contact,
                                                                               self.type,
                                                                               self.value,
                                                                               self.location,
                                                                               self.label)
        __repr__=__str__

def open(dbfile=None,mode=None):
    return ContactsDb(dbfile,mode)

