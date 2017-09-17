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


#ifndef _CTL_PLUGIN_INCLUDE_
#define _CTL_PLUGIN_INCLUDE_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <json-c/json.h>

#ifndef CTL_PLUGIN_MAGIC
  #define CTL_PLUGIN_MAGIC 852369147
#endif

#ifndef STATIC
  #define STATIC static
#endif

#ifndef UNUSED_ARG
  #define UNUSED_ARG(x) UNUSED_ ## x __attribute__((__unused__))
  #define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#endif

typedef struct {
    char *label;
    char *info;
    afb_req request;
    void *context;
} CtlSourceT;


typedef struct {
  long  magic;
  const char *label;
  void *handle;
} CtlPluginMagicT;


typedef struct {
    const char *label;
    const char *info;
    const char *version;
    void *context;
    void *dlHandle;
} CtlPluginT;

typedef void*(*DispatchPluginInstallCbT)(CtlPluginT *plugin, void* handle);


#define MACRO_STR_VALUE(arg) #arg
#define CTLP_REGISTER(pluglabel) CtlPluginMagicT CtlPluginMagic={.magic=CTL_PLUGIN_MAGIC,.label=pluglabel,.handle=NULL}; struct afb_binding_data_v2;
#define CTLP_ONLOAD(plugin, handle) void* CtlPluginOnload(CtlPluginT *plugin, void* handle)
#define CTLP_CAPI(funcname, source, argsJ, queryJ, context) int funcname(CtlSourceT *source, json_object* argsJ, json_object* queryJ, void* context)

#endif /* _CTL_PLUGIN_INCLUDE_ */
