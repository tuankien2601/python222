#
# test_socket.py
#
# A script for testing Series 60 Python socket module.
#
# NOTE: scripts with "*" are supposed to be run versus another client/server
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
#

import appuifw
import e32
#from e32socket import *
from socket import *
import os
import sys
from key_codes import *
import thread

script_exit_flag = False
script_lock = e32.Ao_lock()

def main_menu_setup():
    appuifw.app.menu = [#(u"getservice", getservice),\
                        (u"GPRS client", gprs_client),\
                        (u"IP client/server", ip),\
                        (u"UDP cl/srv", udp_cl_srv),\
                        (u"UDP connect", udp_connect),\
                        (u"Host Resolver", host_resolver),\
                        (u"SSL", ssl_cl),\
                        (u"OBEX sending", obex_send),\
                        (u"RFCOMM sending", rfcomm_send),\
                        (u"* RFCOMM server *", rfcomm_srv),\
                        (u"* RFCOMM client *", rfcomm_cl),\
                        (u"* RFCOMM Known CL*", rfcomm_known_cl),\
                        (u"* OBEX client *", obex_cl),\
                        (u"* OBEX Known CL*", obex_known_cl),\
                        (u"* OBEX server *", obex_srv),\
                        (u"Stressing GRPS", gprs_stress),\
                        (u"other minor", other),]
    appuifw.app.exit_key_handler = exit_key_handler


# ***** GPRS
def gprs_client():
    print "GPRS test..."
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
    print "socket created"
    s.connect(('194.109.137.226', 80))  #dev
#    s.connect(('www.connecting.europe.nokia.com', 80))
#    s.connect(('131.228.55.140', 8080))  #emulator
    print "connected"
    s.send("GET http://www.python.org/pics/pythonHi.gif\x0d\x0a")
    print "send called"
    msg = s.read_all()
    print "read_all called"
    f = open(u"C:\\testSocket\\pythonHi.gif", "w")
    if (msg.startswith('HTTP/')):
        index = msg.find('\x0d\x0a\x0d\x0a')
        f.write(msg[index+4:])
    else:
        f.write(msg)            
    f.close()            
    del s
#    try:
#        content_handler = appuifw.Content_handler()
#        r = content_handler.open(u'c:\\testSocket\\pythonHi.gif')
#    except:
#        print "couldn't open data received"
    print "done."


# ***** IP client/server
def ip_server(lock):
        LF = open(u'c:\\testSocket\\logIPSrv.txt', 'w')
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
        LF.write("create\n")
	LF.flush()
	s.bind(('0.0.0.0', 3333))
#	s.bind(('localhost', 3333))  #this doesn't work
        LF.write("bind\n")
	LF.flush()	
	state = 1
	while state:
		s.listen(5)
		LF.write("listen\n")
		LF.flush()		
                try:
                    bs, address = s.accept()
                    remAdd, port = address
                    LF.write("\ns accepted")
                    LF.flush()
                    LF.write("\ntuple: %s"%(str(address)))
                    LF.write("\nremAddr: %s"%remAdd)
                    LF.write("\nremPort: %d"%port)
                except:
                    LF.write("\nexception risen for accept")
                    type, value, tb = sys.exc_info()
                    LF.write((str(type)+'\n'+str(value)))    
		LF.flush()		
		avail = 1
		echo = ''
		while avail:
			msg = bs.read()
			if msg == '\n':
				avail = 0
				continue
			echo = echo + msg
		LF.write("bs read msg\n")
		LF.flush()		
		if echo == '':
			state = 0
			LF.write("nothing read\n")
			LF.flush()
			continue
		else:
			LF.write("Server has received: ")
			LF.write(echo)
			LF.flush()
			bs.send("A msg from server\n")
		LF.write("\nbs write")
		LF.flush()
		del bs
		del s
		LF.write("\n END")
		LF.close()
                state = 0
        lock.signal()		

def ip_client():   #lock):
    LF = open(u'c:\\testSocket\\logIPCli.txt', 'w')
    index = 0

    while index == 0:	
        msg = "hello\n"
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
        LF.write("cl created\n")
        LF.flush()
        s.connect(('127.0.0.1', 3333))
        LF.write("cl connect\n")
        LF.flush()		
        s.send(msg)
        LF.write("cl write\n")
        LF.flush()
        state = 1
        msg2 = ''
        while state:
            reply = s.read()
            if reply == '\n':
                state = 0
                continue
            msg2 = msg2 + reply

        LF.write("cl read: \n")
        LF.write(msg2)
        LF.flush()
        del s
        LF.write("\n END \n")
        LF.close()
