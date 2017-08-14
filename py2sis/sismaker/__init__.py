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

import os, os.path, sys, re
import sismaker.utils as utils

from shutil import copyfile, rmtree

APP_TEMPLATE_FILE = "pyapp_template.tmp"
APP_TEMPLATE_FILE_PRE = "pyapp_template_pre_SDK20.tmp"
RSC_TEMPLATE_FILE = "pyrsc_template.tmp"
RSC_TEMPLATE_FILE_PRE = "pyrsc_template_pre_SDK20.tmp"
PKG_TEMPLATE_FILE = "pypkg_template.tmp"
PKG_TEMPLATE_FILE_PRE = "pypkg_template_pre_SDK20.tmp"

class SISMaker(object):
    def __init__(self, tempdir=None):
        if tempdir is None:
            tempdir = utils.default_tempdir()
        self.tempdir = os.path.abspath(tempdir)

    def init_tempdir(self):
        if os.path.exists(self.tempdir):
            rmtree(self.tempdir)
        os.makedirs(self.tempdir)

    def del_tempdir(self):
        if os.path.exists(self.tempdir):
            rmtree(self.tempdir)

    def make_sis(self, src, sisfile, uid=None, appname=None, presdk20=False, **kw):
        main_script = utils.find_main_script(src)
        
        if uid is None:
            script = open(main_script, "r").read()
            uid = utils.find_uid(script)
            if uid is None:
                raise ValueError, "No SYMBIAN_UID found in %s" % main_script

        if not re.match(r'(?i)0x[0-9a-f]{8}$', uid):
            raise ValueError, "Invalid UID: %s" % uid

        crc = utils.uidcrc(uid)

        self.init_tempdir()

        if appname is None:
            appname = utils.get_appname(src)

        appfile = os.path.join(self.tempdir, appname+".app")
        pkgfile = os.path.join(self.tempdir, appname+".pkg")
        tmpldir = os.path.join(sys.path[0], "templates")
        
        if presdk20:
            apptmpl = open(os.path.join(tmpldir, APP_TEMPLATE_FILE_PRE), 'rb').read()
            rsctmpl = os.path.join(tmpldir, RSC_TEMPLATE_FILE_PRE)
            pkgtmpl = open(os.path.join(tmpldir, PKG_TEMPLATE_FILE_PRE)).read()
        else:
            apptmpl = open(os.path.join(tmpldir, APP_TEMPLATE_FILE), 'rb').read()
            rsctmpl = os.path.join(tmpldir, RSC_TEMPLATE_FILE)
            pkgtmpl = open(os.path.join(tmpldir, PKG_TEMPLATE_FILE)).read()

        ## copy resource file to temp
        copyfile(rsctmpl, os.path.join(self.tempdir, appname+".rsc"))

        ## copy application files to temp
        if os.path.isdir(src):
            def copysrcfile(arg, dir, names):
                for name in names:
                    path = os.path.join(dir, name)
                    ext = os.path.splitext(name)[1]
                    if ext in ('.pyc', '.pyo'):
                        continue
                    if os.path.isfile(path):
                        dst = os.path.join(self.tempdir, path[len(src)+1:])
                        dstdir = os.path.dirname(dst)
                        if not os.path.exists(dstdir):
                            os.makedirs(dstdir)
                        copyfile(path, dst)
            os.path.walk(src, copysrcfile, None)
        else:
            copyfile(src, os.path.join(self.tempdir, "default.py"))        

        utils.make_app(appfile, apptmpl, int(uid, 16), int(crc, 16))
        ## add files to pkg
        files = []
        def addtopkg(arg, dir, names):
            for name in names:
                path = os.path.join(dir, name)
                if os.path.isfile(path):
                    relative = path[len(self.tempdir)+1:]
                    files.append((relative, relative))
        os.path.walk(self.tempdir, addtopkg, None)

        utils.make_pkg(pkgfile, appname, pkgtmpl, uid, files)
        
        output = utils.make_sis(sisfile, pkgfile, self.tempdir)
      
        return output
