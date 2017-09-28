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

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <systemd/sd-event.h>
#include <json-c/json_object.h>
#include <stdbool.h>
#include <string.h>

#include "signal-composer.hpp"
#include "ctl-config.h"
#include "wrap-json.h"

extern "C"
{

CTLP_LUALOAD
CTLP_REGISTER("lua2c-interface");

typedef struct CtxS {
	struct signalCBT* pluginHandle;
} CtxT;

static CtxT *pluginCtx = NULL;

// Call at initialisation time
CTLP_ONLOAD(plugin, handle) {
	pluginCtx = (CtxT*)calloc (1, sizeof(CtxT));
	pluginCtx->pluginHandle = (struct signalCBT*)handle;

	AFB_NOTICE ("Low-can plugin: label='%s' version='%s' info='%s'",
		plugin->label,
		plugin->version,
		plugin->info);

	return (void*)pluginCtx;
}

CTLP_LUA2C (setSignalValueWrap, label, argsJ)
{
	const char* name = nullptr;
	double resultNum;
	uint64_t timestamp;
	if(! wrap_json_unpack(argsJ, "{ss, sF, sI? !}",
		"name", &name,
		"value", &resultNum,
		"timestamp", &timestamp))
	{
		AFB_ERROR("Fail to set value for label: %s, argsJ: %s", label, json_object_to_json_string(argsJ));
		return -1;
	}
	struct signalValue result = {0,0,1, resultNum, 0, ""};
	pluginCtx->pluginHandle->setSignalValue(name, timestamp*MICRO, result);
	return 0;
}

// extern "C" closure
}
