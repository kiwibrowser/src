// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * `settings-toggle-button` is a toggle that controls a supplied preference.
 */
Polymer({
  is: 'settings-toggle-button',

  behaviors: [SettingsBooleanControlBehavior],

  properties: {
    ariaLabel: {
      type: String,
      reflectToAttribute: false,  // Handled by #control.
      observer: 'onAriaLabelSet_',
      value: '',
    },

    elideLabel: {
      type: Boolean,
      reflectToAttribute: true,
    },
  },

  listeners: {
    'click': 'onHostTap_',
  },

  observers: [
    'onDisableOrPrefChange_(disabled, pref.*)',
  ],

  /** @override */
  focus: function() {
    this.$.control.focus();
  },

  /**
   * Removes the aria-label attribute if it's added by $i18n{...}.
   * @private
   */
  onAriaLabelSet_: function() {
    if (this.hasAttribute('aria-label')) {
      let ariaLabel = this.ariaLabel;
      this.removeAttribute('aria-label');
      this.ariaLabel = ariaLabel;
    }
  },

  /**
   * @return {string}
   * @private
   */
  getAriaLabel_: function() {
    return this.label || this.ariaLabel;
  },

  /** @private */
  onDisableOrPrefChange_: function() {
    if (this.controlDisabled()) {
      this.removeAttribute('actionable');
    } else {
      this.setAttribute('actionable', '');
    }
  },

  /**
   * Handles non cr-toggle button clicks (cr-toggle handles its own click events
   * which don't bubble).
   * @param {!Event} e
   * @private
   */
  onHostTap_: function(e) {
    e.stopPropagation();
    if (this.controlDisabled())
      return;

    this.checked = !this.checked;
    this.notifyChangedByUserInteraction();
    this.fire('change');
  },

  /**
   * @param {!CustomEvent} e
   * @private
   */
  onChange_: function(e) {
    this.checked = /** @type {boolean} */ (e.detail);
    this.notifyChangedByUserInteraction();
  },
});
