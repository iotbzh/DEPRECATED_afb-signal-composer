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
CTLP_LUA_REGISTER("builtin");

static struct signalCBT* pluginCtx = NULL;

// Call at initialisation time
CTLP_ONLOAD(plugin, handle) {
	pluginCtx = (struct signalCBT*)calloc (1, sizeof(struct signalCBT));
	pluginCtx = (struct signalCBT*)handle;

	AFB_NOTICE ("Low-can plugin: label='%s' info='%s'",
		plugin->uid,
		plugin->info);

	return (void*)pluginCtx;
}

CTLP_CAPI (defaultOnReceived, source, argsJ, eventJ)
{
	struct signalCBT* ctx = (struct signalCBT*)source->context;
	AFB_NOTICE("source: %s argj: %s, eventJ %s", source->uid,
		json_object_to_json_string(argsJ),
		json_object_to_json_string(eventJ));
	void* sig = ctx->aSignal;

	json_object* valueJ = nullptr;
	json_object* timestampJ = nullptr;
	double value = 0;
	uint64_t timestamp = 0;
	if(json_object_object_get_ex(eventJ, "value", &valueJ))
		{value = json_object_get_double(valueJ);}
	if(json_object_object_get_ex(eventJ, "timestamp", &timestampJ))
		{timestamp = json_object_get_int64(timestampJ);}

	struct signalValue v = value;
	ctx->setSignalValue(sig, timestamp, v);
	return 0;
}

CTLP_LUA2C (setSignalValueWrap, source, argsJ, responseJ)
{
	const char* name = nullptr;
	double resultNum;
	uint64_t timestamp;
	if(! wrap_json_unpack(argsJ, "{ss, sF, sI? !}",
		"name", &name,
		"value", &resultNum,
		"timestamp", &timestamp))
	{
		AFB_ERROR("Fail to set value for uid: %s, argsJ: %s", source->uid, json_object_to_json_string(argsJ));
		return -1;
	}
	struct signalValue result = resultNum;
	pluginCtx->searchNsetSignalValue(name, timestamp*MICRO, result);
	return 0;
}

// extern "C" closure
}
