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

#include <uuid.h>
#include <string.h>
#include <fnmatch.h>

#include "clientApp.hpp"

extern "C" void setSignalValueHandle(const char* aName, uint64_t timestamp, struct signalValue value)
{
	std::vector<std::shared_ptr<Signal>> signals = Composer::instance().searchSignals(aName);
	if(!signals.empty())
	{
		for(auto& sig: signals)
			{sig->set(timestamp, value);}
	}
}

bool startsWith(const std::string& str, const std::string& pattern)
{
	size_t sep;
	if( (sep = str.find(pattern)) != std::string::npos && !sep)
		{return true;}
	return false;
}

static struct signalCBT pluginHandle = {
	.setSignalValue = setSignalValueHandle,
};

CtlSectionT Composer::ctlSections_[] = {
	[0]={.key="plugins" , .label = "plugins", .info=nullptr,
		.loadCB=PluginConfig,
		.handle=&pluginHandle},
	[1]={.key="sources" , .label = "sources", .info=nullptr,
		 .loadCB=loadSourcesAPI,
		 .handle=nullptr},
	[2]={.key="signals" , .label= "signals", .info=nullptr,
		 .loadCB=loadSignals,
		 .handle=nullptr},
	[3]={.key=nullptr, .label=nullptr, .info=nullptr,
		 .loadCB=nullptr,
		 .handle=nullptr}
};

///////////////////////////////////////////////////////////////////////////////
//                             PRIVATE METHODS                               //
///////////////////////////////////////////////////////////////////////////////

Composer::Composer()
{}

Composer::~Composer()
{
	free(ctlConfig_);
}

CtlActionT* Composer::convert2Action(const std::string& name, json_object* actionJ)
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
		if(startsWith(function, "lua://"))
		{
			std::string fName = std::string(function).substr(6);
			wrap_json_pack(&action, "{ss,ss,so*}",
			"label", name.c_str(),
			"lua", fName.c_str(),
			"args", functionArgsJ);
		}
		else if(startsWith(function, "api://"))
		{
			std::string uri = std::string(function).substr(6);
			std::vector<std::string> uriV = Composer::parseURI(uri);
			if(uriV.size() != 2)
			{
				AFB_ERROR("Miss something in uri either plugin name or function name. Uri has to be like: api://<plugin-name>/<function-name>");
				return nullptr;
			}
			wrap_json_pack(&action, "{ss,ss,ss,so*}",
			"label", name.c_str(),
			"api", uriV[0].c_str(),
			"verb", uriV[1].c_str(),
			"args", functionArgsJ);
		}
		else if(startsWith(function, "plugin://"))
		{
			std::string uri = std::string(function).substr(9);
			std::vector<std::string> uriV = Composer::parseURI(uri);
			if(uriV.size() != 2)
			{
				AFB_ERROR("Miss something in uri either plugin name or function name. Uri has to be like: plugin://<plugin-name>/<function-name>");
				return nullptr;
			}
			json_object *callbackJ = nullptr;
			wrap_json_pack(&callbackJ, "{ss,ss,so*}",
				"plugin", uriV[0].c_str(),
				"function", uriV[1].c_str(),
				"args", functionArgsJ);
			wrap_json_pack(&action, "{ss,so}",
				"label", name.c_str(),
				"callback", callbackJ);
		}
		else
		{
			AFB_ERROR("Wrong function uri specified. You have to specified 'lua://', 'plugin://' or 'api://'. (%s)", function);
			return nullptr;
		}
	}
	if(action) {return ActionLoad(action);}
	return nullptr;
}

