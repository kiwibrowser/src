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
goog.provide('i18n.input.chrome.inputview.elements.content.HandwritingView');

goog.require('goog.dom.classlist');
goog.require('goog.i18n.bidi');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.content.KeysetView');


goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;
var KeysetView = i18n.input.chrome.inputview.elements.content.KeysetView;



/**
 * The handwriting view.
 *
 * @param {!Object} keyData The data includes soft key definition and key
 *     mapping.
 * @param {!Object} layoutData The layout definition.
 * @param {string} keyboardCode The keyboard code.
 * @param {string} languageCode The language code.
 * @param {!i18n.input.chrome.inputview.Model} model The model.
 * @param {string} name The Input Tool name.
 * @param {!goog.events.EventTarget=} opt_eventTarget .
 * @param {i18n.input.chrome.inputview.Adapter=} opt_adapter .
 * @constructor
 * @extends {KeysetView}
 */
i18n.input.chrome.inputview.elements.content.HandwritingView = function(keyData,
    layoutData, keyboardCode, languageCode, model, name, opt_eventTarget,
    opt_adapter) {
  i18n.input.chrome.inputview.elements.content.HandwritingView.base(this,
      'constructor', keyData, layoutData, keyboardCode, languageCode, model,
      name, opt_eventTarget, opt_adapter);
};
var HandwritingView = i18n.input.chrome.inputview.elements.content.
    HandwritingView;
goog.inherits(HandwritingView, KeysetView);


/** @override */
HandwritingView.prototype.activate = function(rawKeyset) {
  goog.dom.classlist.add(this.getElement().parentElement.parentElement,
      Css.HANDWRITING);
  // Clears stroke when switches keyboard.
  if (this.canvasView.hasStrokesOnCanvas()) {
    this.canvasView.reset();
  }
};


/** @override */
HandwritingView.prototype.deactivate = function(rawKeyset) {
  this.adapter.unsetController();
  goog.dom.classlist.remove(this.getElement().parentElement.parentElement,
      Css.HANDWRITING);
};


/**
 * Updates the language code.
 *
 * @param {string} languageCode .
 */
HandwritingView.prototype.setLanguagecode = function(languageCode) {
  this.languageCode = languageCode;
  this.adapter.setController('hwt', this.languageCode);
  this.canvasView.setPrivacyInfoDirection(
      goog.i18n.bidi.isRtlLanguage(languageCode) ? 'rtl' : 'ltr');
};
});  // goog.scope
