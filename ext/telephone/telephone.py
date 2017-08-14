#
# telephone.py
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

import _telephone

_phone=_telephone.Phone()
_is_closed=1

def dial(number):
    global _is_closed
    if _is_closed:
        _phone.open()
        _is_closed=0
    _phone.set_number(number)
    _phone.dial()
def hang_up():
    _phone.hang_up()
