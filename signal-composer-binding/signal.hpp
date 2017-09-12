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
#include <memory>
#include <vector>
#include <ctl-config.h>

#include "observer.hpp"

class Signal;

//TODO: claneys: define observer and observable interface then inherit
class Signal: public Observer, public Subject
{
private:
	std::string id_;
	std::vector<std::string> sources_;
	std::map<long, double> history_; ///< history_ - Hold signal value history in map with <timestamp, value>
	double frequency_;
	std::string unit_;
	CtlActionT* onReceived_;

	std::vector<std::shared_ptr<Signal>> observers_;

	int recursionCheck(const std::string& origId);
public:
	Signal();
	Signal(const std::string& id, std::vector<std::string>& sources, const std::string& unit, double frequency, CtlActionT* onReceived);
	explicit operator bool() const;

	std::string id() const;

	int recursionCheck();
	/* virtual */ void update(double timestamp, double value);

	virtual double average() const;
	virtual double min() const;
	virtual double max() const;
	double lastValue() const;
};
