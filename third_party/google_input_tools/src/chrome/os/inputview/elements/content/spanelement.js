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
goog.provide('i18n.input.chrome.inputview.elements.content.SpanElement');

goog.require('goog.dom.TagName');
goog.require('i18n.input.chrome.inputview.elements.Element');



goog.scope(function() {



/**
 * The wrappered span element.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.SpanElement = function(id, type,
    opt_eventTarget) {
  i18n.input.chrome.inputview.elements.content.SpanElement.base(
      this, 'constructor', id, type, opt_eventTarget);
};
var SpanElement = i18n.input.chrome.inputview.elements.content.SpanElement;
goog.inherits(SpanElement, i18n.input.chrome.inputview.elements.Element);


/** @override */
SpanElement.prototype.createDom = function() {
  var elem = this.getDomHelper().createDom(goog.dom.TagName.SPAN);
  this.setElementInternal(elem);
  elem.id = this.id;
  elem['view'] = this;
};
});  // goog.scope


