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
goog.provide('i18n.input.chrome.inputview.elements.content.CandidateButton');

goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');



goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;
var Css = i18n.input.chrome.inputview.Css;



/**
 * The icon button in the candidate view.
 *
 * @param {string} id .
 * @param {ElementType} type .
 * @param {string} iconCss .
 * @param {string} text .
 * @param {!goog.events.EventTarget=} opt_eventTarget .
 * @param {boolean=} opt_noSeparator .
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.CandidateButton = function(
    id, type, iconCss, text, opt_eventTarget, opt_noSeparator) {
  goog.base(this, id, type, opt_eventTarget);

  /** @type {string} */
  this.text = text;

  /** @type {string} */
  this.iconCss = iconCss;

  /** @private {boolean} */
  this.hasSeperator_ = !opt_noSeparator;
};
var CandidateButton = i18n.input.chrome.inputview.elements.content.
    CandidateButton;
goog.inherits(CandidateButton, i18n.input.chrome.inputview.elements.Element);


/** @type {!Element} */
CandidateButton.prototype.iconCell;


/** @type {!Element} */
CandidateButton.prototype.separatorCell;


/** @override */
CandidateButton.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var dom = this.getDomHelper();
  var elem = this.getElement();
  goog.dom.classlist.addAll(elem, [Css.CANDIDATE_INTER_CONTAINER,
    Css.CANDIDATE_BUTTON]);

  if (this.hasSeperator_) {
    this.separatorCell = this.createSeparator_();
  }

  this.iconCell = dom.createDom(goog.dom.TagName.DIV, Css.TABLE_CELL);
  dom.appendChild(elem, this.iconCell);

  var iconElem = dom.createDom(goog.dom.TagName.DIV, Css.INLINE_DIV);
  if (this.iconCss) {
    goog.dom.classlist.add(iconElem, this.iconCss);
  }
  if (this.text) {
    dom.setTextContent(iconElem, this.text);
  }
  dom.appendChild(this.iconCell, iconElem);
};


/**
 * Creates a separator.
 *
 * @return {!Element} The table cell element.
 * @private
 */
CandidateButton.prototype.createSeparator_ = function() {
  var dom = this.getDomHelper();
  var tableCell = dom.createDom(goog.dom.TagName.DIV,
      i18n.input.chrome.inputview.Css.TABLE_CELL);
  var separator = dom.createDom(goog.dom.TagName.DIV,
      i18n.input.chrome.inputview.Css.CANDIDATE_SEPARATOR);
  separator.style.height = '32%';
  dom.appendChild(tableCell, separator);
  dom.appendChild(this.getElement(), tableCell);
  return tableCell;
};


/** @override */
CandidateButton.prototype.resize = function(width, height) {
  if (this.hasSeperator_) {
    goog.style.setSize(this.separatorCell, 1, height);
  }
  goog.style.setSize(this.iconCell, width - 1, height);

  goog.base(this, 'resize', width, height);
};


/** @override */
CandidateButton.prototype.setVisible = function(visible) {
  var ret = CandidateButton.base(this, 'setVisible', visible);
  if (this.type == ElementType.VOICE_BTN) {
    var elem = this.getElement();
    if (!elem) {
      elem.style.webkitAnimation = (visible ? 'visible' : 'invisible') +
          '-animation .4s ease';
    }
  }
  return ret;
};
});  // goog.scope

