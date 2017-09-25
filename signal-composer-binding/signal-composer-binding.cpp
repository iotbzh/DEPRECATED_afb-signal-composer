/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
 * Author "Romain Forlot" <romain.forlot@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <string>
#include <string.h>
// afb-utilities
#include <wrap-json.h>
#include <filescan-utils.h>

#include "signal-composer-binding.hpp"
#include "signal-composer-apidef.h"
#include "clientApp.hpp"

/// @brief callback for receiving message from low bindings. This will callback
/// an action defined in the configuration files depending on the event received
///
/// @param[in] event - event name
/// @param[in] object - eventual data that comes with the event
void onEvent(const char *event, json_object *object)
{
	AFB_DEBUG("Received event json: %s", json_object_to_json_string(object));
	Composer& composer = Composer::instance();

	std::vector<Signal*> signals = composer.searchSignals(event);
	if(!signals.empty())
	{
		for(auto& sig: signals)
		{
			sig->onReceivedCB(object);
		}
	}
}

static int one_subscribe_unsubscribe(struct afb_req request,
	bool subscribe,
	const std::string& event,
	json_object* args,
	clientAppCtx* cContext)
{
	int err = 0;
	std::vector<Signal*> signals = Composer::instance().searchSignals(event);

	cContext->appendSignals(signals);
	if(subscribe)
		{err = cContext->makeSubscription(request);}
	else
		{err = cContext->makeUnsubscription(request);}

	return err;
}

static int subscribe_unsubscribe(struct afb_req request,
	bool subscribe,
	json_object* args,
	clientAppCtx* cContext)
{
	int rc = 0;
	json_object *event = nullptr;
	if (args == NULL || !json_object_object_get_ex(args, "event", &event))
	{
		rc = one_subscribe_unsubscribe(request, subscribe, "*", args, cContext);
	}
	else if (json_object_get_type(event) == json_type_string)
	{
		rc = one_subscribe_unsubscribe(request, subscribe, json_object_get_string(event), args, cContext);
	}
	else if (json_object_get_type(event) == json_type_array)
	{
		for (int i = 0 ; i < json_object_array_length(event) ; i++)
		{
			json_object *x = json_object_array_get_idx(event, i);
			rc += one_subscribe_unsubscribe(request, subscribe, json_object_get_string(x), args, cContext);
		}
	}
	else {rc = -1;}

	return rc;
}

/// @brief entry point for client subscription request.
static void do_subscribe_unsubscribe(afb_req request, bool subscribe, clientAppCtx* cContext)
{
	int rc = 0;
	json_object *oneArg = nullptr, *args = afb_req_json(request);
	if (json_object_get_type(args) == json_type_array)
	{
		for (int i = 0 ; i < json_object_array_length(args); i++)
		{
			oneArg = json_object_array_get_idx(args, i);
			rc += subscribe_unsubscribe(request, subscribe, oneArg, cContext);
		}
	}
	else
	{
		rc = subscribe_unsubscribe(request, subscribe, args, cContext);
	}

	if(rc >= 0)
		afb_req_success(request, NULL, NULL);
	else
		afb_req_fail(request, "error", NULL);
}

/// @brief entry point for client un-subscription request.
void subscribe(afb_req request)
{
	clientAppCtx *cContext = reinterpret_cast<clientAppCtx*>(afb_req_context_make(request, 0, Composer::createContext, Composer::destroyContext, nullptr));

	do_subscribe_unsubscribe(request, true, cContext);
}

/// @brief entry point for client un-subscription request.
void unsubscribe(afb_req request)
{
	clientAppCtx *cContext = reinterpret_cast<clientAppCtx*>(afb_req_context_make(request, 0, Composer::createContext, Composer::destroyContext, nullptr));

	do_subscribe_unsubscribe(request, false, cContext);
}

/// @brief verb that loads JSON configuration (old SigComp.json file now)
void loadConf(afb_req request)
{
	json_object* args = afb_req_json(request), *fileJ;
	const char* filepath;

	wrap_json_unpack(args, "{s:s}", "filepath", &filepath);
	fileJ = json_object_from_file(filepath);

	if(Composer::instance().loadSignals(fileJ))
		{afb_req_fail_f(request, "Loading configuration or subscription error", "Error code: -1");}
	else
	{

		afb_req_success(request, NULL, NULL);
	}
}

/// @brief entry point to list available signals
void list(afb_req request)
{
	struct json_object *allSignalsJ = json_object_new_array();

	std::vector<Signal*> allSignals = Composer::instance().getAllSignals();
	for(auto& sig: allSignals)
		{json_object_array_add(allSignalsJ, sig->toJSON());}

	if(json_object_array_length(allSignalsJ) && !execConf())
	{
		afb_req_success(request, allSignalsJ, NULL);
	}
	else
	{
		afb_req_fail(request, "error", "No Signals recorded so far");
	}
}

/// @brief entry point for get requests.
void get(struct afb_req request)
{
	int err = 0;
	struct json_object* args = afb_req_json(request), *ans = nullptr,
	*options = nullptr;
	const char* sig;

	// Process about Raw CAN message on CAN bus directly
	err = wrap_json_unpack(args, "{ss,s?o!}", "signals", &sig,
			"options", &options);
	if(err)
	{
		AFB_ERROR("Can't process your request '%s'. Valid arguments are: string for 'signal' and JSON object for 'options' (optionnal)", json_object_to_json_string_ext(args, JSON_C_TO_STRING_PRETTY));
		afb_req_fail(request, "error", NULL);
		return;
	}

	ans = Composer::instance().getsignalValue(sig, options);

	if (json_object_array_length(ans))
		afb_req_success(request, ans, NULL);
	else
		afb_req_fail(request, "error", "No signals found.");

}

int loadConf()
{
	int err = 0;
	std::string bindingDirPath = GetBindingDirPath();
	std::string rootdir = bindingDirPath + "/etc";

	Composer& composer = Composer::instance();
	err = composer.loadConfig(rootdir.c_str());

	return err;
}

int execConf()
{
	Composer& composer = Composer::instance();
	int err = 0;
	CtlConfigExec(composer.ctlConfig());
	std::vector<Signal*> allSignals = composer.getAllSignals();
	ssize_t sigCount = allSignals.size();
	for( Signal*& sig: allSignals)
	{
		sig->attachToSourceSignals(composer);
	}

	for(auto& sig: allSignals)
	{
		if( (err += sig->initialRecursionCheck()) )
		{
			AFB_ERROR("There is an infinite recursion loop in your signals definition. Root coming from signal: %s", sig->id().c_str());
			return err;
		}
	}

	composer.execSignalsSubscription();

	AFB_DEBUG("Signal Composer Control configuration Done.\n signals=%d", (int)sigCount);

	return err;
}
