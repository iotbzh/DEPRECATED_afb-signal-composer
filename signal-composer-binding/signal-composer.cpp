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

ctlSectionsT bindingApp::ctlSections_ = {
	[0]={.key="sources" , .loadCB= loadSources},
	[1]={.key="signals" , .loadCB= SignalsConfig},
	[2]={.key=NULL}
};

bindingApp::bindingApp(std::string filepath)
{
	ctlConfig_ = CtlConfigLoad(filepath.c_str(), ctlSections_);
}

bindingApp::instance(std::string filepath)
{
	static bindingApp bApp(filepath);
	return bApp;
}


void convert2Action(std::string& name, json_object* action)
{
	json_object *functionArgsJ = nullptr;
	const char* function;

	if(action &&
		wrap_json_unpack(action, "{ss,s?o !}", "function", &function, "args", &functionArgs))
	{
		std::string functionS = function;
		*action = nullptr;
		if(functionS.find("lua") != std::string::npos)
		{
			wrap_json_pack(&action, "{ss,s?s,s?o,s?s,s?s,s?s,s?o !}",
			"label", name,
			"lua", function,
			"args", functionArgsJ);
		}
		else if( (ssize_t sep = functionS.find_first_of("/")) != std::string::npos)
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
}

int bindingApp::loadOneSource(json_object* sourcesJ)
{
	json_object *initJ = nullptr, *getSignalJ = nullptr;
	CtlActionT initAction, getSignalAction;
	const char *api, *info;

	int err = wrap_json_unpack(sourcesJ, "{ss,s?s,s?o,s?o !}",
			"api", &api,
			"info", &info,
			"init", &initJ,
			"getSignal", &getSignalJ);
	if (err)
	{
		AFB_ERROR("Missing something api|[info]|[init]|[getSignal] in %s", json_object_get_string(sourcesJ));
		return err;
	}

	convert2Action("init", initJ);
	convert2Action("getSignal", getSignalJ);

	Sources_.push_back(Source(api, info, initJ, getSignalJ));

	return err;
}

int bindingApp::loadSources(struct ConfigSectionT *section, json_object *sourcesJ)
{
	int err = 0;
	if (json_object_get_type(sourcesJ) == json_type_array)
	{
		int count = json_object_array_length(sourcesJ);

		for (int idx = 0; idx < count; idx++) {
			json_object *sourceJ = json_object_array_get_idx(sourcesJ, idx);
			err = loadOneSource(sourceJ);
			if (err) return err;
		}
	}
	else
		{err = loadOneSource(sourcesJ);}

	return err;
}

int bindingApp::loadSources(struct ConfigSectionT *section, json_object *signalsJ)
{
	int err = 0;
	if (json_object_get_type(signalsJ) == json_type_array)
	{
		int count = json_object_array_length(signalsJ);

		for (int idx = 0; idx < count; idx++) {
			json_object *signalJ = json_object_array_get_idx(signalsJ, idx);
			err = loadOneSource(signalJ);
			if (err) return err;
		}
	}
	else
		{err = loadOneSource(signalsJ);}

	return err;
}
