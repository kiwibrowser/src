# Bindings

This directory contains classes and functionality used to implement the V8 bindings layer in Blink. Any reusable bindings components/infrastructure that are independent of `core/` objects (or can be generalized to be independent) should be added to this directory, otherwise they can be kept in `bindings/core/`.

Some of the things you can find here are:

* Functionality to wrap Blink C++ objects with a JavaScript object and maintain wrappers in multiple worlds (see [ScriptWrappable](ScriptWrappable.h), [ActiveScriptWrappable](ActiveScriptWrappable.h))
* Implementation of wrapper tracing (see [documentation](TraceWrapperReference.md))
* Important abstractions for script execution (see [ScriptState](ScriptState.h), [V8PerIsolateData](V8PerIsolateData.h), [V8PerContextData](V8PerContextData.h))
* Utility functions to interface with V8 and convert between V8 and Blink types (see [V8Binding.h](V8Binding.h), [ToV8.h](ToV8.h))
