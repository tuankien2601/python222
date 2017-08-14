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

# NOTE: DELETES AN SMS MESSAGE FROM INBOX!!

import inbox

i=inbox.Inbox()
m=i.sms_messages()
print i.content(m[0])
print i.time(m[0])
print i.address(m[0])
i.delete(m[0])

"""
# Second manual test
import inbox
id=0
def cb(id_cb):
    global id
    id=id_cb
i=inbox.Inbox()
i.bind(cb)
# Send and SMS to your Inbox here. The "id" gets updated id
print i.address(id)
print i.content(id)
i.delete(id)
"""
