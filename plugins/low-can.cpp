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
	struct pluginCBT* pluginHandle;
	json_object *subscriptionBatch;
	allDoorsCtxT allDoorsCtx;
} lowCANCtxT;

void setDoor(doorT* aDoor, const char* eventName, int eventStatus)
{
	if(strcasestr(eventName, "door")) {aDoor->door = eventStatus;}
	else if(strcasestr(eventName, "window")) {aDoor->window = eventStatus;}
	else {AFB_WARNING("Unexpected behavior, this '%s' is not a door ! ", eventName);}
}

// Call at initialisation time
CTLP_ONLOAD(plugin, composerHandle)
{
	lowCANCtxT *pluginCtx= (lowCANCtxT*)calloc (1, sizeof(lowCANCtxT));

	pluginCtx->pluginHandle = (struct pluginCBT*)composerHandle;
	pluginCtx->subscriptionBatch = json_object_new_array();

	AFB_NOTICE ("Low-can plugin: label='%s' version='%s' info='%s'",
		plugin->label,
		plugin->version,
		plugin->info);

	return (void*)pluginCtx;
}

CTLP_CAPI (subscribeToLow, source, argsJ, eventJ, context) {
	lowCANCtxT *pluginCtx = (lowCANCtxT*)source->context;
	json_object* dependsArrayJ = nullptr, *subscribeArgsJ = nullptr, *subscribeFilterJ = nullptr, *responseJ = nullptr, *filterJ = nullptr;
	const char *id = nullptr, *event = nullptr, *unit = nullptr;
	double frequency = 0;
	int err = 0;

	if(eventJ)
	{
		err = wrap_json_unpack(eventJ, "{ss,s?s,s?o,s?s,s?F,s?o !}",
			"id", &id,
			"event", &event,
			"depends", &dependsArrayJ,
			"unit", &unit,
			"frequency", &frequency,
			"getSignalsArgs", &filterJ);
		if(err)
		{
			AFB_ERROR("Problem to unpack JSON object eventJ: %s",
			json_object_to_json_string(eventJ));
			return err;
		}

		if(frequency > 0 && !filterJ)
			{wrap_json_pack(&subscribeFilterJ, "{sf}", "frequency", frequency);}
		else
			{subscribeFilterJ = filterJ;}

		std::string eventStr = std::string(event);
		std::string lowEvent = eventStr.substr(eventStr.find("/")+1);
		err = wrap_json_pack(&subscribeArgsJ, "{ss, so*}",
		"event", lowEvent.c_str(),
		"filter", subscribeFilterJ);
		if(err)
		{
			AFB_ERROR("Error building subscription query object");
			return err;
		}

		json_object_array_add(pluginCtx->subscriptionBatch, subscribeArgsJ);
	}
	else
	{
		AFB_DEBUG("Calling subscribe with %s", json_object_to_json_string_ext(pluginCtx->subscriptionBatch, JSON_C_TO_STRING_PRETTY));
		err = afb_service_call_sync("low-can", "subscribe", pluginCtx->subscriptionBatch, &responseJ);
		if(err)
			{AFB_ERROR("Subscribe to '%s' responseJ:%s", json_object_to_json_string_ext(pluginCtx->subscriptionBatch, JSON_C_TO_STRING_PRETTY), json_object_to_json_string(responseJ));}
	}

	return err;
}

CTLP_CAPI (isOpen, source, argsJ, eventJ, context) {
	const char *eventName = nullptr;
	int eventStatus;
	double timestamp;
	lowCANCtxT *pluginCtx=(lowCANCtxT*)source->context;

	int err = wrap_json_unpack(eventJ, "{ss,sb,s?F}",
		"name", &eventName,
		"value", &eventStatus,
		"timestamp", &timestamp);
	if(err)
	{
		AFB_ERROR("Error parsing event %s", json_object_to_json_string(eventJ));
		return -1;
	}

	struct SignalValue value = {
		.hasBool = true, .boolVal = eventStatus,
		.hasNum = false, .numVal = 0,
		.hasStr = false, .strVal = std::string()
	};
	if(strcasestr(eventName, "front_left"))
	{
		pluginCtx->pluginHandle->setSignalValue(eventName,(long long int)timestamp, value);
		setDoor(&pluginCtx->allDoorsCtx.front_left, eventName, eventStatus);
	}
	else if(strcasestr(eventName, "front_right"))
	{
		pluginCtx->pluginHandle->setSignalValue(eventName,(long long int)timestamp, value);
		setDoor(&pluginCtx->allDoorsCtx.front_right, eventName, eventStatus);
	}
	else if(strcasestr(eventName, "rear_left"))
	{
		pluginCtx->pluginHandle->setSignalValue(eventName,(long long int)timestamp, value);
		setDoor(&pluginCtx->allDoorsCtx.rear_left, eventName, eventStatus);
	}
	else if(strcasestr(eventName, "rear_right"))
	{
		pluginCtx->pluginHandle->setSignalValue(eventName,(long long int)timestamp, value);
		setDoor(&pluginCtx->allDoorsCtx.rear_right, eventName, eventStatus);
	}
	else
	{
		AFB_WARNING("Unexpected behavior, this '%s' is it really a door ! ", json_object_to_json_string(eventJ));
		return -1;
	}

	AFB_DEBUG("This is the situation: source:%s, args:%s, event:%s,\n fld: %s, flw: %s, frd: %s, frw: %s, rld: %s, rlw: %s, rrd: %s, rrw: %s",
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

	return 0;
}

}
