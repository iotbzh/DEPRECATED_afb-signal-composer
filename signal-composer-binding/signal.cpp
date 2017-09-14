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

#include <memory>

#include "signal.hpp"
#include "signal-composer.hpp"

Signal::Signal(const std::string& id,
			std::vector<std::string>& sources,
			const std::string& unit,
			double frequency,
			CtlActionT* onReceived)
:id_(id),
 sourcesSig_(sources),
 frequency_(frequency),
 unit_(unit),
 onReceived_(onReceived)
{}

Signal::operator bool() const
{
	if(id_.empty())
		{return false;}
	return true;
}

bool Signal::operator ==(const Signal& other) const
{
	if(id_ == other.id_) {return true;}
	return false;
}

bool Signal::operator==(const std::string& aName) const
{
	if(id_ == aName) {return true;}
	for( const std::string& src : sourcesSig_)
	{
		if(src == aName) {return true;}
	}
	return false;
}

std::string Signal::id() const
{
	return id_;
}
void update(double timestamp, double value)
{
	AFB_NOTICE("Got an update from observed signal");
}

int Signal::onReceivedCB(json_object *queryJ)
{
	return ActionExecOne(onReceived_, queryJ);
}

void Signal::attach(Signal* obs)
{
	Observers_.push_back(obs);
}

void Signal::attachToSources(bindingApp& bApp)
{
	for (const std::string& srcSig: sourcesSig_)
	{
		if(srcSig.find("/") == std::string::npos)
		{
			std::shared_ptr<Signal> sig = bApp.searchSignal(srcSig);
			if(sig)
			{
				AFB_NOTICE("Attaching %s to %s", id_.c_str(), srcSig.c_str());
				sig->attach(this);
				continue;
			}
			AFB_WARNING("Can't attach. Is %s exists ?", srcSig.c_str());
		}
	}
}

void Signal::notify()
{
	for (int i = 0; i < Observers_.size(); ++i)
	  Observers_[i]->update(timestamp_, value_);
}

int Signal::recursionCheck(const std::string& origId)
{
	for (const auto& obs: Observers_)
	{
		if( id_ == obs->id())
			{return -1;}
		if( origId == obs->id())
			{return -1;}
		if(! obs->recursionCheck(origId))
			{return -1;}
	}
	return 0;
}

int Signal::recursionCheck()
{
	for (const auto& obs: Observers_)
	{
		if( id_ == obs->id())
			{return -1;}
		if( obs->recursionCheck(id_))
			{return -1;}
	}
	return 0;
}
