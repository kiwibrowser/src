// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/extension_urls.h"
#include "extensions/grit/extensions_renderer_resources.h"
#include "extensions/renderer/module_system_test.h"

namespace extensions {
namespace {

class EventUnittest : public ModuleSystemTest {
  void SetUp() override {
    ModuleSystemTest::SetUp();

    env()->RegisterModule(kEventBindings, IDR_EVENT_BINDINGS_JS);
    env()->RegisterModule("json_schema", IDR_JSON_SCHEMA_JS);
    env()->RegisterModule(kSchemaUtils, IDR_SCHEMA_UTILS_JS);
    env()->RegisterModule("uncaught_exception_handler",
                          IDR_UNCAUGHT_EXCEPTION_HANDLER_JS);
    env()->RegisterModule("utils", IDR_UTILS_JS);

    // Mock out the native handler for event_bindings. These mocks will fail if
    // any invariants maintained by the real event_bindings are broken.
    env()->OverrideNativeHandler(
        "event_natives",
        "var assert = requireNative('assert');"
        "exports.$set('attachedListeners', {});"
        "var attachedListeners = exports.attachedListeners;"
        "exports.$set('attachedFilteredListeners', {});"
        "var attachedFilteredListeners = exports.attachedFilteredListeners;"
        "var nextId = 0;"
        "var idToName = {};"
        "exports.$set('AttachEvent', function(eventName) {"
        "  assert.AssertFalse(!!attachedListeners[eventName]);"
        "  attachedListeners[eventName] = 1;"
        "});"
        "exports.$set('DetachEvent', function(eventName) {"
        "  assert.AssertTrue(!!attachedListeners[eventName]);"
        "  delete attachedListeners[eventName];"
        "});"
        "exports.$set('IsEventAttached', function(eventName) {"
        "  return !!attachedListeners[eventName];"
        "});"
        "exports.$set('AttachFilteredEvent', function(name, filters) {"
        "  var id = nextId++;"
        "  idToName[id] = name;"
        "  attachedFilteredListeners[name] ="
        "    attachedFilteredListeners[name] || [];"
        "  attachedFilteredListeners[name][id] = filters;"
        "  return id;"
        "});"
        "exports.$set('DetachFilteredEvent', function(id, manual) {"
        "  var i = attachedFilteredListeners[idToName[id]].indexOf(id);"
        "  attachedFilteredListeners[idToName[id]].splice(i, 1);"
        "});"
        "exports.$set('HasFilteredListener', function(name) {"
        "  return attachedFilteredListeners[name].length;"
        "});");
    env()->OverrideNativeHandler("sendRequest",
                                 "exports.$set('sendRequest', function() {});");
    env()->OverrideNativeHandler(
        "apiDefinitions",
        "exports.$set('GetExtensionAPIDefinitionsForTest', function() {});");
    env()->OverrideNativeHandler("logging",
                                 "exports.$set('DCHECK', function() {});");
    env()->OverrideNativeHandler("schema_registry",
                                 "exports.$set('GetSchema', function() {});");
  }
};

TEST_F(EventUnittest, TestNothing) {
  ExpectNoAssertionsMade();
}

TEST_F(EventUnittest, AddRemoveTwoListeners) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule(
      "test",
      "var assert = requireNative('assert');"
      "var Event = require('event_bindings').Event;"
      "var eventNatives = requireNative('event_natives');"
      "var myEvent = new Event('named-event');"
      "var cb1 = function() {};"
      "var cb2 = function() {};"
      "myEvent.addListener(cb1);"
      "myEvent.addListener(cb2);"
      "myEvent.removeListener(cb1);"
      "assert.AssertTrue(!!eventNatives.attachedListeners['named-event']);"
      "myEvent.removeListener(cb2);"
      "assert.AssertFalse(!!eventNatives.attachedListeners['named-event']);");
  env()->module_system()->Require("test");
}

TEST_F(EventUnittest, EventsThatSupportRulesMustHaveAName) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule(
      "test",
      "var Event = require('event_bindings').Event;"
      "var eventOpts = {supportsRules: true};"
      "var assert = requireNative('assert');"
      "var caught = false;"
      "try {"
      "  var myEvent = new Event(undefined, undefined, eventOpts);"
      "} catch (e) {"
      "  caught = true;"
      "}"
      "assert.AssertTrue(caught);");
  env()->module_system()->Require("test");
}

TEST_F(EventUnittest, NamedEventDispatch) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule(
      "test",
      "var Event = require('event_bindings').Event;"
      "var dispatchEvent = require('event_bindings').dispatchEvent;"
      "var assert = requireNative('assert');"
      "var e = new Event('myevent');"
      "var called = false;"
      "e.addListener(function() { called = true; });"
      "dispatchEvent('myevent', []);"
      "assert.AssertTrue(called);");
  env()->module_system()->Require("test");
}

