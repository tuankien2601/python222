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
# test_telephone.py
#

import e32
import telephone

NUMBER2DIAL = "+3581234567" # Insert number here
SLEEP = 7

telephone.dial(NUMBER2DIAL)
e32.ao_sleep(SLEEP)
telephone.hang_up()
e32.ao_sleep(SLEEP)
telephone.dial(NUMBER2DIAL)
e32.ao_sleep(SLEEP)
telephone.hang_up()
