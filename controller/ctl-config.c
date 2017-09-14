/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, something express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Reference:
 *   Json load using json_unpack https://jansson.readthedocs.io/en/2.9/apiref.html#parsing-and-validating-values
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "filescan-utils.h"
#include "ctl-config.h"


// Load control config file

char* CtlConfigSearch(const char *dirList, const char* fileName) {
    int index;
    char controlFile [CONTROL_MAXPATH_LEN];

    if(fileName) {
        strncpy(controlFile, fileName, CONTROL_MAXPATH_LEN);
    } else {
        strncpy(controlFile, CONTROL_CONFIG_PRE "-", CONTROL_MAXPATH_LEN);
        strncat(controlFile, GetBinderName(), CONTROL_MAXPATH_LEN);
    }

    // search for default dispatch config file
    json_object* responseJ = ScanForConfig(dirList, CTL_SCAN_RECURSIVE, controlFile, ".json");

    // We load 1st file others are just warnings
    for (index = 0; index < json_object_array_length(responseJ); index++) {
        json_object *entryJ = json_object_array_get_idx(responseJ, index);

        char *filename;
        char*fullpath;
        int err = wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_ERROR("CTL-INIT HOOPs invalid JSON entry= %s", json_object_get_string(entryJ));
            return NULL;
        }

        if (index == 0) {
            if (strcasestr(filename, controlFile)) {
                char filepath[CONTROL_MAXPATH_LEN];
                strncpy(filepath, fullpath, sizeof (filepath));
                strncat(filepath, "/", sizeof (filepath));
                strncat(filepath, filename, sizeof (filepath));
                return (strdup(filepath));
            }
        }
    }

    // no config found
    return NULL;
}

int CtlConfigExec(CtlConfigT *ctlConfig) {
    // best effort to initialise everything before starting
    if (ctlConfig->requireJ) {

        void DispatchRequireOneApi(json_object * bindindJ) {
            const char* requireBinding = json_object_get_string(bindindJ);
            int err = afb_daemon_require_api(requireBinding, 1);
            if (err) {
                AFB_WARNING("CTL-LOAD-CONFIG:REQUIRE Fail to get=%s", requireBinding);
            }
        }

        if (json_object_get_type(ctlConfig->requireJ) == json_type_array) {
            for (int idx = 0; idx < json_object_array_length(ctlConfig->requireJ); idx++) {
                DispatchRequireOneApi(json_object_array_get_idx(ctlConfig->requireJ, idx));
            }
        } else {
            DispatchRequireOneApi(ctlConfig->requireJ);
        }
    }

#ifdef CONTROL_SUPPORT_LUA
    int err= LuaConfigExec();
    if (err) goto OnErrorExit;
#endif

    // Loop on every section and process config
    int errcount=0;
    for (int idx = 0; ctlConfig->sections[idx].key != NULL; idx++) {
        errcount += ctlConfig->sections[idx].loadCB(&ctlConfig->sections[idx], NULL);
    }
    return errcount;
}

CtlConfigT *CtlConfigLoad(const char* filepath, CtlSectionT *sections) {
    json_object *ctlConfigJ;
    CtlConfigT *ctlConfig = calloc(1, sizeof (CtlConfigT));
    int err;

#ifdef CONTROL_SUPPORT_LUA
    err= LuaConfigLoad();
    if (err) goto OnErrorExit;
#endif

    json_object* loadJSON(const char* fileName)
    {
        // Search for config in filepath
        const char* filepathFound = CtlConfigSearch(filepath, fileName);
        if(!filepathFound)
        {
            AFB_ERROR("CTL-LOAD-CONFIG No JSON Config found in %s", filepath);
            return NULL;
        }
        return json_object_from_file(filepathFound);
    }

    // Load JSON file
    ctlConfigJ = loadJSON(NULL);
    if (!ctlConfigJ) {
        AFB_ERROR("CTL-LOAD-CONFIG Invalid JSON %s ", filepath);
        goto OnErrorExit;
    }

    AFB_INFO("CTL-LOAD-CONFIG: loading config filepath=%s", filepath);

    json_object *metadataJ;
    int done = json_object_object_get_ex(ctlConfigJ, "metadata", &metadataJ);
    if (done) {
        err = wrap_json_unpack(metadataJ, "{ss,ss,ss,s?s,s?o,s?o !}",
        "label", &ctlConfig->label,
        "version", &ctlConfig->version,
        "api", &ctlConfig->api,
        "info", &ctlConfig->info,
        "require", &ctlConfig->requireJ,
		"files",&ctlConfig->filesJ);
        if (err) {
            AFB_ERROR("CTL-LOAD-CONFIG:METADATA Missing something label|api|version|[info]|[require] in:\n-- %s", json_object_get_string(metadataJ));
            goto OnErrorExit;
        }

        // Should replace this with API name change
        if (ctlConfig->api) {
            err = afb_daemon_rename_api(ctlConfig->api);
            if (err) AFB_WARNING("Fail to rename api to:%s", ctlConfig->api);
        }
    }

    //load config sections
    err = 0;
    ctlConfig->sections = sections;

    for (int idx = 0; sections[idx].key != NULL; idx++) {
        json_object * sectionJ;
        int done = json_object_object_get_ex(ctlConfigJ, sections[idx].key, &sectionJ);
        if (!done) {
            json_object* configJ = NULL;
            const char* fileName;
            AFB_DEBUG("CtlConfigLoad: fail to find '%s' section in config '%s'. Searching deeper", sections[idx].key, filepath);
            if (json_object_get_type(ctlConfig->filesJ) == json_type_array) {
                int filesEnd = json_object_array_length(ctlConfig->filesJ);
                for (int jdx = 0; jdx < filesEnd; jdx++) {
                    fileName = json_object_get_string(json_object_array_get_idx(ctlConfig->filesJ, jdx));
                    configJ = loadJSON(fileName);
                    if(json_object_object_get_ex(configJ, sections[idx].key, &sectionJ))
                    {
                        const char* k = sections[idx].key;
                        err += sections[idx].loadCB(&sections[idx], sectionJ);
                        done++;
                    }
                }
                if(!done) {
                    AFB_ERROR("CtlConfigLoad: fail to find '%s' section in config '%s' with '%s' in its name", sections[idx].key, filepath, fileName);
                    err++;
                }
                if(err) {goto OnErrorExit;}
            } else {
                fileName = json_object_get_string(ctlConfig->filesJ);
                configJ = loadJSON(fileName);
                if(json_object_object_get_ex(configJ, sections[idx].key, &sectionJ)) {
                    err += sections[idx].loadCB(&sections[idx], sectionJ);
                    done++;
                }
                else {
                    AFB_ERROR("CtlConfigLoad: fail to find '%s' section in config '%s' with '%s' in its name", sections[idx].key, filepath, fileName);
                    err++;
                }
                if(err) {goto OnErrorExit;}
            }
        } else {
            err += sections[idx].loadCB(&sections[idx], sectionJ);
        }
    }
    if (err) goto OnErrorExit;

    return (ctlConfig);

OnErrorExit:
    free(ctlConfig);
    return NULL;
}




