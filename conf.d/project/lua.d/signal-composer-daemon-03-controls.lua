--[[
  Copyright (C) 2016 "IoT.bzh"
  Author Fulup Ar Foll <fulup@iot.bzh>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Following function are called when a client activate a control with
  controller api -> APi=control VERB=dispatch
  arguments are
    - source (0) when requesting the control (-1) when releasing
    - control comme from config given with 'args' in onload-middlename-xxxxx.json
    - client is the argument part of the query as providing by client requesting the control.

--]]



-- Simple Happy(granted) Control
function _Button_Happy(source, control, client)

    -- print argument to make sure we understant what we get
    printf ("[-- _Button_Happy --] source=%d control=%s client=%s", source, Dump_Table(control), Dump_Table(client))

    AFB:notice ("[-- _Button_Happy --] To Be Done")

    return 0 -- control granted
end


-- Simple UnHappy(debu) Control
function _Button_UnHappy(source, control, client)

    -- print argument to make sure we understant what we get
    printf ("[-- _Button_UnHappy --] source=%d control=%s client=%s", source, Dump_Table(client), Dump_Table(control))

    AFB:error ("[-- _Button_UnHappy --] To Be Done")
    return 1 -- control is refused
end
