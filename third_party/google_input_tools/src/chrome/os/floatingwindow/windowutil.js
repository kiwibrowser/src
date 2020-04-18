// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.WindowUtil');

goog.require('goog.events');
goog.require('goog.object');


goog.scope(function() {
var WindowUtil = i18n.input.chrome.WindowUtil;


/**
 * The empty floating window url.
 *
 * @private {string}
 */
WindowUtil.EMPTY_WINDOW_URL_ = 'imewindows/window.html';


/**
 * Create an empty floating window.
 *
 * @param {!Function} callback .
 * @param {Object.<string, *>=} opt_overridedOption .
 * @param {string=} opt_urlParameter .
 */
WindowUtil.createWindow = function(callback, opt_overridedOption,
    opt_urlParameter) {
  var options = goog.object.create(
      'ime', true,
      'focused', false,
      'frame', 'none',
      'alphaEnabled', true,
      'hidden', true);
  if (opt_overridedOption) {
    goog.object.forEach(opt_overridedOption, function(value, key) {
      options[key] = value;
    });
  }
  var url = opt_urlParameter ? WindowUtil.EMPTY_WINDOW_URL_ + opt_urlParameter :
      WindowUtil.EMPTY_WINDOW_URL_;
  // Right now Input Tool extension don't support floating window,
  // will support later. So adds condition to guard it.
  if (chrome.app.window && chrome.app.window.create) {
    inputview.createWindow(url, options,
        WindowUtil.createWindowCb_.bind(WindowUtil, callback));
  }
};


/**
 * Callback for "inputview.createWindow".
 *
 * @param {!Function} callback .
 * @param {chrome.app.window.AppWindow} newWindow .
 * @private
 */
WindowUtil.createWindowCb_ = function(callback, newWindow) {
  if (newWindow) {
    var contentWindow = newWindow.contentWindow;
    goog.events.listen(contentWindow, 'load', function() {
      callback(newWindow);
    });
  }
};
});  // goog.scope

