#pragma once

#include <systemd/sd-event.h>
#include <ctl-config.h>

#ifdef __cplusplus
	#include <cstddef>
extern "C"
{
#endif
	#define AFB_BINDING_VERSION 2
	#include <afb/afb-binding.h>
#ifdef __cplusplus
};
#endif

void onEvent(const char *event, struct json_object *object);
int loadConf();
int execConf();
