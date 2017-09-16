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

#include <string.h>

#include "signal-composer.hpp"

CtlSectionT bindingApp::ctlSections_[] = {
	[0]={.key="plugins" ,.label = "plugins", .info=nullptr,
		.loadCB=PluginConfig,
		.handle=nullptr},
	[1]={.key="sources" ,.label = "sources", .info=nullptr,
		 .loadCB=loadSourcesAPI,
		 .handle=nullptr},
	[2]={.key="signals" , .label= "signals", .info=nullptr,
		 .loadCB=loadSignals,
		 .handle=nullptr},
	[3]={.key=nullptr, .label=nullptr, .info=nullptr,
		 .loadCB=nullptr,
		 .handle=nullptr}
};

bindingApp::bindingApp()
{}

bindingApp::~bindingApp()
{
	free(ctlConfig_);
}

int bindingApp::loadConfig(const std::string& filepath)
{
	ctlConfig_ = CtlConfigLoad(filepath.c_str(), ctlSections_);
	if(ctlConfig_ != nullptr) {return 0;}
	return -1;
}

bindingApp& bindingApp::instance()
{
	static bindingApp bApp;
	return bApp;
}

SourceAPI* bindingApp::getSourceAPI(const std::string& api)
{
	for(auto& source: sourcesList_)
	{
		if (source.api() == api)
			{return &source;}
	}
	return nullptr;
}

CtlActionT* bindingApp::convert2Action(const std::string& name, json_object* actionJ)
{
	json_object *functionArgsJ = nullptr, *action = nullptr;
	char *function;
	const char *plugin;

	if(actionJ &&
		!wrap_json_unpack(actionJ, "{ss,s?s,s?o !}", "function", &function,
			"plugin", &plugin,
			"args", &functionArgsJ))
	{
		action = nullptr;
		if(::strcasestr(function, "lua"))
		{
			wrap_json_pack(&action, "{ss,ss,so*}",
			"label", name.c_str(),
			"lua", function,
			"args", functionArgsJ);
		}
		else if(strstr(function, "/"))
		{
			const char* api = strsep(&function, "/");
			//std::string api  = functionS.substr(0, sep);
			//std::string verb = functionS.substr(sep);
			wrap_json_pack(&action, "{ss,ss,ss,so*}",
			"label", name.c_str(),
			"api", api,
			"verb", function,
			"args", functionArgsJ);
		}
		else
		{
			json_object *callbackJ = nullptr;
			wrap_json_pack(&callbackJ, "{ss,ss,so*}",
				"plugin", plugin,
				"function", function,
				"args", functionArgsJ);
			wrap_json_pack(&action, "{ss,so}",
				"label", name.c_str(),
				"callback", callbackJ);
		}
	}
	if(action) {return ActionLoad(action);}
	return nullptr;
}

int bindingApp::loadOneSourceAPI(json_object* sourceJ)
{
	json_object *initJ = nullptr, *getSignalJ = nullptr;
	CtlActionT *initCtl = nullptr, *getSignalCtl = nullptr;
	const char *api, *info;

	int err = wrap_json_unpack(sourceJ, "{ss,s?s,s?o,s?o !}",
			"api", &api,
			"info", &info,
			"init", &initJ,
			"getSignal", &getSignalJ);
	if (err)
	{
		AFB_ERROR("Missing something api|[info]|[init]|[getSignal] in %s", json_object_get_string(sourceJ));
		return err;
	}

	if(ctlConfig_ && ctlConfig_->requireJ)
	{
		const char* requireS = json_object_to_json_string(ctlConfig_->requireJ);
		if(!strcasestr(requireS, api))
			{AFB_WARNING("Caution! You don't specify the API source as required in the metadata section. This API '%s' may not be initialized", api);}
	}

	if(initJ) {initCtl = convert2Action("init", initJ);}
	if(getSignalJ) {getSignalCtl = convert2Action("getSignal", getSignalJ);}

	sourcesList_.push_back(SourceAPI(api, info, initCtl, getSignalCtl));

	return err;
}

int bindingApp::loadSourcesAPI(CtlSectionT* section, json_object *sourcesJ)
{
	int err = 0;
	bindingApp& bApp = instance();
	json_object *sigCompJ = nullptr;

	// add the signal composer itself as source
	if(sourcesJ)
	{
		wrap_json_pack(&sigCompJ, "{ss,ss}",
		"api", "signal-composer",
		"info", "Api on behalf the virtual signals are sent");

		json_object_array_add(sourcesJ, sigCompJ);

		if (json_object_get_type(sourcesJ) == json_type_array)
		{
			int count = json_object_array_length(sourcesJ);

			for (int idx = 0; idx < count; idx++)
			{
				json_object *sourceJ = json_object_array_get_idx(sourcesJ, idx);
				err = bApp.loadOneSourceAPI(sourceJ);
				if (err) return err;
			}
		}
		else
		{
			if ((err = bApp.loadOneSourceAPI(sourcesJ))) return err;
			if (sigCompJ && (err = bApp.loadOneSourceAPI(sigCompJ))) return err;
		}
}

	return err;
}

