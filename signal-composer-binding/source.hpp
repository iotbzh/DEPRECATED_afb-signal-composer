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
#include <ctl-config.h>
#include <signal-composer-binding.hpp>

#include "signal.hpp"

class Source {
private:
	std::string api_;
	std::string info_;
	CtlActionT* init_;
	CtlActionT* getSignal_;

	std::vector<std::shared_ptr<Signal>> signals_;

public:
	Source();
	Source(const std::string& api, const std::string& info, CtlActionT* init, CtlActionT* getSignal);

	std::string api();
	void addSignal(const std::string& id, std::vector<std::string>& sources, const std::string& sClass, const std::string& unit, double frequency, CtlActionT* onReceived);
	std::vector<std::shared_ptr<Signal>> getSignals();
};
