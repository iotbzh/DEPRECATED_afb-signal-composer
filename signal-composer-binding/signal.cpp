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

#include "signal.hpp"

Signal::Signal()
{}

Signal(std::string& id,
	std::vector<std::string>& sources,
	std::string& unit,
	float frequency,
	CtlActionT* onReceived)
:id_(id),
 sources_(sources),
 unit_(unit),
 frequency_(frequency),
 onReceived_(onReceived)
{
	for (const std::string& src: sources)
	{
		if(src.find("/") == std::string::npos)
		{
			AFB_NOTICE("Attaching %s to %s", sig->id(), src);
			if(sig.attach(src))
				{AFB_WARNING("Can't attach. Is %s exists ?", src);}
		}
	}
}

Signal::operator bool() const
{
	if(!id_ || !api_ || !signalName_)
		{return false;}
	return true;
}

int Signal::recursionCheck(const std::string& origId)
{
	for (const auto& obs: Observers_)
	{
		if( id_ == obs.id())
			{return -1;}
		if( origId == obs.id())
			{return -1;}
		if(! obs.recursionCheck(origId))
			{return -1;}
	}
	return 0;
}

std::string Signal::id() const
{
	return id_;
}

int Signal::recursionCheck()
{
	for (const auto& obs: Observers_)
	{
		if( id_ == obs.id())
			{return -1;}
		if(! obs.recursionCheck(id_))
			{return -1;}
	}
	return 0;
}

void update(double timestamp, double value)
{
	AFB_NOTICE("Got an update from observed signal");
}
