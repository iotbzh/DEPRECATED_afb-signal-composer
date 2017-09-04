
static const char _afb_description_v2_signals_composer[] =
    "{\"openapi\":\"3.0.0\",\"$schema\":\"http:iot.bzh/download/openapi/schem"
    "a-3.0/default-schema.json\",\"info\":{\"description\":\"\",\"title\":\"s"
    "ignals-composer-service\",\"version\":\"4.0\",\"x-binding-c-generator\":"
    "{\"api\":\"signals-composer\",\"version\":2,\"prefix\":\"\",\"postfix\":"
    "\"\",\"start\":null,\"onevent\":\"onEvent\",\"init\":\"init_service\",\""
    "scope\":\"\",\"private\":false}},\"servers\":[{\"url\":\"ws://{host}:{po"
    "rt}/api/monitor\",\"description\":\"Signals composer API connected to lo"
    "w level AGL services\",\"variables\":{\"host\":{\"default\":\"localhost\""
    "},\"port\":{\"default\":\"1234\"}},\"x-afb-events\":[{\"$ref\":\"#/compo"
    "nents/schemas/afb-event\"}]}],\"components\":{\"schemas\":{\"afb-reply\""
    ":{\"$ref\":\"#/components/schemas/afb-reply-v2\"},\"afb-event\":{\"$ref\""
    ":\"#/components/schemas/afb-event-v2\"},\"afb-reply-v2\":{\"title\":\"Ge"
    "neric response.\",\"type\":\"object\",\"required\":[\"jtype\",\"request\""
    "],\"properties\":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-reply\""
    "},\"request\":{\"type\":\"object\",\"required\":[\"status\"],\"propertie"
    "s\":{\"status\":{\"type\":\"string\"},\"info\":{\"type\":\"string\"},\"t"
    "oken\":{\"type\":\"string\"},\"uuid\":{\"type\":\"string\"},\"reqid\":{\""
    "type\":\"string\"}}},\"response\":{\"type\":\"object\"}}},\"afb-event-v2"
    "\":{\"type\":\"object\",\"required\":[\"jtype\",\"event\"],\"properties\""
    ":{\"jtype\":{\"type\":\"string\",\"const\":\"afb-event\"},\"event\":{\"t"
    "ype\":\"string\"},\"data\":{\"type\":\"object\"}}}},\"x-permissions\":{}"
    ",\"responses\":{\"200\":{\"description\":\"A complex object array respon"
    "se\",\"content\":{\"application/json\":{\"schema\":{\"$ref\":\"#/compone"
    "nts/schemas/afb-reply\"}}}}}},\"paths\":{\"/subscribe\":{\"description\""
    ":\"Subscribe to a signal object\",\"parameters\":[{\"in\":\"query\",\"na"
    "me\":\"event\",\"required\":false,\"schema\":{\"type\":\"string\"}}],\"r"
    "esponses\":{\"200\":{\"$ref\":\"#/components/responses/200\"}}},\"/unsub"
    "scribe\":{\"description\":\"Unsubscribe previously suscribed signal obje"
    "cts.\",\"parameters\":[{\"in\":\"query\",\"name\":\"event\",\"required\""
    ":false,\"schema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$ref"
    "\":\"#/components/responses/200\"}}},\"/get\":{\"description\":\"Get inf"
    "ormations about a resource or element\",\"responses\":{\"200\":{\"$ref\""
    ":\"#/components/responses/200\"}}},\"/load\":{\"description\":\"Load con"
    "fig file in directory passed as argument searching for pattern 'sig' in "
    "filename\",\"parameters\":[{\"in\":\"query\",\"name\":\"path\",\"require"
    "d\":true,\"schema\":{\"type\":\"string\"}}],\"responses\":{\"200\":{\"$r"
    "ef\":\"#/components/responses/200\"}}}}}"
;

 void subscribe(struct afb_req req);
 void unsubscribe(struct afb_req req);
 void get(struct afb_req req);
 void load(struct afb_req req);

static const struct afb_verb_v2 _afb_verbs_v2_signals_composer[] = {
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
        .verb = "load",
        .callback = load,
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
    .api = "signals-composer",
    .specification = _afb_description_v2_signals_composer,
    .info = "",
    .verbs = _afb_verbs_v2_signals_composer,
    .preinit = NULL,
    .init = init_service,
    .onevent = onEvent,
    .noconcurrency = 0
};

