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

import e32
from audio import Sound

SLEEP = 7

player=Sound()
player.open(u'c:\\Baby_One_More_Time.mid')
player.play()
e32.ao_sleep(SLEEP-2)
player.stop() #does not close the file

player.open(u'c:\\foobar.wav')
player.record()
e32.ao_sleep(SLEEP)
player.stop()
player.close() #needed for sound flushing, otherwise not written
