# Plugins

Plugins are C/C++ shared library that is loaded by the binding to execute some simple routine. Routine could be on reception of a new signal or at sources
initialization time or signal subscription with the respective JSON field **onReceived**, **init** and **getSignals**.
