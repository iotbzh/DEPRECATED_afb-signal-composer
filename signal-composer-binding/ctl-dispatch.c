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
#include <dlfcn.h>
#include <sys/prctl.h>
#include <filescan-utils.h>
#include <wrap-json.h>

#include "ctl-lua.h"
#include "signal-composer-binding.hpp"

typedef void*(*DispatchPluginInstallCbT)(const char* label, const char*version, const char*info);

static afb_req NULL_AFBREQ = {};

typedef struct {
    const char* id;
    const char* source;
    const char* class;
    DispatchActionT* onReceived;
} DispatchSignalT;

typedef struct {
    const char* label;
    const char* info;
    const char* ssource;
    const char* sclass;
    DispatchActionT* actions;
} DispatchHandleT;

typedef struct {
    const char *label;
    const char *info;
    void *context;
    char *sharelib;
    void *dlHandle;
    luaL_Reg *l2cFunc;
    int l2cCount;
} DispatchPluginT;

typedef struct {
    const char* label;
    const char *info;
    const char *version;
    DispatchPluginT *plugin;
    DispatchHandleT **sources;
    DispatchHandleT **signals;
} DispatchConfigT;

// global config handle
static DispatchConfigT *configHandle = NULL;

static int DispatchSignalToIndex(DispatchHandleT **signals, const char* controlLabel) {

    for (int idx = 0; signals[idx]; idx++) {
        if (!strcasecmp(controlLabel, signals[idx]->label)) return idx;
    }
    return -1;
}

