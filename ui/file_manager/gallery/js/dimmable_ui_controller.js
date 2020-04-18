// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Dimmable UI Controller.
 * @param {!HTMLElement} container
 * @constructor
 * @struct
 */
function DimmableUIController(container) {
  /**
   * @private {!HTMLElement}
   * @const
   */
  this.container_ = container;

  /**
   * @private {NodeList}
   */
  this.tools_ = null;

  /**
   * @private {number}
   */
  this.timeoutId_ = 0;

  /**
   * @private {boolean}
   */
  this.isCursorInTools_ = false;

  /**
   * @private {GalleryMode|undefined}
   */
  this.mode_ = undefined;

  /**
   * @private {GallerySubMode|undefined}
   */
  this.subMode_ = undefined;

  /**
   * @private {boolean}
   */
  this.spokenFeedbackEnabled_ = false;

  /**
   * @private {boolean}
   */
  this.loading_ = false;

  /**
   * @private {boolean}
   */
  this.renaming_ = false;

  /**
   * @private {boolean}
   */
  this.disabled_ = false;

  /**
   * @private {number}
   */
  this.madeVisibleAt_ = 0;

  this.container_.addEventListener('click', this.onClick_.bind(this));
  this.container_.addEventListener('mousemove', this.onMousemove_.bind(this));
  this.container_.addEventListener(
      'touchstart', this.onTouchOperation_.bind(this));
  this.container_.addEventListener(
      'touchmove', this.onTouchOperation_.bind(this));
  this.container_.addEventListener(
      'touchend', this.onTouchOperation_.bind(this));
  this.container_.addEventListener(
      'touchcancel', this.onTouchOperation_.bind(this));

  chrome.accessibilityFeatures.spokenFeedback.onChange.addListener(
      this.onGetOrChangedSpokenFeedbackConfiguration_.bind(this));
  chrome.accessibilityFeatures.spokenFeedback.get({},
      this.onGetOrChangedSpokenFeedbackConfiguration_.bind(this));
}

/**
 * Default timeout.
 * @const {number}
 */
DimmableUIController.DEFAULT_TIMEOUT = 3000; // ms

/**
 * We don't allow user to change visibility of tools shorter than this interval.
 * This is necessary not to hide tools immediately after they become visible by
 * touchstart event when user taps UI to make them visible.
 * @const {number}
 */
DimmableUIController.MIN_OPERATION_INTERVAL = 500; // ms

/**
 * Returns true if this controller should be disabled.
 * @param {GalleryMode|undefined} mode
 * @param {GallerySubMode|undefined} subMode
 * @param {boolean} loading
 * @param {boolean} spokenFeedbackEnabled
 * @param {boolean} renaming
 * @return {boolean}
 */
DimmableUIController.shouldBeDisabled = function(
    mode, subMode, loading, spokenFeedbackEnabled, renaming) {
  return spokenFeedbackEnabled || mode === undefined || subMode === undefined ||
      mode === GalleryMode.THUMBNAIL ||
      (mode === GalleryMode.SLIDE && subMode === GallerySubMode.EDIT) ||
      (mode === GalleryMode.SLIDE && subMode === GallerySubMode.BROWSE &&
       (loading || renaming));
};

/**
 * Sets current mode of Gallery.
 * @param {GalleryMode} mode
 * @param {GallerySubMode} subMode
 */
DimmableUIController.prototype.setCurrentMode = function(mode, subMode) {
  if (this.mode_ === mode && this.subMode_ === subMode)
    return;

  this.mode_ = mode;
  this.subMode_ = subMode;
  this.updateAvailability_();
};

/**
 * Sets whether user is renaming an image or not.
 * @param {boolean} renaming
 */
DimmableUIController.prototype.setRenaming = function(renaming) {
  if (this.renaming_ === renaming)
    return;

  this.renaming_ = renaming;
  this.updateAvailability_();
};

/**
 * Sets whether gallery is currently loading an image or not.
 * @param {boolean} loading
 */
DimmableUIController.prototype.setLoading = function(loading) {
  if (this.loading_ === loading)
    return;

  this.loading_ = loading;
  this.updateAvailability_();
};

/**
 * Handles click event.
 * @param {!Event} event An event.
 * @private
 */
DimmableUIController.prototype.onClick_ = function(event) {
  if (this.disabled_ ||
      (event.target &&
       this.isPartOfTools_(/** @type {!HTMLElement} */ (event.target)))) {
    return;
  }

  this.toggle_();
};

/**
 * Handles mousemove event.
 * @private
 */
DimmableUIController.prototype.onMousemove_ = function() {
  if (this.disabled_)
    return;

  this.kick();
};

/**
 * Handles touch event.
 * @private
 */
