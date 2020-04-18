// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Implementation of ScreenContext class: key-value storage for
 * values that are shared between C++ and JS sides.
 */
cr.define('login', function() {
  'use strict';

  function require(condition, message) {
    if (!condition) {
      throw Error(message);
    }
  }

  function checkKeyIsValid(key) {
    var keyType = typeof key;
    require(keyType === 'string', 'Invalid type of key: "' + keyType + '".');
  }

  function checkValueIsValid(value) {
    var valueType = typeof value;
    require(
        (['string', 'boolean', 'number'].indexOf(valueType) != -1 ||
         Array.isArray(value)),
        'Invalid type of value: "' + valueType + '".');
  }

  function ScreenContext() {
    this.storage_ = {};
    this.changes_ = {};
    this.observers_ = {};
  }

  ScreenContext.prototype = {
    /**
     * Returns stored value for |key| or |defaultValue| if key not found in
     * storage. Throws Error if key not found and |defaultValue| omitted.
     */
    get: function(key, defaultValue) {
      checkKeyIsValid(key);
      if (this.hasKey(key)) {
        return this.storage_[key];
      } else if (typeof defaultValue !== 'undefined') {
        return defaultValue;
      } else {
        throw Error('Key "' + key + '" not found.');
      }
    },

    /**
     * Sets |value| for |key|. Returns true if call changes state of context,
     * false otherwise.
     */
    set: function(key, value) {
      checkKeyIsValid(key);
      checkValueIsValid(value);
      if (this.hasKey(key) && this.storage_[key] === value)
        return false;
      this.changes_[key] = value;
      this.storage_[key] = value;
      return true;
    },

    hasKey: function(key) {
      checkKeyIsValid(key);
      return this.storage_.hasOwnProperty(key);
    },

    hasChanges: function() {
      return Object.keys(this.changes_).length > 0;
    },

    /**
     * Applies |changes| to context. Returns Array of changed keys' names.
     */
    applyChanges: function(changes) {
      require(!this.hasChanges(), 'Context has changes.');
      var oldValues = {};
      for (var key in changes) {
        checkKeyIsValid(key);
        checkValueIsValid(changes[key]);
        oldValues[key] = this.storage_[key];
        this.storage_[key] = changes[key];
      }
      var observers = this.cloneObservers_();
      for (var key in changes) {
        if (observers.hasOwnProperty(key)) {
          var keyObservers = observers[key];
          for (var observerIndex in keyObservers)
            keyObservers[observerIndex](changes[key], oldValues[key], key);
        }
      }
      return Object.keys(changes);
    },

    /**
     * Returns changes made on context since previous call.
     */
    getChangesAndReset: function() {
      var result = this.changes_;
      this.changes_ = {};
      return result;
    },

    addObserver: function(key, observer) {
      if (!this.observers_.hasOwnProperty(key))
        this.observers_[key] = [];
      if (this.observers_[key].indexOf(observer) !== -1) {
        console.warn('Observer already registered.');
        return;
      }
      this.observers_[key].push(observer);
    },

    removeObserver: function(observer) {
      for (var key in this.observers_) {
        var observerIndex = this.observers_[key].indexOf(observer);
        if (observerIndex != -1)
          this.observers_[key].splice(observerIndex, 1);
      }
    },

    /**
     * Creates deep copy of observers lists.
     * @private
     */
    cloneObservers_: function() {
      var result = {};
      for (var key in this.observers_)
        result[key] = this.observers_[key].slice();
      return result;
    }
  };

  return {ScreenContext: ScreenContext};
});
