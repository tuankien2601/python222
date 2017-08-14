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

import contacts
import time


# script 1, iterate through contact entries in the default database


# open the default database
def script1():
    db = contacts.open()
    
    for entry_id in db:
        contact = db[entry_id]
        print u'the contact:%s'%contact
    
    db.close()


# script 2, create new empty database and contact

def script2():
    # create new, empty database
    db = contacts.open(u'test_database_1', 'n')
    
    
    # get available field types
    fieldtypes = db.field_types()
    
    
    # add new contact
    contact = db.add_contact()
    contact.add_field(contacts.first_name, value=u'John', label=u'Nickname')
    contact.add_field(contacts.last_name, u'Doe')
    contact.add_field(contacts.date, time.time())
    contact.add_field(contacts.phone_number_mobile, u'76476548')
    contact.commit()
    
    for entry_id in db:
        contact = db[entry_id]
        print u'the contact:%s'%contact
        print u'entry\'s data:%s'%contact.entry_data()
        print u'number of fields=%s'%len(contact)
        for field in contact:
            print u'the field label:%s'%field["label"]
            if(db.field_info(contact.field_info_index(field["fieldindex"]))["storagetype"]
               == contacts.storage_type_datetime):
              print time.ctime(field["value"])
            else:
              print u'the field value:%s'%field["value"]
    
    db.close()

# script 3, modify and delete

def script3():
    # open the database..
    db = contacts.open(u'test_database_2', 'c')
    
    # add new contact
    contact = db.add_contact()
    contact.add_field(contacts.first_name, u'John', u'Nickname')
    contact.add_field(contacts.last_name, u'Doe')
    contact.commit()
    
    print u'the contact at first:%s'%contact
    
    # modify the contact
    contact.begin()
    contact.modify_field(0, u'Henry') # 0 is the index of the field. this is ugly but i think this is more easily fixed in python wrapper.
    
    print u'the contact now:%s'%contact
    
    # delete the first field
    del contact[0]
    
    contact.commit()
    
    print u'and now:%s'%contact
    
    # delete the contact
    del db[contact.entry_data()["uniqueid"]]
    
    db.close()

#script 4, export (and print) some vcards

def script4():
    # open the database..
    db = contacts.open(u'test_database_3', 'n')
    
    # add new contacts
    contact1 = db.add_contact()
    contact1.add_field(contacts.first_name, u'Bill', u'Nickname')
    contact1.add_field(contacts.last_name, u'Mason')
    contact1.commit()
    
    contact2 = db.add_contact()
    contact2.add_field(contacts.first_name, u'Julie')
    contact2.add_field(contacts.last_name, u'Richards')
    contact2.add_field((contacts.phone_number_mobile, contacts.location_work),
                        u'76476547')
    contact2.add_field((contacts.phone_number_mobile, contacts.location_home),
                        u'76476548')
    contact2.commit()
    
    id1 = contact1.entry_data()["uniqueid"]
    id2 = contact2.entry_data()["uniqueid"]
    
    vcards = db.export_vcards((id1, id2), contacts.vcard_exclude_uid|contacts.vcard_include_x)
    
    print vcards
    
    db.close()

# script 5, export and import some vcards

def script5():
    # open the database..
    db = contacts.open(u'test_database_3', 'n')
    
    # get available field types
    fieldtypes = db.field_types()
    
    # add new contacts
    contact1 = db.add_contact()
    contact1.add_field(contacts.first_name, u'Bill', u'Nickname')
    contact1.add_field(contacts.last_name, u'Mason')
    contact1.commit()
    
    contact2 = db.add_contact()
    contact2.add_field(contacts.first_name, u'Julie')
    contact2.add_field(contacts.last_name, u'Richards')
    contact2.commit()
    
    id1 = contact1.entry_data()["uniqueid"]
    id2 = contact2.entry_data()["uniqueid"]
    
    # export "Bill" and "Julie"
    vcards = db.export_vcards((id1, id2))
    
    print u'***see bill and julie here***'
    for entry_id in db:
        print u'the contact:%s'%db[entry_id]
    print u''
    
    # delete "Bill"
    del db[id1]
    
    print u'***now bill has been deleted***'
    for entry_id in db:
        print u'the contact:%s'%db[entry_id]
    print u''
    
    # import "Bill" and "Julie"
    print db.import_vcards(vcards)
    
    print u'***now bill is imported back in vcard***'
    for entry_id in db:
        print u'the contact:%s'%db[entry_id]
    
    db.close()

# script 6, find functionality

def script6():
    # open the database..
    db = contacts.open(u'test_database_3', 'n')
    
    # get available field types
    fieldtypes = db.field_types()
    
    # add new contacts
    contact1 = db.add_contact()
    contact1.add_field(contacts.first_name, u'Bill', u'Nickname')
    contact1.add_field(contacts.last_name, u'Mason')
    contact1.add_field(contacts.country, u'United States')
    contact1.commit()
    
    contact2 = db.add_contact()
    contact2.add_field(contacts.first_name, u'Julie')
    contact2.add_field(contacts.last_name, u'Richards')
    contact2.add_field(contacts.country, u'Canada')
    contact2.commit()
    
    print "search results for \'ichar\':"
    id_tuple = db.find(u'ichar')
    
    for id in id_tuple:
        print db[id]
    
    print ''
    
    print "search results for \'i\':"
    id_tuple = db.find(u'i')
    
    for id in id_tuple:
        print db[id]
    
    print ''
    
    # note that specifying the field types for the search
    # does not necessarily limit the search to only those
    # fields (because c++ api works like that..).
    print "search results for \'i\' in country-field':"
    id_tuple = db.find(u'i', (contacts.country,))
    
    for id in id_tuple:
        print db[id]
    
    db.close()    

# script 7, some general information

def script7():
    # open the database..
    db = contacts.open(u'test_database_3', 'n')
    
    field_types = db.field_types()
    
    print db.field_info(field_types.keys()[0])
    
    db.close()

import appuifw
import e32
lock=e32.Ao_lock()
appuifw.app.menu=[
    (u'show default db',script1),
    (u'create db and contact',script2),
    (u'modify and delete',script3),
    (u'vcard export',script4),
    (u'vcard export&import',script5),
    (u'find',script6),
    (u'show info',script7),
    (u'Exit',lock.signal)]
old_exit_handler=appuifw.app.exit_key_handler
def exit_handler():
    appuifw.app.exit_key_handler=old_exit_handler
    lock.signal()

appuifw.app.exit_key_handler=exit_handler
lock.wait()



