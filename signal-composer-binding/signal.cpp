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

#include "signal.hpp"
#include "signal-composer.hpp"

#define MICRO 1000000

Signal::Signal(const std::string& id, const std::string& event, std::vector<std::string>& depends, const std::string& unit, double frequency, CtlActionT* onReceived, json_object* getSignalsArgs)
:id_(id),
 event_(event),
 dependsSigV_(depends),
 timestamp_(0.0),
 value_({0,0,0,0,0,""}),
 frequency_(frequency),
 unit_(unit),
 onReceived_(onReceived),
 getSignalsArgs_(getSignalsArgs)
{}

Signal::Signal(const std::string& id,
	std::vector<std::string>& depends,
	const std::string& unit,
	double frequency,
	CtlActionT* onReceived)
:id_(id),
 event_(),
 dependsSigV_(depends),
 timestamp_(0.0),
 value_({0,0,0,0,0,""}),
 frequency_(frequency),
 unit_(unit),
 onReceived_(onReceived),
 getSignalsArgs_()
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
	if(id_.find(aName)    != std::string::npos) {return true;}
	if(event_.find(aName) != std::string::npos) {return true;}

	return false;
}

const std::string Signal::id() const
{
	return id_;
}

json_object* Signal::toJSON() const
{
	json_object* queryJ = nullptr;
	std::vector<std::string> dependsSignalName;
	for (const std::string& src: dependsSigV_ )
	{
		ssize_t sep = src.find_first_of("/");
		if(sep != std::string::npos)
		{
			dependsSignalName.push_back(src.substr(sep+1));
		}
	}
	json_object *nameArrayJ = json_object_new_array();
	for (const std::string& lowSig: dependsSignalName)
	{
		json_object_array_add(nameArrayJ, json_object_new_string(lowSig.c_str()));
	}

	wrap_json_pack(&queryJ, "{ss,so*}",
			"id", id_.c_str(),
			"getSignalsArgs", getSignalsArgs_);

	if (!event_.empty()) {json_object_object_add(queryJ, "event", json_object_new_string(event_.c_str()));}
	if (json_object_array_length(nameArrayJ)) {json_object_object_add(queryJ, "depends", nameArrayJ);}
	if (!unit_.empty()) {json_object_object_add(queryJ, "unit", json_object_new_string(unit_.c_str()));}
	if (frequency_) {json_object_object_add(queryJ, "frequency", json_object_new_double(frequency_));}

	if(timestamp_) {json_object_object_add(queryJ, "timestamp", json_object_new_int64(timestamp_));}

	if (value_.hasBool) {json_object_object_add(queryJ, "value", json_object_new_boolean(value_.boolVal));}
	else if (value_.hasNum) {json_object_object_add(queryJ, "value", json_object_new_double(value_.numVal));}
	else if (value_.hasStr) {json_object_object_add(queryJ, "value", json_object_new_string(value_.strVal.c_str()));}

	return queryJ;
}

/// @brief Set Signal timestamp and value property when an incoming
/// signal arrived. Called by a plugin because treatment can't be
/// standard as signals sources format could changes. See low-can plugin
/// example.
///
/// @param[in] timestamp - timestamp of occured signal
/// @param[in] value - value of change
void Signal::set(long long int timestamp, struct SignalValue& value)
{
	timestamp_ = timestamp;
	value_ = value;
	history_[timestamp_] = value_;
}

/// @brief Observer method called when a Observable Signal has changes.
///
/// @param[in] Observable - object from which update come from
void Signal::update(Signal* sig)
{
	AFB_NOTICE("Got an update from observed signal %s", sig->id().c_str());
}

/// @brief Notify observers that there is a change and execute callback defined
/// when signal is received
///
/// @param[in] queryJ - JSON query object to transmit to callback function
///
/// @return 0 if OK, -1 or other if not.
int Signal::onReceivedCB(json_object *queryJ)
{
	int err = onReceived_ ? ActionExecOne(onReceived_, queryJ) : 0;
	notify();
	return err;
}

/// @brief Make a Signal observer observes a Signal observable if not already
/// present in the Observers vector.
///
/// @param[in] obs - pointer to a Signal observable
/*void Signal::attach(Signal* obs)
{
	for ( auto& sig : Observers_)
	{
		if (obs == sig)
			{return;}
	}

	Observers_.push_back(obs);
}*/

/// @brief Make a Signal observer observes Signals observables
/// set in its observable vector.
///
/// @param[in] composer - bindinApp instance
void Signal::attachToSourceSignals(Composer& composer)
{
	for (const std::string& srcSig: dependsSigV_)
	{
		if(srcSig.find("/") == std::string::npos)
		{
			std::vector<Signal*> observables = composer.searchSignals(srcSig);
			if(observables[0])
			{
				AFB_NOTICE("Attaching %s to %s", id_.c_str(), srcSig.c_str());
				observables[0]->addObserver(this);
				continue;
			}
			AFB_WARNING("Can't attach. Is %s exists ?", srcSig.c_str());
		}
	}
}

/// @brief Call update() method on observer Signal with
/// current Signal timestamp and value
/*void Signal::notify() const
{
	for (int i = 0; i < Observers_.size(); ++i)
	  Observers_[i]->update(timestamp_, value_);
}

/// @brief Inner recursion check. Argument is the Signal id coming
/// from the original Signal that made a recursion check.
///
/// @param[in] origId - name of the origine of the recursion check
///
/// @return 0 if no infinite loop detected, -1 if not.
int Signal::recursionCheck(const std::string& origId) const
{
	for (const auto& obs: observersList_)
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

/// @brief Recursion check to ensure that there is no infinite loop
/// in the Observers/Observables structure.
/// This will check that observer signal is not the same than itself
/// then trigger the check against the following eventuals observers
///
/// @return 0 if no infinite loop detected, -1 if not.
int Signal::recursionCheck() const
{
	for (const auto& obs: observersList_)
	{
		if( id_ == obs->id())
			{return -1;}
		if( obs->recursionCheck(id_))
			{return -1;}
	}
	return 0;
}*/

/// @brief Make an average over the last X 'seconds'
///
/// @param[in] seconds - period to calculate the average
///
/// @return Average value
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

/// @brief Find minimum in the recorded values
///
/// @param[in] seconds - period to find the minimum
///
/// @return Minimum value contained in the history
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

/// @brief Find maximum in the recorded values
///
/// @param[in] seconds - period to find the maximum
///
/// @return Maximum value contained in the history
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

/// @brief Return last value recorded
///
/// @return Last value
struct SignalValue Signal::last() const
{
	return history_.rbegin()->second;
}

/*
template<class Signal>
Observable<Signal>::~Observable()
{
	iterator_ itb = observerList_.begin();
	const_iterator_ ite = observerList_.end();

	for(;itb!=ite;++itb)
	{
		(*itb)->delObservable(this);
	}
}

template <class Signal>
Observer<Signal>::~Observer()
{
	const_iterator_ ite = observableList_.end();

	for(iterator_ itb=observableList_.begin();itb!=ite;++itb)
	{
			(*itb)->delObserver(this);
	}
}

template <class Signal>
void Observable<Signal>::notify()
{
	iterator_ itb=observerList_.begin();
	const_iterator_ ite=observerList_.end();

	for(;itb!=ite;++itb)
	{
		(*itb)->update(static_cast<Signal*>(this));
	}
}
*/
