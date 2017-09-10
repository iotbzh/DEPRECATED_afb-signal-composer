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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, something express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "ctl-config.h"

// onload section receive one action or an array of actions
PUBLIC int OnloadConfig(CtlSectionT *section, json_object *actionsJ) {
    CtlActionT *actions;
     
    // Load time parse actions in config file
    if (actionsJ != NULL) {
        actions= ActionLoad(actionsJ);
        section->handle=actions;
        
        if (!actions) {
            AFB_ERROR ("OnloadLoad config fail processing onload actions");
            goto OnErrorExit;
        }
        
    } else {
        // Exec time process onload action now
        actions=(CtlActionT*)section->handle;
        if (!actions) {
            AFB_ERROR ("OnloadLoad Cannot Exec Non Existing Onload Action");
            goto OnErrorExit;
        }

        for (int idx=0; actions[idx].source.label != NULL; idx ++) {
            ActionExecOne(&actions[idx], NULL);
        }              
    }

    return 0;

OnErrorExit:
    return 1;

}
