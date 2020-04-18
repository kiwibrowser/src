// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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

/**
 * @fileoverview The definition of layout event.
 */

goog.provide('i18n.input.chrome.vk.LayoutEvent');

goog.require('goog.events.Event');



/**
 * The Layout Event.
 *
 * @param {i18n.input.chrome.vk.EventType} type The event type.
 * @param {Object} layoutView The layoutView object. See ParsedLayout for
 *     details.
 * @constructor
 * @extends {goog.events.Event}
 */
i18n.input.chrome.vk.LayoutEvent = function(type, layoutView) {
  goog.base(this, type);

  /**
   * The layout view object.
   *
   * @type {Object}
   */
  this.layoutView = layoutView;

  /**
   * The layout code.
   *
   * @type {?string}
   */
  this.layoutCode = layoutView ? layoutView.id : null;
};
goog.inherits(i18n.input.chrome.vk.LayoutEvent, goog.events.Event);
