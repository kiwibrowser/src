// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Files Tooltip.
 *
 * Adds target elements with addTarget or addTargets. Value of aria-label is
 * used as a label of the tooltip.
 *
 * Usage:
 * document.querySelector('files-tooltip').addTargets(
 *     document.querySelectorAll('[has-tooltip]'))
 */
var FilesTooltip = Polymer({
  is: 'files-tooltip',

  properties: {
    /**
     * Delay for showing the tooltip in milliseconds.
     */
    showTimeout: {
      type: Number,
      value: 500,  // ms
      readOnly: true
    },

    /**
     * Delay for hiding the tooltip in milliseconds.
     */
    hideTimeout: {
      type: Number,
      value: 250,  // ms
      readOnly: true
    }
  },

  /**
   * Initializes member variables.
   */
  created: function() {
    /**
     * @private {HTMLElement}
     */
    this.visibleTooltipTarget_ = null;

    /**
     * @private {HTMLElement}
     */
    this.upcomingTooltipTarget_ = null;

    /**
     * @private {number}
     */
    this.showTooltipTimerId_ = 0;

    /**
     * @private {number}
     */
    this.hideTooltipTimerId_ = 0;
  },

  /**
   * Adds an event listener to the body.
   */
  attached: function() {
    document.body.addEventListener(
      'mousedown', this.onDocumentMouseDown_.bind(this));
  },

  /**
   * Adds targets to tooltip.
   * @param {!NodeList} targets
   */
  addTargets: function(targets) {
    for (var i = 0; i < targets.length; i++) {
      this.addTarget(targets[i]);
    }
  },

  /**
   * Adds a target to tooltip.
   * @param {!HTMLElement} target
   */
  addTarget: function(target) {
    target.addEventListener('mouseover', this.onMouseOver_.bind(this, target));
    target.addEventListener('mouseout', this.onMouseOut_.bind(this, target));
    target.addEventListener('focus', this.onFocus_.bind(this, target));
    target.addEventListener('blur', this.onBlur_.bind(this, target));
  },

  /**
   * Hides currently visible tooltip if there is. In some cases, mouseout event
   * is not dispatched. This method is used to handle these cases manually.
   */
  hideTooltip: function() {
    if (this.visibleTooltipTarget_)
      this.initHidingTooltip_(this.visibleTooltipTarget_);
  },

  /**
   * @param {!HTMLElement} target
   * @private
   */
  initShowingTooltip_: function(target) {
    // Some tooltip is already visible.
    if (this.visibleTooltipTarget_) {
      if (this.hideTooltipTimerId_) {
        clearTimeout(this.hideTooltipTimerId_);
        this.hideTooltipTimerId_ = 0;
      }
    }

    if (this.visibleTooltipTarget_ === target)
      return;

    this.upcomingTooltipTarget_ = target;
    if (this.showTooltipTimerId_)
      clearTimeout(this.showTooltipTimerId_);
    this.showTooltipTimerId_ = setTimeout(
        this.showTooltip_.bind(this, target),
        this.visibleTooltipTarget_ ? 0 : this.showTimeout);
  },

  /**
   * @param {!HTMLElement} target
   * @private
   */
  initHidingTooltip_: function(target) {
    // The tooltip is not visible.
    if (this.visibleTooltipTarget_ !== target) {
      if (this.upcomingTooltipTarget_ === target) {
        clearTimeout(this.showTooltipTimerId_);
        this.showTooltipTimerId_ = 0;
      }
      return;
    }

    if (this.hideTooltipTimerId_)
      clearTimeout(this.hideTooltipTimerId_);
    this.hideTooltipTimerId_ = setTimeout(
        this.hideTooltip_.bind(this), this.hideTimeout);
  },

  /**
   * @param {!HTMLElement} target
   * @private
   */
  showTooltip_: function(target) {
    if (this.showTooltipTimerId_) {
      clearTimeout(this.showTooltipTimerId_);
      this.showTooltipTimerId_ = 0;
    }

    this.visibleTooltipTarget_ = target;

    var label = target.getAttribute('aria-label');
    if (!label)
      return;

    this.$.label.textContent = label;
    var rect = target.getBoundingClientRect();

    var top = rect.top + rect.height;
    if (top + this.offsetHeight > document.body.offsetHeight)
      top = rect.top - this.offsetHeight;
    this.style.top = `${Math.round(top)}px`;

    var left = rect.left + rect.width / 2 - this.offsetWidth / 2;
    if (left < 0)
      left = 0;
    if (left > document.body.offsetWidth - this.offsetWidth)
      left = document.body.offsetWidth - this.offsetWidth;
    this.style.left = `${Math.round(left)}px`;

    this.setAttribute('visible', true);
  },

  /**
   * @private
   */
  hideTooltip_: function() {
    if (this.hideTooltipTimerId_) {
      clearTimeout(this.hideTooltipTimerId_);
      this.hideTooltipTimerId_ = 0;
    }

    this.visibleTooltipTarget_ = null;
    this.removeAttribute('visible');
  },

  /**
   * @param {Event} event
   * @private
   */
  onMouseOver_: function(target, event) {
    this.initShowingTooltip_(target);
  },

  /**
   * @param {Event} event
   * @private
   */
  onMouseOut_: function(target, event) {
    this.initHidingTooltip_(target);
  },

  /**
   * @param {Event} event
   * @private
   */
  onFocus_: function(target, event) {
    this.initShowingTooltip_(target);
  },

  /**
   * @param {Event} event
   * @private
   */
  onBlur_: function(target, event) {
    this.initHidingTooltip_(target);
  },

  /**
   * @param {Event} event
   * @private
   */
  onDocumentMouseDown_: function(event) {
    this.hideTooltip_();

    // Additionally prevent any scheduled tooltips from showing up.
    if (this.showTooltipTimerId_) {
      clearTimeout(this.showTooltipTimerId_);
      this.showTooltipTimerId_ = 0;
    }
  }
});
