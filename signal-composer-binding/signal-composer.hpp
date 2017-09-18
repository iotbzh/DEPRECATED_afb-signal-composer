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

#include <memory>
#include <vector>
#include <string>

#include "source.hpp"

class bindingApp
{
private:
	CtlConfigT* ctlConfig_;

	static CtlSectionT ctlSections_[]; ///< Config Section definition (note: controls section index should match handle retrieval in)
	std::vector<SourceAPI> sourcesList_;

	explicit bindingApp(const std::string& filepath);
	bindingApp();
	~bindingApp();

	CtlActionT* convert2Action(const std::string& name, json_object* action);

	int loadOneSourceAPI(json_object* sourcesJ);
	static int loadSourcesAPI(CtlSectionT* section, json_object *sectionJ);

	int loadOneSignal(json_object* signalsJ);
	static int loadSignals(CtlSectionT* section, json_object *sectionJ);

public:
	static bindingApp& instance();
	int loadConfig(const std::string& filepath);
	//void loadSignalsFile(std::string signalsFile);

	CtlConfigT* ctlConfig();
	std::vector<std::shared_ptr<Signal>> getAllSignals();
	SourceAPI* getSourceAPI(const std::string& api);
	std::shared_ptr<Signal> searchSignal(const std::string& aName);
	json_object* getSignalValue(const std::string& sig, json_object* options);

	int execSubscription() const;
};

struct pluginCBT
{
	void (*setSignalValue)(const char* aName, long long int timestamp, struct SignalValue value);
};

extern "C" void setSignalValueHandle(const char* aName, long long int timestamp, struct SignalValue value);
