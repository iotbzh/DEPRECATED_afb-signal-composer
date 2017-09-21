/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Romain Forlot <romain.forlot@iot.bzh>
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
 *
*/

#define _GNU_SOURCE  // needed for vasprintf

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <systemd/sd-event.h>
#include <json-c/json_object.h>
#include <stdbool.h>
#include <string.h>

#include "ctl-config.h"
#include "wrap-json.h"

CTLP_LUALOAD
CTLP_REGISTER("lua2c-interface");

typedef struct {
	struct pluginCBT* pluginHandle;
} lowCANCtxT;

// Call at initialisation time
CTLP_ONLOAD(plugin, handle) {
	lowCANCtxT *pluginCtx= (lowCANCtxT*)calloc (1, sizeof(lowCANCtxT));
	pluginCtx->pluginHandle = (struct pluginCBT*)handle;

	AFB_NOTICE ("Low-can plugin: label='%s' version='%s' info='%s'",
		plugin->label,
		plugin->version,
		plugin->info);

	return (void*)pluginCtx;
}

CTLP_LUA2C (ssetSignalValue, label, argsJ)
{
	AFB_NOTICE("label: %s, argsJ: %s", label, json_object_to_json_string(argsJ));
	return 0;
}
