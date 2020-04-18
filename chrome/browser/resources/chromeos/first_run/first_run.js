// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview First run UI.
 */

// <include src="step.js">

// Transitions durations.
/** @const  */ var DEFAULT_TRANSITION_DURATION_MS = 400;
/** @const  */ var BG_TRANSITION_DURATION_MS = 800;

/**
 * Changes visibility of element with animated transition.
 * @param {Element} element Element which visibility should be changed.
 * @param {boolean} visible Whether element should be visible after transition.
 * @param {number=} opt_transitionDuration Time length of transition in
 *     milliseconds. Default value is DEFAULT_TRANSITION_DURATION_MS.
 * @param {function()=} opt_onFinished Called after transition has finished.
 */
function changeVisibility(
    element, visible, opt_transitionDuration, opt_onFinished) {
  var classes = element.classList;
  // If target visibility is the same as current element visibility.
  if (classes.contains('transparent') === !visible) {
    if (opt_onFinished)
      opt_onFinished();
    return;
  }
  var transitionDuration = (opt_transitionDuration === undefined) ?
      cr.FirstRun.getDefaultTransitionDuration() :
      opt_transitionDuration;
  var style = element.style;
  var oldDurationValue = style.getPropertyValue('transition-duration');
  style.setProperty('transition-duration', transitionDuration + 'ms');
  var transition = visible ? 'show-animated' : 'hide-animated';
  classes.add(transition);
  classes.toggle('transparent');
  element.addEventListener('transitionend', function f() {
    element.removeEventListener('transitionend', f);
    classes.remove(transition);
    if (oldDurationValue)
      style.setProperty('transition-duration', oldDurationValue);
    else
      style.removeProperty('transition-duration');
    if (opt_onFinished)
      opt_onFinished();
  });
  ensureTransitionEndEvent(element, transitionDuration);
}

