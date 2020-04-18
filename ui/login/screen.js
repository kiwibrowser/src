// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Base class for all login WebUI screens.
 */
cr.define('login', function() {
  /** @const */ var CALLBACK_CONTEXT_CHANGED = 'contextChanged';
  /** @const */ var CALLBACK_USER_ACTED = 'userActed';

  function doNothing() {};

  function alwaysTruePredicate() { return true; }

  var querySelectorAll = HTMLDivElement.prototype.querySelectorAll;

  var Screen = function(sendPrefix) {
    this.sendPrefix_ = sendPrefix;
    this.screenContext_ = null;
    this.contextObservers_ = {};
  };

  Screen.prototype = {
    __proto__: HTMLDivElement.prototype,

    /**
     * Prefix added to sent to Chrome messages' names.
     */
    sendPrefix_: null,

    /**
     * Context used by this screen.
     */
    screenContext_: null,

    get context() {
      return this.screenContext_;
    },

    /**
     * Dictionary of context observers that are methods of |this| bound to
     * |this|.
     */
    contextObservers_: null,

    /**
     * Called during screen initialization.
     */
    decorate: doNothing,

    /**
     * Returns minimal size that screen prefers to have. Default implementation
     * returns current screen size.
     * @return {{width: number, height: number}}
     */
    getPreferredSize: function() {
      return {width: this.offsetWidth, height: this.offsetHeight};
    },

    /**
     * Called for currently active screen when screen size changed.
     */
    onWindowResize: doNothing,

    /**
     * @final
     */
    initialize: function() {
      return this.initializeImpl_.apply(this, arguments);
    },

    /**
     * @final
     */
    send: function() {
      return this.sendImpl_.apply(this, arguments);
    },

    /**
     * @final
     */
    addContextObserver: function() {
      return this.addContextObserverImpl_.apply(this, arguments);
    },

    /**
     * @final
     */
    removeContextObserver: function() {
      return this.removeContextObserverImpl_.apply(this, arguments);
    },

    /**
     * @final
     */
    commitContextChanges: function() {
      return this.commitContextChangesImpl_.apply(this, arguments);
    },

    /**
     * Creates and returns new button element with given identifier
     * and on-click event listener, which sends notification about
     * user action to the C++ side.
     *
     * @param {string} id Identifier of a button.
     * @param {string} opt_action_id Identifier of user action.
     * @final
     */
    declareButton: function(id, opt_action_id) {
      var button = this.ownerDocument.createElement('button');
      button.id = id;
      this.declareUserAction(button,
                             { action_id: opt_action_id,
                               event: 'click'
                             });
      return button;
    },

    /**
      * Adds event listener to an element which sends notification
      * about event to the C++ side.
      *
      * @param {Element} element An DOM element
      * @param {Object} options A dictionary of optional arguments:
      *   {string} event: name of event that will be listened,
      *            default: 'click'.
      *   {string} action_id: name of an action which will be sent to
      *                       the C++ side.
      *   {function} condition: a one-argument function which takes
      *              event as an argument, notification is sent to the
      *              C++ side iff condition is true, default: constant
      *              true function.
      * @final
      */
    declareUserAction: function(element, options) {
      var self = this;
      options = options || {};

      var event = options.event || 'click';
      var action_id = options.action_id || element.id;
      var condition = options.condition || alwaysTruePredicate;

      element.addEventListener(event, function(e) {
        if (condition(e))
          self.sendImpl_(CALLBACK_USER_ACTED, action_id);
        e.stopPropagation();
      });
    },

    /**
     * @override
     * @final
     */
    querySelectorAll: function() {
      return this.querySelectorAllImpl_.apply(this, arguments);
    },

    /**
     * Does the following things:
     *  * Creates screen context.
     *  * Looks for elements having "alias" property and adds them as the
     *    proprties of the screen with name equal to value of "alias", i.e. HTML
     *    element <div alias="myDiv"></div> will be stored in this.myDiv.
     *  * Looks for buttons having "action" properties and adds click handlers
     *    to them. These handlers send |CALLBACK_USER_ACTED| messages to
     *    C++ with "action" property's value as payload.
     * @private
     */
    initializeImpl_: function() {
      if (cr.isChromeOS)
        this.screenContext_ = new login.ScreenContext();

      this.decorate();

      this.querySelectorAllImpl_('[alias]').forEach(function(element) {
        var alias = element.getAttribute('alias');
        if (alias in this)
          throw Error('Alias "' + alias + '" of "' + this.name() + '" screen ' +
              'shadows or redefines property that is already defined.');
        this[alias] = element;
        this[element.getAttribute('alias')] = element;
      }, this);
      var self = this;
      this.querySelectorAllImpl_('button[action]').forEach(function(button) {
        button.addEventListener('click', function(e) {
          var action = this.getAttribute('action');
          self.send(CALLBACK_USER_ACTED, action);
          e.stopPropagation();
        });
      });
    },

    /**
     * Sends message to Chrome, adding needed prefix to message name. All
     * arguments after |messageName| are packed into message parameters list.
     *
     * @param {string} messageName Name of message without a prefix.
     * @param {...*} varArgs parameters for message.
     * @private
     */
    sendImpl_: function(messageName, varArgs) {
      if (arguments.length == 0)
        throw Error('Message name is not provided.');
      var fullMessageName = this.sendPrefix_ + messageName;
      var payload = Array.prototype.slice.call(arguments, 1);
      chrome.send(fullMessageName, payload);
    },

    /**
     * Starts observation of property with |key| of the context attached to
     * current screen. This method differs from "login.ScreenContext" in that
     * it automatically detects if observer is method of |this| and make
     * all needed actions to make it work correctly. So it's no need for client
     * to bind methods to |this| and keep resulting callback for
     * |removeObserver| call:
     *
     *   this.addContextObserver('key', this.onKeyChanged_);
     *   ...
     *   this.removeContextObserver('key', this.onKeyChanged_);
     * @private
     */
    addContextObserverImpl_: function(key, observer) {
      var realObserver = observer;
      var propertyName = this.getPropertyNameOf_(observer);
      if (propertyName) {
        if (!this.contextObservers_.hasOwnProperty(propertyName))
          this.contextObservers_[propertyName] = observer.bind(this);
        realObserver = this.contextObservers_[propertyName];
      }
      this.screenContext_.addObserver(key, realObserver);
    },

    /**
     * Removes |observer| from the list of context observers. Supports not only
     * regular functions but also screen methods (see comment to
     * |addContextObserver|).
     * @private
     */
    removeContextObserverImpl_: function(observer) {
      var realObserver = observer;
      var propertyName = this.getPropertyNameOf_(observer);
      if (propertyName) {
        if (!this.contextObservers_.hasOwnProperty(propertyName))
          return;
        realObserver = this.contextObservers_[propertyName];
        delete this.contextObservers_[propertyName];
      }
      this.screenContext_.removeObserver(realObserver);
    },

    /**
     * Sends recent context changes to C++ handler.
     * @private
     */
    commitContextChangesImpl_: function() {
      if (!this.screenContext_.hasChanges())
        return;
      this.sendImpl_(CALLBACK_CONTEXT_CHANGED,
                     this.screenContext_.getChangesAndReset());
    },

    /**
     * Calls standart |querySelectorAll| method and returns its result converted
     * to Array.
     * @private
     */
    querySelectorAllImpl_: function(selector) {
      var list = querySelectorAll.call(this, selector);
      return Array.prototype.slice.call(list);
    },

    /**
     * Called when context changes are recieved from C++.
     * @private
     */
    contextChanged_: function(diff) {
      this.screenContext_.applyChanges(diff);
    },

    /**
     * If |value| is the value of some property of |this| returns property's
     * name. Otherwise returns empty string.
     * @private
     */
    getPropertyNameOf_: function(value) {
      for (var key in this)
        if (this[key] === value)
          return key;
      return '';
    }
  };

  Screen.CALLBACK_USER_ACTED = CALLBACK_USER_ACTED;

  return {
    Screen: Screen
  };
});

