#
# default.py
#
# The default script run by the "Python" application in Series 60 Python
# environment. Offers menu options for running scripts that are found in
# application's directory, or in the \my -directory below it (this is
# where the application manager copies the plain Python scripts sent to
# device's inbox), as well as for launching interactive Python console. 
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

import sys
import os
import appuifw
import series60_console
import e32

def query_and_exec():
    def is_py(x):
        return os.path.splitext(x)[1].lower() == '.py'

    my_script_dir = os.path.join(this_dir,'my')
    script_list = []

    if os.path.exists(my_script_dir):
        script_list = map(lambda x: os.path.join('my',x),\
                          filter(is_py, os.listdir(my_script_dir)))

    script_list += filter(is_py, os.listdir(this_dir))
    index = appuifw.selection_list(map(unicode, script_list))
    if index >= 0:
        execfile(os.path.join(this_dir, script_list[index]), globals())

def exec_interactive():
    import interactive_console
    interactive_console.Py_console(my_console).interactive_loop()

def exec_btconsole():
    import btconsole
    btconsole.main()

def menu_action(f):
    appuifw.app.menu = []
    saved_exit_key_handler = appuifw.app.exit_key_handler

    try:
        try:
            f()
        finally:
            appuifw.app.exit_key_handler = saved_exit_key_handler
            appuifw.app.title = u'Python'
            init_options_menu()
            appuifw.app.body = my_console.text
            appuifw.app.screen='normal'
            sys.stderr = sys.stdout = my_console
    except:
        import traceback
        traceback.print_exc()

def init_options_menu():
    appuifw.app.menu = [(u"Run script",\
                         lambda: menu_action(query_and_exec)),
                        (u"Interactive console",\
                         lambda: menu_action(exec_interactive)),\
                        (u"Bluetooth console",\
                         lambda: menu_action(exec_btconsole)),\
                        (u"About Python",\
                         lambda: appuifw.note(u"See www.python.org for more information.", "info"))]

this_dir = os.path.split(appuifw.app.full_name())[0]
my_console = series60_console.Console()
appuifw.app.body = my_console.text
sys.stderr = sys.stdout = my_console
#from e32 import _stdo
#_stdo(u'c:\\python_error.log')         # low-level error output
init_options_menu()
print str(copyright)+"\nVersion "+e32.pys60_version
