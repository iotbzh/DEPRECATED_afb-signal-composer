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

#include <map>
#include <string>
#include <vector>
#include <ctl-config.h>

#include "observer-pattern.hpp"

#define MICRO 1000000

class Composer;

/// @brief Structure holding a possible value of a Signal
///  as it could be different type of value, we declare all
///  possibility.
///  Not very efficient or optimized, maybe use of Variant in
///  C++17 but this is a bit too new to uses it for now
struct signalValue {
	bool hasBool;
	bool boolVal;
	bool hasNum;
	double numVal;
	bool hasStr;
	std::string strVal;
};

/// @brief Holds a signal (raw or virtual) definition. Value could be of
///  different types, so an intermediate structure is use to store them.
///  A signal could also be a subject or an observer at the same time, this
///  is the behavior retained to compose virtual signals based on raw signals.
///  And this is the way that is used to update and reacts to an underlaying
///  signal change.
class Signal: public Observable<Signal>, public Observer<Signal>
{
private:
	std::string id_;
	std::string event_;
	std::vector<std::string> dependsSigV_;
	uint64_t timestamp_;
	struct signalValue value_;
	std::map<uint64_t, struct signalValue> history_; ///< history_ - Hold signal value history in map with <timestamp, value>
	int retention_;
	double frequency_;
	std::string unit_;
	CtlActionT* onReceived_;
	json_object* getSignalsArgs_;

public:
	bool subscribed_; ///< subscribed_ - boolean value telling if yes or no the signal has been subcribed to the low level binding.
	Signal();
	Signal(const std::string& id, const std::string& event, std::vector<std::string>& depends, const std::string& unit, int retention, double frequency, CtlActionT* onReceived, json_object* getSignalsArgs);
	Signal(const std::string& id, std::vector<std::string>& depends, const std::string& unit, int retention, double frequency, CtlActionT* onReceived);

	explicit operator bool() const;
	bool operator==(const Signal& other) const;
	bool operator==(const std::string& aName) const;

	const std::string id() const;
	json_object* toJSON() const;

	void set(uint64_t timestamp, struct signalValue& value);
	void update(Signal* sig);
	int onReceivedCB(json_object *queryJ);
	void attachToSourceSignals(Composer& composer);

	double average(int seconds = 0) const;
	double minimum(int seconds = 0) const;
	double maximum(int seconds = 0) const;
	struct signalValue last() const;

	int initialRecursionCheck();
	int recursionCheck(Signal* obs);
};

struct signalCBT
{
	void (*setSignalValue)(const char* aName, uint64_t timestamp, struct signalValue value);
};
