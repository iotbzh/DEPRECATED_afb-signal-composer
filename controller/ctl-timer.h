/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef CTL_TIMER_INCLUDE
#define CTL_TIMER_INCLUDE

#include <systemd/sd-event.h>

// ctl-timer.c
// ----------------------
typedef int (*timerCallbackT)(void *context);

typedef struct TimerHandleS {
    int count;
    int delay;
    const char*label;
    void *context;
    timerCallbackT callback;
    sd_event_source *evtSource;
} TimerHandleT;

PUBLIC int TimerEvtInit (void);
PUBLIC afb_event TimerEvtGet(void);
PUBLIC void TimerEvtStart(TimerHandleT *timerHandle, timerCallbackT callback, void *context);
PUBLIC void TimerEvtStop(TimerHandleT *timerHandle);

#endif // CTL_TIMER_INCLUDE
