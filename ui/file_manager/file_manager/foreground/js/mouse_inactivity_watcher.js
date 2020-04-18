// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A controller class detects mouse inactivity and hides "tool" elements.
 *
 * @param {Element} container The main DOM container.
 * @param {number=} opt_timeout Hide timeout in ms.
 * @param {function():boolean=} opt_toolsActive Function that returns |true|
 *     if the tools are active and should not be hidden.
 * @constructor
 */
function MouseInactivityWatcher(container, opt_timeout, opt_toolsActive) {
  this.container_ = container;
  this.timeout_ = opt_timeout || MouseInactivityWatcher.DEFAULT_TIMEOUT;
  this.toolsActive_ = opt_toolsActive || function() { return false; };

  this.onTimeoutBound_ = this.onTimeout_.bind(this);
  this.timeoutID_ = null;
  this.mouseOverTool_ = false;

  this.clientX_ = 0;
  this.clientY_ = 0;

  /**
   * Indicates if the inactivity watcher is enabled or disabled. Use getters
   * and setters.
   * @type {boolean}
   * @private
   **/
  this.disabled_ = false;

  this.container_.addEventListener('mousemove', this.onMouseMove_.bind(this));
  var tools = this.container_.querySelector('.tool');
  for (var i = 0; i < tools.length; i++) {
    tools[i].addEventListener('mouseover', this.onToolMouseOver_.bind(this));
    tools[i].addEventListener('mouseout', this.onToolMouseOut_.bind(this));
  }

  // Show tools when the user touches the screen.
  this.container_.addEventListener(
      'touchstart', this.activityStarted_.bind(this));
  var initiateFading = this.activityStopped_.bind(this, this.timeout_);
  this.container_.addEventListener('touchend', initiateFading);
  this.container_.addEventListener('touchcancel', initiateFading);
  this.container_.addEventListener('focusin', function() {
    this.activityStarted_();
    this.activityStopped_();
  }.bind(this));
  // If pointer goes outside the app window, tools should be hidden immediately.
  document.addEventListener('mouseout', function(event) {
    if (event.relatedTarget === null)
      this.forceTimeout_();
  }.bind(this));
}

/**
 * Default inactivity timeout.
 */
MouseInactivityWatcher.DEFAULT_TIMEOUT = 3000;

/**
 * Defines getter/setter for disabled property to update inactivity state.
 */
MouseInactivityWatcher.prototype = {
  /**
   * @return {boolean}
   */
  get disabled() {
    return this.disabled_;
  },

  /**
   * @param {boolean} value
   */
  set disabled(value) {
    this.disabled_ = value;
    if (value)
      this.kick();
    else
      this.check();
  }
};

/**
 * @param {boolean} on True if show, false if hide.
 */
MouseInactivityWatcher.prototype.showTools = function(on) {
  if (on)
    this.container_.setAttribute('tools', 'true');
  else
    this.container_.removeAttribute('tools');
};

/**
 * To be called when the user started activity. Shows the tools
 * and cancels the countdown.
 * @private
 */
MouseInactivityWatcher.prototype.activityStarted_ = function() {
  this.showTools(true);

  if (this.timeoutID_) {
    clearTimeout(this.timeoutID_);
    this.timeoutID_ = null;
  }
};

/**
 * Called when user activity has stopped. Re-starts the countdown.
 * @param {number=} opt_timeout Timeout.
 * @private
 */
MouseInactivityWatcher.prototype.activityStopped_ = function(opt_timeout) {
  if (this.disabled_ || this.mouseOverTool_ || this.toolsActive_())
    return;

  if (this.timeoutID_)
    clearTimeout(this.timeoutID_);

  this.timeoutID_ = setTimeout(
      this.onTimeoutBound_, opt_timeout || this.timeout_);
};

/**
 * Called when a user performed a short action (such as a click or a key press)
 * that should show the tools if they are not visible.
 * @param {number=} opt_timeout Timeout.
 */
MouseInactivityWatcher.prototype.kick = function(opt_timeout) {
  this.activityStarted_();
  this.activityStopped_(opt_timeout);
};

/**
 * Check if the tools are active and update the tools visibility accordingly.
 */
MouseInactivityWatcher.prototype.check = function() {
  if (this.toolsActive_())
    this.activityStarted_();
  else
    this.activityStopped_();
};

/**
 * Mouse move handler.
 *
 * @param {Event} e Event.
 * @private
 */
MouseInactivityWatcher.prototype.onMouseMove_ = function(e) {
  if (this.clientX_ == e.clientX && this.clientY_ == e.clientY) {
    // The mouse has not moved, must be the cursor change triggered by
    // some of the attributes on the root container. Ignore the event.
    return;
  }
  this.clientX_ = e.clientX;
  this.clientY_ = e.clientY;

  if (this.disabled_)
    return;

  this.kick();
};

/**
 * Mouse over handler on a tool element.
 *
 * @param {Event} e Event.
 * @private
 */
MouseInactivityWatcher.prototype.onToolMouseOver_ = function(e) {
  this.mouseOverTool_ = true;
  if (!this.disabled_)
    this.kick();
};

/**
 * Mouse out handler on a tool element.
 *
 * @param {Event} e Event.
 * @private
 */
MouseInactivityWatcher.prototype.onToolMouseOut_ = function(e) {
  this.mouseOverTool_ = false;
  if (!this.disabled_)
    this.kick();
};

/**
 * Timeout handler.
 * @private
 */
MouseInactivityWatcher.prototype.onTimeout_ = function() {
  this.timeoutID_ = null;
  if (!this.disabled_ && !this.toolsActive_())
    this.showTools(false);
};

/**
 * Force the timer to be timed out immediately.
 * @private
 */
MouseInactivityWatcher.prototype.forceTimeout_ = function() {
  if (this.timeoutID_) {
    clearTimeout(this.timeoutID_);
    this.onTimeout_();
  }
};