#    lock.signal()		    

def ip():
    """this script runs ip client and ip server in different threads"""
    print "IP client/server test..."
    syncS = e32.Ao_lock()
#    syncC = e32.Ao_lock()
    thread.start_new_thread(ip_server, (syncS, ))
    thread.start_new_thread(ip_client, ())  #syncC, ))
    syncS.wait()
#    syncC.wait()
    del syncS
#    del syncC
    print " executed"

def ssl_cl():
    LF = open(u"C:\\testSocket\\ssl.txt", "w")  
    try:
        print "ssl client"
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
        print ("\ntcp socket created")
#        s.connect(('198.65.143.195', 443))			 #DEVICE
        s.connect(('www.connecting.europe.nokia.com', 443))	 #EMULATOR

        print ("\ntcp socket connected")
        LF.write("\ntcp socket connected")
        LF.flush()
      
        sec = ssl(s)
        print ("\nssl socket created")
        LF.write("\nssl socket created")
        LF.flush()

#        sec.write("GET https://www.fortify.net/sslcheck.html\x0d\x0a")		#DEV
        sec.write("GET https://www.connecting.europe.nokia.com/mnc/resources/images/mnc/ncp_logo_blue.gif\x0d\x0a") 	#EM
        print ("\nssl.write")
        LF.write("\nssl.write")
        LF.flush()

#        html = sec.read()
        html = sec.read(8192)
        print ("\nssl.read")
        LF.write("\nssl.read")
        LF.flush()
	        
        HTML = open(u"C:\\testSocket\\sslTEST.html","w")
        HTML.write(html)
        HTML.close()
        
        del sec
        del s
        print ("\nboth socket deleted")
        LF.write("\nboth socket deleted")
        LF.flush()     
    except:
        print "\nexception risen"
        type, value, tb = sys.exc_info()
        print((str(type)+'\n'+str(value)))
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()
    LF.close()
        
# ***** RFCOM Send towards pc
def rfcomm_send():
    print "RFCOMM client test 2 pc"
    LF = open(u"C:\\testSocket\\rfcommCliPC.txt", "w")  
    s = socket(AF_BT, SOCK_STREAM, BTPROTO_RFCOMM)
    LF.write("s created")
    LF.flush()
    try:
        addr, serv = bt_discover()
        print("\nbt_discover called")
        print("\naddress: %s, len dict %d"%(addr, len(serv)))
	print("\n%s"%serv)
        LF.write("\nbt_discover called")
        LF.write("\naddress: %s, len dict %d"%(addr, len(serv)))
	LF.write("\n%s"%serv)
        LF.flush()
        list = serv.keys()
        channel = appuifw.popup_menu(list, u"Choose a remote service")
        s.connect((addr, serv[(list[channel])]))

#        s.connect((addr, serv[u"Py RFCOMM service"]))
        print "connected"
        print "port used: %d"%(serv[(list[channel])])      
        LF.write("\nConnected")
        LF.flush()
        s.send("Greetings from Python!\n")
        print "wrote..."
        LF.write("\ns write called. Next deletion")
        LF.flush()       
    except:
        LF.write("\nexception risen for bt_discover")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))    
        LF.flush()       
    LF.close()
    del s
    print "done"


