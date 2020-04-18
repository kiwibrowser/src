// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'cr-checkbox' is a component similar to native checkbox. It
 * fires a 'change' event *only* when its state changes as a result of a user
 * interaction. By default it assumes there will be child(ren) passed in to be
 * used as labels. If no label will be provided, a .no-label class should be
 * added to hide the spacing between the checkbox and the label container.
 */
Polymer({
  is: 'cr-checkbox',

  behaviors: [Polymer.PaperRippleBehavior],

  properties: {
    checked: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
      observer: 'checkedChanged_',
      notify: true,
    },

    disabled: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
      observer: 'disabledChanged_',
    },
  },

  hostAttributes: {
    'aria-disabled': 'false',
    'aria-checked': 'false',
    role: 'checkbox',
    tabindex: 0,
  },

  listeners: {
    'click': 'onClick_',
    'keypress': 'onKeyPress_',
    'focus': 'onFocus_',
    'blur': 'onBlur_',
  },

  /** @override */
  ready: function() {
    this.removeAttribute('unresolved');
  },

  /** @private */
  checkedChanged_: function() {
    this.setAttribute('aria-checked', this.checked ? 'true' : 'false');
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

  /** @private */
  onFocus_: function() {
    this.ensureRipple();
    this.$$('paper-ripple').holdDown = true;
  },

  /** @private */
  onBlur_: function() {
    this.ensureRipple();
    this.$$('paper-ripple').holdDown = false;
  },

  /**
   * @param {!Event} e
   * @private
   */
  shouldHandleEvent_: function(e) {
    // Actions on a link within the label should not change checkbox state.
    return !this.disabled && e.target.tagName != 'A';
  },

  /**
   * @param {!Event} e
   * @private
   */
  onClick_: function(e) {
    if (!this.shouldHandleEvent_(e))
      return;

    // Prevent |click| event from bubbling. It can cause parents of this
    // elements to erroneously re-toggle this control.
    e.stopPropagation();
    e.preventDefault();

    this.toggleState_(false);
  },

  /**
   * @param {boolean} fromKeyboard
   * @private
   */
  toggleState_: function(fromKeyboard) {
    this.checked = !this.checked;

    if (!fromKeyboard) {
      this.ensureRipple();
      this.$$('paper-ripple').holdDown = false;
    }

    this.fire('change', this.checked);
  },

  /**
   * @param {!KeyboardEvent} e
   * @private
   */
  onKeyPress_: function(e) {
    if (!this.shouldHandleEvent_(e) || (e.key != ' ' && e.key != 'Enter'))
      return;

    e.preventDefault();
    this.toggleState_(true);
  },

  /** @private */
  onButtonFocus_: function() {
    // Forward 'focus' to the enclosing element, so that a subsequent 'Space'
    // keystroke does not trigger both 'keypress' and 'click' which would toggle
    // the state twice erroneously.
    this.focus();
  },

  // customize the element's ripple
  _createRipple: function() {
    this._rippleContainer = this.$.checkbox;
    let ripple = Polymer.PaperRippleBehavior._createRipple();
    ripple.id = 'ink';
    ripple.setAttribute('recenters', '');
    ripple.classList.add('circle', 'toggle-ink');
    return ripple;
  },
});
