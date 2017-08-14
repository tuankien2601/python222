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


import e32db

def query(db,statement):
  DBV = e32db.Db_view()
  DBV.prepare(db, unicode(statement))
  n = DBV.count_line()
  DBV.first_line()
  data=[]
  for i in xrange(n):
    DBV.get_line()
    line=[]
    for j in xrange(DBV.col_count()):
      line.append(DBV.col(1+j))
      t=DBV.col_type(1+j)
      print "Column type: %d"%t
      if t != 15:
        print "Raw value: %s"%repr(DBV.col_raw(1+j))
      if t==10:
        print "Raw time: %s"%repr(DBV.col_rawtime(1+j))
        print "Formatted: %s"%e32db.format_rawtime(DBV.col_rawtime(1+j))
        
    data.append(tuple(line))
    DBV.next_line()
  del DBV
  return data

db=e32db.Dbms()
FILE=u'c:\\bardb'
if os.path.isfile(FILE):
	os.remove(FILE)
db.create(FILE)
db.open(FILE)
db.execute(u'create table data (a long varchar, b integer)')
db.execute(u"insert into data values('"+('x'*10)+"',42)")
print query(db,u'select * from data')
#db.execute(u"insert into data values('"+('x'*100)+"',42)")
#print query(db,u'select * from data')
#db.execute(u"insert into data values('"+('x'*200)+"',42)")
#print query(db,u'select * from data')
#db.execute(u"insert into data values('"+('x'*300)+"',42)")
#print query(db,u'select * from data')
#db.execute(u"insert into data values('"+('x'*3000)+"',42)")
#print query(db,u'select * from data')
#db.execute(u"insert into data values('"+('x'*100000)+"',42)")
print query(db,u'select * from data')

def dbexec(statement):
  print "Executing %s"%statement
  db.execute(unicode(statement))

dbexec(u"insert into data values('foobar',7)")

try:
  dat=(('bit','1'),
       ('tinyint','1'),
       ('unsigned tinyint','1'),
       ('smallint','1'),
       ('unsigned smallint','1'),
       ('integer','1'),
       ('unsigned integer','1'),
       ('counter','1'),
       ('bigint',str(2**60)),
       ('real','1.234'),
       ('float','1.234'),
       ('double','1.234'),
       ('double precision','1.234'),
       ('timestamp',"#20-oct/2004#"), 
       ('time',"#20-oct/2004#"), 
       ('date',"#20-oct/2004#"),
       ('date',"#%s#"%e32db.format_time(0)),
       ('date',"#%s#"%e32db.format_time(12345678)),
       ('char(8)',"'foo'"),
       ('varchar(8)',"'bar'"),
       ('long varchar',"'baz'"),
#       ('binary(8)',"'quux'") # not supported...
       )
  
  for k in range(len(dat)):
    dbexec('create table t%d (x %s)'%(k,dat[k][0]))
    dbexec('insert into t%d values (%s)'%(k,dat[k][1]))
    print query(db,u'select * from t%d'%k)
finally:
  print "Closing database."
  db.close()

