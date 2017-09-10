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

#ifndef _CTL_CONFIG_INCLUDE_
#define _CTL_CONFIG_INCLUDE_

#define _GNU_SOURCE
#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <json-c/json.h>
#include <filescan-utils.h>
#include <wrap-json.h>

#include "ctl-plugin.h"

#ifndef CONTROL_MAXPATH_LEN
  #define CONTROL_MAXPATH_LEN 255
#endif

#ifndef CONTROL_CONFIG_PRE
  #define CONTROL_CONFIG_PRE "onload"
#endif

#ifndef CTL_PLUGIN_EXT
  #define CTL_PLUGIN_EXT ".ctlso"
#endif



typedef enum {
    CTL_TYPE_NONE=0,
    CTL_TYPE_API,
    CTL_TYPE_CB,
    CTL_TYPE_LUA,
} CtlActionTypeT;


typedef struct {
    CtlActionTypeT type;
    const char* api;
    const char* call;
    json_object *argsJ;
    int (*actionCB)(CtlSourceT *source, json_object *argsJ, json_object *queryJ);
    CtlSourceT source;
} CtlActionT;

typedef struct {
    const char* label;
    const char *info;
    CtlActionT *actions;
} DispatchHandleT;

typedef struct ConfigSectionS {
  const char *key;
  const char *label;
  const char *info;
  int (*loadCB)(struct ConfigSectionS *section, json_object *sectionJ);
  void *handle;  
} CtlSectionT;

typedef struct {
    const char* api;
    const char* label;
    const char *info;
    const char *version;
    json_object *requireJ;
    CtlSectionT *sections;
} CtlConfigT;


#ifdef CONTROL_SUPPORT_LUA
  #include "ctl-lua.h"
#else
 typedef void* Lua2cWrapperT;
#endif


// ctl-action.c
PUBLIC CtlActionT *ActionLoad(json_object *actionsJ);
PUBLIC int ActionExecOne(CtlActionT* action, json_object *queryJ);
PUBLIC int ActionLoadOne(CtlActionT *action, json_object *actionJ);

// ctl-config.c
PUBLIC CtlConfigT *CtlConfigLoad(const char* filepath, CtlSectionT *sections);
PUBLIC int CtlConfigExec(CtlConfigT *ctlConfig);

// ctl-onload.c
PUBLIC int OnloadConfig(CtlSectionT *section, json_object *actionsJ);


// ctl-plugin.c
PUBLIC int PluginConfig(CtlSectionT *section, json_object *pluginsJ);
PUBLIC int PluginGetCB (CtlActionT *action , json_object *callbackJ);


#endif /* _CTL_CONFIG_INCLUDE_ */