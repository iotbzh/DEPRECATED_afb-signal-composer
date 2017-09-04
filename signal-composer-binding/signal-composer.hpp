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

 #include <cstddef>
 #include <string>
 #include <vector>
 #include <set>
 #include <map>
 #include <json-c/json.h>
 #include <systemd/sd-event.h>
 extern "C"
 {
	#define AFB_BINDING_VERSION 2
	#include <afb/afb-binding.h>
 };

 struct TimedEvent {
	int interval;
	afb_event event;
	std::string name;
	std::string eventName;
 };

 struct Signal {
	std::string name;
	std::string source;
	std::string sig_class;
	std::string type;
 };

 class SignalComposer
 {
 public:
	SignalComposer();
	void treatMessage(json_object *message);
	bool subscribe(afb_req request);
	bool unsubscribe(afb_req request);
	bool get(afb_req request, json_object **json);
	void tick(sd_event_source *source, const long &now, void *interv);
	void startTimer(const int &t);
	~SignalComposer();
	int parseConfigAndSubscribe(const std::string& confd);
	static bool startsWith(const std::string &s, const std::string &val);
	static void callBackFromSubscribe(void *handle, int iserror, json_object *result);
 private:
	std::map<std::string, afb_event> events;
	std::map<int, std::vector<TimedEvent>> timedEvents;
	std::map<std::string, std::map<std::string, Signal>> registeredObjects;
	std::map<std::string, std::set<std::string>> lowMessagesToObjects;
	std::set<int> timers;
	std::string generateId() const;
	json_object *generateJson(const std::string &messageObject, std::vector<std::string> *fields = nullptr);
	void registerObjects(const std::string& uri, std::map<std::string, Signal>& properties);
	std::map<std::string, std::map<std::string, Signal>> loadDefinitions(json_object* definitionsJ) const;
	void loadResources(json_object* resourcesJ, std::map<std::string, std::map<std::string, Signal>>& properties);
	int subscribeRegisteredObjects() const;
 };
