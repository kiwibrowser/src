// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
 * A mock text-to-speech engine for tests.
 * This class has functions and callbacks necessary for Select-to-Speak
 * to function. It keeps track of the utterances currently being spoken,
 * and whether TTS should be speaking or is stopped.
 */
var MockTts = function() {
  /**
   * @type {Array<string>}
   * @private
   */
  this.pendingUtterances_ = [];

  /**
   * @type {boolean}
   * @private
   */
  this.currentlySpeaking_ = false;

  /**
   * A list of callbacks to call each time speech is requested.
   * These are stored such that the last one should be called
   * first. Each should only be used once.
   * @type {Array<function(string)>}
   * @private
   */
  this.speechCallbackStack_ = [];

  /**
   * Options object for speech.
   * @type {onEvent: !function({type: string, charIndex: number})}
   * @private
   */
  this.options_ = null;
};

MockTts.prototype = {
  // Functions based on methods in
  // https://developer.chrome.com/extensions/tts
  speak: function(utterance, options) {
    this.pendingUtterances_.push(utterance);
    this.currentlySpeaking_ = true;
    if (options && options.onEvent) {
      this.options_ = options;
      this.options_.onEvent({type: 'start', charIndex: 0});
    }
    if (this.speechCallbackStack_.length > 0) {
      this.speechCallbackStack_.pop()(utterance);
    }
  },
  stop: function() {
    this.pendingUtterances_ = [];
    this.currentlySpeaking_ = false;
    if (this.options_) {
      this.options_.onEvent({type: 'cancelled'});
      this.options_ = null;
    }
  },
  getVoices: function(callback) {
    callback([{voiceName: 'English US', lang: 'English'}]);
  },
  isSpeaking: function(callback) {
    callback(this.currentlySpeaking_);
  },
  // Functions for testing
  currentlySpeaking: function() {
    return this.currentlySpeaking_;
  },
  pendingUtterances: function() {
    return this.pendingUtterances_;
  },
  setOnSpeechCallbacks: function(callbacks) {
    this.speechCallbackStack_ = callbacks.reverse();
  }
};
