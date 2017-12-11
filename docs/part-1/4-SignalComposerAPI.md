# Signal Composer API

## subscribe/unsubscribe

Using subscribe you can get update on change for signals you chose and you can
using wildcard to subscribe several signals in the same time.

```json
signal-composer subscribe {"signal": "rear_left*"}
ON-REPLY 1:signal-composer/subscribe: {"jtype":"afb-reply","request":{"status":"success","uuid":"3d4b743b-7ac6-4d3c-8fce-721107f9dee5"}}
```

Then event comes up like the following:

```json
ON-EVENT signal-composer/257b343e-8ea9-4cd7-8f9e-1904fa77f8f2({"event":"signal-composer\/257b343e-8ea9-4cd7-8f9e-1904fa77f8f2","data":{"uid":"rear_left_door","event":"low-can\/messages.doors.rear_left.open","timestamp":4833910845032292484,"value":false},"jtype":"afb-event"})
```

Unsubscribe happens the same way. When no more signals are holded by the client
then it unsubscribe from the *AGL Application Framework* event handle.

## get

You can get a signal value be requesting the API with the verb *get*:

```json
signal-composer get {"signal": "vehicle_speed", "options": {"average": 10}}
signal-composer get {"signal": "vehicle_speed", "options": {"minimum": 10}}
signal-composer get {"signal": "vehicle_speed", "options": {"maximum": 10}}
signal-composer get {"signal": "vehicle_speed"}
```

You apply apply some simple mathematical function by default present in the
binding, by default **last** is used:

- **average**: make an average on X latest seconds.
- **minimum**: return the minimum value found in the X latest seconds.
- **maximum**: return the maximum value found in the X latest seconds.
- **last**: return the latest value.

## list

Verb **list** will output the list of defined signals.

## loadConf

Verb **loadConf** let you add new files to be able to add new **sources** or
**signals**.

```json
signal-composer loadConf {"file": "/path/to/your/json/file.json"}
```
