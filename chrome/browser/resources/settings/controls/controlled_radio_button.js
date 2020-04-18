// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'controlled-radio-button',

  behaviors: [
    PrefControlBehavior,
    CrRadioButtonBehavior,
  ],

  properties: {
    disabled: {
      type: Boolean,
      computed: 'computeDisabled_(pref.*)',
      reflectToAttribute: true,
      observer: 'disabledChanged_',
    },

    name: {
      type: String,
      notify: true,
    },
  },

  /**
   * @return {boolean} Whether the button is disabled.
   * @private
   */
  computeDisabled_: function() {
    return this.pref.enforcement == chrome.settingsPrivate.Enforcement.ENFORCED;
  },

  /**
   * @param {boolean} current
   * @param {boolean} previous
   * @private
   */
  disabledChanged_: function(current, previous) {
    if (previous === undefined && !this.disabled)
      return;

    this.setAttribute('tabindex', this.disabled ? -1 : 0);
    this.setAttribute('aria-disabled', this.disabled ? 'true' : 'false');
  },

  /**
   * @return {boolean}
   * @private
   */
  showIndicator_: function() {
    return this.disabled &&
        this.name == Settings.PrefUtil.prefToString(assert(this.pref));
  },

  /**
   * @param {!Event} e
   * @private
   */
  onIndicatorTap_: function(e) {
    // Disallow <controlled-radio-button on-click="..."> when disabled.
    e.preventDefault();
    e.stopPropagation();
  },
});
