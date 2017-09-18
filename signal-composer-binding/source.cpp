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

SourceAPI::SourceAPI(const std::string& api, const std::string& info, CtlActionT* init, CtlActionT* getSignals):
	api_(api), info_(info), init_(init), getSignals_(getSignals)
{}

std::string SourceAPI::api() const
{
	return api_;
}

void SourceAPI::addSignal(const std::string& id, const std::string& event, std::vector<std::string>& depends, const std::string& sClass, const std::string& unit, double frequency, CtlActionT* onReceived, json_object* getSignalsArgs)
{
	std::shared_ptr<Signal> sig = std::make_shared<Signal>(id, event, depends, unit, frequency, onReceived, getSignalsArgs);

	signalsMap_[sig] = false;
}

std::vector<std::shared_ptr<Signal>> SourceAPI::getSignals() const
{
	std::vector<std::shared_ptr<Signal>> signals;
	for (auto& sig: signalsMap_)
	{
		signals.push_back(sig.first);
	}
	return signals;
}

std::shared_ptr<Signal> SourceAPI::searchSignal(const std::string& name) const
{
	for (auto& sig: signalsMap_)
	{
		if(*sig.first == name) {return sig.first;}
	}
	return nullptr;
}

int SourceAPI::makeSubscription()
{
	int err = 0;
	if(getSignals_)
	{
		for(auto& sig: signalsMap_)
		{
			json_object* signalJ = sig.first->toJSON();
			if(!signalJ)
			{
				AFB_ERROR("Error building JSON query object to subscribe to for signal %s", sig.first->id().c_str());
				err = -1;
				break;
			}
			err += sig.second ? 0:ActionExecOne(getSignals_, signalJ);
			if(err)
				{AFB_WARNING("Fails to subscribe to signal %s", sig.first->id().c_str());}
			else
				{sig.second = true;}
		}
	}

	return err;
}