# ***** RFCOMM server
def rfcomm_srv():
    LF = open(u"C:\\testSocket\\rfcommSrv.txt", "w")  
    s = socket(AF_BT, SOCK_STREAM, BTPROTO_RFCOMM)
    print("s created and opened")
    LF.write("s created and opened")
    LF.flush()
    ch = bt_rfcomm_get_available_server_channel(s)
    print("\ns asked for available channel: %d"%ch)
    LF.write("\ns asked for available channel: %d"%ch)
    LF.flush()
    try:
        s.bind(("whatever", ch))
    except:
        print "exception for bind!"
        LF.write("\nexception risen for s.bind")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()         
    try:
        s.listen(5)
    except:
        print "exception for listen!"
        LF.write("\nexception risen for s.listen")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()         

    try:    
        bt_advertise_service(u"Py RFCOMM service", s, True, RFCOMM)
        print "service advertised"
        LF.write("\nservice advertised")
        LF.flush()
    except:
        print "exception for service advertising!"
        LF.write("\nexception risen for advertise service")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()         

    try:
        mode = AUTH
        set_security(s, mode)
        print "set security"
        LF.write("\nsecurity set")
        LF.flush()
    except:
        print "exception for security set!"
        LF.write("\nexception risen for set security")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()         
       
    try:
        bs, remAdd = s.accept()
        print "accepted"
        LF.write("\ns accepted. Rem.Addr.: %s"%remAdd)
        LF.flush()
        LF.write("\nremAddr: %s"%remAdd)
    except:
        print "exception for accept!"
        LF.write("\nexception risen for accept")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))    
    LF.flush()
    msg = bs.read(30)
#    msg = bs.read_all()
    LF.write("\ns read(30) called")
    LF.flush()
    print "read 30"
    if (len(msg) == 0):
        print "NOTHING READ!"
        LF.write("\nNOTHING read!!!!")
        LF.flush()
    else:    
        recf = open(u"C:\\testSocket\\RFC_receivingFile.txt", "w")    
        recf.write(msg)
        recf.close()
        print "message received: "
        print(msg)

    try:    
        bt_advertise_service(u"Py RFCOMM service", s, False, RFCOMM)
        print "stop advertising service"
        LF.write("\nstop advertising")
        LF.flush()
    except:
        print "exception for stop advertising!"
        LF.write("\nexception risen for stopping advertise service")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()         

    LF.write("\ns is going to be deleted")
    LF.close()  
    del bs
    del s
    print " executed"

# ***** RFCOMM client
def rfcomm_cl():
    print "RFCOMM client test..."
    LF = open(u"C:\\testSocket\\rfcommCli.txt", "w")  
    s = socket(AF_BT, SOCK_STREAM, BTPROTO_RFCOMM)
    LF.write("s created")
    LF.flush()
    try:
        addr, serv = bt_discover()
        print("\nbt_discover called")
        print("\naddress: %s, len dict %d"%(addr, len(serv)))
	print("\n%s"%serv)
        LF.write("\nbt_discover called")
        LF.write("\naddress: %s, len dict %d"%(addr, len(serv)))
	LF.write("\n%s"%serv)
        LF.flush()
#        s.connect(addr, serv[u"Serial Port"])
        s.connect((addr, serv[u"Py RFCOMM service"]))
        print "connected"
        LF.write("\nConnected")
        LF.flush()
        s.send("Greetings from Python!\n")
        print "wrote..."
        LF.write("\ns write called. Next deletion")
        LF.flush()       
    except:
        LF.write("\nexception risen for bt_discover")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))    
        LF.flush()       
    LF.close()
    del s
    print "done"


# ***** RFCOMM Known client
def rfcomm_known_cl():
    LF = open(u"C:\\testSocket\\rfcomm_K_Cli.txt", "w")  
    s = socket(AF_BT, SOCK_STREAM, BTPROTO_RFCOMM)
    LF.write("s created")
    LF.flush()
    try:
        # it will connect to Michele's 6600
        print "I'll try to connect to Michele_6600"
        #print "works against S60 BTP2P"
        addr, serv = bt_discover("00:60:57:b7:22:1e")
        LF.write("\nbt_discover called")
        LF.write("\naddress: %s, len dict %d"%(addr, len(serv)))
	LF.write("\n%s"%serv)
        LF.flush()
        print("\naddress: %s, len dict %d"%(addr, len(serv)))
	print("\n%s"%serv)
        s.connect((addr, serv[u"Py RFCOMM service"]))
        LF.write("\nConnected")
        LF.flush()
        print "connected"
        s.send("Greetings from Python!\n")
        LF.write("\ns write called. Next deletion")
        LF.flush()
        print "data wrote"
    except:
        print "exception risen"
        LF.write("\nexception risen for bt_discover")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))    
        LF.flush()       
        print((str(type)+'\n'+str(value)))    
    LF.close()
    del s
    print "\ndone"

