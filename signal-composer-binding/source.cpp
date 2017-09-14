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

#include "source.hpp"

SourceAPI::SourceAPI()
{}

SourceAPI::SourceAPI(const std::string& api, const std::string& info, CtlActionT* init, CtlActionT* getSignal):
	api_(api), info_(info), init_(init), getSignal_(getSignal)
{}

std::string SourceAPI::api() const
{
	return api_;
}

void SourceAPI::addSignal(const std::string& id, std::vector<std::string>& sources, const std::string& sClass, const std::string& unit, double frequency, CtlActionT* onReceived)
{
	std::shared_ptr<Signal> sig = std::make_shared<Signal>(id, sources, unit, frequency, onReceived);

	signals_.push_back(sig);
}

std::vector<std::shared_ptr<Signal>> SourceAPI::getSignals() const
{
	return signals_;
}

std::shared_ptr<Signal> SourceAPI::searchSignal(const std::string& name) const
{
	for (auto& sig: signals_)
	{
		if(*sig == name) {return sig;}
	}
	return nullptr;
}
