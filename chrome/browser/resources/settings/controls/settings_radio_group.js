// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * `cr-radio-group` wraps a radio-group and set of radio-buttons that control
 *  a supplied preference.
 *
 * Example:
 *      <settings-radio-group pref="{{prefs.settings.foo}}"
 *          label="Foo Options." buttons="{{fooOptionsList}}">
 *      </settings-radio-group>
 */
Polymer({
  is: 'settings-radio-group',

  behaviors: [PrefControlBehavior],

  properties: {
    /**
     * IronSelectableBehavior selected attribute.
     */
    selected: {type: String, notify: true, observer: 'selectedChanged_'},
  },

  hostAttributes: {
    role: 'none',
  },

  observers: [
    'prefChanged_(pref.*)',
  ],

  /** @private */
  prefChanged_: function() {
    const pref = /** @type {!chrome.settingsPrivate.PrefObject} */ (this.pref);
    this.selected = Settings.PrefUtil.prefToString(pref);
  },

  /** @private */
  selectedChanged_: function(selected) {
    if (!this.pref)
      return;
    this.set(
        'pref.value', Settings.PrefUtil.stringToPrefValue(selected, this.pref));
  },
});
