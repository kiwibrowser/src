// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cr.ui', function() {
  /**
   * Creates a new button element. The repeating button behaves like a
   * keyboard button, which auto-repeats if held. This button is designed
   * for use with controls such as brightness and volume adjustment buttons.
   * @constructor
   * @extends {HTMLButtonElement}
   */
  var RepeatingButton = cr.ui.define('button');

  /**
   * DOM Events that may be fired by the Repeating button.
   */
  RepeatingButton.Event = {BUTTON_HELD: 'buttonHeld'};

  RepeatingButton.prototype = {
    __proto__: HTMLButtonElement.prototype,

    /**
     * Delay in milliseconds before the first repeat trigger of the button
     * held action.
     * @type {number}
     * @private
     */
    holdDelayTime_: 500,

    /**
     * Delay in milliseconds between triggers of the button held action.
     * @type {number}
     * @private
     */
    holdRepeatIntervalTime_: 50,

    /**
     * Callback function ID when repeated intervals trigger. Initialized when
     * the button is held for an initial delay period and cleared when the
     * button is released.
     * @type {number|undefined}
     * @private
     */
    intervalCallbackId_: undefined,

    /**
     * Callback function to arm the repeat timer. Initialized when the button
     * is pressed and cleared when the interval timer is set or the button is
     * released.
     * @type {number|undefined}
     * @private
     */
    armRepeaterCallbackId_: undefined,

    /**
     * Initializes the button.
     */
    decorate: function() {
      this.addEventListener('mousedown', this.buttonDown_.bind(this));
      this.addEventListener('mouseup', this.buttonUp_.bind(this));
      this.addEventListener('mouseout', this.buttonUp_.bind(this));
      this.addEventListener('touchstart', this.touchStart_.bind(this));
      this.addEventListener('touchend', this.buttonUp_.bind(this));
      this.addEventListener('touchcancel', this.buttonUp_.bind(this));
    },

    /**
     * Called when the user initiates a touch gesture.
     * @param {!Event} e The triggered event.
     * @private
     */
    touchStart_: function(e) {
      // Block system level gestures to prevent double tap to zoom. Also,
      // block following mouse event to prevent double firing of the button
      // held action in the case of a tap. Otherwise, a single tap action in
      // webkit generates the following event sequence: touchstart, touchend,
      // mouseover, mousemove, mousedown, mouseup and click.
      e.preventDefault();
      this.buttonDown_(e);
    },

    /**
     * Called when the user presses this button.
     * @param {!Event} e The triggered event.
     * @private
     */
    buttonDown_: function(e) {
      this.clearTimeout_();
      // Trigger the button held action immediately, after an initial delay and
      // then repeated based on a fixed time increment. The time intervals are
      // in agreement with the defaults for the ChromeOS keyboard and virtual
      // keyboard.
      // TODO(kevers): Consider adding a common location for picking up the
      //               initial delay and repeat interval.
      this.buttonHeld_();
      var self = this;
      var armRepeaterCallback = function() {
        // In the event of a click/tap operation, this button has already been
        // released by the time this timeout triggers. Test to ensure that the
        // button is still being held (i.e. clearTimeout has not been called).
        if (typeof self.armRepeaterCallbackId_ != 'undefined') {
          self.armRepeaterCallbackId_ = undefined;
          self.buttonHeld_();
          self.intervalCallbackId_ = setInterval(
              self.buttonHeld_.bind(self), self.holdRepeatIntervalTime_);
        }
      };
      this.armRepeaterCallbackId_ =
          setTimeout(armRepeaterCallback, this.holdDelayTime_);
    },

    /**
     * Called when the user releases this button.
     * @param {!Event} e The triggered event.
     * @private
     */
    buttonUp_: function(e) {
      this.clearTimeout_();
    },

    /**
     * Resets the interval callback.
     * @private
     */
    clearTimeout_: function() {
      if (typeof this.armRepeaterCallbackId_ != 'undefined') {
        clearTimeout(this.armRepeaterCallbackId_);
        this.armRepeaterCallbackId_ = undefined;
      }
      if (typeof this.intervalCallbackId_ != 'undefined') {
        clearInterval(this.intervalCallbackId_);
        this.intervalCallbackId_ = undefined;
      }
    },

    /**
     * Dispatches the action associated with keeping this button pressed.
     * @private
     */
    buttonHeld_: function() {
      cr.dispatchSimpleEvent(this, RepeatingButton.Event.BUTTON_HELD);
    },

    /**
     * Getter for the initial delay before repeating.
     * @type {number} The delay in milliseconds.
     */
    get repeatDelay() {
      return this.holdDelayTime_;
    },

    /**
     * Setter for the initial delay before repeating.
     * @type {number} The delay in milliseconds.
     */
    set repeatDelay(delay) {
      this.holdDelayTime_ = delay;
    },

    /**
     * Getter for the repeat interval.
     * @type {number} The repeat interval in milliseconds.
     */
    get repeatInterval() {
      return this.holdRepeatIntervalTime_;
    },

    /**
     * Setter for the repeat interval.
     * @type {number} The interval in milliseconds.
     */
    set repeatInterval(delay) {
      this.holdRepeatIntervalTime_ = delay;
    }
  };

  return {RepeatingButton: RepeatingButton};
});
