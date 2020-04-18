// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// require: cr.js
// require: cr/event_target.js

/**
 * @fileoverview This creates a log object which listens to and
 * records all sync events.
 */

cr.define('chrome.sync', function() {
  'use strict';

  var eventsByCategory = {
    notifier: [
      'onIncomingNotification',
      'onNotificationStateChange',
    ],

    manager: [
      'onActionableError',
      'onChangesApplied',
      'onChangesComplete',
      'onClearServerDataFailed',
      'onClearServerDataSucceeded',
      'onConnectionStatusChange',
      'onEncryptedTypesChanged',
      'onEncryptionComplete',
      'onInitializationComplete',
      'onPassphraseAccepted',
      'onPassphraseRequired',
      'onStopSyncingPermanently',
      'onSyncCycleCompleted',
    ],

    transaction: [
      'onTransactionWrite',
    ],

    protocol: [
      'onProtocolEvent',
    ]
  };

  /**
   * Creates a new log object which then immediately starts recording
   * sync events.  Recorded entries are available in the 'entries'
   * property and there is an 'append' event which can be listened to.
   * @constructor
   * @extends {cr.EventTarget}
   */
  var Log = function() {
    var self = this;

    /**
     * Creates a callback function to be invoked when an event arrives.
     */
    var makeCallback = function(categoryName, eventName) {
      return function(e) {
        self.log_(categoryName, eventName, e.details);
      };
    };

    for (var categoryName in eventsByCategory) {
      for (var i = 0; i < eventsByCategory[categoryName].length; ++i) {
        var eventName = eventsByCategory[categoryName][i];
        chrome.sync.events.addEventListener(
            eventName,
            makeCallback(categoryName, eventName));
      }
    }
  }

  Log.prototype = {
    __proto__: cr.EventTarget.prototype,

    /**
     * The recorded log entries.
     * @type {array}
     */
    entries: [],

    /**
     * Records a single event with the given parameters and fires the
     * 'append' event with the newly-created event as the 'detail'
     * field of a custom event.
     * @param {string} submodule The sync submodule for the event.
     * @param {string} event The name of the event.
     * @param {dictionary} details A dictionary of event-specific details.
     */
    log_: function(submodule, event, details) {
      var entry = {
        submodule: submodule,
        event: event,
        date: new Date(),
        details: details,
        textDetails: ''
      };
      entry.textDetails = JSON.stringify(entry.details, null, 2);
      this.entries.push(entry);
      // Fire append event.
      var e = cr.doc.createEvent('CustomEvent');
      e.initCustomEvent('append', false, false, entry);
      this.dispatchEvent(e);
    }
  };

  return {
    log: new Log()
  };
});
