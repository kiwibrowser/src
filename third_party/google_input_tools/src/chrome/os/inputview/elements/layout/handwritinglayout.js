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
goog.provide('i18n.input.chrome.inputview.elements.layout.HandwritingLayout');

goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.layout.LinearLayout');

goog.scope(function() {



/**
 * The linear layout.
 *
 * @param {string} id The id.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.layout.LinearLayout}
 */
i18n.input.chrome.inputview.elements.layout.HandwritingLayout = function(id,
    opt_eventTarget) {
  goog.base(this, id, opt_eventTarget);

  /** @override */
  this.type = i18n.input.chrome.ElementType.
      HANDWRITING_LAYOUT;
};
goog.inherits(i18n.input.chrome.inputview.elements.layout.HandwritingLayout,
    i18n.input.chrome.inputview.elements.layout.LinearLayout);
var HandwritingLayout =
    i18n.input.chrome.inputview.elements.layout.HandwritingLayout;


/** @override */
HandwritingLayout.prototype.createDom = function() {
  goog.base(this, 'createDom');

  goog.dom.classlist.add(this.getElement(), i18n.input.chrome.inputview.Css.
      HANDWRITING_LAYOUT);
};


/** @override */
HandwritingLayout.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);
};

});  // goog.scope