DimmableUIController.prototype.onTouchOperation_ = function() {
  if (this.disabled_)
    return;

  this.kick();
};

/**
 * Handles mouseover event.
 * @private
 */
DimmableUIController.prototype.onMouseover_ = function() {
  if (this.disabled_)
    return;

  this.isCursorInTools_ = true;
};

/**
 * Handles mouseout event.
 * @private
 */
DimmableUIController.prototype.onMouseout_ = function() {
  if (this.disabled_)
    return;

  this.isCursorInTools_ = false;
};

/**
 * Returns true if element is a part of tools.
 * @param {!HTMLElement} element A html element.
 * @return {boolean} True if element is a part of tools.
 * @private
 */
DimmableUIController.prototype.isPartOfTools_ = function(element) {
  for (var i = 0; i < this.tools_.length; i++) {
    if (this.tools_[i].contains(element))
      return true;
  }
  return false;
};

/**
 * Toggles visibility of UI.
 * @private
 */
DimmableUIController.prototype.toggle_ = function() {
  if (this.isToolsVisible_())
    this.show_(false);
  else
    this.kick();
};

/**
 * Returns true if UI is visible.
 * @return {boolean} True if UI is visible.
 * @private
 */
DimmableUIController.prototype.isToolsVisible_ = function() {
  return this.container_.hasAttribute('tools');
};

/**
 * Shows UI.
 * @param {boolean} show True to show UI.
 * @private
 */
DimmableUIController.prototype.show_ = function(show) {
  if (this.isToolsVisible_() === show)
    return;

  if (show) {
    this.madeVisibleAt_ = Date.now();
    this.container_.setAttribute('tools', true);
  } else {
    if (Date.now() - this.madeVisibleAt_ <
        DimmableUIController.MIN_OPERATION_INTERVAL) {
      return;
    }

    this.container_.removeAttribute('tools');
    this.clearTimeout_();
  }
};

/**
 * Clears current timeout.
 * @private
 */
DimmableUIController.prototype.clearTimeout_ = function() {
  if (!this.timeoutId_)
    return;

  clearTimeout(this.timeoutId_);
  this.timeoutId_ = 0;
};

/**
 * Extends current timeout.
 * @param {number=} opt_timeout Timeout.
 * @private
 */
DimmableUIController.prototype.extendTimeout_ = function(opt_timeout) {
  this.clearTimeout_();

  var timeout = opt_timeout || DimmableUIController.DEFAULT_TIMEOUT;
  this.timeoutId_ = setTimeout(this.onTimeout_.bind(this), timeout);
};

/**
 * Handles timeout.
 * @private
 */
DimmableUIController.prototype.onTimeout_ = function() {
  // If mouse cursor is on tools, extend timeout.
  if (this.isCursorInTools_) {
    this.extendTimeout_();
    return;
  }

  this.show_(false /* hide */);
};

/**
 * Updates availability of this controller with spoken feedback configuration.
 * @param {Object} details
 * @private
 */
DimmableUIController.prototype.onGetOrChangedSpokenFeedbackConfiguration_ =
    function(details) {
  this.spokenFeedbackEnabled_ = !!details.value;
  this.updateAvailability_();
};

/**
 * Sets tools which are controlled by this controller.
 * This method must not be called more than once for an instance.
 * @param {!NodeList} tools Tools.
 */
DimmableUIController.prototype.setTools = function(tools) {
  assert(this.tools_ === null);

  this.tools_ = tools;

  for (var i = 0; i < this.tools_.length; i++) {
    this.tools_[i].addEventListener('mouseover', this.onMouseover_.bind(this));
    this.tools_[i].addEventListener('mouseout', this.onMouseout_.bind(this));
  }
};

/**
 * Shows UI and set timeout.
 * @param {number=} opt_timeout Timeout.
 */
DimmableUIController.prototype.kick = function(opt_timeout) {
  if (this.disabled_)
    return;

  this.show_(true);
  this.extendTimeout_(opt_timeout);
};

/**
 * Updates availability.
 * @private
 */
DimmableUIController.prototype.updateAvailability_ = function() {
  var disabled = DimmableUIController.shouldBeDisabled(
      this.mode_, this.subMode_, this.loading_, this.spokenFeedbackEnabled_,
      this.renaming_);

  if (this.disabled_ === disabled)
    return;

  this.disabled_ = disabled;

  if (this.disabled_) {
    this.isCursorInTools_ = false;
    this.show_(true);
    this.clearTimeout_();
  } else {
    this.kick();
  }
};

/**
 * Sets cursor's state as out of tools. Mouseout event is not dispatched for
 * some cases even when mouse cursor goes out of elements. This method is used
 * to handle these cases manually.
 */
DimmableUIController.prototype.setCursorOutOfTools = function() {
  this.isCursorInTools_ = false;
};
