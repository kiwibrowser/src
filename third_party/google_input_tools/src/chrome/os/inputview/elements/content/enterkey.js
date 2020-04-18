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
goog.provide('i18n.input.chrome.inputview.elements.content.EnterKey');

goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');

goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;



/**
 * The enter key.
 *
 * @param {string} id The id.
 * @param {string} iconCssClass The css class for the icon.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.EnterKey = function(id,
    iconCssClass, opt_eventTarget) {
  goog.base(this, id, ElementType.ENTER_KEY, '', iconCssClass,
      opt_eventTarget);
};
goog.inherits(i18n.input.chrome.inputview.elements.content.EnterKey,
    i18n.input.chrome.inputview.elements.content.FunctionalKey);
var EnterKey = i18n.input.chrome.inputview.elements.content.EnterKey;


/** @override */
EnterKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  this.disableBackgroundHighlight();
};

});  // goog.scope
