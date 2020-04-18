// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var CHECK = requireNative('logging').CHECK;
var eventBindings = bindingUtil ? undefined : require('event_bindings');
var idGeneratorNatives = requireNative('id_generator');
var utils = require('utils');
var validate = bindingUtil ? undefined : require('schemaUtils').validate;
var webRequestInternal = getInternalApi ?
    getInternalApi('webRequestInternal') :
    require('binding').Binding.create('webRequestInternal').generate();

function validateListenerArguments(
    eventName, extraArgSchemas, listenerArguments) {
  if (bindingUtil)
    bindingUtil.validateCustomSignature(eventName, listenerArguments);
  else
    validate(listenerArguments, extraArgSchemas);
}

function getUniqueSubEventName(eventName) {
  return eventName + '/' + idGeneratorNatives.GetNextId();
}

function createSubEvent(name, argSchemas) {
  if (bindingUtil) {
    var supportsFilters = false;
    var supportsLazyListeners = true;
    return bindingUtil.createCustomEvent(name, undefined,
                                         supportsFilters,
                                         supportsLazyListeners);
  }
  return new eventBindings.Event(name, argSchemas);
}

// WebRequestEventImpl object. This is used for special webRequest events
// with extra parameters. Each invocation of addListener creates a new named
// sub-event. That sub-event is associated with the extra parameters in the
// browser process, so that only it is dispatched when the main event occurs
// matching the extra parameters.
//
// Example:
//   chrome.webRequest.onBeforeRequest.addListener(
//       callback, {urls: 'http://*.google.com/*'});
//   ^ callback will only be called for onBeforeRequests matching the filter.
function WebRequestEventImpl(eventName, opt_argSchemas, opt_extraArgSchemas,
                             opt_eventOptions, opt_webViewInstanceId) {
  if (typeof eventName != 'string')
    throw new Error('chrome.WebRequestEvent requires an event name.');

  if (bindingUtil)
    bindingUtil.addCustomSignature(eventName, opt_extraArgSchemas);

  this.eventName = eventName;
  this.argSchemas = opt_argSchemas;
  this.extraArgSchemas = opt_extraArgSchemas;
  this.webViewInstanceId = opt_webViewInstanceId || 0;
  this.subEvents = [];
  if (eventBindings) {
    var eventOptions = eventBindings.parseEventOptions(opt_eventOptions);
    CHECK(!eventOptions.supportsRules, eventName + ' supports rules');
    CHECK(eventOptions.supportsListeners,
          eventName + ' does not support listeners');
  }
}
$Object.setPrototypeOf(WebRequestEventImpl.prototype, null);

// Test if the given callback is registered for this event.
WebRequestEventImpl.prototype.hasListener = function(cb) {
  return this.findListener_(cb) > -1;
};

// Test if any callbacks are registered fur thus event.
WebRequestEventImpl.prototype.hasListeners = function() {
  return this.subEvents.length > 0;
};

// Registers a callback to be called when this event is dispatched. If
// opt_filter is specified, then the callback is only called for events that
// match the given filters. If opt_extraInfo is specified, the given optional
// info is sent to the callback.
WebRequestEventImpl.prototype.addListener =
    function(cb, opt_filter, opt_extraInfo) {
  // NOTE(benjhayden) New APIs should not use this subEventName trick! It does
  // not play well with event pages. See downloads.onDeterminingFilename and
  // ExtensionDownloadsEventRouter for an alternative approach.
  var subEventName = getUniqueSubEventName(this.eventName);
  // Note: this could fail to validate, in which case we would not add the
  // subEvent listener.
  validateListenerArguments(this.eventName, this.extraArgSchemas,
                            $Array.slice(arguments, 1));
  webRequestInternal.addEventListener(
      cb, opt_filter, opt_extraInfo, this.eventName, subEventName,
      this.webViewInstanceId);

  var subEvent = createSubEvent(subEventName, this.argSchemas);
  var subEventCallback = cb;
  if (opt_extraInfo && $Array.indexOf(opt_extraInfo, 'blocking') >= 0) {
    var eventName = this.eventName;
    subEventCallback = function() {
      var requestId = arguments[0].requestId;
      try {
        var result = $Function.apply(cb, null, arguments);
        webRequestInternal.eventHandled(
            eventName, subEventName, requestId, result);
      } catch (e) {
        webRequestInternal.eventHandled(
            eventName, subEventName, requestId);
        throw e;
      }
    };
  } else if (
      opt_extraInfo && $Array.indexOf(opt_extraInfo, 'asyncBlocking') >= 0) {
    var eventName = this.eventName;
    subEventCallback = function() {
      var details = arguments[0];
      var requestId = details.requestId;
      var handledCallback = function(response) {
        webRequestInternal.eventHandled(
            eventName, subEventName, requestId, response);
      };
      $Function.apply(cb, null, [details, handledCallback]);
    };
  }
  $Array.push(this.subEvents,
      {subEvent: subEvent, callback: cb, subEventCallback: subEventCallback});
  subEvent.addListener(subEventCallback);
};

// Unregisters a callback.
WebRequestEventImpl.prototype.removeListener = function(cb) {
  var idx;
  while ((idx = this.findListener_(cb)) >= 0) {
    var e = this.subEvents[idx];
    e.subEvent.removeListener(e.subEventCallback);
    if (e.subEvent.hasListeners()) {
      console.error(
          'Internal error: webRequest subEvent has orphaned listeners.');
    }
    $Array.splice(this.subEvents, idx, 1);
  }
};

WebRequestEventImpl.prototype.findListener_ = function(cb) {
  for (var i in this.subEvents) {
    var e = this.subEvents[i];
    if (e.callback === cb) {
      if (e.subEvent.hasListener(e.subEventCallback))
        return i;
      console.error('Internal error: webRequest subEvent has no callback.');
    }
  }

  return -1;
};

WebRequestEventImpl.prototype.addRules = function(rules, opt_cb) {
  throw new Error('This event does not support rules.');
};

WebRequestEventImpl.prototype.removeRules =
    function(ruleIdentifiers, opt_cb) {
  throw new Error('This event does not support rules.');
};

WebRequestEventImpl.prototype.getRules = function(ruleIdentifiers, cb) {
  throw new Error('This event does not support rules.');
};

function WebRequestEvent() {
  privates(WebRequestEvent).constructPrivate(this, arguments);
}

// Our util code requires we construct a new WebRequestEvent via a call to
// 'new WebRequestEvent', which wouldn't work well with calling a v8::Function.
// Provide a wrapper for native bindings to call into.
function createWebRequestEvent(eventName, opt_argSchemas, opt_extraArgSchemas,
                               opt_eventOptions, opt_webViewInstanceId) {
  return new WebRequestEvent(eventName, opt_argSchemas, opt_extraArgSchemas,
                             opt_eventOptions, opt_webViewInstanceId);
}

utils.expose(WebRequestEvent, WebRequestEventImpl, {
  functions: [
    'hasListener',
    'hasListeners',
    'addListener',
    'removeListener',
    'addRules',
    'removeRules',
    'getRules',
  ],
});

exports.$set('WebRequestEvent', WebRequestEvent);
exports.$set('createWebRequestEvent', createWebRequestEvent);