int bindingApp::loadOneSignal(json_object* signalJ)
{
	json_object *onReceivedJ = nullptr, *sourcesJ = nullptr;
	CtlActionT* onReceivedCtl;
	const char *id = nullptr,
			   *sClass = nullptr,
			   *unit = nullptr;
	double frequency;
	std::string api;
	std::vector<std::string> sourcesV;
	ssize_t sep;

	int err = wrap_json_unpack(signalJ, "{ss,so,s?s,s?s,s?F,s?o !}",
			"id", &id,
			"source", &sourcesJ,
			"class", &sClass,
			"unit", &unit,
			"frequency", &frequency,
			"onReceived", &onReceivedJ);
	if (err)
	{
		AFB_ERROR("Missing something id|source|[class]|[unit]|[frequency]|[onReceived] in %s", json_object_get_string(signalJ));
		return err;
	}

	// Process sources JSON object
	if (json_object_get_type(sourcesJ) == json_type_array)
	{
		int count = json_object_array_length(sourcesJ);
		for(int i = 0; i < count; i++)
		{
			std::string sourceStr = json_object_get_string(json_object_array_get_idx(sourcesJ, i));
			if( (sep = sourceStr.find("/")) != std::string::npos)
			{
				AFB_ERROR("Signal composition needs to use signal 'id', don't use full low level signal name");
				return -1;
			}
			sourcesV.push_back(sourceStr);
		}
	}
	else
	{
		std::string sourceStr = json_object_get_string(sourcesJ);
		if( (sep = sourceStr.find("/")) != std::string::npos)
		{
			api = sourceStr.substr(0, sep);
		}
		sourcesV.push_back(sourceStr);
	}

	// Set default sClass is not specified
	sClass = !sClass ? "state" : sClass;
	unit = !unit ? "" : unit;

	// Get an action handler
	onReceivedCtl = convert2Action("onReceived", onReceivedJ);

	SourceAPI* src = getSourceAPI(api) ? getSourceAPI(api):getSourceAPI("signal-composer");
	if( src != nullptr)
		{src->addSignal(id, sourcesV, sClass, unit, frequency, onReceivedCtl);}
	else
		{err = -1;}

	return err;
}

int bindingApp::loadSignals(CtlSectionT* section, json_object *signalsJ)
{
	int err = 0;
	bindingApp& bApp = instance();

	if(signalsJ)
	{
		if (json_object_get_type(signalsJ) == json_type_array)
		{
			int count = json_object_array_length(signalsJ);

			for (int idx = 0; idx < count; idx++)
			{
				json_object *signalJ = json_object_array_get_idx(signalsJ, idx);
				err = bApp.loadOneSignal(signalJ);
				if (err) return err;
			}
		}
		else
			{err = bApp.loadOneSignal(signalsJ);}
	}

	return err;
}

std::shared_ptr<Signal> bindingApp::searchSignal(const std::string& aName)
{
	std::string api;
	size_t sep = aName.find_first_of("/");
	if(sep != std::string::npos)
	{
		api = aName.substr(0, sep);
		SourceAPI* source = getSourceAPI(api);
		return source->searchSignal(aName);
	}
	else
	{
		std::vector<std::shared_ptr<Signal>> allSignals = getAllSignals();
		for (std::shared_ptr<Signal>& sig : allSignals)
		{
			if(*sig == aName)
				{return sig;}
		}
	}
	return nullptr;
}

std::vector<std::shared_ptr<Signal>> bindingApp::getAllSignals()
{
	std::vector<std::shared_ptr<Signal>> allSignals;
	for( auto& source : sourcesList_)
	{
		std::vector<std::shared_ptr<Signal>> srcSignals = source.getSignals();
		allSignals.insert(allSignals.end(), srcSignals.begin(), srcSignals.end());
	}

	return allSignals;
}

CtlConfigT* bindingApp::ctlConfig()
{
	return ctlConfig_;
}

int bindingApp::execSubscription() const
{
	int err = 0;
	for(const SourceAPI& srcAPI: sourcesList_)
	{
		if (srcAPI.api() != std::string(ctlConfig_->api))
		{
			err += srcAPI.makeSubscription();
		}
	}
	return err;
}

json_object* bindingApp::getSignalValue(const std::string& sig, json_object* options)
{
	const char **opts = nullptr;
	json_object *response = nullptr;

	wrap_json_unpack(options, "{s?s?s?s?!}",
		&opts[0],
		&opts[1],
		&opts[2],
		&opts[3]);

	std::shared_ptr<Signal> sigP = searchSignal(sig);
	wrap_json_pack(&response, "{ss}",
		"signal", sigP->id().c_str());
	for(int idx=0; idx < sizeof(opts); idx++)
	{
		bool avg = false, min = false, max = false, last = false;
		if (strcasestr(opts[idx], "average") && !avg)
		{
			avg = true;
			double value = sigP->average();
			json_object_object_add(response, "average",
				json_object_new_double(value));
		}
		if (strcasestr(opts[idx], "min") && !min)
		{
			min = true;
			double value = sigP->minimum();
			json_object_object_add(response, "min",
				json_object_new_double(value));
		}
		if (strcasestr(opts[idx], "max") && !max)
		{
			max = true;
			double value = sigP->maximum();
			json_object_object_add(response, "max",
				json_object_new_double(value));
		}
		if (strcasestr(opts[idx], "last") && !last)
		{
			last = true;
			double value = sigP->last();
			json_object_object_add(response, "last",
				json_object_new_double(value));
		}
	}

	if (!opts)
	{
		double value = sigP->last();
		json_object_object_add(response, "last",
			json_object_new_double(value));
	}

	return response;
}
