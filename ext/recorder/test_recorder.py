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


import audio
import appuifw
import time
import e32

import series60_console
my_console = series60_console.Console()
appuifw.app.body = my_console.text
sys.stderr = sys.stdout = my_console

MID_FILE = u'c:\\Baby_One_More_Time.mid'
REC_FILE = u'c:\\recorder.wav'

player1 = audio.Sound.open(REC_FILE)
player2 = audio.Sound.open(MID_FILE)

script_exit_flag = False
script_lock = e32.Ao_lock()

def main_menu_setup():
    appuifw.app.menu = [(u"Start record", start_record),\
                        (u"Stop record", stop_record),\
                        (u"Play recorded", play_record),\
                        (u"Play midi", play_midi),\
                        (u"Stop midi", stop_midi),\
                        (u"Exit", exit)]

def start_record():
    player2.stop()
    player1.stop()
    print "Recording..."
    player1.record()
            
def stop_record():
    player1.stop()
    print "Stopped."

def play_record():
    print "Playing..."
    player2.stop()
    player1.play()

def play_midi():
    print "Playing midi..."
    player1.stop()
    player2.play()

def stop_midi():
    print "Midi playing stopped."
    player2.stop()   

def exit():
    player1.stop()
    player1.close() #this needs to be done if we e.g. play a midi file when exiting
    player2.stop()
    player2.close()
    global script_exit_flag
    script_exit_flag = True
    script_lock.signal()

def exit_key_handler():
    player1.stop()
    player1.close()
    player2.stop()
    player2.close()
    global script_lock
    appuifw.app.exit_key_handler = None
    script_lock.signal()    

if __name__ == '__main__':
    old_title = appuifw.app.title
    appuifw.app.title = u"Recorder"
    main_menu_setup()
    appuifw.app.exit_key_handler = exit_key_handler
    script_lock.wait()
    if script_exit_flag == True:
        appuifw.app.set_exit()
    appuifw.app.menu = []
    appuifw.app.title = old_title
    
