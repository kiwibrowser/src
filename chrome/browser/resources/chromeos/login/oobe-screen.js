// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('login', function() {
  /** @const */ var CALLBACK_USER_ACTED = 'userActed';

  var OobeScreenBehavior = {
    properties: {
      /**
       * Internal storage of |this.context|. Short name has been choosen for
       * reason: such name doesn't take much space in HTML data bindings, which
       * are used very often.
       * C binded to the native part of the context, that means that all the
       * changes in the native part appear in C automatically. Reverse is not
       * true, you should use:
       *    this.context.set(...);
       *    this.context.commitContextChanges();
       * to send updates to the native part.
       * TODO(dzhioev): make binding two-way.
       */
      C: Object,

      name: String
    },

    /**
     * The login.Screen which is hosting |this|.
     */
    screen_: null,

    /**
     * Dictionary of context observers that are methods of |this| bound to
     * |this|.
     */
    contextObservers_: null,

    /**
     * login.ScreenContext used for sharing data with native backend.
     */
    context: null,

    /**
     * Called when the screen is being registered.
     */
    initialize: function() {},

    ready: function() {
      if (this.decorate_) {
        this.initialize();
      } else {
        this.ready_ = true;
      }
    },

    userActed: function(e) {
      this.send(
          CALLBACK_USER_ACTED,
          e.detail.sourceEvent.target.getAttribute('action'));
    },

    i18n: function(args) {
      if (!(args instanceof Array))
        args = [args];
      args[0] = 'login_' + this.name + '_' + args[0];
      return loadTimeData.getStringF.apply(loadTimeData, args);
    },

    /**
     * Called by login.Screen when the screen is beeing registered.
     */
    decorate: function(screen) {
      this.screen_ = screen;
      this.context = screen.screenContext_;
      this.C = this.context.storage_;
      this.contextObservers_ = {};
      var self = this;
      if (this.ready_) {
        this.initialize();
      } else {
        this.decorate_ = true;
      }
    },

    /**
     * Should be called for every context field which is used in Polymer
     * declarative data bindings (e.g. {{C.fieldName}}).
     */
    registerBoundContextField: function(fieldName) {
      this.addContextObserver(fieldName, this.onContextFieldChanged_);
    },

    onContextFieldChanged_: function(_, _, fieldName) {
      this.notifyPath('C.' + fieldName, this.C[fieldName]);
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
     * @override
     * @final
     */
    querySelector: function() {
      return this.querySelectorImpl_.apply(this, arguments);
    },

    /**
     * @override
     * @final
     */
    querySelectorAll: function() {
      return this.querySelectorAllImpl_.apply(this, arguments);
    },

    /**
     * See login.Screen.send.
     * @private
     */
    sendImpl_: function() {
      return this.screen_.send.apply(this.screen_, arguments);
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
      this.context.addObserver(key, realObserver);
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
      this.context.removeObserver(realObserver);
    },

    /**
     * See login.Screen.commitContextChanges.
     * @private
     */
    commitContextChangesImpl_: function() {
      return this.screen_.commitContextChanges.apply(this.screen_, arguments);
    },

    /**
     * Calls |querySelector| method of the shadow dom and returns the result.
     * @private
     */
    querySelectorImpl_: function(selector) {
      return this.shadowRoot.querySelector(selector);
    },


    /**
     * Calls standart |querySelectorAll| method of the shadow dom and returns
     * the result converted to Array.
     * @private
     */
    querySelectorAllImpl_: function(selector) {
      var list = this.shadowRoot.querySelectorAll(selector);
      return Array.prototype.slice.call(list);
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

  return {OobeScreenBehavior: OobeScreenBehavior};
});