int Composer::loadOneSourceAPI(json_object* sourceJ)
{
	json_object *initJ = nullptr,
				*getSignalsJ = nullptr,
				*onReceivedJ = nullptr;
	CtlActionT  *initCtl = nullptr,
				*getSignalsCtl = nullptr,
				*onReceivedCtl = nullptr;
	const char *api, *info;
	int retention = 0;

	int err = wrap_json_unpack(sourceJ, "{ss,s?s,s?o,s?o,s?o, s?i !}",
			"api", &api,
			"info", &info,
			"init", &initJ,
			"getSignals", &getSignalsJ,
			// Signals field to make signals conf by sources
			"onReceived", &onReceivedJ,
			"retention", &retention);
	if (err)
	{
		AFB_ERROR("Missing something api|[info]|[init]|[getSignals] in %s", json_object_get_string(sourceJ));
		return err;
	}

	if(ctlConfig_ && ctlConfig_->requireJ)
	{
		const char* requireS = json_object_to_json_string(ctlConfig_->requireJ);
		if(!strcasestr(requireS, api))
			{AFB_WARNING("Caution! You don't specify the API source as required in the metadata section. This API '%s' may not be initialized", api);}
	}

	initCtl = initJ ? convert2Action("init", initJ) : nullptr;

	// Define default action to take to subscibe souce's signals if none
	// defined.
	if(!getSignalsJ)
	{
		std::string function = "api://" + std::string(api) + "/subscribe";
		wrap_json_pack(&getSignalsJ, "{ss}", "function", function.c_str());
	}
	getSignalsCtl = convert2Action("getSignals", getSignalsJ);

	onReceivedCtl = onReceivedJ ? convert2Action("onReceived", onReceivedJ) : nullptr;

	sourcesListV_.push_back(std::make_shared<SourceAPI>(api, info, initCtl, getSignalsCtl, onReceivedCtl, retention));

	return err;
}

int Composer::loadSourcesAPI(CtlSectionT* section, json_object *sourcesJ)
{
	int err = 0;
	Composer& composer = instance();

	if(sourcesJ)
	{
		json_object *sigCompJ = nullptr;
		// add the signal composer itself as source
		wrap_json_pack(&sigCompJ, "{ss,ss}",
		"api", afbBindingV2.api,
		"info", "Api on behalf the virtual signals are sent");

		json_object_array_add(sourcesJ, sigCompJ);

		if (json_object_get_type(sourcesJ) == json_type_array)
		{
			int count = json_object_array_length(sourcesJ);

			for (int idx = 0; idx < count; idx++)
			{
				json_object *sourceJ = json_object_array_get_idx(sourcesJ, idx);
				err = composer.loadOneSourceAPI(sourceJ);
				if (err) return err;
			}
		}
		else
		{
			if ((err = composer.loadOneSourceAPI(sourcesJ))) return err;
			if (sigCompJ && (err = composer.loadOneSourceAPI(sigCompJ))) return err;
		}
	}
	else
		{err += Composer::instance().initSourcesAPI();}

	return err;
}

int Composer::loadOneSignal(json_object* signalJ)
{
	json_object *onReceivedJ = nullptr,
				*dependsJ = nullptr,
				*getSignalsArgs = nullptr;
	CtlActionT* onReceivedCtl;
	const char *id = nullptr,
			   *event = nullptr,
			   *unit = nullptr;
	int retention;
	double frequency=0.0;
	std::vector<std::string> dependsV;
	ssize_t sep;
	std::shared_ptr<SourceAPI> src = nullptr;

	int err = wrap_json_unpack(signalJ, "{ss,s?s,s?o,s?o,s?F,s?s,s?F,s?o !}",
			"id", &id,
			"event", &event,
			"depends", &dependsJ,
			"getSignalsArgs", &getSignalsArgs,
			"retention", &retention,
			"unit", &unit,
			"frequency", &frequency,
			"onReceived", &onReceivedJ);
	if (err)
	{
		AFB_ERROR("Missing something id|[event|depends]|[getSignalsArgs]|[retention]|[unit]|[frequency]|[onReceived] in %s", json_object_get_string(signalJ));
		return err;
	}

	// event or depends field manadatory
	if( (!event && !dependsJ) || (event && dependsJ) )
	{
		AFB_ERROR("Missing something id|[event|depends]|[getSignalsArgs]|[retention]|[unit]|[frequency]|[onReceived] in %s. Or you declare event AND depends, only one of them is needed.", json_object_get_string(signalJ));
		return -1;
	}

	// Declare a raw signal
	if(event)
	{
		std::string eventStr = std::string(event);
		if( (sep = eventStr.find("/")) == std::string::npos)
		{
			AFB_ERROR("Missing something in event declaration. Has to be like: <api>/<event>");
			return -1;
		}
		std::string api = eventStr.substr(0, sep);
		src = getSourceAPI(api);
	}
	else
	{
		event = "";
	}
	// Process depends JSON object to declare virtual signal dependencies
	if (dependsJ)
	{
		if(json_object_get_type(dependsJ) == json_type_array)
		{
			int count = json_object_array_length(dependsJ);
			for(int i = 0; i < count; i++)
			{
				std::string sourceStr = json_object_get_string(json_object_array_get_idx(dependsJ, i));
				if( (sep = sourceStr.find("/")) != std::string::npos)
				{
					AFB_ERROR("Signal composition needs to use signal 'id', don't use full low level signal name");
					return -1;
				}
				dependsV.push_back(sourceStr);
			}
		}
		else
		{
			std::string sourceStr = json_object_get_string(dependsJ);
			if( (sep = sourceStr.find("/")) != std::string::npos)
			{
				AFB_ERROR("Signal composition needs to use signal 'id', don't use full low level signal name");
				return -1;
			}
			dependsV.push_back(sourceStr);
		}
		if(!src) {src=*sourcesListV_.rbegin();}
	}

	// Set default retention if not specified
	if(!retention)
	{
		retention = src->signalsDefault().retention ?
			src->signalsDefault().retention :
			30;
	}
	unit = !unit ? "" : unit;

	// Set default onReceived action if not specified
	if(!onReceivedJ)
	{
		onReceivedCtl = src->signalsDefault().onReceived ?
			src->signalsDefault().onReceived :
			nullptr;
	}
	else {onReceivedCtl = convert2Action("onReceived", onReceivedJ);}

	if(src != nullptr)
		{src->addSignal(id, event, dependsV, retention, unit, frequency, onReceivedCtl, getSignalsArgs);}
	else
		{err = -1;}

	return err;
}

