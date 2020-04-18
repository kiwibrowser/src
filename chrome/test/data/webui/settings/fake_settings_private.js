// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Fake implementation of chrome.settingsPrivate for testing. */
cr.define('settings', function() {
  /**
   * Creates a deep copy of the object.
   * @param {!Object} obj
   * @return {!Object}
   */
  function deepCopy(obj) {
    return JSON.parse(JSON.stringify(obj));
  }

  /**
   * Fake of chrome.settingsPrivate API. Use by setting
   * CrSettingsPrefs.deferInitialization to true, then passing a
   * FakeSettingsPrivate to settings-prefs#initialize().
   * @constructor
   * @param {Array<!settings.FakeSettingsPrivate.Pref>=} opt_initialPrefs
   * @implements {SettingsPrivate}
   */
  function FakeSettingsPrivate(opt_initialPrefs) {
    this.prefs = {};

    if (!opt_initialPrefs)
      return;
    for (const pref of opt_initialPrefs)
      this.addPref_(pref.type, pref.key, pref.value);
  }

  FakeSettingsPrivate.prototype = {
    // chrome.settingsPrivate overrides.
    onPrefsChanged: new FakeChromeEvent(),

    getAllPrefs: function(callback) {
      // Send a copy of prefs to keep our internal state private.
      const prefs = [];
      for (const key in this.prefs)
        prefs.push(deepCopy(this.prefs[key]));

      // Run the callback asynchronously to test that the prefs aren't actually
      // used before they become available.
      setTimeout(callback.bind(null, prefs));
    },

    setPref: function(key, value, pageId, callback) {
      const pref = this.prefs[key];
      assertNotEquals(undefined, pref);
      assertEquals(typeof value, typeof pref.value);
      assertEquals(Array.isArray(value), Array.isArray(pref.value));

      if (this.failNextSetPref_) {
        callback(false);
        this.failNextSetPref_ = false;
        return;
      }
      assertNotEquals(true, this.disallowSetPref_);

      const changed = JSON.stringify(pref.value) != JSON.stringify(value);
      pref.value = deepCopy(value);
      callback(true);

      // Like chrome.settingsPrivate, send a notification when prefs change.
      if (changed)
        this.sendPrefChanges([{key: key, value: deepCopy(value)}]);
    },

    getPref: function(key, callback) {
      const pref = this.prefs[key];
      assertNotEquals(undefined, pref);
      callback(deepCopy(pref));
    },

    // Functions used by tests.

    /** Instructs the API to return a failure when setPref is next called. */
    failNextSetPref: function() {
      this.failNextSetPref_ = true;
    },

    /** Instructs the API to assert (fail the test) if setPref is called. */
    disallowSetPref: function() {
      this.disallowSetPref_ = true;
    },

    allowSetPref: function() {
      this.disallowSetPref_ = false;
    },

    /**
     * Notifies the listeners of pref changes.
     * @param {!Object<{key: string, value: *}>} changes
     */
    sendPrefChanges: function(changes) {
      const prefs = [];
      for (const change of changes) {
        const pref = this.prefs[change.key];
        assertNotEquals(undefined, pref);
        pref.value = change.value;
        prefs.push(deepCopy(pref));
      }
      this.onPrefsChanged.callListeners(prefs);
    },

    // Private methods for use by the fake API.

    /**
     * @param {!chrome.settingsPrivate.PrefType} type
     * @param {string} key
     * @param {*} value
     */
    addPref_: function(type, key, value) {
      this.prefs[key] = {
        type: type,
        key: key,
        value: value,
      };
    },
  };

  return {FakeSettingsPrivate: FakeSettingsPrivate};
});

/**
 * @type {Array<{key: string,
 *               type: chrome.settingsPrivate.PrefType,
 *               values: !Array<*>}>}
 */
settings.FakeSettingsPrivate.Pref;
