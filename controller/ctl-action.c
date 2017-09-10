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
 * Reference:
 *   Json load using json_unpack https://jansson.readthedocs.io/en/2.9/apiref.html#parsing-and-validating-values
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "ctl-config.h"


PUBLIC int ActionExecOne(CtlActionT* action, json_object *queryJ) {
    int err;


    switch (action->type) {
        case CTL_TYPE_API:
        {
            json_object *returnJ;

            // if query is empty increment usage count and pass args
            if (!queryJ || json_object_get_type(queryJ) != json_type_object) {
                json_object_get(action->argsJ);
                queryJ = action->argsJ;
            } else if (action->argsJ) {

                // Merge queryJ and argsJ before sending request
                if (json_object_get_type(action->argsJ) == json_type_object) {

                    json_object_object_foreach(action->argsJ, key, val) {
                        json_object_object_add(queryJ, key, val);
                    }
                } else {
                    json_object_object_add(queryJ, "args", action->argsJ);
                }
            }
            
            json_object_object_add(queryJ, "label", json_object_new_string(action->source.label));

            int err = afb_service_call_sync(action->api, action->call, queryJ, &returnJ);
            if (err) {
                static const char*format = "ActionExecOne(Api) api=%s verb=%s args=%s";
                AFB_ERROR(format, action->api, action->call, action->source.label);
                goto OnErrorExit;
            }
            break;
        }

#ifdef CONTROL_SUPPORT_LUA
        case CTL_TYPE_LUA:
            err = LuaCallFunc(action, queryJ);
            if (err) {
                AFB_ERROR("ActionExecOne(Lua) label=%s func=%s args=%s", action->source.label, action->call, json_object_get_string(action->argsJ));
                goto OnErrorExit;
            }
            break;
#endif

        case CTL_TYPE_CB:
            err = (*action->actionCB) (&action->source, action->argsJ, queryJ);
            if (err) {
                AFB_ERROR("ActionExecOne(Callback) label%s func=%s args=%s", action->source.label, action->call, json_object_get_string(action->argsJ));
                goto OnErrorExit;
            }
            break;

        default:
        {
            AFB_ERROR("ActionExecOne(unknown) API type label=%s", action->source.label);
            goto OnErrorExit;
        }
    }

    return 0;

OnErrorExit:
    return -1;
}


// unpack individual action object

PUBLIC int ActionLoadOne(CtlActionT *action, json_object *actionJ) {
    char *api = NULL, *verb = NULL, *lua = NULL;
    int err, modeCount = 0;
    json_object *callbackJ=NULL;

    err = wrap_json_unpack(actionJ, "{ss,s?s,s?o,s?s,s?s,s?s,s?o !}"
            , "label", &action->source.label, "info", &action->source.info, "callback", &callbackJ, "lua", &lua, "api", &api, "verb", &verb, "args", &action->argsJ);
    if (err) {
        AFB_ERROR("ACTION-LOAD-ONE Missing something label|info|callback|lua|(api+verb)|args in:\n--  %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }

    if (lua) {
        action->type = CTL_TYPE_LUA;
        action->call = lua;
        modeCount++;
    }

    if (api && verb) {
        action->type = CTL_TYPE_API;
        action->api = api;
        action->call = verb;
        modeCount++;
    }

    if (callbackJ) {
        action->type = CTL_TYPE_CB;
        modeCount++;        
        err = PluginGetCB (action, callbackJ);
        if (err) goto OnErrorExit;
        
    }

    // make sure at least one mode is selected
    if (modeCount == 0) {
        AFB_ERROR("ACTION-LOAD-ONE No Action Selected lua|callback|(api+verb) in %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }

    if (modeCount > 1) {
        AFB_ERROR("ACTION-LOAD-ONE:ToMany arguments lua|callback|(api+verb) in %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }
    return 0;

OnErrorExit:
    return 1;
};

PUBLIC CtlActionT *ActionLoad(json_object *actionsJ) {
    int err;
    CtlActionT *actions;

    // action array is close with a nullvalue;
    if (json_object_get_type(actionsJ) == json_type_array) {
        int count = json_object_array_length(actionsJ);
        actions = calloc(count + 1, sizeof (CtlActionT));

        for (int idx = 0; idx < count; idx++) {
            json_object *actionJ = json_object_array_get_idx(actionsJ, idx);
            err = ActionLoadOne(&actions[idx], actionJ);
            if (err) goto OnErrorExit;
        }

    } else {
        actions = calloc(2, sizeof (CtlActionT));
        err = ActionLoadOne(&actions[0], actionsJ);
        if (err) goto OnErrorExit;
    }

    return actions;

OnErrorExit:
    return NULL;

}
