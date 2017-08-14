#
# calendar.py
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

import _calendar


def revdict(d):
    return dict([(d[k],k) for k in d.keys()])

# maps
replicationmap={"open":_calendar.rep_open,
                "private":_calendar.rep_private,
                "restricted":_calendar.rep_restricted}
_replicationreversemap=revdict(replicationmap)
entrytypemap={"appointment":_calendar.entry_type_appt,
              "event":_calendar.entry_type_event,
              "anniversary":_calendar.entry_type_anniv,
              "todo":_calendar.entry_type_todo}
_entrytypereversemap=revdict(entrytypemap)


# Calendar database class
class CalendarDb(object):
    def __init__(self,dbfile=None,mode=None):
        if dbfile is None:
            self._db=_calendar.open()
        else:
            if mode is None:
                self._db=_calendar.open(unicode(dbfile))
            else:
                self._db=_calendar.open(unicode(dbfile),mode)
    def __iter__(self):
        entry_ids=list()
        # do not return the native iterator since it can be confused by deleting entries
        # from the db while iterating.
        for id in self._db:
            entry_ids.append(id)
        return iter(entry_ids)
    def __len__(self):
        return len(self._db)
    def __delitem__(self,key):
        del self._db[key]
    def __getitem__(self,key):
        _entry = self._db[key]
        if _entry.type()==_calendar.entry_type_appt:
            return CalendarDb.AppointmentEntry(_entry,self)
        elif _entry.type()==_calendar.entry_type_event:
            return CalendarDb.EventEntry(_entry,self)
        elif _entry.type()==_calendar.entry_type_anniv:
            return CalendarDb.AnniversaryEntry(_entry,self)
        elif _entry.type()==_calendar.entry_type_todo:
            return CalendarDb.TodoEntry(_entry,self)
    def add_appointment(self):
        return CalendarDb.AppointmentEntry(self._db.add_entry(_calendar.entry_type_appt),self,locked='as_new_entry')
    def add_event(self):
        return CalendarDb.EventEntry(self._db.add_entry(_calendar.entry_type_event),self,locked='as_new_entry')
    def add_anniversary(self):
        return CalendarDb.AnniversaryEntry(self._db.add_entry(_calendar.entry_type_anniv),self,locked='as_new_entry')
    def add_todo(self):
        return CalendarDb.TodoEntry(self._db.add_entry(_calendar.entry_type_todo),self,locked='as_new_entry')
    def _create_filter(self,appointments,events,anniversaries,todos):
        filter=0
        if appointments:
            filter|=_calendar.appts_inc_filter
        if events:
            filter|=_calendar.events_inc_filter
        if anniversaries:
            filter|=_calendar.annivs_inc_filter
        if todos:
            filter|=_calendar.todos_inc_filter
        return filter
    def monthly_instances(self,month,appointments=0,events=0,anniversaries=0,todos=0):
        return self._db.monthly_instances(month,self._create_filter(appointments,events,anniversaries,todos))
    def daily_instances(self,day,appointments=0,events=0,anniversaries=0,todos=0):
        return self._db.daily_instances(day,self._create_filter(appointments,events,anniversaries,todos))
    def find_instances(self,start_date,end_date,search_string=u'',appointments=0,events=0,anniversaries=0,todos=0):
        return self._db.find_instances(start_date,end_date,unicode(search_string),self._create_filter(appointments,events,anniversaries,todos))
    def export_vcalendars(self,entry_ids):
        return self._db.export_vcals(entry_ids)
    def import_vcalendars(self,vcalendar_string):
        return list(self._db.import_vcals(vcalendar_string))
    def compact(self):
        return self._db.compact()
    def add_todo_list(self,name=None):
        if name is None:
            return self._db.add_todo_list()
        else:
            return self._db.add_todo_list(unicode(name))
    
    # Entry class
    class Entry(object):
        def __init__(self,_entry,db,locked=0):
            self._entry=_entry
            self._db=db
            self._locked=locked        
        def _autocommit(self):
            if not self._locked:
                self._entry.commit()
        def _set_content(self,content):
            self._entry.set_content(unicode(content))
            self._autocommit()
        def _content(self):
            return self._entry.content()
        content=property(_content,_set_content)
        def _get_type(self):
            return _entrytypereversemap[self._entry.type()]
        type=property(_get_type)
        def _unique_id(self):
            return self._entry.unique_id()
        id=property(_unique_id)
        def set_repeat(self,repeat):
            if not repeat:
                repeat={"type":"no_repeat"}
            self._entry.set_repeat_data(repeat)
            self._autocommit()
        def get_repeat(self):
            repeat=self._entry.repeat_data()
            if repeat["type"]=="no_repeat":
                return None
            return self._entry.repeat_data()
        def _set_location(self,location):
            self._entry.set_location(unicode(location))
            self._autocommit()
        def _location(self):
            return self._entry.location()
        location=property(_location,_set_location)
        def _last_modified(self):
            return self._entry.last_modified()
        last_modified=property(_last_modified)
        def _set_priority(self,priority):
            self._entry.set_priority(priority)
            self._autocommit()
        def _priority(self):
            return self._entry.priority()
        priority=property(_priority,_set_priority)
        def _set_alarm(self,alarm_datetime):
            if alarm_datetime is None:
                self._entry.cancel_alarm()
            else:
                self._entry.set_alarm(alarm_datetime)
            self._autocommit()
        def _get_alarm(self):
            if self._entry.has_alarm():
                return self._entry.alarm_datetime()
            else:
                return None
        alarm=property(_get_alarm,_set_alarm)
        def _set_replication(self, status):
            self._entry.set_replication(replicationmap[status])
            self._autocommit()
        def _replication(self):
            if _replicationreversemap.has_key(self._entry.replication()):
                return _replicationreversemap[self._entry.replication()]
            return "unknown"
        replication=property(_replication,_set_replication)
        def _cross_out(self,value):
            if value:
                if self._entry.type()==_calendar.entry_type_todo:
                    self._entry.set_crossed_out(time.time()) # default time
                else:
                    self._entry.set_crossed_out(1)
            else:
               self._entry.set_crossed_out(0)
            self._autocommit()
        def _is_crossed_out(self):
            return self._entry.is_crossed_out()
        crossed_out=property(_is_crossed_out,_cross_out)
        def _start_datetime(self):
            return self._entry.start_datetime()
        start_time=property(_start_datetime)
        def _end_datetime(self):
            return self._entry.end_datetime()
        end_time=property(_end_datetime)
        def set_time(self,start=None,end=None):
            if start is None:
                start=end
            if end is None:
                end=start
            if end is None and start is None:
                if self._entry.type()==_calendar.entry_type_todo:
                    self._entry.make_undated()
                    return None
                else:
                    raise RuntimeError,"only todos can be made undated" 
            self._entry.set_start_and_end_datetime(start,end)
            self._autocommit()
        def begin(self):
            if self._locked:
                raise RuntimeError('entry already open')
            self._locked=1
        def commit(self):
            if not self._locked:
                raise RuntimeError('entry not open')
            self._entry.commit()
            self._locked=0
        def rollback(self):
            if not self._locked:
                raise RuntimeError('entry not open')
            if self._locked == 'as_new_entry':
                # clear the content of new uncommited entry by creating a new _entry.
                self._entry=self._db._db.add_entry(self._entry.type())
            else:
                # clear the content of old committed entry by fetching the last committed data from the database.
                self._entry=self._db._db[self._entry.unique_id()]
            self._locked=0
        def as_vcalendar(self):
            return self._db.export_vcalendars((self.id,))
        def __del__(self):
            if self._locked:
                import warnings
                warnings.warn("entry still locked in destructor", RuntimeWarning)
        
    # AppointmentEntry class
    class AppointmentEntry(Entry):
        def __init__(self,_entry,db,locked=0):
            CalendarDb.Entry.__init__(self,_entry,db,locked)
        def __str__(self):
            return '<AppointmentEntry #%d: "%s">'%(self.id,self.content)
    
    # EventEntry class
    class EventEntry(Entry):
        def __init__(self,_entry,db,locked=0):
            CalendarDb.Entry.__init__(self,_entry,db,locked)
        def __str__(self):
            return '<EventEntry #%d: "%s">'%(self.id,self.content)

    # AnniversaryEntry class
    class AnniversaryEntry(Entry):
        def __init__(self,_entry,db,locked=0):
            CalendarDb.Entry.__init__(self,_entry,db,locked)
        def __str__(self):
            return '<AnniversaryEntry #%d: "%s">'%(self.id,self.content)

    # TodoEntry class
    class TodoEntry(Entry):
        def __init__(self,_entry,db,locked=0):
            CalendarDb.Entry.__init__(self,_entry,db,locked)
        def __str__(self):
            return '<TodoEntry #%d: "%s">'%(self.id,self.content)
        def _get_cross_out_time(self):
            if self._entry.is_crossed_out():
                return self._entry.crossed_out_date()
            else:
                return None
        def _set_cross_out_time(self,cross_out_datetime):
            if cross_out_datetime==0:
                raise ValueError, "illegal datetime value"
            self._entry.set_crossed_out(cross_out_datetime)
            self._autocommit()
        cross_out_time=property(_get_cross_out_time,_set_cross_out_time)
        def _set_todo_list(self,list_id):
            self._entry.set_todo_list(list_id)
            self._autocommit()
        def _todo_list_id(self):
            return self._entry.todo_list_id()
        todo_list=property(_todo_list_id,_set_todo_list)

    # Todo list handling
    class TodoListDict(object):
        def __init__(self,db):
            self._db=db
        def __getitem__(self, list_id):
            return CalendarDb.TodoList(list_id,self._db)
        def __delitem__(self, list_id):
            self._db._db.remove_todo_list(list_id)
        def __iter__(self):
            return iter(self._db._db.todo_lists())
        def __len__(self):
            return len(self._db._db.todo_lists())
        def _default_list(self):
            return self._db._db.default_todo_list()
        default_list=property(_default_list)
    class TodoList(object):
        def __init__(self,list_id,db):
            self._list_id=list_id
            self._db=db
        def __iter__(self):
            return iter(self._db._db.todos_in_list(self._list_id))
        def __getitem__(self,index):
            return self._db._db.todos_in_list(self._list_id)[index]
        def __len__(self):
            return len(self._db._db.todos_in_list(self._list_id))
        def _get_id(self):
            return self._list_id
        id=property(_get_id)
        def _set_name(self,newname):
            self._db._db.rename_todo_list(self._list_id,unicode(newname))
        def _get_name(self):
            return self._db._db.todo_lists()[self._list_id]
        name=property(_get_name,_set_name)
    todo_lists=property(lambda self: CalendarDb.TodoListDict(self))

        
# Module methods
def open(dbfile=None,mode=None):
    return CalendarDb(dbfile,mode)
