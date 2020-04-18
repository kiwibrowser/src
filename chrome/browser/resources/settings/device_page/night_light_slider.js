// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

/**
 * @fileoverview
 * night-light-slider is used to set the custom automatic schedule of the
 * Night Light feature, so that users can set their desired start and end
 * times.
 */

const HOURS_PER_DAY = 24;
const MIN_KNOBS_DISTANCE_MINUTES = 60;
const OFFSET_MINUTES_6PM = 18 * 60;
const TOTAL_MINUTES_PER_DAY = 24 * 60;

Polymer({
  is: 'night-light-slider',

  behaviors: [
    PrefsBehavior,
    Polymer.IronA11yKeysBehavior,
    Polymer.IronResizableBehavior,
    Polymer.PaperInkyFocusBehavior,
  ],

  properties: {
    /**
     * Whether the element is ready and fully rendered.
     * @private
     */
    isReady_: Boolean,

    /**
     * Whether the window is in RTL locales.
     * @private
     */
    isRTL_: Boolean,

    /**
     * Whether to use the 24-hour format for the time shown in the label
     * bubbles.
     * @private
     */
    shouldUse24Hours_: Boolean,
  },

  listeners: {
    'iron-resize': 'onResize_',
  },

  observers: [
    'updateKnobs_(prefs.ash.night_light.custom_start_time.*, ' +
        'prefs.ash.night_light.custom_end_time.*, isRTL_, isReady_)',
    'hourFormatChanged_(prefs.settings.clock.use_24hour_clock.*)',
  ],

  keyBindings: {
    'left': 'onLeftKey_',
    'right': 'onRightKey_',
  },

  /**
   * The object currently being dragged. Either the start or end knobs.
   * @type {?Object}
   * @private
   */
  dragObject_: null,

  /** @override */
  attached: function() {
    // Build the legend markers.
    const markersContainer = this.$.markersContainer;
    const width = markersContainer.offsetWidth;
    for (let i = 0; i <= HOURS_PER_DAY; ++i) {
      const marker = document.createElement('div');
      marker.className = 'markers';
      markersContainer.appendChild(marker);
      marker.style.left = (i * 100 / HOURS_PER_DAY) + '%';
    }

    this.isRTL_ = window.getComputedStyle(this).direction == 'rtl';

    this.$.sliderContainer.addEventListener('contextmenu', function(e) {
      // Prevent the context menu from interfering with dragging the knobs using
      // touch.
      e.preventDefault();
      return false;
    });

    this.async(function() {
      // This is needed to make sure that the positions of the knobs and their
      // label bubbles are correctly updated when the display settings page is
      // opened for the first time after login. The page need to be fully
      // rendered.
      this.isReady_ = true;
    });
  },

  /**
   * Invoked when the element is resized and the knobs positions need to be
   * updated.
   * @private
   */
  onResize_: function() {
    this.updateKnobs_();
  },

  /**
   * Called when the value of the pref associated with whether to use the
   * 24-hour clock format is changed. This will also refresh the slider.
   * @private
   */
  hourFormatChanged_: function() {
    this.shouldUse24Hours_ = /** @type {boolean} */ (
        this.getPref('settings.clock.use_24hour_clock').value);
  },

  /**
   * Gets the style of legend div determining its absolute left position.
   * @param {number} percent The value of the div's left as a percent (0 - 100).
   * @param {boolean} isRTL whether window is in RTL locale.
   * @return {string} The CSS style of the legend div.
   * @private
   */
  getLegendStyle_: function(percent, isRTL) {
    percent = isRTL ? 100 - percent : percent;
    return 'left: ' + percent + '%';
  },

  /**
   * Expands or un-expands the knob being dragged along with its corresponding
   * label bubble.
   * @param {boolean} expand True to expand, and false to un-expand.
   * @private
   */
  setExpanded_: function(expand) {
    let knob = this.$.startKnob;
    let label = this.$.startLabel;
    if (this.dragObject_ == this.$.endKnob) {
      knob = this.$.endKnob;
      label = this.$.endLabel;
    }

    knob.classList.toggle('expanded-knob', expand);
    label.classList.toggle('expanded-knob', expand);
  },

  /**
   * If one of the two knobs is focused, this function blurs it.
   * @private
   */
  blurAnyFocusedKnob_: function() {
    const activeElement = this.shadowRoot.activeElement;
    if (activeElement == this.$.startKnob || activeElement == this.$.endKnob)
      activeElement.blur();
  },

  /**
   * Start dragging the target knob.
   * @private
   */
  startDrag_: function(event) {
    event.preventDefault();

    // Only handle start or end knobs. Use the "knob-inner" divs just to display
    // the knobs.
    if (event.target == this.$.startKnob ||
        event.target == this.$.startKnob.firstElementChild) {
      this.dragObject_ = this.$.startKnob;
    } else if (event.target == this.$.endKnob ||
               event.target == this.$.endKnob.firstElementChild) {
      this.dragObject_ = this.$.endKnob;
    } else {
      return;
    }

    this.setExpanded_(true);

    // Focus is only given to the knobs by means of keyboard tab navigations.
    // When we start dragging, we don't want to see any focus halos around any
    // knob.
    this.blurAnyFocusedKnob_();

    // However, our night-light-slider element must get the focus.
    this.focus();
  },

  /**
   * Continues dragging the selected knob if any.
   * @private
   */
  continueDrag_: function(event) {
    if (!this.dragObject_)
      return;

    event.stopPropagation();
    switch (event.detail.state) {
      case 'start':
        this.startDrag_(event);
        break;
      case 'track':
        this.doKnobTracking_(event);
        break;
      case 'end':
        this.endDrag_(event);
        break;
    }
  },

  /**
   * Updates the knob's corresponding pref value in response to dragging, which
   * will in turn update the location of the knob and its corresponding label
   * bubble and its text contents.
   * @private
   */
  doKnobTracking_: function(event) {
    const deltaRatio =
        Math.abs(event.detail.ddx) / this.$.sliderBar.offsetWidth;
    const deltaMinutes = Math.floor(deltaRatio * TOTAL_MINUTES_PER_DAY);
    if (deltaMinutes <= 0)
      return;

    const knobPref = this.dragObject_ == this.$.startKnob ?
        'ash.night_light.custom_start_time' :
        'ash.night_light.custom_end_time';

    const ddx = this.isRTL_ ? event.detail.ddx * -1 : event.detail.ddx;
    if (ddx > 0) {
      // Increment the knob's pref by the amount of deltaMinutes.
      this.incrementPref_(knobPref, deltaMinutes);
    } else {
      // Decrement the knob's pref by the amount of deltaMinutes.
      this.decrementPref_(knobPref, deltaMinutes);
    }
  },

  /**
   * Ends the dragging.
   * @private
   */
  endDrag_: function(event) {
    event.preventDefault();
    this.setExpanded_(false);
    this.dragObject_ = null;
  },

  /**
   * Gets the given knob's offset ratio with respect to its parent element
   * (which is the slider bar).
   * @param {HTMLDivElement} knob Either one of the two knobs.
   * @return {number}
   * @private
   */
  getKnobRatio_: function(knob) {
    return parseFloat(knob.style.left) / this.$.sliderBar.offsetWidth;
  },

  /**
   * Converts the time of day, given as |hour| and |minutes|, to its language-
   * sensitive time string representation.
   * @param {number} hour The hour of the day (0 - 23).
   * @param {number} minutes The minutes of the hour (0 - 59).
   * @param {boolean} shouldUse24Hours Whether to use the 24-hour time format.
   * @return {string}
   * @private
   */
  getLocaleTimeString_: function(hour, minutes, shouldUse24Hours) {
    const d = new Date();
    d.setHours(hour);
    d.setMinutes(minutes);
    d.setSeconds(0);
    d.setMilliseconds(0);

    return d.toLocaleTimeString(
        [], {hour: '2-digit', minute: '2-digit', hour12: !shouldUse24Hours});
  },

  /**
   * Converts the |offsetMinutes| value (which the number of minutes since
   * 00:00) to its language-sensitive time string representation.
   * @param {number} offsetMinutes The time of day represented as the number of
   * minutes from 00:00.
   * @param {boolean} shouldUse24Hours Whether to use the 24-hour time format.
   * @return {string}
   * @private
   */
  getTimeString_: function(offsetMinutes, shouldUse24Hours) {
    const hour = Math.floor(offsetMinutes / 60);
    const minute = Math.floor(offsetMinutes % 60);

    return this.getLocaleTimeString_(hour, minute, shouldUse24Hours);
  },

  /**
   * Using the current start and end times prefs, this function updates the
   * knobs and their label bubbles and refreshes the slider.
   * @private
   */
  updateKnobs_: function() {
    const startOffsetMinutes = /** @type {number} */ (
        this.getPref('ash.night_light.custom_start_time').value);
    this.updateKnobLeft_(this.$.startKnob, startOffsetMinutes);
    const endOffsetMinutes = /** @type {number} */ (
        this.getPref('ash.night_light.custom_end_time').value);
    this.updateKnobLeft_(this.$.endKnob, endOffsetMinutes);
    this.refresh_();
  },

  /**
   * Updates the absolute left coordinate of the given |knob| based on the time
   * it represents given as an |offsetMinutes| value.
   * @param {HTMLDivElement} knob
   * @param {number} offsetMinutes
   * @private
   */
  updateKnobLeft_: function(knob, offsetMinutes) {
    const offsetAfter6pm =
        (offsetMinutes + TOTAL_MINUTES_PER_DAY - OFFSET_MINUTES_6PM) %
        TOTAL_MINUTES_PER_DAY;
    let ratio = offsetAfter6pm / TOTAL_MINUTES_PER_DAY;

    if (ratio == 0) {
      // If the ratio is 0, then there are two possibilities:
      // - The knob time is 6:00 PM on the left side of the slider.
      // - The knob time is 6:00 PM on the right side of the slider.
      // We need to check the current knob offset ratio to determine which case
      // it is.
      const currentKnobRatio = this.getKnobRatio_(knob);
      ratio = currentKnobRatio > 0.5 ? 1.0 : 0.0;
    }
    ratio = this.isRTL_ ? (1.0 - ratio) : ratio;
    knob.style.left = (ratio * this.$.sliderBar.offsetWidth) + 'px';
  },

  /**
   * Refreshes elements of the slider other than the knobs (the label bubbles,
   * and the progress bar).
   * @private
   */
  refresh_: function() {
    // The label bubbles have the same left coordinates as their corresponding
    // knobs.
    this.$.startLabel.style.left = this.$.startKnob.style.left;
    this.$.endLabel.style.left = this.$.endKnob.style.left;

    // In RTL locales, the relative positions of the knobs are flipped for the
    // purpose of calculating the styles of the progress bars below.
    const rtl = this.isRTL_;
    const endKnob = rtl ? this.$.startKnob : this.$.endKnob;
    const startKnob = rtl ? this.$.endKnob : this.$.startKnob;
    const startProgress = rtl ? this.$.endProgress : this.$.startProgress;
    const endProgress = rtl ? this.$.startProgress : this.$.endProgress;

    // The end progress bar starts from either the start knob or the start of
    // the slider (whichever is to its left) and ends at the end knob.
    const endProgressLeft = startKnob.offsetLeft >= endKnob.offsetLeft ?
        '0px' :
        startKnob.style.left;
    endProgress.style.left = endProgressLeft;
    endProgress.style.width =
        (parseFloat(endKnob.style.left) - parseFloat(endProgressLeft)) + 'px';

    // The start progress bar starts at the start knob, and ends at either the
    // end knob or the end of the slider (whichever is to its right).
    const startProgressRight = endKnob.offsetLeft < startKnob.offsetLeft ?
        this.$.sliderBar.offsetWidth :
        endKnob.style.left;
    startProgress.style.left = startKnob.style.left;
    startProgress.style.width =
        (parseFloat(startProgressRight) - parseFloat(startKnob.style.left)) +
        'px';

    this.fixLabelsOverlapIfAny_();
  },

  /**
   * If the label bubbles overlap, this function fixes them by moving the end
   * label up a little.
   * @private
   */
  fixLabelsOverlapIfAny_: function() {
    const startLabel = this.$.startLabel;
    const endLabel = this.$.endLabel;
    const distance = Math.abs(
        parseFloat(startLabel.style.left) - parseFloat(endLabel.style.left));
    // Both knobs have the same width, but the one being dragged is scaled up by
    // 125%.
    if (distance <= (1.25 * startLabel.offsetWidth)) {
      // Shift the end label up so that it doesn't overlap with the start label.
      endLabel.classList.add('end-label-overlap');
    } else {
      endLabel.classList.remove('end-label-overlap');
    }
  },

  /**
   * Given the |prefPath| that corresponds to one knob time, it gets the value
   * of the pref that corresponds to the other knob.
   * @param {string} prefPath
   * @return {number}
   * @private
   */
  getOtherKnobPrefValue_: function(prefPath) {
    if (prefPath == 'ash.night_light.custom_start_time') {
      return /** @type {number} */ (
          this.getPref('ash.night_light.custom_end_time').value);
    }

    return /** @type {number} */ (
        this.getPref('ash.night_light.custom_start_time').value);
  },

  /**
   * Increments the value of the pref whose path is given by |prefPath| by the
   * amount given in |increment|.
   * @param {string} prefPath
   * @param {number} increment
   * @private
   */
  incrementPref_: function(prefPath, increment) {
    let value = this.getPref(prefPath).value + increment;

    const otherValue = this.getOtherKnobPrefValue_(prefPath);
    if (otherValue > value &&
        ((otherValue - value) < MIN_KNOBS_DISTANCE_MINUTES)) {
      // We are incrementing the minutes offset moving towards the other knob.
      // We have a minimum 60 minutes overlap threshold. Move this knob to the
      // other side of the other knob.
      //
      // Was:
      // ------ (+) --- 59 MIN --- + ------->>
      //
      // Now:
      // ------ + --- 60 MIN --- (+) ------->>
      //
      // (+) ==> Knob being moved.
      value = otherValue + MIN_KNOBS_DISTANCE_MINUTES;
    }

    // The knobs are allowed to wrap around.
    this.setPrefValue(prefPath, (value % TOTAL_MINUTES_PER_DAY));
  },

  /**
   * Decrements the value of the pref whose path is given by |prefPath| by the
   * amount given in |decrement|.
   * @param {string} prefPath
   * @param {number} decrement
   * @private
   */
  decrementPref_: function(prefPath, decrement) {
    let value =
        /** @type {number} */ (this.getPref(prefPath).value) - decrement;

    const otherValue = this.getOtherKnobPrefValue_(prefPath);
    if (value > otherValue &&
        ((value - otherValue) < MIN_KNOBS_DISTANCE_MINUTES)) {
      // We are decrementing the minutes offset moving towards the other knob.
      // We have a minimum 60 minutes overlap threshold. Move this knob to the
      // other side of the other knob.
      //
      // Was:
      // <<------ + --- 59 MIN --- (+) -------
      //
      // Now:
      // <<------ (+) --- 60 MIN --- + ------
      //
      // (+) ==> Knob being moved.
      value = otherValue - MIN_KNOBS_DISTANCE_MINUTES;
    }

    // The knobs are allowed to wrap around.
    if (value < 0)
      value += TOTAL_MINUTES_PER_DAY;

    this.setPrefValue(prefPath, Math.abs(value) % TOTAL_MINUTES_PER_DAY);
  },

  /**
   * Gets the pref path of the currently focused knob. Returns null if no knob
   * is currently focused.
   * @return {?string}
   * @private
   */
  getFocusedKnobPrefPathIfAny_: function() {
    const focusedElement = this.shadowRoot.activeElement;
    if (focusedElement == this.$.startKnob)
      return 'ash.night_light.custom_start_time';

    if (focusedElement == this.$.endKnob)
      return 'ash.night_light.custom_end_time';

    return null;
  },

  /**
   * Handles the 'left' key event.
   * @private
   */
  onLeftKey_: function(e) {
    e.preventDefault();
    const knobPref = this.getFocusedKnobPrefPathIfAny_();
    if (!knobPref)
      return;

    if (this.isRTL_)
      this.incrementPref_(knobPref, 1);
    else
      this.decrementPref_(knobPref, 1);
  },

  /**
   * Handles the 'right' key event.
   * @private
   */
  onRightKey_: function(e) {
    e.preventDefault();
    const knobPref = this.getFocusedKnobPrefPathIfAny_();
    if (!knobPref)
      return;

    if (this.isRTL_)
      this.decrementPref_(knobPref, 1);
    else
      this.incrementPref_(knobPref, 1);
  },

  /**
   * @return {boolean} Whether either of the two knobs is focused.
   * @private
   */
  isEitherKnobFocused_: function() {
    const activeElement = this.shadowRoot.activeElement;
    return activeElement == this.$.startKnob || activeElement == this.$.endKnob;
  },

  /**
   * Overrides _createRipple() from PaperInkyFocusBehavior to create the ripple
   * only on a knob if it's focused, or on a dummy hidden element so that it
   * doesn't show.
   * @protected
   */
  _createRipple: function() {
    if (this.isEitherKnobFocused_()) {
      this._rippleContainer = this.shadowRoot.activeElement;
    } else {
      // We can't just skip the ripple creation and return early with null here.
      // The code inherited from PaperInkyFocusBehavior expects that this
      // function returns a ripple element. So to avoid crashes, we'll setup the
      // ripple to be created under a hidden element.
      this._rippleContainer = this.$.dummyRippleContainer;
    }

    return Polymer.PaperInkyFocusBehaviorImpl._createRipple.call(this);
  },

  /**
   * Handles focus events on the start and end knobs.
   * @private
   */
  onFocus_: function() {
    this.ensureRipple();

    if (this.hasRipple()) {
      this._ripple.style.display = '';
      this._ripple.holdDown = true;
    }
  },

  /**
   * Handles blur events on the start and end knobs.
   * @private
   */
  onBlur_: function() {
    if (this.hasRipple()) {
      this._ripple.remove();
      this._ripple = null;
    }
  },

  /** @private */
  _focusedChanged: function(receivedFocusFromKeyboard) {
    // Overrides the _focusedChanged() from the PaperInkyFocusBehavior so that
    // it does nothing. This function is called only once for the entire
    // night-light-slider element even when focus is moved between the two
    // knobs. This doesn't allow us to decide on which knob the ripple will be
    // created. Hence we handle focus and blur explicitly above.
  }
});

})();
