
static const char _afb_description_v2_signal_composer[] =
    "{\"openapi\":\"3.0.0\",\"$schema\":\"http://iot.bzh/download/openapi/sch"
    "ema-3.0/default-schema.json\",\"info\":{\"description\":\"\",\"title\":\""
    "signals-composer-service\",\"version\":\"4.0\",\"x-binding-c-generator\""
    ":{\"api\":\"signal-composer\",\"version\":2,\"prefix\":\"\",\"postfix\":"
    "\"\",\"start\":null,\"onevent\":\"onEvent\",\"preinit\":\"loadConf\",\"i"
    "nit\":\"execConf\",\"scope\":\"\",\"private\":false}},\"servers\":[{\"ur"
    "l\":\"ws://{host}:{port}/api/monitor\",\"description\":\"Signals compose"
    "r API connected to low level AGL services\",\"variables\":{\"host\":{\"d"
    "efault\":\"localhost\"},\"port\":{\"default\":\"1234\"}},\"x-afb-events\""
    ":[{\"$ref\":\"#/components/schemas/afb-event\"}]}],\"components\":{\"sch"
    "emas\":{\"afb-reply\":{\"$ref\":\"#/components/schemas/afb-reply-v2\"},\""
    "afb-event\":{\"$ref\":\"#/components/schemas/afb-event-v2\"},\"afb-reply"
    "-v2\":{\"title\":\"Generic response.\",\"type\":\"object\",\"required\":"
    "[\"jtype\",\"request\"],\"properties\":{\"jtype\":{\"type\":\"string\",\""
    "const\":\"afb-reply\"},\"request\":{\"type\":\"object\",\"required\":[\""
    "status\"],\"properties\":{\"status\":{\"type\":\"string\"},\"info\":{\"t"
    "ype\":\"string\"},\"token\":{\"type\":\"string\"},\"uuid\":{\"type\":\"s"
    "tring\"},\"reqid\":{\"type\":\"string\"}}},\"response\":{\"type\":\"obje"
    "ct\"}}},\"afb-event-v2\":{\"type\":\"object\",\"required\":[\"jtype\",\""
    "event\"],\"properties\":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-"
    "event\"},\"event\":{\"type\":\"string\"},\"data\":{\"type\":\"object\"}}"
    "}},\"x-permissions\":{},\"responses\":{\"200\":{\"description\":\"A comp"
    "lex object array response\",\"content\":{\"application/json\":{\"schema\""
    ":{\"$ref\":\"#/components/schemas/afb-reply\"}}}}}},\"paths\":{\"/subscr"
    "ibe\":{\"description\":\"Subscribe to a signal object\",\"parameters\":["
    "{\"in\":\"query\",\"name\":\"event\",\"required\":false,\"schema\":{\"ty"
    "pe\":\"string\"}}],\"responses\":{\"200\":{\"$ref\":\"#/components/respo"
    "nses/200\"}}},\"/unsubscribe\":{\"description\":\"Unsubscribe previously"
    " suscribed signal objects.\",\"parameters\":[{\"in\":\"query\",\"name\":"
    "\"event\",\"required\":false,\"schema\":{\"type\":\"string\"}}],\"respon"
    "ses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}},\"/get\":{\"d"
    "escription\":\"Get informations about a resource or element\",\"response"
    "s\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}},\"/list\":{\"de"
    "scription\":\"List all signals already configured\",\"responses\":{\"200"
    "\":{\"$ref\":\"#/components/responses/200\"}}},\"/loadConf\":{\"descript"
    "ion\":\"Load config file in directory passed as argument searching for p"
    "attern 'sig' in filename\",\"parameters\":[{\"in\":\"query\",\"name\":\""
    "path\",\"required\":true,\"schema\":{\"type\":\"string\"}}],\"responses\""
    ":{\"200\":{\"$ref\":\"#/components/responses/200\"}}}}}"
;

 void subscribe(struct afb_req req);
 void unsubscribe(struct afb_req req);
 void get(struct afb_req req);
 void list(struct afb_req req);
 void loadConf(struct afb_req req);

static const struct afb_verb_v2 _afb_verbs_v2_signal_composer[] = {
    {
        .verb = "subscribe",
        .callback = subscribe,
        .auth = NULL,
        .info = "Subscribe to a signal object",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "unsubscribe",
        .callback = unsubscribe,
        .auth = NULL,
        .info = "Unsubscribe previously suscribed signal objects.",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "get",
        .callback = get,
        .auth = NULL,
        .info = "Get informations about a resource or element",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "list",
        .callback = list,
        .auth = NULL,
        .info = "List all signals already configured",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = "loadConf",
        .callback = loadConf,
        .auth = NULL,
        .info = "Load config file in directory passed as argument searching for pattern 'sig' in filename",
        .session = AFB_SESSION_NONE_V2
    },
    {
        .verb = NULL,
        .callback = NULL,
        .auth = NULL,
        .info = NULL,
        .session = 0
	}
};

const struct afb_binding_v2 afbBindingV2 = {
    .api = "signal-composer",
    .specification = _afb_description_v2_signal_composer,
    .info = "",
    .verbs = _afb_verbs_v2_signal_composer,
    .preinit = loadConf,
    .init = execConf,
    .onevent = onEvent,
    .noconcurrency = 0
};

