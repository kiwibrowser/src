// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Prototype for first-run tutorial steps.
 */

cr.define('cr.FirstRun', function() {
  var Step = cr.ui.define('div');

  Step.prototype = {
    __proto__: HTMLDivElement.prototype,

    // Name of step.
    name_: null,

    // Button leading to next tutorial step.
    nextButton_: null,

    // Default control for this step.
    defaultControl_: null,

    decorate: function() {
      this.name_ = this.getAttribute('id');
      var controlsContainer = this.getElementsByClassName('controls')[0];
      if (!controlsContainer)
        throw Error('Controls not found.');
      this.nextButton_ =
          controlsContainer.getElementsByClassName('next-button')[0];
      if (!this.nextButton_)
        throw Error('Next button not found.');
      this.nextButton_.addEventListener(
          'click', (function(e) {
                     chrome.send('nextButtonClicked', [this.getName()]);
                     e.stopPropagation();
                   }).bind(this));
      this.defaultControl_ = controlsContainer.children[0];
    },

    /**
     * Returns name of the string.
     */
    getName: function() {
      return this.name_;
    },

    /**
     * Hides the step.
     * @param {boolean} animated Whether transition should be animated.
     * @param {function()=} opt_onHidden Called after step has been hidden.
     */
    hide: function(animated, opt_onHidden) {
      var transitionDuration =
          animated ? cr.FirstRun.getDefaultTransitionDuration() : 0;
      changeVisibility(this, false, transitionDuration, function() {
        this.classList.add('hidden');
        if (opt_onHidden)
          opt_onHidden();
      }.bind(this));
    },

    /**
     * Shows the step.
     * @param {boolean} animated Whether transition should be animated.
     * @param {function(Step)=} opt_onShown Called after step has been shown.
     */
    show: function(animated, opt_onShown) {
      var transitionDuration =
          animated ? cr.FirstRun.getDefaultTransitionDuration() : 0;
      this.classList.remove('hidden');
      changeVisibility(this, true, transitionDuration, function() {
        if (opt_onShown)
          opt_onShown(this);
      }.bind(this));
    },

    /**
     * Sets position of the step.
     * @param {object} position Parameter with optional fields |top|,
     *     |right|, |bottom|, |left| holding corresponding offsets.
     */
    setPosition: function(position) {
      var style = this.style;
      ['top', 'right', 'bottom', 'left'].forEach(function(property) {
        if (position.hasOwnProperty(property))
          style.setProperty(property, position[property] + 'px');
      });
    },

    /**
     * Makes default control focused. Default control is a first control in
     * current implementation.
     */
    focusDefaultControl: function() {
      this.defaultControl_.focus();
    },

    /**
     * Updates UI when voice interaction is enabled by the device.
     */
    setVoiceInteractionEnabled: function() {
      if (this.name_ == 'app-list')
        $('voice-interaction-text').hidden = false;
    },
  };

  var Bubble = cr.ui.define('div');

  // List of rules declaring bubble's arrow position depending on text direction
  // and shelf alignment. Every rule has required field |position| with list
  // of classes that should be applied to arrow element if this rule choosen.
  // The rule is suitable if its |shelf| and |dir| fields are correspond
  // to current shelf alignment and text direction. Missing fields behaves like
  // '*' wildcard. The last suitable rule in list is choosen for arrow style.
  var ARROW_POSITION = {
    'app-list': [
      {position: ['points-down', 'left']},
      {dir: 'rtl', position: ['points-down', 'right']},
      {shelf: 'left', position: ['points-left', 'top']},
      {shelf: 'right', position: ['points-right', 'top']}
    ],
    'tray': [
      {position: ['points-right', 'top']},
      {dir: 'rtl', shelf: 'bottom', position: ['points-left', 'top']},
      {shelf: 'left', position: ['points-left', 'top']}
    ],
    'help': [
      {position: ['points-right', 'bottom']},
      {dir: 'rtl', shelf: 'bottom', position: ['points-left', 'bottom']},
      {shelf: 'left', position: ['points-left', 'bottom']}
    ]
  };

  var DISTANCE_TO_POINTEE = 10;
  var MINIMAL_SCREEN_OFFSET = 10;
  var ARROW_LENGTH = 6;  // Keep synced with .arrow border-width.

  Bubble.prototype = {
    __proto__: Step.prototype,

    // Element displaying arrow.
    arrow_: null,

    // Unit vector directed along the bubble arrow.
    direction_: null,

    /**
     * In addition to base class 'decorate' this method creates arrow and
     * sets some properties related to arrow.
     */
    decorate: function() {
      Step.prototype.decorate.call(this);
      this.arrow_ = document.createElement('div');
      this.arrow_.classList.add('arrow');
      this.appendChild(this.arrow_);
      var inputDirection = document.documentElement.getAttribute('dir');
      var shelfAlignment = document.documentElement.getAttribute('shelf');
      var isSuitable = function(rule) {
        var inputDirectionMatch =
            !rule.hasOwnProperty('dir') || rule.dir === inputDirection;
        var shelfAlignmentMatch =
            !rule.hasOwnProperty('shelf') || rule.shelf === shelfAlignment;
        return inputDirectionMatch && shelfAlignmentMatch;
      };
      var lastSuitableRule = null;
      var rules = ARROW_POSITION[this.getName()];
      rules.forEach(function(rule) {
        if (isSuitable(rule))
          lastSuitableRule = rule;
      });
      assert(lastSuitableRule);
      lastSuitableRule.position.forEach(function(cls) {
        this.arrow_.classList.add(cls);
      }.bind(this));
      var list = this.arrow_.classList;
      if (list.contains('points-up'))
        this.direction_ = [0, -1];
      else if (list.contains('points-right'))
        this.direction_ = [1, 0];
      else if (list.contains('points-down'))
        this.direction_ = [0, 1];
      else  // list.contains('points-left')
        this.direction_ = [-1, 0];
    },

    /**
     * Sets position of bubble in such a maner that bubble's arrow points to
     * given point.
     * @param {Array} point Bubble arrow should point to this point after
     *     positioning. |point| has format [x, y].
     * @param {offset} number Additional offset from |point|.
     */
    setPointsTo: function(point, offset) {
      var shouldShowBefore = this.hidden;
      // "Showing" bubble in order to make offset* methods work.
      if (shouldShowBefore) {
        this.style.setProperty('opacity', '0');
        this.show(false);
      }
      var arrow = [
        this.arrow_.offsetLeft + this.arrow_.offsetWidth / 2,
        this.arrow_.offsetTop + this.arrow_.offsetHeight / 2
      ];
      var totalOffset = DISTANCE_TO_POINTEE + offset;
      var left = point[0] - totalOffset * this.direction_[0] - arrow[0];
      var top = point[1] - totalOffset * this.direction_[1] - arrow[1];
      // Force bubble to be inside screen.
      if (this.arrow_.classList.contains('points-up') ||
          this.arrow_.classList.contains('points-down')) {
        left = Math.max(left, MINIMAL_SCREEN_OFFSET);
        left = Math.min(
            left,
            document.body.offsetWidth - this.offsetWidth -
                MINIMAL_SCREEN_OFFSET);
      }
      if (this.arrow_.classList.contains('points-left') ||
          this.arrow_.classList.contains('points-right')) {
        top = Math.max(top, MINIMAL_SCREEN_OFFSET);
        top = Math.min(
            top,
            document.body.offsetHeight - this.offsetHeight -
                MINIMAL_SCREEN_OFFSET);
      }
      this.style.setProperty('left', left + 'px');
      this.style.setProperty('top', top + 'px');
      if (shouldShowBefore) {
        this.hide(false);
        this.style.removeProperty('opacity');
      }
    },

    /**
     * Sets position of bubble. Overrides Step.setPosition to adjust offsets
     * in case if its direction is the same as arrow's direction.
     * @param {object} position Parameter with optional fields |top|,
     *     |right|, |bottom|, |left| holding corresponding offsets.
     */
    setPosition: function(position) {
      var arrow = this.arrow_;
      // Increasing offset if it's from side where bubble points to.
      [['top', 'points-up'], ['right', 'points-right'],
       ['bottom', 'points-down'], ['left', 'points-left']]
          .forEach(function(mapping) {
            if (position.hasOwnProperty(mapping[0]) &&
                arrow.classList.contains(mapping[1])) {
              position[mapping[0]] += ARROW_LENGTH + DISTANCE_TO_POINTEE;
            }
          });
      Step.prototype.setPosition.call(this, position);
    },
  };

  var HelpStep = cr.ui.define('div');

  HelpStep.prototype = {
    __proto__: Bubble.prototype,

    decorate: function() {
      Bubble.prototype.decorate.call(this);
      var helpButton = this.getElementsByClassName('help-button')[0];
      helpButton.addEventListener('click', function(e) {
        chrome.send('helpButtonClicked');
        e.stopPropagation();
      });
    },
  };

  var DecorateStep = function(el) {
    if (el.id == 'help')
      HelpStep.decorate(el);
    else if (el.classList.contains('bubble'))
      Bubble.decorate(el);
    else
      Step.decorate(el);
  };

  return {DecorateStep: DecorateStep};
});
