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
#pragma once

#include <vector>
#include <string>
#include <ctl-config.h>
#include <json-c/json.h>
#include <systemd/sd-event.h>
extern "C"
{
	#define AFB_BINDING_VERSION 2
	#include <afb/afb-binding.h>
};

#include "sources.hpp"

class bindingApp
{
private:
	CtlConfigT ctlConfig_;

	static CtlSectionT ctlSections_[]; ///< Config Section definition (note: controls section index should match handle retrieval in)
	std::vector<Source> Sources_;

	bindingApp(std::string filepath);
	~bindingApp();

	int loadOneSource(json_object* sourcesJ);
	int loadSources(struct ConfigSectionT *section, json_object *sectionJ);
	void convert2Action(std::string& name, json_object* action);

	int loadOneSignal(json_object* signalsJ);
	int loadSignals(struct ConfigSectionT *section, json_object *sectionJ);

public:
	static bindingApp& instance(std::string confFile);
	void loadSignalsFile(std::string signalsFile);
}
