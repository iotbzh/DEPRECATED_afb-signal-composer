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
#include "signal.hpp"

class SourceAPI {
private:
	std::string api_;
	std::string info_;
	CtlActionT* init_;
	CtlActionT* getSignals_;

	std::map<std::string, std::shared_ptr<Signal>> signalsMap_;

public:
	SourceAPI();
	SourceAPI(const std::string& api, const std::string& info, CtlActionT* init, CtlActionT* getSignal);

	int init();
	std::string api() const;
	void addSignal(const std::string& id, const std::string& event, std::vector<std::string>& sources, const std::string& sClass, const std::string& unit, double frequency, CtlActionT* onReceived, json_object* getSignalsArgs);

	std::vector<std::shared_ptr<Signal>> getSignals() const;
	std::vector<std::shared_ptr<Signal>> searchSignals(const std::string& name);

	int makeSubscription();
};