cr.define('cr.FirstRun', function() {
  return {
    // Whether animated transitions are enabled.
    transitionsEnabled_: false,

    // SVG element representing UI background.
    background_: null,

    // Container for background.
    backgroundContainer_: null,

    // Mask element describing transparent "holes" in background.
    mask_: null,

    // Pattern used for creating rectangular holes.
    rectangularHolePattern_: null,

    // Pattern used for creating round holes.
    roundHolePattern_: null,

    // Dictionary keeping all available tutorial steps by their names.
    steps_: {},

    // Element representing step currently shown for user.
    currentStep_: null,

    /**
     * Initializes internal structures and preparing steps.
     */
    initialize: function() {
      disableTextSelectAndDrag();
      this.transitionsEnabled_ = loadTimeData.getBoolean('transitionsEnabled');

      // Note: we don't use $() here because these are SVGElements, not
      // HTMLElements.
      this.background_ = getSVGElement('background');
      this.mask_ = getSVGElement('mask');
      this.rectangularHolePattern_ = getSVGElement('rectangular-hole-pattern');
      this.rectangularHolePattern_.removeAttribute('id');
      this.roundHolePattern_ = getSVGElement('round-hole-pattern');
      this.roundHolePattern_.removeAttribute('id');

      this.backgroundContainer_ = $('background-container');
      var stepElements = document.getElementsByClassName('step');
      for (var i = 0; i < stepElements.length; ++i) {
        var step = stepElements[i];
        cr.FirstRun.DecorateStep(step);
        this.steps_[step.getName()] = step;
      }
      this.setBackgroundVisible(true, function() {
        chrome.send('initialized');
      });
    },

    /**
     * Hides all elements and background.
     */
    finalize: function() {
      // At first we hide holes (job 1) and current step (job 2) simultaneously,
      // then background.
      var jobsLeft = 2;
      var onJobDone = function() {
        --jobsLeft;
        if (jobsLeft)
          return;
        this.setBackgroundVisible(false, function() {
          chrome.send('finalized');
        });
      }.bind(this);
      this.doHideCurrentStep_(function(name) {
        if (name)
          chrome.send('stepHidden', [name]);
        onJobDone();
      });
      this.removeHoles(onJobDone);
    },

    /**
     * Adds transparent rectangular hole to background.
     * @param {number} x X coordinate of top-left corner of hole.
     * @param {number} y Y coordinate of top-left corner of hole.
     * @param {number} widht Width of hole.
     * @param {number} height Height of hole.
     */
    addRectangularHole: function(x, y, width, height) {
      var hole = this.rectangularHolePattern_.cloneNode();
      hole.setAttribute('x', x);
      hole.setAttribute('y', y);
      hole.setAttribute('width', width);
      hole.setAttribute('height', height);
      this.mask_.appendChild(hole);
      setTimeout(function() {
        changeVisibility(hole, true);
      }, 0);
    },

    /**
     * Adds transparent round hole to background.
     * @param {number} x X coordinate of circle center.
     * @param {number} y Y coordinate of circle center.
     * @param {number} radius Radius of circle.
     */
    addRoundHole: function(x, y, radius) {
      var hole = this.roundHolePattern_.cloneNode();
      hole.setAttribute('cx', x);
      hole.setAttribute('cy', y);
      hole.setAttribute('r', radius);
      this.mask_.appendChild(hole);
      setTimeout(function() {
        changeVisibility(hole, true);
      }, 0);
    },

    /**
     * Removes all holes previously added by |addHole|.
     * @param {function=} opt_onHolesRemoved Called after all holes have been
     *     hidden.
     */
    removeHoles: function(opt_onHolesRemoved) {
      var mask = this.mask_;
      var holes =
          Array.prototype.slice.call(mask.getElementsByClassName('hole'));
      var holesLeft = holes.length;
      if (!holesLeft) {
        if (opt_onHolesRemoved)
          opt_onHolesRemoved();
        return;
      }
      holes.forEach(function(hole) {
        changeVisibility(
            hole, false, this.getDefaultTransitionDuration(), function() {
              mask.removeChild(hole);
              --holesLeft;
              if (!holesLeft && opt_onHolesRemoved)
                opt_onHolesRemoved();
            });
      }.bind(this));
    },

    /**
     * Hides currently active step and notifies chrome after step has been
     * hidden.
     */
    hideCurrentStep: function() {
      assert(this.currentStep_);
      this.doHideCurrentStep_(function(name) {
        chrome.send('stepHidden', [name]);
      });
    },

    /**
     * Hides currently active step.
     * @param {function(string)=} opt_onStepHidden Called after step has been
     *     hidden.
     */
    doHideCurrentStep_: function(opt_onStepHidden) {
      if (!this.currentStep_) {
        if (opt_onStepHidden)
          opt_onStepHidden();
        return;
      }
      var name = this.currentStep_.getName();
      this.currentStep_.hide(true, function() {
        this.currentStep_ = null;
        if (opt_onStepHidden)
          opt_onStepHidden(name);
      }.bind(this));
    },

    /**
     * Shows step with given name in given position.
     * @param {Object} stepParams Step params dictionary containing:
     * |name|: Name of the step.
     * |position|: Optional parameter with optional fields |top|, |right|,
     *     |bottom|, |left| used for step positioning.
     * |pointWithOffset|: Optional parameter for positioning bubble. Contains
     *     [x, y, offset], where (x, y) - point to which bubble points,
     *     offset - distance between arrow and point.
     * |voiceInteractionEnabled|: Optional boolean value to indicate if voice
     *     interaction is enabled by the device.
     */
    showStep: function(stepParams) {
      assert(!this.currentStep_);
      if (!this.steps_.hasOwnProperty(stepParams.name))
        throw Error('Step "' + stepParams.name + '" not found.');
      var step = this.steps_[stepParams.name];
      if (stepParams.position)
        step.setPosition(stepParams.position);
      if (stepParams.pointWithOffset) {
        step.setPointsTo(
            stepParams.pointWithOffset.slice(0, 2),
            stepParams.pointWithOffset[2]);
      }
      if (stepParams.voiceInteractionEnabled)
        step.setVoiceInteractionEnabled();
      step.show(true, function(step) {
        step.focusDefaultControl();
        this.currentStep_ = step;
        chrome.send('stepShown', [stepParams.name]);
      }.bind(this));
    },

    /**
     * Sets visibility of the background.
     * @param {boolean} visibility Whether background should be visible.
     * @param {function()=} opt_onCompletion Called after visibility has
     *     changed.
     */
    setBackgroundVisible: function(visible, opt_onCompletion) {
      changeVisibility(
          this.backgroundContainer_, visible,
          this.getBackgroundTransitionDuration(), opt_onCompletion);
    },

    /**
     * Returns default duration of animated transitions, in ms.
     */
    getDefaultTransitionDuration: function() {
      return this.transitionsEnabled_ ? DEFAULT_TRANSITION_DURATION_MS : 0;
    },

    /**
     * Returns duration of transitions of background shield, in ms.
     */
    getBackgroundTransitionDuration: function() {
      return this.transitionsEnabled_ ? BG_TRANSITION_DURATION_MS : 0;
    }
  };
});

/**
 * Initializes UI.
 */
window.onload = function() {
  cr.FirstRun.initialize();
};
