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

#include <string>
#include <memory>
#include <vector>

class Signal;

class Signal
{
private:
	std::string api_;
	std::string name_;
	std::vector<> history_;
	float frequency_;
	st::string unit_;
	float min_;
	float max_;
	float last_;

	std::vector<std::shared_ptr<Signal>> Observers_;

public:
	void notify();
	void infiniteRecursionCheck();
	void attach();
	void detach();
	void notify();
}