static int DispatchOneSignal(DispatchSourceT source, DispatchHandleT **signals, const char* controlLabel, json_object *queryJ, afb_req request) {
    int err;

    if (!configHandle) {
        AFB_ERROR("DISPATCH-CTL-API: (Hoops/Bug!!!) No Config Loaded");
        return -1;
    }

    if (!configHandle->signals) {
        AFB_ERROR("DISPATCH-CTL-API: No Signal Action in Json config label=%s version=%s", configHandle->label, configHandle->version);
        return -1;
    }

    int index = DispatchSignalToIndex(signals, controlLabel);
    if (index < 0 || !signals[index]->actions) {
        AFB_ERROR("DISPATCH-CTL-API:NotFound/Error label=%s in Json Signal Config File", controlLabel);
        return -1;
    }

    // Fulup (Bug/Feature) in current version is unique to every onload profile
    if (configHandle->plugin && configHandle->plugin->l2cCount) {
        LuaL2cNewLib (configHandle->plugin->label, configHandle->plugin->l2cFunc, configHandle->plugin->l2cCount);
    }

    // loop on action for this control
    DispatchActionT *actions = signals[index]->actions;
    for (int idx = 0; actions[idx].label; idx++) {

        switch (actions[idx].mode) {
            case CTL_MODE_API:
            {
                json_object *returnJ;

                // if query is empty increment usage count and pass args
                if (!queryJ || json_object_get_type(queryJ) != json_type_object) {
                    json_object_get(actions[idx].argsJ);
                    queryJ= actions[idx].argsJ;
                } else if (actions[idx].argsJ) {

                    // Merge queryJ and argsJ before sending request
                    if (json_object_get_type(actions[idx].argsJ) ==  json_type_object) {
                        json_object_object_foreach(actions[idx].argsJ, key, val) {
                        json_object_object_add(queryJ, key, val);
                        }
                    } else {
                        json_object_object_add(queryJ, "args", actions[idx].argsJ);
                    }
                }

                int err = afb_service_call_sync(actions[idx].api, actions[idx].call, queryJ, &returnJ);
                if (err) {
                    static const char*format = "DispatchOneSignal(Api) api=%s verb=%s args=%s";
                    if (afb_req_is_valid(request))afb_req_fail_f(request, "DISPATCH-CTL-MODE:API", format, actions[idx].label, actions[idx].api, actions[idx].call);
                    else AFB_ERROR(format, actions[idx].api, actions[idx].call, actions[idx].label);
                    return -1;
                }
                break;
            }

#ifdef CONTROL_SUPPORT_LUA
            case CTL_MODE_LUA:
                err = LuaCallFunc(source, &actions[idx], queryJ);
                if (err) {
                    static const char*format = "DispatchOneSignal(Lua) label=%s func=%s args=%s";
                    if (afb_req_is_valid(request)) afb_req_fail_f(request, "DISPATCH-CTL-MODE:Lua", format, actions[idx].label, actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    else AFB_ERROR(format, actions[idx].label, actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    return -1;
                }
                break;
#endif

            case CTL_MODE_CB:
                err = (*actions[idx].actionCB) (source, actions[idx].label, actions[idx].argsJ, queryJ, configHandle->plugin->context);
                if (err) {
                    static const char*format = "DispatchOneSignal(Callback) label%s func=%s args=%s";
                    if (afb_req_is_valid(request)) afb_req_fail_f(request, "DISPATCH-CTL-MODE:Cb", format, actions[idx].label, actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    else AFB_ERROR(format, actions[idx].label, actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    return -1;
                }
                break;

            default:
            {
                static const char*format = "DispatchOneSignal(unknown) mode control=%s action=%s";
                AFB_ERROR(format, signals[index]->label);
                if (afb_req_is_valid(request))afb_req_fail_f(request, "DISPATCH-CTL-MODE:Unknown", format, signals[index]->label);
            }
        }
    }

    // everything when fine
    if (afb_req_is_valid(request)) afb_req_success(request, NULL, signals[index]->label);
    return 0;
}

// Event name is mapped on control label and executed as a standard control
/*
void DispatchOneEvent(const char *evtLabel, json_object *eventJ) {
    DispatchHandleT **events = configHandle->events;

    (void) DispatchOneSignal(CTL_SOURCE_EVENT, events, evtLabel, eventJ, NULL_AFBREQ);
}
*/
// Event name is mapped on control label and executed as a standard control

int DispatchSources() {
    if (!configHandle) return 1;

    int err = 0;
    DispatchHandleT **sources = configHandle->sources;
    ssize_t i = 0, nSources = sizeof(*sources) / sizeof(DispatchHandleT);

    while(i < nSources) {
        const char* sourceLabel = sources[i++]->label;
        err = DispatchOneSignal(CTL_SOURCE_ONLOAD, sources, sourceLabel, NULL, NULL_AFBREQ);
    }

    return err;
}

void ctlapi_dispatch(afb_req request) {
    DispatchHandleT **signals = configHandle->signals;
    json_object *queryJ, *argsJ=NULL;
    const char *target;
    DispatchSourceT source= CTL_SOURCE_UNKNOWN;

    queryJ = afb_req_json(request);
    int err = wrap_json_unpack(queryJ, "{s:s, s?i s?o !}", "target", &target, "source", &source, "args", &argsJ);
    if (err) {
        afb_req_fail_f(request, "CTL-DISPTACH-INVALID", "missing target or args not a valid json object query=%s", json_object_get_string(queryJ));
        return;
    }

    (void) DispatchOneSignal(source, signals, target, argsJ, request);
}

// Wrapper to Lua2c plugin command add context dans delegate to LuaWrapper
int DispatchOneL2c(lua_State* luaState, char *funcname, Lua2cFunctionT callback) {
#ifndef CONTROL_SUPPORT_LUA
    AFB_ERROR("DISPATCH-ONE-L2C: LUA support not selected (cf:CONTROL_SUPPORT_LUA) in config.cmake");
    return 1;
#else
    int err=Lua2cWrapper(luaState, funcname, callback, configHandle->plugin->context);
    return err;
#endif
}


// List Avaliable Configuration Files

void ctlapi_config(struct afb_req request) {
    json_object*tmpJ;
    char *dirList;
    // Compile some default directories to browse
    char defaultConfPath[CONTROL_MAXPATH_LEN];
    strncpy(defaultConfPath, GetBindingDirPath(), sizeof(GetBindingDirPath()));
    strncat(defaultConfPath, "/etc:", sizeof(defaultConfPath) - strlen(defaultConfPath) - 1);
    strncat(defaultConfPath, GetBindingDirPath(), sizeof(defaultConfPath) - strlen(defaultConfPath) - 1);
    strncat(defaultConfPath, "/data", sizeof(defaultConfPath) - strlen(defaultConfPath) - 1);


    json_object* queryJ = afb_req_json(request);
    if (queryJ && json_object_object_get_ex(queryJ, "cfgpath", &tmpJ)) {
        dirList = strdup(json_object_get_string(tmpJ));
    } else {

        dirList = getenv("CONTROL_CONFIG_PATH");
        if (!dirList) dirList = defaultConfPath;
        AFB_NOTICE("CONFIG-MISSING: use default CONTROL_CONFIG_PATH=%s", defaultConfPath);
    }

    // get list of config file
    struct json_object *responseJ = ScanForConfig(dirList, CTL_SCAN_RECURSIVE, "onload", "json");

    if (json_object_array_length(responseJ) == 0) {
        afb_req_fail(request, "CONFIGPATH:EMPTY", "No Config Found in CONTROL_CONFIG_PATH");
    } else {
        afb_req_success(request, responseJ, NULL);
    }

    return;
}

// unpack individual action object

static int DispatchLoadOneAction(DispatchConfigT *controlConfig, json_object *actionJ, DispatchActionT *action) {
    char *api = NULL, *verb = NULL, *callback = NULL, *lua = NULL, *function = NULL;
    int err, modeCount = 0;

    err = wrap_json_unpack(actionJ, "{s?s,s?s,s?s,s?s,s?s,s?s,s?s,s?o !}"
            , "label", &action->label
            , "info", &action->info
            , "function", &function
            , "callback", &callback
            , "lua", &lua
            , "api", &api, "verb", &verb
            , "args", &action->argsJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-ACTION Missing something label|info|callback|lua|(api+verb)|args in %s", json_object_get_string(actionJ));
        return -1;
    }

    // Generic way to specify a C or LUA actions
    if(function) {
        if(strcasestr(function, "lua")) {
            action->mode = CTL_MODE_LUA;
            action->call = function;
            modeCount++;
        }
        else if(controlConfig->plugin) {
            action->mode = CTL_MODE_CB;
            action->call = callback;
            modeCount++;

            action->actionCB = dlsym(controlConfig->plugin->dlHandle, callback);
            if (!action->actionCB) {
                AFB_ERROR("DISPATCH-LOAD-ACTION fail to find calbback=%s in %s", callback, controlConfig->plugin->sharelib);
                return -1;
            }
        }
        // If label not already set then use function name
        if(! &action->label) action->label = function;
    }

    if (lua) {
        action->mode = CTL_MODE_LUA;
        action->call = lua;
        modeCount++;
        // If label not already set then use function name
        if(! &action->label) action->label = lua;
    }

    if (api && verb) {
        action->mode = CTL_MODE_API;
        action->api = api;
        action->call = verb;
        modeCount++;
        char* apiVerb = strdup(api);
        apiVerb = strncat(apiVerb, "/", sizeof(*apiVerb) - strlen(apiVerb) - 1);
        apiVerb = strncat(apiVerb, verb, sizeof(*apiVerb) - strlen(apiVerb) - 1);
        // If label not already set then use function name
        if(! &action->label) action->label = apiVerb;
        free(apiVerb);
    }

    if (callback && controlConfig->plugin) {
        action->mode = CTL_MODE_CB;
        action->call = callback;
        modeCount++;

        action->actionCB = dlsym(controlConfig->plugin->dlHandle, callback);
        if (!action->actionCB) {
            AFB_ERROR("DISPATCH-LOAD-ACTION fail to find calbback=%s in %s", callback, controlConfig->plugin->sharelib);
            return -1;
        }
    }

    // make sure at least one mode is selected
    if (modeCount == 0) {
        AFB_ERROR("DISPATCH-LOAD-ACTION No Action Selected lua|callback|(api+verb) in %s", json_object_get_string(actionJ));
        return -1;
    }

    if (modeCount > 1) {
        AFB_ERROR("DISPATCH-LOAD-ACTION:Too Many arguments lua|callback|(api+verb) in %s", json_object_get_string(actionJ));
        return -1;
    }
    return 0;
};

static DispatchActionT *DispatchLoadActions(DispatchConfigT *controlConfig, json_object *actionsJ) {
    int err;
    DispatchActionT *actions;

    // action array is close with a nullvalue;
    if (json_object_get_type(actionsJ) == json_type_array) {
        int count = json_object_array_length(actionsJ);
        actions = calloc(count + 1, sizeof (DispatchActionT));

        for (int idx = 0; idx < count; idx++) {
            json_object *actionJ = json_object_array_get_idx(actionsJ, idx);
            err = DispatchLoadOneAction(controlConfig, actionJ, &actions[idx]);
            if (err) return NULL;
        }

    } else {
        actions = calloc(2, sizeof (DispatchActionT));
        err = DispatchLoadOneAction(controlConfig, actionsJ, &actions[0]);
        if (err) return NULL;
    }

    return actions;
}

static void DispatchLoadPlugin(DispatchConfigT *controlConfig, json_object* sourcesJ, json_object* pluginJ)
{
    int err = 0;
    json_object *lua2csJ = NULL;
    DispatchPluginT *dPlugin= calloc(1, sizeof(DispatchPluginT));
    controlConfig->plugin = dPlugin;
    const char*ldSearchPath=NULL;

    err = wrap_json_unpack(pluginJ, "{ss,s?s,s?s,ss,s?o!}",
            "label", &dPlugin->label, "info", &dPlugin->info, "ldpath", &ldSearchPath, "sharelib", &dPlugin->sharelib, "lua2c", &lua2csJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:ONLOAD Plugin missing label|[info]|sharelib|[lua2c] in %s", json_object_get_string(sourcesJ));
        return;
    }

    // if search path not in Json config file, then try default
    if (!ldSearchPath) ldSearchPath= strncat(GetBindingDirPath(), "/data", sizeof(GetBindingDirPath()) - strlen(GetBindingDirPath()) - 1);

    // search for default policy config file
    json_object *pluginPathJ = ScanForConfig(ldSearchPath, CTL_SCAN_RECURSIVE, dPlugin->sharelib, NULL);
    if (!pluginPathJ || json_object_array_length(pluginPathJ) == 0) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:PLUGIN Missing plugin=%s in path=%s", dPlugin->sharelib, ldSearchPath);
        return;
    }

    char *filename;
    char*fullpath;
    err = wrap_json_unpack(json_object_array_get_idx(pluginPathJ, 0), "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:PLUGIN HOOPs invalid plugin file path = %s", json_object_get_string(pluginPathJ));
        return;
    }

    if (json_object_array_length(pluginPathJ) > 1) {
        AFB_WARNING("DISPATCH-LOAD-CONFIG:PLUGIN plugin multiple instances in searchpath will use %s/%s", fullpath, filename);
    }

    char pluginpath[CONTROL_MAXPATH_LEN];
    strncpy(pluginpath, fullpath, sizeof (pluginpath));
    strncat(pluginpath, "/", sizeof (pluginpath)-strlen(pluginpath)-1);
    strncat(pluginpath, filename, sizeof (pluginpath)-strlen(pluginpath)-1);
    dPlugin->dlHandle = dlopen(pluginpath, RTLD_NOW);
    if (!dPlugin->dlHandle) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:PLUGIN Fail to load pluginpath=%s err= %s", pluginpath, dlerror());
        return;
    }

    CtlPluginMagicT *ctlPluginMagic = (CtlPluginMagicT*) dlsym(dPlugin->dlHandle, "CtlPluginMagic");
    if (!ctlPluginMagic || ctlPluginMagic->magic != CTL_PLUGIN_MAGIC) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:Plugin symbol'CtlPluginMagic' missing or !=  CTL_PLUGIN_MAGIC plugin=%s", pluginpath);
        return;
    } else {
        AFB_NOTICE("DISPATCH-LOAD-CONFIG:Plugin %s successfully registered", ctlPluginMagic->label);
    }

    // Jose hack to make verbosity visible from sharelib
    struct afb_binding_data_v2 *afbHidenData = dlsym(dPlugin->dlHandle, "afbBindingV2data");
    if (afbHidenData) *afbHidenData = afbBindingV2data;

    // Push lua2cWrapper @ into plugin
    Lua2cWrapperT *lua2cInPlug = dlsym(dPlugin->dlHandle, "Lua2cWrap");
#ifndef CONTROL_SUPPORT_LUA
    if (lua2cInPlug) *lua2cInPlug = NULL;
#else
    // Lua2cWrapper is part of binder and not expose to dynamic link
    if (lua2cInPlug) *lua2cInPlug = DispatchOneL2c;

    {
        int Lua2cAddOne(luaL_Reg *l2cFunc, const char* l2cName, int index) {
            char funcName[CONTROL_MAXPATH_LEN];
            strncpy(funcName, "lua2c_", sizeof(funcName));
            strncat(funcName, l2cName, sizeof(funcName)-strlen(funcName)-1);

            Lua2cFunctionT l2cFunction= (Lua2cFunctionT)dlsym(dPlugin->dlHandle, funcName);
            if (!l2cFunction) {
                AFB_ERROR("DISPATCH-LOAD-CONFIG:Plugin symbol'%s' missing err=%s", funcName, dlerror());
                return 1;
            }
            l2cFunc[index].func=(void*)l2cFunction;
            l2cFunc[index].name=strdup(l2cName);

            return 0;
        }

        int errCount = 0;
        luaL_Reg *l2cFunc=NULL;
        int count=0;

        // look on l2c command and push them to LUA
        if (json_object_get_type(lua2csJ) == json_type_array) {
            int length = json_object_array_length(lua2csJ);
            l2cFunc = calloc(length + 1, sizeof (luaL_Reg));
            for (count=0; count < length; count++) {
                int err;
                const char *l2cName = json_object_get_string(json_object_array_get_idx(lua2csJ, count));
                err = Lua2cAddOne(l2cFunc, l2cName, count);
                if (err) errCount++;
            }
        } else {
            l2cFunc = calloc(2, sizeof (luaL_Reg));
            const char *l2cName = json_object_get_string(lua2csJ);
            errCount = Lua2cAddOne(l2cFunc, l2cName, 0);
            count=1;
        }
        if (errCount) {
            AFB_ERROR("DISPATCH-LOAD-CONFIG:Plugin %d symbols not found in plugin='%s'", errCount, pluginpath);
            return;
        } else {
            dPlugin->l2cFunc= l2cFunc;
            dPlugin->l2cCount= count;
        }
    }
#endif
    DispatchPluginInstallCbT ctlPluginOnload = dlsym(dPlugin->dlHandle, "CtlPluginOnload");
    if (ctlPluginOnload) {
        dPlugin->context = (*ctlPluginOnload) (controlConfig->label, controlConfig->version, controlConfig->info);
    }
}