# ***** Obex Client
def obex_cl():
    print "Obex client test..."
    print "works against Obex server"
    LF = open(u"C:\\testSocket\\obexCli.txt", "w")  
    try:
        addr, serv = bt_obex_discover()
        print("\nbt_discover called")
        print("\naddress: %s, len dict %d"%(addr, len(serv)))
	print("\n%s"%serv)
        LF.write("\nbt_discover called")
        LF.write("\naddress: %s, len dict %d"%(addr, len(serv)))
	LF.write("\n%s"%serv)
        LF.flush()
        bt_obex_send_file(addr, serv[u"Py Obex Service"], u"c:\\testSocket\\pythonHi.gif")
        print "file sent"
        LF.write("\nfile Sent")
        LF.flush()
    except:
        print "exception risen"
        LF.write("\nexception risen for bt_discover")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))    
        LF.flush()       
        print((str(type)+'\n'+str(value)))    
    LF.close()
    print "done"

# ***** OBEX Known client
def obex_known_cl():
    LF = open(u"C:\\testSocket\\obex_K_Cli.txt", "w")  
    LF.write("s created")
    LF.flush()
    try:
        print "I'll try to connect to Michele_6600"
        addr, serv = bt_obex_discover("00:60:57:b7:22:1e")
        LF.write("\nbt_discover called")
        LF.write("\naddress: %s, len dict %d"%(addr, len(serv)))
	LF.write("\n%s"%serv)
        LF.flush()
        print("\naddress: %s, len dict %d"%(addr, len(serv)))
	print("\n%s"%serv)
        bt_obex_send_file(addr, serv[u"Py Obex Service"], u"c:\\testSocket\\pythonHi.gif")
        print "file sent"
        LF.write("\nfile Sent")
        LF.flush()
    except:
        print "exception risen"
        LF.write("\nexception risen for bt_discover")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))    
        LF.flush()       
        print((str(type)+'\n'+str(value)))    
    LF.close()
    print "\ndone"

# ***** Obex Server
def obex_srv():
    LF = open(u"C:\\testSocket\\obexSrv.txt", "w")

    s = socket(AF_BT, SOCK_STREAM, BTPROTO_RFCOMM)

    s.bind(("whatever", 20))

    try:    
        bt_advertise_service(u"Py Obex Service", s, True, OBEX)
        print "service 1 advertised"
        LF.write("\nservice 1 advertised")
        LF.flush()
    except:
        print "exception for service advertising!"
        LF.write("\nexception risen for advertise service")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()         

    s2 = socket(AF_BT, SOCK_STREAM, BTPROTO_RFCOMM)
    s2.bind(("whatever", 22))
    try:    
        bt_advertise_service(u"Py Another Obex Service", s2, True, OBEX)
        print "service 2 advertised"
        LF.write("\nservice 2 advertised")
        LF.flush()
    except:
        print "exception for service advertising!"
        LF.write("\nexception risen for advertise service")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()
                
    try:
        mode = AUTH
        set_security(s, mode) 

        print "set security"
        LF.write("\nsecurity set")
        LF.flush()
    except:
        print "exception for security set!"
        LF.write("\nexception risen for set security")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()         
       
    try:
        LF.write("\nHERE")
        LF.flush()

        bt_obex_receive(s, u'c:\\testSocket\\obexReceivedFile.gif')
        LF.write("\nHERE2")
        LF.flush()

        f=open(u'c:\\testSocket\\obexReceivedFile.gif','r')
        if f:
            print "obj received"
            f.close()
        else:
            print "nothing received"
            
        LF.write("\nobj received")
        LF.flush()
    except:
        print "exception on receiving"
        LF.write("\nexception on receiving")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))    

#    choices = [u"Yes"]
#    popup_menu = appuifw.popup_menu(choices, u"Close? please choose:-)")

    try:    
        bt_advertise_service(u"Py Obex Service", s, False, OBEX)
        print "stop advertising service on 12"
        LF.write("\nstop advertising")
        LF.flush()
    except:
        print "exception for stop advertising!"
        LF.write("\nexception risen for stopping advertise service")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))
        LF.flush()         
     
    LF.close()
    del s
    del s2
    print " executed"