cr.define('login', function() {
  return {
    /**
     * Creates class and object for screen.
     * Methods specified in EXTERNAL_API array of prototype
     * will be available from C++ part.
     * Example:
     *     login.createScreen('ScreenName', 'screen-id', {
     *       foo: function() { console.log('foo'); },
     *       bar: function() { console.log('bar'); }
     *       EXTERNAL_API: ['foo'];
     *     });
     *     login.ScreenName.register();
     *     var screen = $('screen-id');
     *     screen.foo(); // valid
     *     login.ScreenName.foo(); // valid
     *     screen.bar(); // valid
     *     login.ScreenName.bar(); // invalid
     *
     * @param {string} name Name of created class.
     * @param {string} id Id of div representing screen.
     * @param {(function()|Object)} proto Prototype of object or function that
     *     returns prototype.
     */
    createScreen: function(name, id, template) {
      if (typeof template == 'function')
        template = template();

      var apiNames = template.EXTERNAL_API || [];
      for (var i = 0; i < apiNames.length; ++i) {
        var methodName = apiNames[i];
        if (typeof template[methodName] !== 'function')
          throw Error('External method "' + methodName + '" for screen "' +
              name + '" not a function or undefined.');
      }

      function checkPropertyAllowed(propertyName) {
        if (propertyName.charAt(propertyName.length - 1) === '_' &&
            (propertyName in login.Screen.prototype)) {
          throw Error('Property "' + propertyName + '" of "' + id + '" ' +
              'shadows private property of login.Screen prototype.');
        }
      };

      var Constructor = function() {
        login.Screen.call(this, 'login.' + name + '.');
      };
      Constructor.prototype = Object.create(login.Screen.prototype);
      var api = {};

      Object.getOwnPropertyNames(template).forEach(function(propertyName) {
        if (propertyName === 'EXTERNAL_API')
          return;

        checkPropertyAllowed(propertyName);

        var descriptor =
            Object.getOwnPropertyDescriptor(template, propertyName);
        Object.defineProperty(Constructor.prototype, propertyName, descriptor);

        if (apiNames.indexOf(propertyName) >= 0) {
          api[propertyName] = function() {
              var screen = $(id);
              return screen[propertyName].apply(screen, arguments);
          };
        }
      });

      Constructor.prototype.name = function() { return id; };

      api.contextChanged = function() {
        var screen = $(id);
        screen.contextChanged_.apply(screen, arguments);
      }

      api.register = function(opt_lazy_init) {
        var screen = $(id);
        screen.__proto__ = new Constructor();

        if (opt_lazy_init !== undefined && opt_lazy_init)
          screen.deferredInitialization = function() { screen.initialize(); }
        else
          screen.initialize();
        Oobe.getInstance().registerScreen(screen);
      };

      cr.define('login', function() {
        var result = {};
        result[name] = api;
        return result;
      });
    }
  };
});