static DispatchHandleT *DispatchLoadSignal(DispatchConfigT *controlConfig, json_object *controlJ) {
    json_object *actionsJ, *permissionsJ;
    int err;

    DispatchHandleT *dispatchHandle = calloc(1, sizeof (DispatchHandleT));
    err = wrap_json_unpack(controlJ, "{ss,s?s,ss,ss,s?o,so !}"
        , "id", &dispatchHandle->label
        , "info", &dispatchHandle->info
        , "source", &dispatchHandle->ssource
        , "class", &dispatchHandle->sclass
        , "permissions", &permissionsJ
        , "onReceived", &actionsJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:CONTROL Missing something label|[info]|actions in %s", json_object_get_string(controlJ));
        return NULL;
    }

    dispatchHandle->actions = DispatchLoadActions(controlConfig, actionsJ);
    if (!dispatchHandle->actions) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:CONTROL Error when parsing actions %s", dispatchHandle->label);
        return NULL;
    }
    return dispatchHandle;
}

static DispatchHandleT *DispatchLoadSource(DispatchConfigT *controlConfig, json_object *sourcesJ) {
    json_object *actionsJ = NULL, *pluginJ = NULL;
    int err;

    DispatchHandleT *dispatchSources = calloc(1, sizeof (DispatchHandleT));
    err = wrap_json_unpack(sourcesJ, "{ss,s?s,s?o,s?o,s?o !}",
            "api", &dispatchSources->label, "info", &dispatchSources->info, "plugin", &pluginJ, "actions", &actionsJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:ONLOAD Missing something label|[info]|[plugin]|[actions] in %s", json_object_get_string(sourcesJ));
        return NULL;
    }

    // best effort to initialise everything before starting
    err = afb_daemon_require_api(dispatchSources->label, 1);
    if (err) {
        AFB_WARNING("DISPATCH-LOAD-CONFIG:REQUIRE Fail to get=%s", dispatchSources->label);
    }

    if (pluginJ) {
        DispatchLoadPlugin(controlConfig, sourcesJ, pluginJ);
    }

    dispatchSources->actions = DispatchLoadActions(controlConfig, actionsJ);
    if (!dispatchSources->actions) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:ONLOAD Error when parsing actions %s", dispatchSources->label);
        return NULL;
    }
    return dispatchSources;
}

