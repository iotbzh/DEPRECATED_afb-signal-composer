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
#include "signal-composer.hpp"

/// @brief callback for receiving message from low bindings. This will callback
/// an action defined in the configuration files depending on the event received
///
/// @param[in] event - event name
/// @param[in] object - eventual data that comes with the event
void onEvent(const char *event, json_object *object)
{
	AFB_DEBUG("Received event json: %s", json_object_to_json_string(object));
	bindingApp& bApp = bindingApp::instance();

	std::shared_ptr<Signal> sig = bApp.searchSignal(event);
	if(sig != nullptr)
	{
		sig->onReceivedCB(object);
	}
}

/// @brief entry point for client subscription request.
void subscribe(afb_req request)
{
	if(true)
		afb_req_success(request, NULL, NULL);
	else
		afb_req_fail(request, "error", NULL);
}

/// @brief entry point for client un-subscription request.
void unsubscribe(afb_req request)
{
	if(true)
		afb_req_success(request, NULL, NULL);
	else
		afb_req_fail(request, "error when unsubscribe", NULL);
}

/// @brief verb that loads JSON configuration (old SigComp.json file now)
void loadConf(afb_req request)
{
	json_object* args = afb_req_json(request), *fileJ;
	const char* filepath;

	wrap_json_unpack(args, "{s:s}", "filepath", &filepath);
	fileJ = json_object_from_file(filepath);

	if(bindingApp::instance().loadSignals(fileJ))
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

	std::vector<std::shared_ptr<Signal>> allSignals = bindingApp::instance().getAllSignals();
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

	ans = bindingApp::instance().getSignalValue(sig, options);

	if (ans)
		afb_req_success(request, ans, NULL);
	else
		afb_req_fail(request, "error", NULL);
}

int loadConf()
{
	int err = 0;
	const char* rootdir = strncat(GetBindingDirPath(), "/etc",
		sizeof(GetBindingDirPath()) - strlen(GetBindingDirPath()) -1);

	bindingApp& bApp = bindingApp::instance();
	err = bApp.loadConfig(rootdir);

	return err;
}

int execConf()
{
	bindingApp& bApp = bindingApp::instance();
	int err = 0;
	CtlConfigExec(bApp.ctlConfig());
	std::vector<std::shared_ptr<Signal>> allSignals = bApp.getAllSignals();
	ssize_t sigCount = allSignals.size();
	for( std::shared_ptr<Signal>& sig: allSignals)
	{
		sig->attachToSourceSignals(bApp);
	}

	for(auto& sig: allSignals)
	{
		if( (err += sig->recursionCheck()) )
		{
			AFB_ERROR("There is an infinite recursion loop in your signals definition. Root coming from signal: %s", sig->id().c_str());
			return err;
		}
	}

	bApp.execSubscription();

	AFB_DEBUG("Signal Composer Control configuration Done.\n signals=%d", (int)sigCount);

	return err;
}
