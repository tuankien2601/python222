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
# test_camera.py
#

import camera
import e32

print "Dir camera: ", dir(camera)

maxZoom = camera.max_zoom()
print "Max zoom: ", maxZoom
flash = camera.flash_modes()
print "Flash modes: ", flash
exp = camera.exposure_modes()
print "Exp modes: ", exp
white = camera.white_balance_modes()
print "White modes: ", white

picture = camera.take_photo(mode='RGB12') 
picture.save(u'low.jpg')
print "low res file written"

picture = camera.take_photo(mode='RGB16')
picture.save(u'middle.jpg')
print "middle res file written"

picture = camera.take_photo(mode='RGB')
picture.save(u'high.jpg')
print "high res file written"
