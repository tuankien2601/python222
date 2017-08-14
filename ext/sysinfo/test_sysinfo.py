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

import sysinfo

print "OS: "
print sysinfo.os_version()
print "SW: "
print sysinfo.sw_version()
print "IMEI: "
print sysinfo.imei()
print "Bat: "
print sysinfo.battery()
print "Net: "
print sysinfo.signal()
print "Ram: "
print sysinfo.total_ram()
print "Rom: "
print sysinfo.total_rom()
print "MaxRamDrive: "
print sysinfo.max_ramdrive_size()
print "Twips: "
print sysinfo.display_twips()
print "Pixels: "
print sysinfo.display_pixels()
print "RamFree: "
print sysinfo.free_ram()
print "DriveSpace: "
print sysinfo.free_drivespace()
print "RingType: "
print sysinfo.ring_type()
