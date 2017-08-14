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

#   Simple Web Socket client example

import appuifw
import thread
import e32
#import socket

def thread_func(lock):
#    LF = open(u'c:\\logfile.txt', 'w')
#    lmsg = "init"
#    LF.write(lmsg)
    
    import socket
    aSSrv = socket.SocketServ()
    aSSrv.Connect(5)

    aSocket = socket.Socket()
#    lmsg = "\nsocket server open"
#    LF.write(lmsg)

    try:
        aSocket.Open(aSSrv, 0x0800, 1, 6)
#        lmsg = "\nsocket opened"
#        LF.write(lmsg)
    except:
        pass
    
    try:
#        aSocket.Connect('216.239.59.99', 80)   #google
        aSocket.Connect('193.166.223.20', 80)   #fmi
#        lmsg = "\nsocket connected"
#        LF.write(lmsg)
    except:
        pass

    try:
        aSocket.Write("GET /index.html\x0d\x0a")
#        lmsg = "\nWRITE called"
#        LF.write(lmsg)
    except:
        pass
    
    GENERR = 0
    RECEIVED = 0
    msg = ''
    while (RECEIVED ==0):
        try:
            chunk = " "
#            i=0
            while len(chunk) > 0:             
                chunk = aSocket.RecvOneOrMore()  
                msg = msg + chunk
#                i=i+1
#            lmsg = "\nRecvOneOrMore called"
#            LF.write(lmsg)
#            LF.write("\nlast chunk is:\n")
#            LF.write(chunk)
#            LF.write("\nIterations: ")
#            LF.write(i)
            RECEIVED = 1
            GENERR = 0    #first time it usually fails
        except:
#            LF.write("\nERROR in receiving")
            GENERR = 1
            RECEIVED = 0

    if GENERR != 1:
        try:
            f = open(u'c:\\downloaded.html', 'w')
            f.write(msg)
            f.close()
#            lmsg = "\nother file operations"
#            LF.write(lmsg)    
        except:
            pass

#    LF.close()
    del aSocket
    del aSSrv
    lock.signal()

sync = e32.Ao_lock()
thread.start_new_thread(thread_func, (sync,))
sync.wait()
print "\ndone"
try:
    content_handler = appuifw.Content_handler()
    r = content_handler.open(u'c:\\downloaded.html')
    if r != u"OK":
         appuifw.note(r, "info")
except:
    print "couldn't open data received"


