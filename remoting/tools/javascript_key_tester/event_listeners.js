/* Copyright 2015 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/** @constructor */
var PNaClEvent = function() {
  /** @type {string} */
  this.type = "";
  /** @type {number} */
  this.modifiers = 0;
  /** @type {number} */
  this.keyCode = 0;
  /** @type {string} */
  this.characterText = '';
  /** @type {string} */
  this.code = '';
};


/**
 * @param {Element} pnaclLog
 * @param {Element} textLog
 * @param {Element} pnaclPlugin
 * @param {Element} pnaclListener
 * @constructor
 */
var EventListeners = function(pnaclLog, textLog, pnaclPlugin, pnaclListener) {
  this.pnaclPlugin_ = pnaclPlugin;
  this.pnaclListener_ = pnaclListener;
  this.textLog_ = textLog;

  this.onKeyDownHandler_ = this.onKeyDown_.bind(this);
  this.onKeyUpHandler_ = this.onKeyUp_.bind(this);
  this.onKeyPressHandler_ = this.onKeyPress_.bind(this);
  this.onMessageHandler_ = this.onMessage_.bind(this);
  this.onPluginBlurHandler_ = this.onPluginBlur_.bind(this);
  this.onWindowBlurHandler_ = this.onWindowBlur_.bind(this);

  this.pnaclChordTracker_ = new ChordTracker(pnaclLog);

  this.startTime_ = new Date();
};

/**
 * Start listening for keyboard events.
 */
EventListeners.prototype.activate = function() {
  window.addEventListener('blur', this.onWindowBlurHandler_, false);
  document.body.addEventListener('keydown', this.onKeyDownHandler_, false);
  document.body.addEventListener('keyup', this.onKeyUpHandler_, false);
  document.body.addEventListener('keypress', this.onKeyPressHandler_, false);
  this.pnaclListener_.addEventListener('message', this.onMessageHandler_, true);
  this.pnaclPlugin_.addEventListener('blur', this.onPluginBlurHandler_, false);
  this.onPluginBlur_();
};

/**
 * Stop listening for keyboard events.
 */
EventListeners.prototype.deactivate = function() {
  window.removeEventListener('blur', this.onWindowBlurHandler_, false);
  document.body.removeEventListener('keydown', this.onKeyDownHandler_, false);
  document.body.removeEventListener('keyup', this.onKeyUpHandler_, false);
  document.body.removeEventListener('keypress', this.onKeyPressHandler_, false);
  this.pnaclListener_.removeEventListener(
      'message', this.onMessageHandler_, true);
  this.pnaclPlugin_.removeEventListener(
      'blur', this.onPluginBlurHandler_, false);
};

/**
 * @param {Event} event
 * @private
 */
EventListeners.prototype.onKeyDown_ = function(event) {
  this.appendToTextLog_(this.jsonifyJavascriptKeyEvent_(event, 'keydown'));
};

/**
 * @param {Event} event
 * @return {void}
 */
EventListeners.prototype.onKeyUp_ = function(event) {
  this.appendToTextLog_(this.jsonifyJavascriptKeyEvent_(event, 'keyup'));
}

/**
 * @param {Event} event
 * @return {void}
 */
EventListeners.prototype.onKeyPress_ = function(event) {
  this.appendToTextLog_(this.jsonifyJavascriptKeyEvent_(event, 'keypress'));
}

/**
 * @param {Event} event
 * @return {void}
 */
EventListeners.prototype.onMessage_ = function (event) {
  var pnaclEvent = /** @type {PNaClEvent} */ (event['data']);

  this.appendToTextLog_(this.jsonifyPnaclInputEvent_(pnaclEvent));
  var title = this.jsonifyPnaclInputEvent_(pnaclEvent, 2);
  if (pnaclEvent.type == "KEYDOWN") {
    this.pnaclChordTracker_.addKeyDownEvent(pnaclEvent.code, title);
  }
  if (pnaclEvent.type == "KEYUP") {
    this.pnaclChordTracker_.addKeyUpEvent(pnaclEvent.code, title);
  }
  if (pnaclEvent.type == "CHAR") {
    this.pnaclChordTracker_.addCharEvent(pnaclEvent.characterText, title);
  }
}

/**
 * @return {void}
 */
EventListeners.prototype.onPluginBlur_ = function() {
  this.pnaclPlugin_.focus();
};

/**
 * @return {void}
 */
EventListeners.prototype.onWindowBlur_ = function() {
  this.pnaclChordTracker_.releaseAllKeys();
};

/**
 * @param {string} str
 * @private
 */
EventListeners.prototype.appendToTextLog_ = function(str) {
  var div = document.createElement('div');
  div.innerText = (new Date() - this.startTime_) + ':' + str;
  this.textLog_.appendChild(div);
};

/**
 * @param {Event} event
 * @param {string} eventName
 * @param {number=} opt_space
 * @return {string}
 * @private
 */
EventListeners.prototype.jsonifyJavascriptKeyEvent_ =
    function(event, eventName, opt_space) {
  return "JavaScript '" + eventName + "' event = " + JSON.stringify(
      event,
      ['type', 'alt', 'shift', 'control', 'meta', 'charCode', 'keyCode',
       'keyIdentifier', 'repeat', 'code'],
      opt_space);
};

/**
 * @param {PNaClEvent} event
 * @param {number=} opt_space
 * @return {string}
 * @private
 */
EventListeners.prototype.jsonifyPnaclInputEvent_ =
    function(event, opt_space) {
  return "PNaCl InputEvent = " + JSON.stringify(
      event,
      ['type',
       'modifiers', 'keyCode', 'characterText', 'code',
       'button', 'position', 'clickcount', 'movement',
       'delta', 'ticks', 'scrollByPage' ],
      opt_space);
};
