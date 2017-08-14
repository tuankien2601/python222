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

import os.path

DEVICE_RELEASE_FROM='/epoc32/release/'+DEVICE_PLATFORM+'/'+DEVICE_BUILD
DEVICE_RELEASE_TO='epoc32/release/'+DEVICE_PLATFORM+'/'+DEVICE_BUILD

EMU_RELEASE_FROM='/epoc32/release/'+EMU_PLATFORM+'/'+EMU_BUILD
EMU_RELEASE_TO='epoc32/release/'+EMU_PLATFORM+'/'+EMU_BUILD

INCLUDES_TO='epoc32/include/python'

def IF(condition, value, default=None):
    if default is None:
        default=()
    if condition:
        return value
    else:
        return default

SDK_FILES=[]
for entry in (
    {'from': DEVICE_RELEASE_FROM,
     'to': DEVICE_RELEASE_TO,
     'files': ('python222.dll',
               'python222.lib',
               'python_appui.dll',
               'python_appui.lib')},
    {'from': EMU_RELEASE_FROM,
     'to': EMU_RELEASE_TO,
     'files': ('python222.dll',
               'python222.lib',
               'python_appui.dll',
               'python_appui.lib')},
    {'from': EMU_RELEASE_FROM+'/z/system/apps/python',
     'to': EMU_RELEASE_TO+'/z/system/apps/python',
     'files': ('python.app','python.rsc',)+
     IF(S60_VERSION>=28, ('python_aif.mif',), ('python.aif',))},
    {'from': EMU_RELEASE_FROM+'/z/private/10003A3F/apps',
     'to': EMU_RELEASE_TO+'/z/private/10003A3F/apps',
     'files': IF(S60_VERSION>=28,('python_reg.rsc',))},
    {'from': EMU_RELEASE_FROM+'/z/system/data',
     'to': EMU_RELEASE_TO+'/z/system/data',
     'files': ('appuifwmodule.rsc',)},
    {'from': EMU_RELEASE_FROM+'/z/system/libs',
     'to': EMU_RELEASE_TO+'/z/system/libs',
     'files': (   'e32db.pyd',
                  'e32socket.pyd')+
     IF(WITH_LOCATION_MODULE,('location.pyd',))+
     IF(WITH_MESSAGING_MODULE,('messaging.pyd',))+
     #               '_testcapi.pyd',
     ('_contacts.pyd',
     '_sysinfo.pyd',
     'zlib.pyd')+
    IF(S60_VERSION>12,('_camera.pyd',))+('_telephone.pyd',
                                       '_recorder.pyd',
                                       '_graphics.pyd',
                                       '_calendar.pyd',
                                       'inbox.pyd')},
{'from': EMU_RELEASE_FROM+'/z/system/programs',
 'to': EMU_RELEASE_TO+'/z/system/programs',
     'files': ('python_launcher.dll',)},
    {'from': SRC_DIR,
     'to': EMU_RELEASE_TO+'/z/system/apps/python',
     'files': ('app/default.py',
               'app/filebrowser.py',
               'app/interactive_console.py',
               'extras/snake.py',
               'extras/ball.py',
               'extras/imgviewer.py',
               'extras/keyviewer.py',
               )},
    {'from': SRC_DIR,
     'to': EMU_RELEASE_TO+'/z/system/libs',
     'files': ('core/Lib/anydbm.py',
               'core/Lib/atexit.py',
               'core/Lib/base64.py',
               'core/Lib/btconsole.py',
               'core/Lib/code.py',
               'core/Lib/codecs.py',
               'core/Lib/codeop.py',
               'core/Lib/copy.py',
               'core/Lib/copy_reg.py',
               'app/dir_iter.py',
               'app/series60_console.py',
               'core/Lib/e32dbm.py',
               'core/Lib/httplib.py',
               'core/Lib/keyword.py',
               'core/Lib/key_codes.py',
               'core/Lib/linecache.py',
               'core/Lib/mimetools.py',
               'core/Lib/ntpath.py',
               'core/Lib/os.py',
               'core/Lib/pack.py',
               'core/Lib/quopri.py',
               'core/Lib/random.py',
               'core/Lib/re.py',
               'core/Lib/repr.py',
               'core/Lib/rfc822.py',
               'core/Lib/shutil.py',
               'core/Lib/site.py',
               'core/Lib/sre.py',
               'core/Lib/sre_compile.py',
               'core/Lib/sre_constants.py',
               'core/Lib/sre_parse.py',
               'core/Lib/stat.py',
               'core/Lib/string.py',
               'core/Lib/StringIO.py',
               'core/Lib/traceback.py',
               'core/Lib/types.py',
               'core/Lib/urllib.py',
               'core/Lib/urlparse.py',
               'core/Lib/uu.py',
               'core/Lib/warnings.py',
               'core/Lib/whichdb.py',
               'core/Lib/whrandom.py',
               'core/Lib/__future__.py',
               'core/Lib/zipfile.py',
               'ext/contacts/contacts.py',
               'ext/telephone/telephone.py',
               'ext/recorder/audio.py',
               'ext/graphics/graphics.py',
               'ext/sysinfo/sysinfo.py',
               'ext/calendar/calendar.py',
               'ext/camera/camera.py',
               'ext/socket/socket.py',
               'ext/socket/select.py',
               'appui/appuifw/appuifw.py')},
    {'from': SRC_DIR,
     'to': EMU_RELEASE_TO+'/z/system/libs/encodings',
     'files': ('core/Lib/encodings/aliases.py',
               'core/Lib/encodings/ascii.py',
               'core/Lib/encodings/base64_codec.py',
               'core/Lib/encodings/charmap.py',
               'core/Lib/encodings/hex_codec.py',
               'core/Lib/encodings/latin_1.py',
               'core/Lib/encodings/raw_unicode_escape.py',
               'core/Lib/encodings/unicode_escape.py',
               'core/Lib/encodings/unicode_internal.py',
               'core/Lib/encodings/utf_16.py',
               'core/Lib/encodings/utf_16_be.py',
               'core/Lib/encodings/utf_16_le.py',
               'core/Lib/encodings/utf_7.py',
               'core/Lib/encodings/utf_8.py',
               'core/Lib/encodings/uu_codec.py',
               'core/Lib/encodings/__init__.py',
               'core/Lib/encodings/zlib_codec.py')},
    {'from': SRC_DIR,
     'to': INCLUDES_TO,
     'files': ('appui/Python_appui.h',
               'core/include/abstract.h',
               'core/include/bitset.h',
               'core/include/bufferobject.h',
               'core/include/cellobject.h',
               'core/include/ceval.h',
               'core/include/classobject.h',
               'core/include/cobject.h',
               'core/include/codecs.h',
               'core/include/compile.h',
               'core/include/complexobject.h',
               'core/include/cStringIO.h',
               'core/include/descrobject.h',
               'core/include/dictobject.h',
               'core/include/errcode.h',
               'core/include/eval.h',
               'core/include/fileobject.h',
               'core/include/floatobject.h',
               'core/include/frameobject.h',
               'core/include/funcobject.h',
               'core/include/graminit.h',
               'core/include/grammar.h',
               'core/include/import.h',
               'core/include/intobject.h',
               'core/include/intrcheck.h',
               'core/include/iterobject.h',
               'core/include/listobject.h',
               'core/include/longintrepr.h',
               'core/include/longobject.h',
               'core/include/marshal.h',
               'core/include/metagrammar.h',
               'core/include/methodobject.h',
               'core/include/modsupport.h',
               'core/include/moduleobject.h',
               'core/include/node.h',
               'core/include/object.h',
               'core/include/objimpl.h',
               'core/include/opcode.h',
               'core/include/osdefs.h',
               'core/include/parsetok.h',
               'core/include/patchlevel.h',
               'core/include/pgenheaders.h',
               'core/include/pydebug.h',
               'core/include/pyerrors.h',
               'core/include/pyfpe.h',
               'core/include/pygetopt.h',
               'core/include/pymactoolbox.h',
               'core/include/pymem.h',
               'core/include/pyport.h',
               'core/include/pystate.h',
               'core/include/Python.h',
               'core/include/pythonrun.h',
               'core/include/pythread.h',
               'core/include/py_curses.h',
               'core/include/rangeobject.h',
               'core/include/sliceobject.h',
               'core/include/stringobject.h',
               'core/include/structmember.h',
               'core/include/structseq.h',
               'core/include/symtable.h',
               'core/include/sysmodule.h',
               'core/include/token.h',
               'core/include/traceback.h',
               'core/include/tupleobject.h',
               'core/include/ucnhash.h',
               'core/include/unicodeobject.h',
               'core/include/weakrefobject.h',
               'core/objects/unicodetype_db.h',
               'core/parser/parser.h',
               'core/parser/pgen.h',
               'core/parser/tokenizer.h',
               'core/python/importdl.h',
               'core/symbian/CSPyInterpreter.h',
               'core/symbian/pyconfig.h',
               'core/symbian/python_globals.h',
               'core/symbian/symbian_python_ext_util.h',
               'core/symbian/sdkversion.h',
               'core/symbian/python_aif.mbg')}
    ):
    for f in entry['files']:
        SDK_FILES.append((os.path.normpath(entry['from']+'/'+f),
                          os.path.normpath(entry['to']+'/'+os.path.basename(f))))