TEST_F(EventUnittest, AddListenerWithFiltersThrowsErrorByDefault) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule("test",
                        "var Event = require('event_bindings').Event;"
                        "var assert = requireNative('assert');"
                        "var e = new Event('myevent');"
                        "var filter = [{"
                        "  url: {hostSuffix: 'google.com'},"
                        "}];"
                        "var caught = false;"
                        "try {"
                        "  e.addListener(function() {}, filter);"
                        "} catch (e) {"
                        "  caught = true;"
                        "}"
                        "assert.AssertTrue(caught);");
  env()->module_system()->Require("test");
}

TEST_F(EventUnittest, FilteredEventsAttachment) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule(
      "test",
      "var Event = require('event_bindings').Event;"
      "var assert = requireNative('assert');"
      "var bindings = requireNative('event_natives');"
      "var eventOpts = {supportsListeners: true, supportsFilters: true};"
      "var e = new Event('myevent', undefined, eventOpts);"
      "var cb = function() {};"
      "var filters = {url: [{hostSuffix: 'google.com'}]};"
      "e.addListener(cb, filters);"
      "assert.AssertTrue(bindings.HasFilteredListener('myevent'));"
      "e.removeListener(cb);"
      "assert.AssertFalse(bindings.HasFilteredListener('myevent'));");
  env()->module_system()->Require("test");
}

TEST_F(EventUnittest, DetachFilteredEvent) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule(
      "test",
      "var Event = require('event_bindings').Event;"
      "var assert = requireNative('assert');"
      "var bindings = requireNative('event_natives');"
      "var eventOpts = {supportsListeners: true, supportsFilters: true};"
      "var e = new Event('myevent', undefined, eventOpts);"
      "var cb1 = function() {};"
      "var cb2 = function() {};"
      "var filters = {url: [{hostSuffix: 'google.com'}]};"
      "e.addListener(cb1, filters);"
      "e.addListener(cb2, filters);"
      "privates(e).impl.detach_();"
      "assert.AssertFalse(bindings.HasFilteredListener('myevent'));");
  env()->module_system()->Require("test");
}

TEST_F(EventUnittest, AttachAndRemoveSameFilteredEventListener) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule(
      "test",
      "var Event = require('event_bindings').Event;"
      "var assert = requireNative('assert');"
      "var bindings = requireNative('event_natives');"
      "var eventOpts = {supportsListeners: true, supportsFilters: true};"
      "var e = new Event('myevent', undefined, eventOpts);"
      "var cb = function() {};"
      "var filters = {url: [{hostSuffix: 'google.com'}]};"
      "e.addListener(cb, filters);"
      "e.addListener(cb, filters);"
      "assert.AssertTrue(bindings.HasFilteredListener('myevent'));"
      "e.removeListener(cb);"
      "assert.AssertTrue(bindings.HasFilteredListener('myevent'));"
      "e.removeListener(cb);"
      "assert.AssertFalse(bindings.HasFilteredListener('myevent'));");
  env()->module_system()->Require("test");
}

TEST_F(EventUnittest, AddingFilterWithUrlFieldNotAListThrowsException) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule(
      "test",
      "var Event = require('event_bindings').Event;"
      "var assert = requireNative('assert');"
      "var eventOpts = {supportsListeners: true, supportsFilters: true};"
      "var e = new Event('myevent', undefined, eventOpts);"
      "var cb = function() {};"
      "var filters = {url: {hostSuffix: 'google.com'}};"
      "var caught = false;"
      "try {"
      "  e.addListener(cb, filters);"
      "} catch (e) {"
      "  caught = true;"
      "}"
      "assert.AssertTrue(caught);");
  env()->module_system()->Require("test");
}

TEST_F(EventUnittest, MaxListeners) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(
      env()->module_system());
  env()->RegisterModule(
      "test",
      "var Event = require('event_bindings').Event;"
      "var assert = requireNative('assert');"
      "var eventOpts = {supportsListeners: true, maxListeners: 1};"
      "var e = new Event('myevent', undefined, eventOpts);"
      "var cb = function() {};"
      "var caught = false;"
      "try {"
      "  e.addListener(cb);"
      "} catch (e) {"
      "  caught = true;"
      "}"
      "assert.AssertTrue(!caught);"
      "try {"
      "  e.addListener(cb);"
      "} catch (e) {"
      "  caught = true;"
      "}"
      "assert.AssertTrue(caught);");
  env()->module_system()->Require("test");
}

}  // namespace
}  // namespace extensions