int Composer::loadSignals(CtlSectionT* section, json_object *signalsJ)
{
	int err = 0;
	Composer& composer = instance();

	if(signalsJ)
	{
		if (json_object_get_type(signalsJ) == json_type_array)
		{
			int count = json_object_array_length(signalsJ);

			for (int idx = 0; idx < count; idx++)
			{
				json_object *signalJ = json_object_array_get_idx(signalsJ, idx);
				err = composer.loadOneSignal(signalJ);
				if (err) return err;
			}
		}
		else
			{err = composer.loadOneSignal(signalsJ);}
	}

	return err;
}

void Composer::processOptions(const std::map<std::string, int>& opts, std::shared_ptr<Signal> sig, json_object* response) const
{
	for(const auto& o: opts)
	{
		bool avg = false, min = false, max = false, last = false;
		if (o.first.compare("average") && !avg)
		{
			avg = true;
			double value = sig->average(o.second);
			json_object_object_add(response, "value",
				json_object_new_double(value));
		}
		else if (o.first.compare("minimum") && !min)
		{
			min = true;
			double value = sig->minimum();
			json_object_object_add(response, "value",
				json_object_new_double(value));
		}
		else if (o.first.compare("maximum") && !max)
		{
			max = true;
			double value = sig->maximum();
			json_object_object_add(response, "value",
				json_object_new_double(value));
		}
		else if (o.first.compare("last") && !last)
		{
			last = true;
			struct signalValue value = sig->last();
			if(value.hasBool)
			{
				json_object_object_add(response, "value",
					json_object_new_boolean(value.boolVal));
			}
			else if(value.hasNum)
			{
				json_object_object_add(response, "value",
					json_object_new_double(value.numVal));
			}
			else if(value.hasStr)
			{
				json_object_object_add(response, "value",
					json_object_new_string(value.strVal.c_str()));
			}
			else
			{
				json_object_object_add(response, "value",
					json_object_new_string("No recorded value so far."));
			}
		}
		else
		{
			json_object_object_add(response, "value",
				json_object_new_string("No recorded value so far."));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//                             PUBLIC METHODS                                //
///////////////////////////////////////////////////////////////////////////////

Composer& Composer::instance()
{
	static Composer composer;
	return composer;
}

void* Composer::createContext(void* ctx)
{
	uuid_t x;
	char cuid[38];
	uuid_generate(x);
	uuid_unparse(x, cuid);
	clientAppCtx* ret = new clientAppCtx(cuid);

	return (void*)ret;
}

void Composer::destroyContext(void* ctx)
{
	free(ctx);
}

std::vector<std::string> Composer::parseURI(const std::string& uri)
{
	std::vector<std::string> uriV;
	std::string delimiters = "/";

	std::string::size_type start = 0;
	auto pos = uri.find_first_of(delimiters, start);
	while(pos != std::string::npos)
	{
		if(pos != start) // ignore empty tokens
			uriV.emplace_back(uri, start, pos - start);
		start = pos + 1;
		pos = uri.find_first_of(delimiters, start);
	}

	if(start < uri.length()) // ignore trailing delimiter
	uriV.emplace_back(uri, start, uri.length() - start); // add what's left of the string

	return uriV;
}

int Composer::loadConfig(const std::string& filepath)
{
	ctlConfig_ = CtlConfigLoad(filepath.c_str(), ctlSections_);
	if(ctlConfig_ != nullptr) {return 0;}
	return -1;
}

int Composer::loadSignals(json_object* signalsJ)
{
	return loadSignals(nullptr, signalsJ);
}

CtlConfigT* Composer::ctlConfig()
{
	return ctlConfig_;
}

int Composer::initSourcesAPI()
{
	int err = 0;
	for(auto& src: sourcesListV_)
	{
		err += src->init();
	}
	return err;
}

std::shared_ptr<SourceAPI> Composer::getSourceAPI(const std::string& api)
{
	for(auto& source: sourcesListV_)
	{
		if (source->api() == api)
			{return source;}
	}
	return nullptr;
}

std::vector<std::shared_ptr<Signal>> Composer::getAllSignals()
{
	std::vector<std::shared_ptr<Signal>> allSignals;
	for( auto& source : sourcesListV_)
	{
		std::vector<std::shared_ptr<Signal>> srcSignals = source->getSignals();
		allSignals.insert(allSignals.end(), srcSignals.begin(), srcSignals.end());
	}

	return allSignals;
}

std::vector<std::shared_ptr<Signal>> Composer::searchSignals(const std::string& aName)
{
	std::string api;
	std::vector<std::shared_ptr<Signal>> signals;
	size_t sep = aName.find_first_of("/");
	if(sep != std::string::npos)
	{
		api = aName.substr(0, sep);
		std::shared_ptr<SourceAPI> source = getSourceAPI(api);
		return source->searchSignals(aName);
	}
	else
	{
		std::vector<std::shared_ptr<Signal>> allSignals = getAllSignals();
		for (std::shared_ptr<Signal>& sig : allSignals)
		{
			if(*sig == aName)
				{signals.emplace_back(sig);}
		}
	}
	return signals;
}

json_object* Composer::getsignalValue(const std::string& sig, json_object* options)
{
	std::map<std::string, int> opts;
	json_object *response = nullptr, *finalResponse = json_object_new_array();

	wrap_json_unpack(options, "{s?i, s?i, s?i, s?i !}",
		"average", &opts["average"],
		"minimum", &opts["minimum"],
		"maximum", &opts["maximum"],
		"last", &opts["last"]);

	std::vector<std::shared_ptr<Signal>> sigP = searchSignals(sig);
	if(!sigP.empty())
	{
		for(auto& sig: sigP)
		{
			wrap_json_pack(&response, "{ss}",
				"signal", sig->id().c_str());
			if (opts.empty())
			{
				struct signalValue value = sig->last();
				if(value.hasBool)
				{
					json_object_object_add(response, "value",
						json_object_new_boolean(value.boolVal));
				}
				else if(value.hasNum)
				{
					json_object_object_add(response, "value",
						json_object_new_double(value.numVal));
				}
				else if(value.hasStr)
				{
					json_object_object_add(response, "value",
						json_object_new_string(value.strVal.c_str()));
				}
				else
				{
					json_object_object_add(response, "value",
						json_object_new_string("No recorded value so far."));
				}
			}
			else
				{processOptions(opts, sig, response);}
			json_object_array_add(finalResponse, response);
		}
	}

	return finalResponse;
}

int Composer::execSignalsSubscription()
{
	int err = 0;
	for(std::shared_ptr<SourceAPI> srcAPI: sourcesListV_)
	{
		if (srcAPI->api() != std::string(ctlConfig_->api))
		{
			err = srcAPI->makeSubscription();
		}
	}
	return err;
}
