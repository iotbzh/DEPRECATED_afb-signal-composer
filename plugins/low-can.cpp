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

#include "ctl-plugin.h"
#include "wrap-json.h"


#include "signal-composer.hpp"

extern "C"
{

CTLP_REGISTER("low-can");

typedef struct {
	bool door;
	bool window;
} doorT;

typedef struct {
	doorT front_left;
	doorT front_right;
	doorT rear_left;
	doorT rear_right;
} allDoorsCtxT;

typedef struct {
	bindingApp* bApp;
	allDoorsCtxT allDoorsCtx;
} lowCANCtxT;

// Call at initialisation time
CTLP_ONLOAD(plugin, bAppHandle)
{
	lowCANCtxT *pluginCtx= (lowCANCtxT*)calloc (1, sizeof(lowCANCtxT));
	allDoorsCtxT allDoorsCtx = allDoorsCtxT();
	::memset(&allDoorsCtx, 0, sizeof(allDoorsCtxT));
	pluginCtx->allDoorsCtx = allDoorsCtx;

	pluginCtx->bApp = (bindingApp*)bAppHandle;

	AFB_NOTICE ("Low-can plugin: label='%s' version='%s' info='%s'",
		plugin->label,
		plugin->version,
		plugin->info);

	return (void*)pluginCtx;
}

CTLP_CAPI (subscribeToLow, source, argsJ, eventJ, context) {
	json_object* signalArrayJ = NULL, *subscribeArgsJ = NULL, *subscribeFilterJ = NULL, *responseJ = NULL;
	const char* unit = NULL;
	double frequency = 0;
	int err = 0;

	err = wrap_json_unpack(eventJ, "{so,s?s,s?F !}",
		"signal", &signalArrayJ,
		"unit", &unit,
		"frequency", &frequency);
	if(err)
	{
		AFB_ERROR("Problem to unpack JSON object eventJ: %s",
		json_object_to_json_string(eventJ));
		return err;
	}

	if(frequency > 0)
	{
		wrap_json_pack(&subscribeFilterJ, "{sf}", "frequency", frequency);
	}

	for (int idx = 0; idx < json_object_array_length(signalArrayJ); idx++)
	{
		json_object* aSignalJ = json_object_array_get_idx(signalArrayJ, idx);
		err = wrap_json_pack(&subscribeArgsJ, "{ss, so*}",
		"event", json_object_get_string(aSignalJ),
		"filter", subscribeFilterJ);
		if(err)
		{
			AFB_ERROR("Error building subscription query object");
			return err;
		}
		AFB_DEBUG("Calling subscribe with %s", json_object_to_json_string_ext(subscribeArgsJ, JSON_C_TO_STRING_PRETTY));
		err = afb_service_call_sync("low-can", "subscribe", subscribeArgsJ, &responseJ);
		if(err)
		{
			AFB_ERROR("Can't find api 'low-can'");
			return err;
		}
	}

	return err;
}

CTLP_CAPI (isOpen, source, argsJ, eventJ, context) {
	const char* eventName;
	bool eventStatus;
	double timestamp;
	lowCANCtxT *pluginCtx=(lowCANCtxT*)context;

	AFB_NOTICE("This is the situation: source:%s, args:%s, event:%s,\n fld: %s, flw: %s, frd: %s, frw: %s, rld: %s, rlw: %s, rrd: %s, rrw: %s",
		source->label,
		json_object_to_json_string(argsJ),
		json_object_to_json_string(eventJ),
		pluginCtx->allDoorsCtx.front_left.door ? "true":"false",
		pluginCtx->allDoorsCtx.front_left.window ? "true":"false",
		pluginCtx->allDoorsCtx.front_right.door ? "true":"false",
		pluginCtx->allDoorsCtx.front_right.window ? "true":"false",
		pluginCtx->allDoorsCtx.rear_left.door ? "true":"false",
		pluginCtx->allDoorsCtx.rear_left.window ? "true":"false",
		pluginCtx->allDoorsCtx.rear_right.door ? "true":"false",
		pluginCtx->allDoorsCtx.rear_right.window ? "true":"false"
	);

	int err = wrap_json_unpack(eventJ, "{ss,s?b,s?F}",
		"name", &eventName,
		"value", &eventStatus,
		"timestamp", &timestamp);
	if(err)
	{
		AFB_ERROR("Error parsing event %s", json_object_to_json_string(eventJ));
		return -1;
	}

	if(strcasestr(eventName, "front_left"))
	{
		if(strcasestr(eventName, "door")) {pluginCtx->allDoorsCtx.front_left.door = eventStatus;}
		else if(strcasestr(eventName, "window")) {pluginCtx->allDoorsCtx.front_left.window = eventStatus;}
		else {AFB_WARNING("Unexpected behavior, this '%s' is not a door ! ", json_object_to_json_string(eventJ));}
	}
	else if(strcasestr(eventName, "front_right"))
	{
		if(strcasestr(eventName, "door")) {pluginCtx->allDoorsCtx.front_right.door = eventStatus;}
		else if(strcasestr(eventName, "window")) {pluginCtx->allDoorsCtx.front_right.window = eventStatus;}
		else {AFB_WARNING("Unexpected behavior, this '%s' is not a door ! ", json_object_to_json_string(eventJ));}
	}
	else if(strcasestr(eventName, "rear_left"))
	{
		if(strcasestr(eventName, "door")) {pluginCtx->allDoorsCtx.rear_left.door = eventStatus;}
		else if(strcasestr(eventName, "window")) {pluginCtx->allDoorsCtx.rear_left.window = eventStatus;}
		else {AFB_WARNING("Unexpected behavior, this '%s' is not a door ! ", json_object_to_json_string(eventJ));}
	}
	else if(strcasestr(eventName, "rear_right"))
	{
		if(strcasestr(eventName, "door")) {pluginCtx->allDoorsCtx.rear_right.door = eventStatus;}
		else if(strcasestr(eventName, "window")) {pluginCtx->allDoorsCtx.rear_right.window = eventStatus;}
		else {AFB_WARNING("Unexpected behavior, this '%s' is not a door ! ", json_object_to_json_string(eventJ));}
	}
	else {AFB_WARNING("Unexpected behavior, this '%s' is not a door ! ", json_object_to_json_string(eventJ));}

	return 0;
}
}