static DispatchConfigT *DispatchLoadConfig(const char* filepath) {
    json_object *controlConfigJ, *ignoreJ;
    int err;

    // Load JSON file
    controlConfigJ = json_object_from_file(filepath);
    if (!controlConfigJ) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:JsonLoad invalid JSON %s ", filepath);
        return NULL;
    }

    AFB_INFO("DISPATCH-LOAD-CONFIG: loading config filepath=%s", filepath);

    json_object *metadataJ = NULL, *sourcesJ = NULL, *signalsJ = NULL;
    err = wrap_json_unpack(controlConfigJ, "{s?s,s?o,s?o,s?o !}", "$schema", &ignoreJ, "metadata", &metadataJ, "sources", &sourcesJ, "signals", &signalsJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG Missing something metadata|[sources]|[signals] in %s", json_object_get_string(controlConfigJ));
        return NULL;
    }

    DispatchConfigT *controlConfig = calloc(1, sizeof (DispatchConfigT));
    if (metadataJ) {
        const char*ctlname=NULL;
        err = wrap_json_unpack(metadataJ, "{ss,s?s,s?s,ss !}", "label", &controlConfig->label, "version", &controlConfig->version, "name", &ctlname, "info", &controlConfig->info);
        if (err) {
            AFB_ERROR("DISPATCH-LOAD-CONFIG:METADATA Missing something label|version|[label] in %s", json_object_get_string(metadataJ));
            return NULL;
        }

        // if ctlname is provided change process name now
        if (ctlname) {
            err= prctl(PR_SET_NAME, ctlname,NULL,NULL,NULL);
            if (err) AFB_WARNING("Fail to set Process Name to:%s",ctlname);
        }
    }

    if (sourcesJ) {
        DispatchHandleT *dispatchHandle;

        if (json_object_get_type(sourcesJ) != json_type_array) {
            controlConfig->sources = (DispatchHandleT**) calloc(2, sizeof (void*));
            dispatchHandle = DispatchLoadSource(controlConfig, sourcesJ);
            controlConfig->sources[0] = dispatchHandle;
        } else {
            int length = json_object_array_length(sourcesJ);
            controlConfig->sources = (DispatchHandleT**) calloc(length + 1, sizeof (void*));

            for (int jdx = 0; jdx < length; jdx++) {
                json_object *sourcesJ = json_object_array_get_idx(sourcesJ, jdx);
                dispatchHandle = DispatchLoadSource(controlConfig, sourcesJ);
                controlConfig->sources[jdx] = dispatchHandle;
            }
        }
    }

    if (signalsJ) {
        DispatchHandleT* dispatchHandle;

        if (json_object_get_type(signalsJ) != json_type_array) {
            controlConfig->signals = (DispatchHandleT**) calloc(2, sizeof (void*));
            dispatchHandle = DispatchLoadSignal(controlConfig, signalsJ);
            controlConfig->signals[0] = dispatchHandle;
        } else {
            int length = json_object_array_length(signalsJ);
            controlConfig->signals = (DispatchHandleT**) calloc(length + 1, sizeof (void*));

            for (int jdx = 0; jdx < length; jdx++) {
                json_object *controlJ = json_object_array_get_idx(signalsJ, jdx);
                dispatchHandle = DispatchLoadSignal(controlConfig, controlJ);
                controlConfig->signals[jdx] = dispatchHandle;
            }
        }
    }
