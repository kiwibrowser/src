// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Interface for registration of listeners of events that can
 * wake the extension. All EventListeners must invoke
 * |addOnStartup| in the first event loop in order to
 * properly receive events that woke the extension. When adding a new
 * EventListener, be sure to also add it to |mr.Init.addEventListeners_|.
 */

goog.provide('mr.EventListener');

goog.require('mr.EventAnalytics');
goog.require('mr.Module');
goog.require('mr.PersistentData');
goog.require('mr.PersistentDataManager');



/**
 * Listens for an extension event conditionally, and forwards the event to a
 * designated module.
 *
 * This is useful for event listeners that are not added when the extension
 * initially starts up (mDNS listeners, for instance). When an event
 * listener is ready to be added for the first time (and the module is ready
 * to handle the event), |addListener()| should be called. By doing so, the
 * event listener will be automatically added back at the top level the next
 * time the extension wakes up from suspension, unless |removeListener()| is
 * called.
 *
 * @implements {mr.PersistentData}
 * @template EVENT
 */
mr.EventListener = class {
  /**
   * @param {!mr.EventAnalytics.Event} eventType The event type for this
   *     listener to record with analytics.
   * @param {string} name Name of the handler for PersistentData.
   * @param {mr.ModuleId} moduleId Name of the module handling the events.
   * @param {!EVENT} eventHandler The event handler to listen to.
   * @param {...*} listenerArgs Additional arguments when adding the listener,
   *     such as filters.
   */
  constructor(eventType, name, moduleId, eventHandler, ...listenerArgs) {
    /** @private @const {!mr.EventAnalytics.Event} */
    this.eventType_ = eventType;

    /** @private @const {string} */
    this.name_ = name;

    /** @private @const {mr.ModuleId} */
    this.moduleId_ = moduleId;

    /** @private @const {!EVENT} */
    this.eventHandler_ = eventHandler;

    /** @private @const {!Array<*>} */
    this.listenerArgs_ = listenerArgs;

    /**
     * This field is stored as temporary data. Set to true if the listener was
     * added, and will be added back the next time the extension wakes up via
     * |addOnStartup()|.
     * @private {boolean}
     */
    this.hasListener_ = false;

    /** @private @const {?function(*)} */
    this.listener_ = (...args) => this.dispatchEvent_(...args);
  }

  /**
   * Adds back the event listener that was added before the last suspension.
   * This method is called by mr.Init during the first event loop only.
   */
  addOnStartup() {
    mr.PersistentDataManager.register(this);
  }

  /**
   * Helper method to add the listener to the event.
   * @private
   */
  doAddListener_() {
    this.eventHandler_.addListener(this.listener_, ...this.listenerArgs_);
  }

  /**
   * Adds event listener. No-ops if listener is already added. Event
   * listeners will be added back automatically the next time extension wakes
   * up, during |addOnStartup|.
   */
  addListener() {
    if (this.hasListener_) {
      return;
    }
    this.hasListener_ = true;
    this.doAddListener_();
  }

  /**
   * Removes event listener. The next time extension wakes up, the event
   * listener will not be added back during |addOnStartup|.
   */
  removeListener() {
    if (!this.hasListener_) {
      return;
    }
    this.eventHandler_.removeListener(this.listener_);
    this.hasListener_ = false;
  }

  /**
   * Implementations may override this method validate an incoming event before
   * it is forwarded to the designated module.
   * @param {...*} args
   * @return {boolean} true if the event can be forwarded to the module.
   */
  validateEvent(...args) {
    return true;
  }

  /**
   * The entry point for incoming events. First the event will be validated.
   * After that, the event will be forwarded to the module, which is loaded if
   * necessary. Since asynchronous event handling is required, a value
   * representing asynchronous handling will be returned synchronously, as
   * required by the event.
   * @param {...*} args Parameters for the incoming event.
   * @return {*} false if the event is invalid. Otherwise, a value that
   *     represents that event will be handled asynchronously will be returned.
   * @private
   */
  dispatchEvent_(...args) {
    mr.EventAnalytics.recordEvent(this.eventType_);
    if (!this.validateEvent(...args)) {
      return false;
    }
    mr.Module.load(this.moduleId_)
        .then(module => module.handleEvent(this.eventHandler_, ...args));
    return this.deferredReturnValue();
  }

  /**
   * Returns |true| if the event listener is already registered.
   * @return {boolean}
   */
  isRegistered() {
    return this.hasListener_;
  }

  /**
   * @override
   */
  getStorageKey() {
    return 'mr.EventListener.' + this.name_;
  }

  /**
   * @override
   */
  getData() {
    return [this.hasListener_];
  }

  /**
   * @override
   */
  loadSavedData() {
    const hadListener =
        /** @type {boolean} */ (
            mr.PersistentDataManager.getTemporaryData(this));
    if (hadListener) {
      this.addListener();
    }
  }

  /**
   * Implementations may override this method to provide a return value in the
   * case the event is not handled synchronously. The default value is
   * undefined.
   * @return {*}
   */
  deferredReturnValue() {}
};