# ***** Obex sending towards unknown device
def obex_send():
    print "it will send a file towards an unknown device"
    LF = open(u"C:\\testSocket\\obexSending.txt", "w")  
    try:
        addr, serv = bt_obex_discover()
        print("\nbt_discover called")
        LF.write("\nbt_discover called")
        LF.write("\naddress: %s, len dict %d"%(addr, len(serv)))
	LF.write("\n%s"%serv)
        LF.flush()
        list = serv.keys()
        channel = appuifw.popup_menu(list, u"Choose a remote service")
        bt_obex_send_file(addr, serv[(list[channel])], u"c:\\testSocket\\pythonHi.gif")
        print "file sent"
        print "port used: %d"%(serv[(list[channel])])      
        LF.write("\nfile Sent")
        LF.flush()
    except:
        print "exception risen"
        LF.write("\nexception risen for bt_discover")
        type, value, tb = sys.exc_info()
        LF.write((str(type)+'\n'+str(value)))    
        LF.flush()       
        print((str(type)+'\n'+str(value)))    
    LF.close()
    print "done"
    


# ***** Host Resolver
def host_resolver():
    print "Host Resolver test..."
    LF = open(u'c:\\testSocket\\HostResolver.txt', 'w')
    LF.write("HR created\n")
    LF.flush() 
    addr =  gethostbyname("www.nokia.com")
    LF.write("gethostbyname. Addr is: ")
    LF.write(addr)
    LF.flush()
    print("\ngethostbyname. Addr is: ")
    print(addr)
    addr =  gethostbyname("147.243.3.73")
    LF.write("gethostbyname giving IP. Addr is: ")
    LF.write(addr)
    LF.flush()
    print("\ngethostbyname giving IP. Addr is: ")
    print(addr)

    addr =  gethostbyname_ex("www.nokia.com")
    a, b, c = addr
    print("\ngethostbyname_ex: ")
    print a, b, c
    LF.write("gethostbyname_ex. Addr is: ")
    LF.write(a)
    LF.write(str(b))
    LF.write(str(c))
    LF.flush()
  
    try:
        print("\ngethostbyaddress: ")
        (a, b, c) =  gethostbyaddr("147.243.3.73")        
        print a, (str(b)), c
        LF.write("\ngethostbyaddress. Name is: %s %s %s " %(a,(str(b)),(str(c))))
        LF.flush()
    except:
        print("\nException risen")
        LF.write("\nexception risen")
        type, value, tb = sys.exc_info()
        LF.write("\nEXCEPTION:")
        LF.write((str(type)+'\n'+str(value)))    
        print((str(type)+'\n'+str(value)))    

    try:
        print("\ngethostbyaddress giving DNS: ")
        (a, b, c) =  gethostbyaddr("www.nokia.com")        
        print a, (str(b)), c
        LF.write("\ngethostbyaddress giving DNS. Name is: %s %s %s " %(a,(str(b)),(str(c))))
        LF.flush()
    except:
        print("\nException risen")
        LF.write("\nexception risen")
        type, value, tb = sys.exc_info()
        LF.write("\nEXCEPTION:")
        LF.write((str(type)+'\n'+str(value)))    
        print((str(type)+'\n'+str(value)))    

    LF.close()
    print "\nExecuted"

# ***** UDP client/server
def udp_server(lock):
    LF = open(u"C:\\testSocket\\udpSrv.txt", "w")
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
    LF.write("\nUDP socket created")
    LF.flush()
    try:
        s.bind(('0.0.0.0', 1001))
        LF.write("\nbind called")
        LF.flush()
        thread.start_new_thread(udp_client, ())
        data, address = s.recvfrom(14)
        addr, port = address
        LF.write("\nrecvfrom called")
        LF.flush()
        if not data:
            LF.write("\nClient has exited")
        else:
            LF.write("\nreceived message: %s"%data)
            LF.write("\ntuple: %s"%(str(address)))
            LF.write("\nremote address: %s"%addr)
            LF.write("\nremote port: %d"%port)
    except:
        LF.write("\nexception risen")
        type, value, tb = sys.exc_info()
        LF.write("\nEXCEPTION:")
        LF.write((str(type)+'\n'+str(value)))
    LF.write("\ntest ended")    
    LF.close()
    del s
    lock.signal()

