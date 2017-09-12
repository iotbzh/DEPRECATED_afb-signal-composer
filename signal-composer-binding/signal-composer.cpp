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

#include "signal-composer.hpp"

CtlSectionT bindingApp::ctlSections_[] = {
	[0]={.key="sources" ,.label = "sources", .info=nullptr,
		 .loadCB=loadSources,
		 .handle=nullptr},
	[1]={.key="signals" , .label= "signals", .info=nullptr,
		 .loadCB=loadSignals,
		 .handle=nullptr},
	[2]={.key=nullptr, .label=nullptr, .info=nullptr,
		 .loadCB=nullptr,
		 .handle=nullptr}
};

bindingApp::bindingApp()
{}

void bindingApp::loadConfig(const std::string& filepath)
{
	ctlConfig_ = CtlConfigLoad(filepath.c_str(), ctlSections_);
}

bindingApp& bindingApp::instance()
{
	static bindingApp bApp;
	return bApp;
}

Source* bindingApp::getSource(const std::string& api)
{
	for(auto& source: sourcesList_)
	{
		if (source.api() == api)
			{return &source;}
	}
	return nullptr;
}

CtlActionT* bindingApp::convert2Action(const std::string& name, json_object* action)
{
	json_object *functionArgsJ = nullptr;
	const char* function;

	if(action &&
		wrap_json_unpack(action, "{ss,s?o !}", "function", &function, "args", &functionArgsJ))
	{
		std::string functionS = function;
		json_object* action = nullptr;
		ssize_t sep;
		if(functionS.find("lua") != std::string::npos)
		{
			wrap_json_pack(&action, "{ss,s?s,s?o,s?s,s?s,s?s,s?o !}",
			"label", name,
			"lua", function,
			"args", functionArgsJ);
		}
		else if( (sep = functionS.find_first_of("/")) != std::string::npos)
		{
			std::string api  = functionS.substr(0, sep);
			std::string verb = functionS.substr(sep);
			wrap_json_pack(&action, "{ss,s?s,s?o,s?s,s?s,s?s,s?o !}",
			"label", name,
			"api", api,
			"verb", verb,
			"args", functionArgsJ);
		}
		else
		{
			wrap_json_pack(&action, "{ss,s?s,s?o,s?s,s?s,s?s,s?o !}",
			"label", name,
			"callback", function,
			"args", functionArgsJ);
		}
	}
	return ActionLoad(action);
}

int bindingApp::loadOneSource(json_object* sourceJ)
{
	json_object *initJ = nullptr, *getSignalJ = nullptr;
	CtlActionT *initCtl, *getSignalCtl;
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

	initCtl = convert2Action("init", initJ);
	getSignalCtl =convert2Action("getSignal", getSignalJ);

	sourcesList_.push_back(Source(api, info, initCtl, getSignalCtl));

	return err;
}

int bindingApp::loadSources(CtlSectionT* section, json_object *sourcesJ)
{
	int err = 0;
	bindingApp& bApp = instance();
	// add the signal composer itself as source
	json_object *sigCompJ = nullptr;
	wrap_json_pack(&sigCompJ, "{ss,ss,so,so !}",
	"api", "signal-composer",
	"info", "Api on behalf the virtual signals are sent",
	"init", nullptr,
	"getSignal", nullptr);

	json_object_array_add(sourcesJ, sigCompJ);

	if (json_object_get_type(sourcesJ) == json_type_array)
	{
		int count = json_object_array_length(sourcesJ);

		for (int idx = 0; idx < count; idx++)
		{
			json_object *sourceJ = json_object_array_get_idx(sourcesJ, idx);
			err = bApp.loadOneSource(sourceJ);
			if (err) return err;
		}
	}
	else
	{
		if ((err = bApp.loadOneSource(sourcesJ))) return err;
		if ((err = bApp.loadOneSource(sigCompJ))) return err;
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
		AFB_ERROR("Missing something id|source|class|[unit]|[frequency]|onReceived in %s", json_object_get_string(signalJ));
		return err;
	}

	// Process sources JSON object
	// TODO: claneys, really needs to factorize JSON array processing
	if (json_object_get_type(sourcesJ) == json_type_array)
	{
		int count = json_object_array_length(sourcesJ);
		for(int i = 0; i < count; i++)
		{
			std::string sourceStr = json_object_get_string(sourcesJ);
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
	sClass = "state";

	// Get an action handler
	onReceivedCtl = convert2Action("onReceived", onReceivedJ);

	Source* src = getSource(api);
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

	return err;
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
