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
goog.provide('i18n.input.chrome.message.Event');

goog.require('goog.events.Event');



/**
 * The message event.
 *
 * @param {i18n.input.chrome.message.Type} type .
 * @param {*} msg .
 * @param {Function=} opt_sendResponse .
 * @constructor
 * @extends {goog.events.Event}
 */
i18n.input.chrome.message.Event = function(type, msg, opt_sendResponse) {
  goog.base(this, type);

  /** @type {*} */
  this.msg = msg;

  /** @type {Function} */
  this.sendResponse = opt_sendResponse || null;
};
goog.inherits(i18n.input.chrome.message.Event,
    goog.events.Event);

