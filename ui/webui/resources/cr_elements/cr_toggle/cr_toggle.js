// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'cr-toggle' is a component for showing an on/off switch. It
 * fires a 'change' event *only* when its state changes as a result of a user
 * interaction. Besides just clicking the element, its state can be changed by
 * dragging (pointerdown+pointermove) the element towards the desired direction.
 */
Polymer({
  is: 'cr-toggle',

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
    'aria-pressed': 'false',
    role: 'button',
    tabindex: 0,
  },

  listeners: {
    'pointerdown': 'onPointerDown_',
    'pointerup': 'onPointerUp_',
    'click': 'onTap_',
    'keypress': 'onKeyPress_',
    'focus': 'onFocus_',
    'blur': 'onBlur_',
  },

  /** @private {?Function} */
  boundPointerMove_: null,

  /**
   * Number of pixels required to move to consider the pointermove event as
   * intentional.
   * @type {number}
   */
  MOVE_THRESHOLD_PX: 5,

  /**
   * Whether the state of the toggle has already taken into account by
   * |pointeremove| handlers. Used in the 'click' handler.
   * @private {boolean}
   */
  handledInPointerMove_: false,

  /** @override */
  attached: function() {
    let direction = this.matches(':host-context([dir=rtl]) cr-toggle') ? -1 : 1;

    this.boundPointerMove_ = (e) => {
      // Prevent unwanted text selection to occur while moving the pointer, this
      // is important.
      e.preventDefault();

      let diff = e.clientX - this.pointerDownX_;
      if (Math.abs(diff) < this.MOVE_THRESHOLD_PX)
        return;

      this.handledInPointerMove_ = true;

      let shouldToggle = (diff * direction < 0 && this.checked) ||
          (diff * direction > 0 && !this.checked);
      if (shouldToggle)
        this.toggleState_(false);
    };
  },

  /** @private */
  checkedChanged_: function() {
    this.setAttribute('aria-pressed', this.checked ? 'true' : 'false');
  },

  /** @private */
  disabledChanged_: function() {
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

  /** @private */
  onPointerUp_: function(e) {
    this.removeEventListener('pointermove', this.boundPointerMove_);
  },

  /**
   * @param {!PointerEvent} e
   * @private
   */
  onPointerDown_: function(e) {
    // Don't do anything if this was not a primary button click or touch event.
    if (e.button != 0)
      return;

    // This is necessary to have follow up pointer events fire on |this|, even
    // if they occur outside of its bounds.
    this.setPointerCapture(e.pointerId);
    this.pointerDownX_ = e.clientX;
    this.handledInPointerMove_ = false;
    this.addEventListener('pointermove', this.boundPointerMove_);
  },

  /** @private */
  onTap_: function(e) {
    // Prevent |click| event from bubbling. It can cause parents of this
    // elements to erroneously re-toggle this control.
    e.stopPropagation();
    e.preventDefault();

    // User gesture has already been taken care of inside |pointermove|
    // handlers, Do nothing here.
    if (this.handledInPointerMove_)
      return;

    // If no pointermove event fired, then user just clicked on the
    // toggle button and therefore it should be toggled.
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
    if (e.key == ' ' || e.key == 'Enter') {
      e.preventDefault();
      this.toggleState_(true);
    }
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
    this._rippleContainer = this.$.knob;
    let ripple = Polymer.PaperRippleBehavior._createRipple();
    ripple.id = 'ink';
    ripple.setAttribute('recenters', '');
    ripple.classList.add('circle', 'toggle-ink');
    return ripple;
  },
});