def udp_client():  
    LF = open(u"C:\\testSocket\\udpCl.txt", "w")
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
    LF.write("UDP client socket created")
    LF.flush()
    i = 0
    LF.write("\nsendto return: ")
    LF.flush()
    while i == 0:
        i = s.sendto("hello michele\n", ('127.0.0.1', 1001))
        LF.write("%d"%i)
        LF.flush()
        if i>0:
            LF.write("\nSent data: %d"%i)
    LF.write("\ntest ended")    
    LF.close()
    del s

def udp_cl_srv():
    print "UDP client/server test..."
    syncS = e32.Ao_lock()
    thread.start_new_thread(udp_server, (syncS, ))
    syncS.wait()
    del syncS
    print " executed"

#********* UDP using connect
def udp_conn_srv(lock):
    LF = open(u"C:\\testSocket\\udpSrv.txt", "w")
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
    LF.write("\nUDP socket created")
    LF.flush()

    try:
        s.bind(('0.0.0.0', 1002))
        LF.write("\nbind called")
        LF.flush()
        thread.start_new_thread(udp_conn_cl, ())
        LF.write("\nnew thread started")
        LF.flush()

#        data, address = s.recv(14, MSG_WAITALL)
        data = s.recv(14)
        LF.write("\nrecvfrom called and retreived: %s"%(str(data)))
        LF.flush()
        if not data:
            LF.write("\nClient has exited")
        else:
            LF.write("\nreceived message: %s"%data)
    except:
        LF.write("\nexception risen")
        type, value, tb = sys.exc_info()
        LF.write("\nEXCEPTION:")
        LF.write((str(type)+'\n'+str(value)))
    LF.write("\ntest ended")    
    LF.close()
    del s
    lock.signal()

def udp_conn_cl():  
    LF = open(u"C:\\testSocket\\udpConnectCL.txt", "w")
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)
    LF.write("UDP client socket created")
    LF.flush()
    s.connect(('127.0.0.1', 1002))  
    LF.write("UDP socket connect() called")
    LF.flush()
    ret = s.send("hello michele\n")
    LF.write("\nData Sent. Return: %s"%(str(ret)))
    LF.write("\ntest ended")    
    LF.close()
    del s

def udp_connect():
    print "UDP connect CL/SRV test..."
    syncS = e32.Ao_lock()
    thread.start_new_thread(udp_conn_srv, (syncS, ))
    syncS.wait()
    del syncS
    print " executed"   


#****** getservice
"""def getservice():
    import e32socket
    try:
        print ("getservice by number")
        name = e32socket.getservice(80, e32socket.AF_INET, e32socket.SOCK_STREAM, e32socket.IPPROTO_TCP)
        print (str(name))
        print ("getservice by name")
        port =  e32socket.getservice("http", e32socket.AF_INET, e32socket.SOCK_STREAM, e32socket.IPPROTO_TCP)
        print (str(port))
    except:
        type, value, tb = sys.exc_info()
        print("\nEXCEPTION:")
        print((str(type)+'\n'+str(value)))
"""

# ***** Stressing GPRS
def gprs_stress():
     LF = open(u"C:\\testSocket\\stressingGPRS.txt", "w")
     It = range(20)
     LF.write("Socket creation")
     for i in It:
        try:
            s = socket(i-1)
            del s
        except:
            LF.write("\nexception risen with %d" %(i-1))
            type, value, tb = sys.exc_info()
            LF.write("\nEXCEPTION:")
            LF.write((str(type)+'\n'+str(value)))

     LF.flush() 
     LF.write("\n***OPEN")

     try:
         s = socket()
         LF.write("\nSHOULD NEVER HAPPEN")
     except:
         LF.write("\nexception risen for bad creation()")
         type, value, tb = sys.exc_info()
         LF.write((str(type)+'\n'+str(value)))
     LF.flush() 
     try:
         s = socket(AF_INET)
         LF.write("\nSHOULD NEVER HAPPEN")
     except:
         LF.write("\nexception risen for Socket(AF_INET)")
         type, value, tb = sys.exc_info()
         LF.write((str(type)+'\n'+str(value)))
     LF.flush() 
     try:
         s = socket(AF_INET, 2, IPPROTO_TCP)
         LF.write("\nSHOULD NEVER HAPPEN")
     except:
         LF.write("\nexception risen for Socket(AF_INET,2,IPPROTO_TCP) ")
         type, value, tb = sys.exc_info()
         LF.write((str(type)+'\n'+str(value)))
     LF.flush()  
     LF.write("\n***CONNECT")

     # try to connect before opening
     s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)

     try:
         s.connect(('', 80))
         LF.write("\nTest FAILED: connected")
     except:      
         type, value, tb = sys.exc_info()
         LF.write("\nEXCEPTION: ")
         LF.write((str(type)+'\n'+str(value)))

     LF.write("\n***WRITE")
     LF.flush() 
