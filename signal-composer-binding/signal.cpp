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

#include <float.h>
#include <fnmatch.h>
#include <memory>

#include "signal.hpp"
#include "signal-composer.hpp"

#define MICRO 1000000

Signal::Signal(const std::string& id,
			std::vector<std::string>& sources,
			const std::string& unit,
			double frequency,
			CtlActionT* onReceived)
:id_(id),
 signalSigList_(sources),
 timestamp_(0.0),
 value_({0,0,0,0,0,""}),
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

bool Signal::operator ==(const std::string& aName) const
{
	if(! fnmatch(aName.c_str(), id_.c_str(), FNM_CASEFOLD)) {return true;}
	for( const std::string& src : signalSigList_)
	{
		if(! fnmatch(aName.c_str(), src.c_str(), FNM_CASEFOLD)) {return true;}
		if( src.find(aName) != std::string::npos) {return true;}
	}
	return false;
}

const std::string Signal::id() const
{
	return id_;
}

json_object* Signal::toJSON() const
{
	json_object* queryJ = nullptr;
	std::vector<std::string> lowSignalName;
	for (const std::string& src: signalSigList_ )
	{
		ssize_t sep = src.find_first_of("/");
		if(sep != std::string::npos)
		{
			lowSignalName.push_back(src.substr(sep+1));
		}
	}
	json_object* nameArray = json_object_new_array();
	for (const std::string& lowSig: lowSignalName)
	{
		json_object_array_add(nameArray, json_object_new_string(lowSig.c_str()));
	}
	/*json_object_object_add(queryJ, "signal", nameArray);
	json_object_object_add(queryJ, "unit", json_object_new_string(unit_.c_str()));
	json_object_object_add(queryJ, "unit", json_object_new_double(frequency_));*/
	wrap_json_pack(&queryJ, "{so,ss*,sf*}",
			"signal", nameArray,
			"unit", unit_.c_str(),
			"frequency", frequency_);

	return queryJ;
}

void Signal::update(long long int timestamp, struct SignalValue value)
{
	AFB_NOTICE("Got an update from observed signal. Timestamp: %lld, vb: %d, vn: %lf, vs: %s", timestamp, value.boolVal, value.numVal, value.strVal.c_str());
}

/// @brief Notify observers that there is a change and execute callback defined
/// when signal is received
///
/// @param[in] queryJ - JSON query object to transmit to callback function
///
/// @return 0 if OK, -1 or other if not.
int Signal::onReceivedCB(json_object *queryJ)
{
	notify();
	return onReceived_ ? ActionExecOne(onReceived_, queryJ) : 0;
}

void Signal::attach(Signal* obs)
{
	Observers_.push_back(obs);
}

void Signal::attachToSourceSignals(bindingApp& bApp)
{
	for (const std::string& srcSig: signalSigList_)
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

double Signal::average(int seconds) const
{
	long long int begin = history_.begin()->first;
	long long int end = !seconds ?
		begin+(seconds*MICRO) :
		history_.rbegin()->first;
	double total = 0.0;
	int nbElt = 0;

	for (const auto& val: history_)
	{
		if(val.first >= end)
			{break;}
		if(val.second.hasNum)
		{
			total += val.second.numVal;
			nbElt++;
		}
		else
		{
			AFB_ERROR("There isn't numerical value to compare with in that signal '%s'. Stored value : bool %d, num %lf, str: %s",
			id_.c_str(),
			val.second.boolVal,
			val.second.numVal,
			val.second.strVal.c_str());
			break;
		}
	}

	return total / nbElt;
}
double Signal::minimum() const
double Signal::minimum(int seconds) const
{
	long long int begin = history_.begin()->first;
	long long int end = !seconds ?
	begin+(seconds*MICRO) :
	history_.rbegin()->first;

	double min = DBL_MAX;
	for (auto& v : history_)
	{
		if(v.first >= end)
			{break;}
		else if(v.second.hasNum && v.second.numVal < min)
			{min = v.second.numVal;}
		else
		{
			AFB_ERROR("There isn't numerical value to compare with in that signal '%s'. Stored value : bool %d, num %lf, str: %s",
			id_.c_str(),
			v.second.boolVal,
			v.second.numVal,
			v.second.strVal.c_str());
			break;
		}
	}
	return min;
}

double Signal::maximum() const
double Signal::maximum(int seconds) const
{
	long long int begin = history_.begin()->first;
	long long int end = !seconds ?
	begin+(seconds*MICRO) :
	history_.rbegin()->first;

	double max = 0.0;
	for (auto& v : history_)
	{
		if(v.first >= end)
		{break;}
		else if(v.second.hasNum && v.second.hasNum > max)
			{max = v.second.numVal;}
		else
		{
			AFB_ERROR("There isn't numerical value to compare with in that signal '%s'. Stored value : bool %d, num %lf, str: %s",
			id_.c_str(),
			v.second.boolVal,
			v.second.numVal,
			v.second.strVal.c_str());
			break;
		}
	}
	return max;
}
struct SignalValue Signal::last() const
{
	return history_.rbegin()->second;
}
