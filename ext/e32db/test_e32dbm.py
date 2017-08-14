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


# This script tries to exercise most of the functions in the e32dbm
# API.

import e32dbm
import os
import random
import sys

FILE=u'c:\\testdb'
# execfile('c:\\system\\libs\\test_e32dbm.py')
    
def gendata(datasize):
    testdata={} 
    for k in range(datasize):		
        testdata['key%d'%k]='value%07d'%int(random.random()*10000000);
    return testdata

def genword(length=8):
    result=[]
    for k in range(length):
        result.append(random.choice('abcdefghijklmnopqrstuvwxyz'))
    return ''.join(result)

def torturetest(db,datasize,n_operations):
    """Insert a given number of random data into the given db, and
    then perform the given number of random updates, deletes, inserts,
    syncs and reorganizes on it. At the end, check that the database
    contents match with the contents of a normal dictionary where the
    same updates were made."""
    for k in db.keys():
        del db[k]
    print "Generating %d records of data..."%datasize
    refdata=gendata(datasize)
    print "Inserting data..."
    for k in refdata:
        db[k]=refdata[k]
    wordlength=8
    def diag(msg):
        sys.stdout.write(msg)
        sys.stdout.flush()

    for i in range(n_operations):
        r=random.random()
        if r<0.20: # insert
            diag('I')
            new_key=genword()
            new_value=genword()
            db[new_key]=new_value
            refdata[new_key]=new_value
        elif r<0.4: # delete
            diag('D')
            key=random.choice(refdata.keys())
            del db[key]
            del refdata[key]
        elif r<0.95: # update
            diag('U')
            key=random.choice(refdata.keys())
            new_value=genword()
            db[key]=new_value
            refdata[key]=new_value
        elif r<0.975: #reorganize
            if hasattr(db,'reorganize'):
                diag('R')
                db.reorganize()
            else:
                diag('r')
        else: #sync
            if hasattr(db,'sync'):
                diag('S')
                db.sync()
            else:
                diag('s')
    dbcontent={}
    for k in db.keys():
        dbcontent[k]=db[k]
    asserteq(dbcontent,refdata,"content match")

def dotime(name,code):
    totaltime=0
    if verbose:
        print "%12s: "%name
        sys.stdout.flush()
    for line in code:
        if verbose:
            print("%s: "%(line)),
            sys.stdout.flush()
        start=time.clock()
        try:
            exec(line)
        except:
            print "Exception on line %s:"%line
            traceback.print_exc(None,sys.stdout)
            break
        stop=time.clock()
        totaltime+=stop-start
        if verbose:
            print "%2.3f s"%(stop-start)
            sys.stdout.flush()
    print "%12s total: %f s"%(name, totaltime)
    sys.stdout.flush()
    return totaltime
        
class TestFailed(Exception):
	pass

def asserteq(value,correctvalue,name):
	if value != correctvalue:
		raise TestFailed("Test '%s' failed: value %s, should be %s"%(name,value,correctvalue))

def clean():
	if os.path.isfile(FILE+".e32dbm"): os.remove(FILE+".e32dbm")	
	
try:
    try:
        for fastflag in '','f':
            if fastflag:
                print "--- Testing fast mode:"
            else:
                print "--- Testing normal mode:"
			
            clean()
            db=e32dbm.open(FILE,'c'+fastflag)
            print "Setting value:"
            db['a']='b'
            print "Setting another value:"
            db['foo']='bar'
            print "Re-setting value:"
            db['foo']='baz'
            print "Reading back values:"
            asserteq(db['a'],'b',"read value")
            asserteq(db['foo'],'baz',"read value")
            db.close()
			
            print "Opening file:"
            db=e32dbm.open(FILE,'w'+fastflag)
            print "Reading back values:"
            asserteq(db['a'],'b',"read value")
            asserteq(db['foo'],'baz',"read value")
            print "delete item:"
            del db['foo']
            asserteq(db.has_key('foo'),False,"deleted key must not exist")
            db['foo']='bar'
            asserteq(db.has_key('foo'),True,"key must exist after inserting it back")
            db.close()
            
            db=e32dbm.open(FILE,'c'+fastflag)		
            print "Reorganizing database."
            db.reorganize()
            db.clear()
            asserteq(len(db),0,"len must be 0 after clear")
            nvalues=10
            testdata=gendata(nvalues)
            print "Inserting %d values"%nvalues
            db.update(testdata)
            asserteq(len(db),len(testdata),"db len must match testdata len")
            print "Syncing"
            db.sync()
            db.close()
            
            print "Reading data back"
            db=e32dbm.open(FILE,"w"+fastflag)
            
            asserteq(len(db),10,"len(db)")
            
            testkeys=testdata.keys()
            testkeys.sort()
            testvalues=[testdata[x] for x in testkeys]
            
            dbkeys=list(db.iterkeys())
            dbkeys.sort()
            asserteq(dbkeys,testkeys,"iterkeys")
            dbvalues=list(db.itervalues())
            dbvalues.sort()
            testvalues_sorted=list(testvalues)
            testvalues_sorted.sort()
            asserteq(dbvalues,testvalues_sorted,"itervalues")
            
            items=db.items()
            print "Items: %s"%items
            asserteq(dict(items),testdata,"compare items")
            
            values=db.values()
            print "Values: %s"%values
            values.sort()
            
            asserteq(values,testvalues_sorted, "compare values")
            
            keys=db.keys()
            print "Keys: %s"%keys
            keys.sort()
            asserteq(keys,testkeys, "compare keys")
            
            popkey=testkeys[0]
            popvalue=testvalues[0]
            asserteq(db.pop(popkey),popvalue,"pop")
            asserteq(db.has_key(popkey),False,"key must not exist after pop")
            popped_item=db.popitem()
            asserteq(db.has_key(popped_item[0]),False,"key must not exist after popitem")
            print "Running torture test:"
            torturetest(db,100,100)
            
            #print "Running torture test on dumbdbm:"
            #import dumbdbm
            #db=dumbdbm.open('c:\\dumbdbmtestdb','c')
            ##torturetest(db,1000,100)
            #db.close()
            #
            #print "Running torture test on notsodumbdbm:"
            #import notsodumbdbm
            #db=notsodumbdbm.open('c:\\notsodumbdbmtestdb','c')
            #torturetest(db,1000,100)
            #db.close()
            # 
            print "Finished."
            db.close()
            clean()
        print "--- All tests passed."			
    except:
        print "Exception thrown:"
        import traceback
        traceback.print_exc()
        import pdb
        pdb.pm()
finally:
    if db.db:
        db.close()
    clean()
