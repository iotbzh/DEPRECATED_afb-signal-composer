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

#include <ctl-config.h>

#include "signal-composer-binding.hpp"
#include "signal-composer-apidef.h"
#include "wrap-json.h"
#include "signal-composer.hpp"

SignalComposer SigComp;
static CtlConfigT *ctlConfig=NULL;

/// @brief callback for receiving message from low binding. Treatment itself is made in SigComp class.
void onEvent(const char *event, json_object *object)
{
	SigComp.treatMessage(object);
}
/// @brief entry point for client subscription request. Treatment itself is made in SigComp class.
void subscribe(afb_req request)
{
	if(SigComp.subscribe(request))
		afb_req_success(request, NULL, NULL);
	else
		afb_req_fail(request, "error", NULL);
}

/// @brief entry point for client un-subscription request. Treatment itself is made in SigComp class.
void unsubscribe(afb_req request)
{
	if(SigComp.unsubscribe(request))
		afb_req_success(request, NULL, NULL);
	else
		afb_req_fail(request, "error when unsubscribe", NULL);
}

/// @brief verb that loads JSON configuration (old SigComp.json file now)
void loadConf(afb_req request)
{
	json_object* args = afb_req_json(request);
	const char* confd;

	wrap_json_unpack(args, "{s:s}", "path", &confd);
	int ret = SigComp.parseConfigAndSubscribe(confd);
	if( ret == 0)
		afb_req_success(request, NULL, NULL);
	else
		afb_req_fail_f(request, "Loading configuration or subscription error", "Error code: %d", ret);
}

/// @brief entry point for get requests. Treatment itself is made in SigComp class.
void get(afb_req request)
{
	json_object *jobj;
	if(SigComp.get(request, &jobj))
	{
		afb_req_success(request, jobj, NULL);
	} else {
		afb_req_fail(request, "error", NULL);
	}
}

/// @brief entry point for systemD timers. Treatment itself is made in SigComp class.
/// @param[in] source: systemD timer, t: time of tick, data: interval (ms).
int ticked(sd_event_source *source, uint64_t t, void* data)
{
	SigComp.tick(source, t, data);
	return 0;
}

int loadConf()
{
	int errcount=0;

	ctlConfig = CtlConfigLoad();

	#ifdef CONTROL_SUPPORT_LUA
		errcount += LuaLibInit();
	#endif

	return errcount;
}

int execConf()
{
	int err = CtlConfigExec();

	AFB_DEBUG ("Signal Composer Control configuration Done errcount=%d", errcount);
	return errcount;
}
