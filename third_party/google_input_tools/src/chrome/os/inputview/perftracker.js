// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
goog.provide('i18n.input.chrome.inputview.PerfTracker');

goog.require('i18n.input.chrome.Statistics');


goog.scope(function() {



/**
 * The tracker for the performance.
 *
 * @param {PerfTracker.TickName} htmlLoadedTickName .
 * @constructor
 */
i18n.input.chrome.inputview.PerfTracker = function(
    htmlLoadedTickName) {
  /**
   * The time when this tracker starts.
   *
   * @private {number}
   */
  this.startInMs_ = new Date().getTime();

  this.tick(htmlLoadedTickName,
      window['InputViewPageStartLoading'],
      'Time elapsed before 0');
};
var PerfTracker = i18n.input.chrome.inputview.PerfTracker;


/** @private {boolean} */
PerfTracker.prototype.stopped_ = false;


/**
 * The name of the tick.
 *
 * @enum {string}
 */
PerfTracker.TickName = {
  BACKGROUND_HTML_LOADED: 'BackgroundHtmlLoaded',
  NACL_LOADED: 'NaclLoaded',
  BACKGROUND_SETTINGS_FETCHED: 'BackgroundSettingsFetched',
  HTML_LOADED: 'HtmlLoaded',
  KEYBOARD_CREATED: 'KeyboardCreated',
  KEYBOARD_SHOWN: 'KeyboardShown',
  KEYSET_LOADED: 'KeysetLoaded',
  LAYOUT_LOADED: 'LayoutLoaded'
};


/**
 * Resets this performance tracker.
 */
PerfTracker.prototype.restart = function() {
  this.startInMs_ = new Date().getTime();
  this.stopped_ = false;
};


/**
 * Stops the performance tracker.
 */
PerfTracker.prototype.stop = function() {
  this.stopped_ = true;
};


/**
 * Ticks with a custom message.
 *
 * @param {PerfTracker.TickName} tickName .
 * @param {number=} opt_startInMs The timestamp used as start, if not
 *     specified, use this.startInMs_.
 * @param {string=} opt_msg Extra log message to describe the logging in more
 *     detail.
 */
PerfTracker.prototype.tick = function(tickName, opt_startInMs, opt_msg) {
  if (this.stopped_) {
    return;
  }

  var startInMs = opt_startInMs || this.startInMs_;
  var cost = new Date().getTime() - startInMs;
  var msg = tickName + '  -  ' + cost;
  if (opt_msg) {
    msg += '  -  ' + opt_msg;
  }
  console.log(msg);
  i18n.input.chrome.Statistics.getInstance().recordLatency(
      'InputMethod.VirtualKeyboard.InitLatency.' + tickName, cost);
};

});  // goog.scope

