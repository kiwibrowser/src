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
goog.provide('i18n.input.chrome.inputview.elements.content.BackspaceKey');

goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');



goog.scope(function() {



/**
 * The backspace key.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {string} text The text.
 * @param {string} iconCssClass The css class for the icon.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @param {string=} opt_textCssClass The css class for the text.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.BackspaceKey = function(id, type,
    text, iconCssClass, opt_eventTarget, opt_textCssClass) {
  goog.base(this, id, type, text, iconCssClass, opt_eventTarget,
      opt_textCssClass);

  this.pointerConfig.longPressWithPointerUp = true;
  this.pointerConfig.longPressDelay = 300;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.BackspaceKey,
    i18n.input.chrome.inputview.elements.content.FunctionalKey);
});  // goog.scope

