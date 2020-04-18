// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * A prompt panel for the editor.
 *
 * @param {!HTMLElement} container Container element.
 * @param {function(string, ...string)} displayStringFunction A formatting
 *     function.
 * @constructor
 * @struct
 */
function ImageEditorPrompt(container, displayStringFunction) {
  this.container_ = container;
  this.displayStringFunction_ = displayStringFunction;

  /**
   * @type {HTMLDivElement}
   * @private
   */
  this.wrapper_ = null;

  /**
   * @type {HTMLDivElement}
   * @private
   */
  this.prompt_ = null;

  /**
   * @type {number}
   * @private
   */
  this.timer_ = 0;
};

/**
 * Reset the prompt.
 */
ImageEditorPrompt.prototype.reset = function() {
  this.cancelTimer();
  if (this.wrapper_) {
    this.container_.removeChild(this.wrapper_);
    this.wrapper_ = null;
    this.prompt_ = null;
  }
};

/**
 * Cancel the delayed action.
 */
ImageEditorPrompt.prototype.cancelTimer = function() {
  if (this.timer_) {
    clearTimeout(this.timer_);
    this.timer_ = 0;
  }
};

/**
 * Schedule the delayed action.
 * @param {function()} callback Callback.
 * @param {number} timeout Timeout.
 */
ImageEditorPrompt.prototype.setTimer = function(callback, timeout) {
  this.cancelTimer();
  var self = this;
  this.timer_ = setTimeout(function() {
    self.timer_ = 0;
    callback();
  }, timeout);
};

/**
 * Show the prompt.
 *
 * @param {string} text The prompt text.
 * @param {number=} opt_timeout Timeout in ms.
 * @param {...Object} var_args varArgs for the formatting function.
 */
ImageEditorPrompt.prototype.show = function(text, opt_timeout, var_args) {
  var args = [text].concat(Array.prototype.slice.call(arguments, 2));
  var message = this.displayStringFunction_.apply(null, args);
  this.showStringAt('center', message, opt_timeout);
};

/**
 * Show the position at the specific position.
 *
 * @param {string} pos The 'pos' attribute value.
 * @param {string} text The prompt text.
 * @param {number} timeout Timeout in ms.
 * @param {...Object} var_args varArgs for the formatting function.
 */
ImageEditorPrompt.prototype.showAt = function(pos, text, timeout, var_args) {
  var args = [text].concat(Array.prototype.slice.call(arguments, 3));
  var message = this.displayStringFunction_.apply(null, args);
  this.showStringAt(pos, message, timeout);
};

/**
 * Show the string in the prompt
 *
 * @param {string} pos The 'pos' attribute value.
 * @param {string} text The prompt text.
 * @param {number=} opt_timeout Timeout in ms.
 */
ImageEditorPrompt.prototype.showStringAt = function(pos, text, opt_timeout) {
  this.reset();
  if (!text)
    return;

  var document = this.container_.ownerDocument;
  this.wrapper_ =
      assertInstanceof(document.createElement('div'), HTMLDivElement);
  this.wrapper_.className = 'prompt-wrapper';
  this.wrapper_.setAttribute('pos', pos);
  this.container_.appendChild(this.wrapper_);

  this.prompt_ =
      assertInstanceof(document.createElement('div'), HTMLDivElement);
  this.prompt_.className = 'prompt';

  // Create an extra wrapper which opacity can be manipulated separately.
  var tool = document.createElement('div');
  tool.className = 'dimmable';
  this.wrapper_.appendChild(tool);
  tool.appendChild(this.prompt_);

  this.prompt_.textContent = text;

  var close = document.createElement('div');
  close.className = 'close';
  close.addEventListener('click', this.hide.bind(this));
  this.prompt_.appendChild(close);

  setTimeout(
      this.prompt_.setAttribute.bind(this.prompt_, 'state', 'fadein'), 0);

  if (opt_timeout)
    this.setTimer(this.hide.bind(this), opt_timeout);
};

/**
 * Hide the prompt.
 */
ImageEditorPrompt.prototype.hide = function() {
  if (!this.prompt_)
    return;
  this.prompt_.setAttribute('state', 'fadeout');
  // Allow some time for the animation to play out.
  this.setTimer(this.reset.bind(this), 500);
};