/*
    if (eventsJ) {
        DispatchHandleT *dispatchHandle;

        if (json_object_get_type(eventsJ) != json_type_array) {
            controlConfig->events = (DispatchHandleT**) calloc(2, sizeof (void*));
            dispatchHandle = DispatchLoadSignal(controlConfig, eventsJ);
            controlConfig->events[0] = dispatchHandle;
        } else {
            int length = json_object_array_length(eventsJ);
            controlConfig->events = (DispatchHandleT**) calloc(length + 1, sizeof (void*));

            for (int jdx = 0; jdx < length; jdx++) {
                json_object *eventJ = json_object_array_get_idx(eventsJ, jdx);
                dispatchHandle = DispatchLoadSignal(controlConfig, eventJ);
                controlConfig->events[jdx] = dispatchHandle;
            }
        }
    }
*/
    return controlConfig;
}


// Load default config file at init

int DispatchInit() {
    int index, luaLoaded = 0;
    char controlFile [CONTROL_MAXPATH_LEN];
    // Compile some default directories to browse
    char defaultConfPath[CONTROL_MAXPATH_LEN];
    strncpy(defaultConfPath, GetBindingDirPath(), sizeof(GetBindingDirPath()));
    strncat(defaultConfPath, "/etc:", sizeof(defaultConfPath) - strlen(defaultConfPath) - 1);
    strncat(defaultConfPath, GetBindingDirPath(), sizeof(defaultConfPath) - strlen(defaultConfPath) - 1);
    strncat(defaultConfPath, "/data", sizeof(defaultConfPath) - strlen(defaultConfPath) - 1);

    const char *dirList = getenv("CONTROL_CONFIG_PATH");
    if (!dirList) dirList = defaultConfPath;

    strncpy(controlFile, CONTROL_CONFIG_PRE "-", CONTROL_MAXPATH_LEN);
    strncat(controlFile, GetBinderName(), CONTROL_MAXPATH_LEN-strlen(controlFile)-1);

    // search for default dispatch config file
    json_object* responseJ = ScanForConfig(dirList, CTL_SCAN_RECURSIVE, controlFile, "json");

    // We load 1st file others are just warnings
    for (index = 0; index < json_object_array_length(responseJ); index++) {
        json_object *entryJ = json_object_array_get_idx(responseJ, index);

        char *filename;
        char*fullpath;
        int err = wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_ERROR("DISPATCH-INIT HOOPs invalid JSON entry= %s", json_object_get_string(entryJ));
            return -1;
        }

        if (index == 0) {
            if (strcasestr(filename, controlFile)) {
                char filepath[CONTROL_MAXPATH_LEN];
                strncpy(filepath, fullpath, sizeof (filepath));
                strncat(filepath, "/", sizeof (filepath)-strlen(filepath)-1);
                strncat(filepath, filename, sizeof (filepath)-strlen(filepath)-1);
                configHandle = DispatchLoadConfig(filepath);
                if (!configHandle) {
                    AFB_ERROR("DISPATCH-INIT:ERROR Fail loading [%s]", filepath);
                    return -1;
                }
                luaLoaded = 1;
                break;
            }
        } else {
            AFB_WARNING("DISPATCH-INIT:WARNING Secondary Signal Config Ignored %s/%s", fullpath, filename);
        }
    }

    // no dispatch config found remove control API from binder
    if (!luaLoaded) {
        AFB_WARNING("DISPATCH-INIT:WARNING (setenv CONTROL_CONFIG_PATH) No Config '%s-*.json' in '%s'", controlFile, dirList);
    }

    AFB_NOTICE("DISPATCH-INIT:SUCCES: Signal Dispatch Init");
    return 0;
}
