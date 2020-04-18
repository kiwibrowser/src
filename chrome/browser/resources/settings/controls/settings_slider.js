// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * settings-slider wraps a paper-slider. It maps the slider's values from a
 * linear UI range to a range of real values.  When |value| does not map exactly
 * to a tick mark, it interpolates to the nearest tick.
 *
 * Unlike paper-slider, there is no distinction between value and
 * immediateValue; when either changes, the |value| property is updated.
 */
Polymer({
  is: 'settings-slider',

  behaviors: [CrPolicyPrefBehavior],

  properties: {
    /** @type {!chrome.settingsPrivate.PrefObject} */
    pref: Object,

    /** @type {!Array<number>} Values corresponding to each tick. */
    tickValues: {type: Array, value: []},

    /**
     * A scale factor used to support fractional pref values since paper-slider
     * only supports integers. This is not compatible with |tickValues|,
     * i.e. if |scale| is not 1 then |tickValues| must be empty.
     */
    scale: {
      type: Number,
      value: 1,
    },

    min: Number,

    max: Number,

    labelMin: String,

    labelMax: String,

    disabled: Boolean,

    /** @private */
    disableSlider_: {
      computed: 'computeDisableSlider_(pref.*, disabled)',
      type: Boolean,
    },
  },

  observers: [
    'valueChanged_(pref.*, tickValues.*)',
  ],

  /**
   * Sets the |pref.value| property to the value corresponding to the knob
   * position after a user action.
   * @private
   */
  onSliderChanged_: function() {
    const sliderValue = isNaN(this.$.slider.immediateValue) ?
        this.$.slider.value :
        this.$.slider.immediateValue;

    let newValue;
    if (this.tickValues && this.tickValues.length > 0)
      newValue = this.tickValues[sliderValue];
    else
      newValue = sliderValue / this.scale;

    this.set('pref.value', newValue);
  },

  /** @private */
  computeDisableSlider_: function() {
    return this.disabled || this.isPrefEnforced();
  },

  /**
   * Updates the knob position when |pref.value| changes. If the knob is still
   * being dragged, this instead forces |pref.value| back to the current
   * position.
   * @private
   */
  valueChanged_: function() {
    // If |tickValues| is empty, simply set current value to the slider.
    if (this.tickValues.length == 0) {
      this.$.slider.value =
          /** @type {number} */ (this.pref.value) * this.scale;
      return;
    }
    assert(this.scale == 1);

    // First update the slider settings if |tickValues| was set.
    const numTicks = Math.max(1, this.tickValues.length);
    this.$.slider.max = numTicks - 1;
    // Limit the number of ticks to 10 to keep the slider from looking too busy.
    const MAX_TICKS = 10;
    this.$.slider.snaps = numTicks < MAX_TICKS;
    this.$.slider.maxMarkers = numTicks < MAX_TICKS ? numTicks : 0;

    if (this.$.slider.dragging && this.tickValues.length > 0 &&
        this.pref.value != this.tickValues[this.$.slider.immediateValue]) {
      // The value changed outside settings-slider but we're still holding the
      // knob, so set the value back to where the knob was.
      // Async so we don't confuse Polymer's data binding.
      this.async(function() {
        const newValue = this.tickValues[this.$.slider.immediateValue];
        this.set('pref.value', newValue);
      });
      return;
    }

    // Convert from the public |value| to the slider index (where the knob
    // should be positioned on the slider).
    let sliderIndex = this.tickValues.length > 0 ?
        this.tickValues.indexOf(/** @type {number} */ (this.pref.value)) :
        0;
    if (sliderIndex == -1) {
      // No exact match.
      sliderIndex = this.findNearestIndex_(
          this.tickValues,
          /** @type {number} */ (this.pref.value));
    }
    this.$.slider.value = sliderIndex;
  },

  /**
   * Returns the index of the item in |arr| closest to |value|.
   * @param {!Array<number>} arr
   * @param {number} value
   * @return {number}
   * @private
   */
  findNearestIndex_: function(arr, value) {
    let closestIndex;
    let minDifference = Number.MAX_VALUE;
    for (let i = 0; i < arr.length; i++) {
      const difference = Math.abs(arr[i] - value);
      if (difference < minDifference) {
        closestIndex = i;
        minDifference = difference;
      }
    }

    assert(typeof closestIndex != 'undefined');
    return closestIndex;
  },

  /**
   * TODO(scottchen): temporary fix until polymer gesture bug resolved. See:
   * https://github.com/PolymerElements/paper-slider/issues/186
   * @private
   */
  resetTrackLock_: function() {
    Polymer.Gestures.gestures.tap.reset();
  },
});
