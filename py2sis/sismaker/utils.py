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

import os, sys, re

UID_OFFSET_IN_APP = "0x3a4"

ERRMSG = "'%s' utility not found. Please make sure "\
         "you have the Symbian SDK correctly installed "\
         "and configured"

def uidcrc(uid):
    p = os.popen("uidcrc 0x10000079 0x100039CE "+uid, "r")
    s = p.read().strip()
    if not s:
        raise IOError, ERRMSG % 'uidcrc'
    uid1,uid2,uid3,crc = s.split()
    return crc

def make_sis(sisfile, pkgfile, searchpath):
    cmd = 'makesis -d"%s" %s %s' % (searchpath, pkgfile, sisfile)
    p = os.popen(cmd)
    s = p.read()
    if not s:
        raise IOError, ERRMSG % 'makesis'
    return s

def find_main_script(src):
    if not os.path.exists(src):
        raise ValueError, "File or directory not found: %s" % src
    if os.path.isfile(src):
        if not src.endswith('.py'):
            raise ValueError, "Source file does not end in .py"
        main_script = src
    else:
        main_script = os.path.join(src, "default.py")
        if not os.path.exists(main_script):
            raise ValueError, "No default.py found in %s" % src        
    return main_script

def get_appname(src):
    appname = os.path.basename(src)
    if os.path.isfile(src):
        appname = os.path.splitext(appname)[0]
    return appname

def find_uid(src):
    m = re.search(r"SYMBIAN_UID\s*=\s*(0x[0-9a-fA-F]{8})", src)
    if not m:
        return
    return m.group(1)

def default_tempdir():
    return os.path.join(sys.path[0], "temp")

def reverse(L):
    L.reverse()
    return L

def atoi(s):
    # Little-endian conversion from a 4-char string to an int.
    sum = 0L
    for x in reverse([x for x in s[0:4]]):
        sum = (sum << 8) + ord(x)
    return sum

def itoa(x):
    # Little-endian conversion from an int to a 4-character string.
    L=[chr(x>>24), chr((x>>16)&0xff), chr((x>>8)&0xff), chr(x&0xff)]
    L.reverse()
    return ''.join(L)

def make_app(appfile, template, uid, chksum):
    offset = int(UID_OFFSET_IN_APP, 16)
    #
    # copy the template .app file with proper name
    # and set the UID and checksum fields suitably
    #
    dotapp_name = appfile
    dotapp = file(dotapp_name, 'wb')
    appbuf = template
    csum = atoi(appbuf[24:28])
    crc1 = itoa(chksum)
    crc2 = itoa(( uid + csum ) & 0xffffffffL)
    if offset:
        temp = appbuf[0:8] + itoa(uid) + crc1 + appbuf[16:24] + crc2 +\
               appbuf[28:offset] + itoa(uid) + appbuf[(offset+4):]
    else:
        temp = appbuf[0:8] + itoa(uid) + crc1 + appbuf[16:24] + crc2 + appbuf[28:]
    dotapp.write(temp)

def make_pkg(pkgfile, appname, template, uid, files):
    file = open(pkgfile, "w")
    file.write(template % (appname, uid))
    appdir = "!:\\system\\apps\\%s\\" % appname
    for src,dst in files:
        dstpath = appdir + dst
        file.write('"%s"\t\t-"%s"\n' % (src,dstpath))
    file.close()
