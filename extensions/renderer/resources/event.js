// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(robwu): Fix indentation.

  var exceptionHandler = require('uncaught_exception_handler');
  var eventNatives = requireNative('event_natives');
  var logging = requireNative('logging');
  var schemaRegistry = requireNative('schema_registry');
  var sendRequest = require('sendRequest').sendRequest;
  var utils = require('utils');
  var validate = require('schemaUtils').validate;

  // Schemas for the rule-style functions on the events API that
  // only need to be generated occasionally, so populate them lazily.
  var ruleFunctionSchemas = {
    __proto__: null,
    // These values are set lazily:
    // addRules: {},
    // getRules: {},
    // removeRules: {}
  };

  // This function ensures that |ruleFunctionSchemas| is populated.
  function ensureRuleSchemasLoaded() {
    if (ruleFunctionSchemas.addRules)
      return;
    var eventsSchema = schemaRegistry.GetSchema("events");
    var eventType = utils.lookup(eventsSchema.types, 'id', 'events.Event');

    ruleFunctionSchemas.addRules =
        utils.lookup(eventType.functions, 'name', 'addRules');
    ruleFunctionSchemas.getRules =
        utils.lookup(eventType.functions, 'name', 'getRules');
    ruleFunctionSchemas.removeRules =
        utils.lookup(eventType.functions, 'name', 'removeRules');
  }

  // A map of event names to the event object that is registered to that name.
  var attachedNamedEvents = {__proto__: null};

  // A map of functions that massage event arguments before they are dispatched.
  // Key is event name, value is function.
  var eventArgumentMassagers = {__proto__: null};

  // An attachment strategy for events that aren't attached to the browser.
  // This applies to events with the "unmanaged" option and events without
  // names.
  function NullAttachmentStrategy(event) {
    this.event_ = event;
  }
  $Object.setPrototypeOf(NullAttachmentStrategy.prototype, null);

  NullAttachmentStrategy.prototype.onAddedListener =
      function(listener) {
    // For named events, we still inform the messaging bindings when a listener
    // is registered to allow for native checking if a listener is registered.
    if (this.event_.eventName &&
        this.event_.listeners.length == 0) {
      eventNatives.AttachUnmanagedEvent(this.event_.eventName);
    }
  };

  NullAttachmentStrategy.prototype.onRemovedListener =
      function(listener) {
    if (this.event_.eventName &&
        this.event_.listeners.length == 0) {
      this.detach(true);
    }
  };

  NullAttachmentStrategy.prototype.detach = function(manual) {
    if (this.event_.eventName)
      eventNatives.DetachUnmanagedEvent(this.event_.eventName);
  };

  NullAttachmentStrategy.prototype.getListenersByIDs = function(ids) {
    // |ids| is for filtered events only.
    return this.event_.listeners;
  };

  // Handles adding/removing/dispatching listeners for unfiltered events.
  function UnfilteredAttachmentStrategy(event) {
    this.event_ = event;
  }
  $Object.setPrototypeOf(UnfilteredAttachmentStrategy.prototype, null);

  UnfilteredAttachmentStrategy.prototype.onAddedListener =
      function(listener) {
    // Only attach / detach on the first / last listener removed.
    if (this.event_.listeners.length == 0)
      eventNatives.AttachEvent(this.event_.eventName,
                               this.event_.eventOptions.supportsLazyListeners);
  };

  UnfilteredAttachmentStrategy.prototype.onRemovedListener =
      function(listener) {
    if (this.event_.listeners.length == 0)
      this.detach(true);
  };

  UnfilteredAttachmentStrategy.prototype.detach = function(manual) {
    eventNatives.DetachEvent(this.event_.eventName, manual,
                             this.event_.eventOptions.supportsLazyListeners);
  };

  UnfilteredAttachmentStrategy.prototype.getListenersByIDs = function(ids) {
    // |ids| is for filtered events only.
    return this.event_.listeners;
  };

  function FilteredAttachmentStrategy(event) {
    this.event_ = event;
    this.listenerMap_ = {__proto__: null};
  }
  $Object.setPrototypeOf(FilteredAttachmentStrategy.prototype, null);

  utils.defineProperty(FilteredAttachmentStrategy, 'idToEventMap',
      {__proto__: null});

  FilteredAttachmentStrategy.prototype.onAddedListener = function(listener) {
    var id = eventNatives.AttachFilteredEvent(
                 this.event_.eventName, listener.filters || {},
                 this.event_.eventOptions.supportsLazyListeners);
    if (id == -1)
      throw new Error("Can't add listener");
    listener.id = id;
    this.listenerMap_[id] = listener;
    FilteredAttachmentStrategy.idToEventMap[id] = this.event_;
  };

  FilteredAttachmentStrategy.prototype.onRemovedListener = function(listener) {
    this.detachListener(listener, true);
  };

  FilteredAttachmentStrategy.prototype.detachListener =
      function(listener, manual) {
    if (listener.id == undefined)
      throw new Error("listener.id undefined - '" + listener + "'");
    var id = listener.id;
    delete this.listenerMap_[id];
    delete FilteredAttachmentStrategy.idToEventMap[id];
    eventNatives.DetachFilteredEvent(
        id, manual, this.event_.eventOptions.supportsLazyListeners);
  };

  FilteredAttachmentStrategy.prototype.detach = function(manual) {
    for (var i in this.listenerMap_)
      this.detachListener(this.listenerMap_[i], manual);
  };

  FilteredAttachmentStrategy.prototype.getListenersByIDs = function(ids) {
    var result = [];
    for (var i = 0; i < ids.length; i++)
      $Array.push(result, this.listenerMap_[ids[i]]);
    return result;
  };

  function parseEventOptions(opt_eventOptions) {
    return $Object.assign({
      __proto__: null,
    }, {
      // Event supports adding listeners with filters ("filtered events"), for
      // example as used in the webNavigation API.
      //
      // event.addListener(listener, [filter1, filter2]);
      supportsFilters: false,

      // Events supports vanilla events. Most APIs use these.
      //
      // event.addListener(listener);
      supportsListeners: true,

      // Event supports lazy listeners, where an extension can register a
      // listener to be used to "wake up" a lazy context.
      supportsLazyListeners: true,

      // Event supports adding rules ("declarative events") rather than
      // listeners, for example as used in the declarativeWebRequest API.
      //
      // event.addRules([rule1, rule2]);
      supportsRules: false,

      // Event is unmanaged in that the browser has no knowledge of its
      // existence; it's never invoked, doesn't keep the renderer alive, and
      // the bindings system has no knowledge of it.
      //
      // Both events created by user code (new chrome.Event()) and messaging
      // events are unmanaged, though in the latter case the browser *does*
      // interact indirectly with them via IPCs written by hand.
      unmanaged: false,
    }, opt_eventOptions);
  }

  // Event object.  If opt_eventName is provided, this object represents
  // the unique instance of that named event, and dispatching an event
  // with that name will route through this object's listeners. Note that
  // opt_eventName is required for events that support rules.
  //
  // Example:
  //   var Event = require('event_bindings').Event;
  //   chrome.tabs.onChanged = new Event("tab-changed");
  //   chrome.tabs.onChanged.addListener(function(data) { alert(data); });
  //   Event.dispatch("tab-changed", "hi");
  // will result in an alert dialog that says 'hi'.
  //
  // If opt_eventOptions exists, it is a dictionary that contains the boolean
  // entries "supportsListeners" and "supportsRules".
  // If opt_webViewInstanceId exists, it is an integer uniquely identifying a
  // <webview> tag within the embedder. If it does not exist, then this is an
  // extension event rather than a <webview> event.
  function EventImpl(opt_eventName, opt_argSchemas, opt_eventOptions,
                     opt_webViewInstanceId) {
    this.eventName = opt_eventName;
    this.argSchemas = opt_argSchemas;
    this.listeners = [];
    this.eventOptions = parseEventOptions(opt_eventOptions);
    this.webViewInstanceId = opt_webViewInstanceId || 0;

    if (!this.eventName) {
      if (this.eventOptions.supportsRules)
        throw new Error("Events that support rules require an event name.");
      // Events without names cannot be managed by the browser by definition
      // (the browser has no way of identifying them).
      this.eventOptions.unmanaged = true;
    }

    // Track whether the event has been destroyed as a sanity check.
    this.destroyed = false;

    if (this.eventOptions.unmanaged)
      this.attachmentStrategy = new NullAttachmentStrategy(this);
    else if (this.eventOptions.supportsFilters)
      this.attachmentStrategy = new FilteredAttachmentStrategy(this);
    else
      this.attachmentStrategy = new UnfilteredAttachmentStrategy(this);
  }
  $Object.setPrototypeOf(EventImpl.prototype, null);

  // callback is a function(args, dispatch). args are the args we receive from
  // dispatchEvent(), and dispatch is a function(args) that dispatches args to
  // its listeners.
  function registerArgumentMassager(name, callback) {
    if (eventArgumentMassagers[name])
      throw new Error("Massager already registered for event: " + name);
    eventArgumentMassagers[name] = callback;
  }

  // Dispatches a named event with the given argument array. The args array is
  // the list of arguments that will be sent to the event callback.
  // |listenerIds| contains the ids of matching listeners, or is an empty array
  // for all listeners.
  function dispatchEvent(name, args, listenerIds) {
    var event = attachedNamedEvents[name];
    if (!event)
      return;

    var dispatchArgs = function(args) {
      var result = event.dispatch_(args, listenerIds);
      if (result)
        logging.DCHECK(!result.validationErrors, result.validationErrors);
      return result;
    };

    if (eventArgumentMassagers[name])
      eventArgumentMassagers[name](args, dispatchArgs);
    else
      dispatchArgs(args);
  }

  // Registers a callback to be called when this event is dispatched.
  EventImpl.prototype.addListener = function(cb, filters) {
    if (!this.eventOptions.supportsListeners)
      throw new Error("This event does not support listeners.");
    if (this.eventOptions.maxListeners &&
        this.getListenerCount_() >= this.eventOptions.maxListeners) {
      throw new Error("Too many listeners for " + this.eventName);
    }
    if (filters) {
      if (!this.eventOptions.supportsFilters)
        throw new Error("This event does not support filters.");
      if (filters.url && !(filters.url instanceof Array))
        throw new Error("filters.url should be an array.");
      if (filters.serviceType &&
          !(typeof filters.serviceType === 'string')) {
        throw new Error("filters.serviceType should be a string.")
      }
    }
    var listener = {callback: cb, filters: filters};
    this.attach_(listener);
    $Array.push(this.listeners, listener);
  };

  EventImpl.prototype.attach_ = function(listener) {
    this.attachmentStrategy.onAddedListener(listener);

    if (this.listeners.length == 0) {
      if (this.eventName) {
        if (attachedNamedEvents[this.eventName]) {
          throw new Error("Event '" + this.eventName +
                          "' is already attached.");
        }
        attachedNamedEvents[this.eventName] = this;
      }
    }
  };

  // Unregisters a callback.
  EventImpl.prototype.removeListener = function(cb) {
    if (!this.eventOptions.supportsListeners)
      throw new Error("This event does not support listeners.");

    var idx = this.findListener_(cb);
    if (idx == -1)
      return;

    var removedListener = $Array.splice(this.listeners, idx, 1)[0];
    this.attachmentStrategy.onRemovedListener(removedListener);

    if (this.listeners.length == 0) {
      if (this.eventName) {
        if (!attachedNamedEvents[this.eventName]) {
          throw new Error(
              "Event '" + this.eventName + "' is not attached.");
        }
        delete attachedNamedEvents[this.eventName];
      }
    }
  };

  // Test if the given callback is registered for this event.
  EventImpl.prototype.hasListener = function(cb) {
    if (!this.eventOptions.supportsListeners)
      throw new Error("This event does not support listeners.");
    return this.findListener_(cb) > -1;
  };

  // Test if any callbacks are registered for this event.
  EventImpl.prototype.hasListeners = function() {
    return this.getListenerCount_() > 0;
  };

  // Returns the number of listeners on this event.
  EventImpl.prototype.getListenerCount_ = function() {
    if (!this.eventOptions.supportsListeners)
      throw new Error("This event does not support listeners.");
    return this.listeners.length;
  };

  // Returns the index of the given callback if registered, or -1 if not
  // found.
  EventImpl.prototype.findListener_ = function(cb) {
    for (var i = 0; i < this.listeners.length; i++) {
      if (this.listeners[i].callback == cb) {
        return i;
      }
    }

    return -1;
  };

  EventImpl.prototype.dispatch_ = function(args, listenerIDs) {
    if (this.destroyed) {
      throw new Error(this.eventName + ' was already destroyed');
    }
    if (!this.eventOptions.supportsListeners)
      throw new Error("This event does not support listeners.");

    if (this.argSchemas && logging.DCHECK_IS_ON()) {
      try {
        validate(args, this.argSchemas);
      } catch (e) {
        e.message += ' in ' + this.eventName;
        throw e;
      }
    }

    // Make a copy of the listeners in case the listener list is modified
    // while dispatching the event.
    var listeners = $Array.slice(
        this.attachmentStrategy.getListenersByIDs(listenerIDs));

    var results = [];
    for (var i = 0; i < listeners.length; i++) {
      try {
        var result = this.wrapper.dispatchToListener(listeners[i].callback,
                                                     args);
        if (result !== undefined)
          $Array.push(results, result);
      } catch (e) {
        exceptionHandler.handle('Error in event handler for ' +
            (this.eventName ? this.eventName : '(unknown)'),
          e);
      }
    }
    if (results.length)
      return {results: results};
  }

  // Can be overridden to support custom dispatching.
  EventImpl.prototype.dispatchToListener = function(callback, args) {
    return $Function.apply(callback, null, args);
  }

  // Dispatches this event object to all listeners, passing all supplied
  // arguments to this function each listener.
  EventImpl.prototype.dispatch = function(varargs) {
    return this.dispatch_($Array.slice(arguments), undefined);
  };

  // Detaches this event object from its name.
  EventImpl.prototype.detach_ = function() {
    this.attachmentStrategy.detach(false);
  };

  EventImpl.prototype.destroy_ = function() {
    this.listeners.length = 0;
    this.detach_();
    this.destroyed = true;
  };

  EventImpl.prototype.addRules = function(rules, opt_cb) {
    if (!this.eventOptions.supportsRules)
      throw new Error("This event does not support rules.");

    // Takes a list of JSON datatype identifiers and returns a schema fragment
    // that verifies that a JSON object corresponds to an array of only these
    // data types.
    function buildArrayOfChoicesSchema(typesList) {
      return {
        __proto__: null,
        'type': 'array',
        'items': {
          __proto__: null,
          'choices': $Array.map(typesList, function(el) {
            return {
              __proto__: null,
              '$ref': el,
            };
          }),
        }
      };
    }

    // Validate conditions and actions against specific schemas of this
    // event object type.
    // |rules| is an array of JSON objects that follow the Rule type of the
    // declarative extension APIs. |conditions| is an array of JSON type
    // identifiers that are allowed to occur in the conditions attribute of each
    // rule. Likewise, |actions| is an array of JSON type identifiers that are
    // allowed to occur in the actions attribute of each rule.
    function validateRules(rules, conditions, actions) {
      var conditionsSchema = buildArrayOfChoicesSchema(conditions);
      var actionsSchema = buildArrayOfChoicesSchema(actions);
      $Array.forEach(rules, function(rule) {
        validate([rule.conditions], [conditionsSchema]);
        validate([rule.actions], [actionsSchema]);
      });
    };

    if (!this.eventOptions.conditions || !this.eventOptions.actions) {
      throw new Error('Event ' + this.eventName + ' misses ' +
                      'conditions or actions in the API specification.');
    }

    validateRules(rules,
                  this.eventOptions.conditions,
                  this.eventOptions.actions);

    ensureRuleSchemasLoaded();
    // We remove the first parameter from the validation to give the user more
    // meaningful error messages.
    validate([this.webViewInstanceId, rules, opt_cb],
        $Array.slice(ruleFunctionSchemas.addRules.parameters, 1));
    sendRequest(
      "events.addRules",
      [this.eventName, this.webViewInstanceId, rules,  opt_cb],
      ruleFunctionSchemas.addRules.parameters);
  }

  EventImpl.prototype.removeRules = function(ruleIdentifiers, opt_cb) {
    if (!this.eventOptions.supportsRules)
      throw new Error("This event does not support rules.");
    ensureRuleSchemasLoaded();
    // We remove the first parameter from the validation to give the user more
    // meaningful error messages.
    validate([this.webViewInstanceId, ruleIdentifiers, opt_cb],
        $Array.slice(ruleFunctionSchemas.removeRules.parameters, 1));
    sendRequest("events.removeRules",
                [this.eventName,
                 this.webViewInstanceId,
                 ruleIdentifiers,
                 opt_cb],
                ruleFunctionSchemas.removeRules.parameters);
  }

  EventImpl.prototype.getRules = function(ruleIdentifiers, cb) {
    if (!this.eventOptions.supportsRules)
      throw new Error("This event does not support rules.");
    ensureRuleSchemasLoaded();
    // We remove the first parameter from the validation to give the user more
    // meaningful error messages.
    validate([this.webViewInstanceId, ruleIdentifiers, cb],
        $Array.slice(ruleFunctionSchemas.getRules.parameters, 1));

    sendRequest(
      "events.getRules",
      [this.eventName, this.webViewInstanceId, ruleIdentifiers, cb],
      ruleFunctionSchemas.getRules.parameters);
  }

  function Event() {
    privates(Event).constructPrivate(this, arguments);
  }
  utils.expose(Event, EventImpl, {
    functions: [
      'addListener',
      'removeListener',
      'hasListener',
      'hasListeners',
      'dispatchToListener',
      'dispatch',
      'addRules',
      'removeRules',
      'getRules',
    ],
  });

  // NOTE: Event is (lazily) exposed as chrome.Event from dispatcher.cc.
  exports.$set('Event', Event);

  exports.$set('dispatchEvent', dispatchEvent);
  exports.$set('parseEventOptions', parseEventOptions);
  exports.$set('registerArgumentMassager', registerArgumentMassager);