#     s.connect('131.228.55.140', 8080)
     s.connect(('194.109.137.226', 80))

     s.send("GET http://www.python.org/pics/pythonHi.gif\x0d\x0a")
     LF.write("\n***READ")
     try:
         msg = s.read(0)
         LF.write("\nTest FAILED: send")
     except:      
         type, value, tb = sys.exc_info()
         LF.write("\nEXCEPTION: ")
         LF.write((str(type)+'\n'+str(value)))
     LF.flush() 
     msg = ''
     try:    
         msg = s.read(3765)
         x = len(msg)
         LF.write("\n%d"%x)      
         f = open(u"C:\\testSocket\\pythonHi.gif", "w")
         f.write(msg)
         f.close()
         LF.write(" data read & file written...")      
     except:      
         type, value, tb = sys.exc_info()
         LF.write("\nEXCEPTION: ")
         LF.write((str(type)+'\n'+str(value)))
     LF.flush() 

     f = open(u"C:\\testSocket\\pythonHi.gif", "w")
     if (msg.startswith('HTTP/')):
        index = msg.find('\x0d\x0a\x0d\x0a')
        f.write(msg[index+4:])
     else:
        f.write(msg)            
     f.close()
#     LF.close()
     del s

     #testing read_all() towards a big file
     print("\nTest downloading of a big file")
     LF.write("\n\n=======\ntesting read_all() towards a big file")
     LF.flush()   
     s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
     try:
#         s.connect('131.228.55.140', 8080)
         s.connect(('65.114.4.69', 80))
         s.send("GET http://www.comics.com/comics/dilbert/archive/images/dilbert2004080130741.jpg\x0d\x0a")
     except:
         type, value, tb = sys.exc_info()
         LF.write("\nEXCEPTION before real stuff: ")
         LF.write((str(type)+'\n'+str(value)))
         LF.flush()             
     try: 
         msg = s.read_all()
         LF.write("\nread_all() read big data")
         LF.flush()
     except:      
         type, value, tb = sys.exc_info()
         LF.write("\nEXCEPTION in read_all(): ")
         LF.write((str(type)+'\n'+str(value)))
         LF.flush()           
     
     f = open(u"C:\\testSocket\\bigFile.jpg","w")
     f.write(msg)
     f.close()
     f = open(u"C:\\testSocket\\bigFile.jpg","w")
     if (msg.startswith('HTTP/')):
        index = msg.find('\x0d\x0a\x0d\x0a')
        f.write(msg[index+4:])
     else:
        f.write(msg)        
     f.close()
     LF.close()
     print "Done."

#**** other
def other():
    try:
        print "other test: connect_ex"
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
        print ("\ntcp socket created")
        s.connect_ex(('216.239.59.99', 80))
        print ("YOU SHOULD NOT SEE THIS")
    except:
        print "\nexception risen"
        type, value, tb = sys.exc_info()
        print((str(type)+'\n'+str(value)))

    try:
        print "\nother test: fileno"
        s.fileno()
        print ("YOU SHOULD NOT SEE THIS")
    except:
        print "\nexception risen"
        type, value, tb = sys.exc_info()
        print((str(type)+'\n'+str(value)))

    try:
        s.close
        del s
    except:
        print "\nexception risen"
        type, value, tb = sys.exc_info()
        print((str(type)+'\n'+str(value)))      

def exit_key_handler():
    appuifw.app.title = old_title
    global script_lock
    script_lock.signal()

old_title = appuifw.app.title
appuifw.app.title = u"Socket testing console"

try:
    os.chdir("C:\\testSocket")
except:    
    os.mkdir("C:\\testSocket")
    os.chdir("C:\\testSocket")
main_menu_setup()
#appuifw.app.exit_key_handler = exit_key_handler
script_lock.wait()
if script_exit_flag == True:
    appuifw.app.set_exit()
    appuifw.app.menu = []
    appuifw.app.title = old_title
