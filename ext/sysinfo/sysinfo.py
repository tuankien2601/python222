#
# sysinfo.py
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

try: 
    import _sysinfo
except SystemError:
    raise ImportError('Sysinfo facility in use')

def os_version():
    return _sysinfo.os_version()
def sw_version():
    return _sysinfo.sw_version()
def imei():
    return _sysinfo.imei()
def battery():
    return _sysinfo.battery()
def signal():
    return _sysinfo.signal()
def total_ram():
    return _sysinfo.total_ram()
def total_rom():
    return _sysinfo.total_rom()
def max_ramdrive_size():
    return _sysinfo.max_ramdrive_size()
def display_twips():
    return _sysinfo.display_twips()
def display_pixels():
    return _sysinfo.display_pixels()
def free_ram():
    return _sysinfo.free_ram()
def free_drivespace():
    return _sysinfo.free_drivespace()
def ring_type():
    val=_sysinfo.ring_type()
    if val == 0:
        return 'normal'
    elif val == 1:
        return 'ascending'
    elif val == 2:
        return 'ring_once'
    elif val == 3:
        return 'beep'
    elif val == 4:
        return 'silent'
    else:
        return 'unknown'
