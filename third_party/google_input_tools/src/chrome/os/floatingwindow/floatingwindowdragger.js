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
goog.provide('i18n.input.chrome.FloatingWindowDragger');

goog.require('goog.fx.Dragger');

goog.scope(function() {



/**
 * The dragger for floating window.
 *
 * @param {!chrome.app.window.AppWindow|!Window} win The window.
 * @param {Element} handle A handle to control the drag
 * @constructor
 * @extends {goog.fx.Dragger}
 */
i18n.input.chrome.FloatingWindowDragger = function(win, handle) {
  goog.base(this, handle);

  /** @private {!chrome.app.window.AppWindow|!Window} */
  this.floatingWindow_ = win;
};
var FloatingWindowDragger = i18n.input.chrome.FloatingWindowDragger;
goog.inherits(FloatingWindowDragger, goog.fx.Dragger);


/** @override */
FloatingWindowDragger.prototype.defaultAction = function(x, y) {
  var outerBounds = this.floatingWindow_.outerBounds;
  if (outerBounds) {
    outerBounds.setPosition(outerBounds.left + x, outerBounds.top + y);
  } else {
    this.floatingWindow_.moveTo(this.floatingWindow_.screenX + x,
                                this.floatingWindow_.screenY + y);
  }
};
});  // goog.scope
