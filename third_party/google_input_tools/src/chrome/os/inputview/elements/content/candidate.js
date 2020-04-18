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
goog.provide('i18n.input.chrome.inputview.elements.content.Candidate');

goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.message.Name');

goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;
var Css = i18n.input.chrome.inputview.Css;
var TagName = goog.dom.TagName;
var Name = i18n.input.chrome.message.Name;



/**
 * The candidate component.
 *
 * @param {string} id .
 * @param {!Object} candidate .
 * @param {i18n.input.chrome.inputview.elements.content.Candidate.Type}
 *     candidateType .
 * @param {number} height .
 * @param {boolean} isDefault .
 * @param {number=} opt_width .
 * @param {goog.events.EventTarget=} opt_eventTarget .
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.Candidate = function(id,
    candidate, candidateType, height, isDefault, opt_width,
    opt_eventTarget) {
  goog.base(this, id, ElementType.CANDIDATE, opt_eventTarget);

  /** @type {!Object} */
  this.candidate = candidate;

  /** @type {i18n.input.chrome.inputview.elements.content.Candidate.Type} */
  this.candidateType = candidateType;

  /** @type {number} */
  this.width = opt_width || 0;

  /** @type {number} */
  this.height = height;

  /** @type {boolean} */
  this.isDefault = isDefault;
};
var Candidate = i18n.input.chrome.inputview.elements.content.Candidate;
goog.inherits(Candidate, i18n.input.chrome.inputview.elements.Element);


/**
 * The type of this candidate.
 *
 * @enum {number}
 */
Candidate.Type = {
  CANDIDATE: 0,
  NUMBER: 1,
  TOOLTIP: 2
};


/** @private {Element} */
Candidate.prototype.wrapper_ = null;


/** @override */
Candidate.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var dom = this.getDomHelper();
  var elem = this.getElement();
  goog.dom.classlist.add(elem, Css.CANDIDATE);
  if (this.candidate['isEmoji']) {
    goog.dom.classlist.add(elem, Css.EMOJI_FONT);
  }
  if (this.candidateType == Candidate.Type.NUMBER) {
    goog.dom.classlist.add(elem, Css.CANDIDATE_NUMBER);
  }
  if (this.candidateType == Candidate.Type.TOOLTIP) {
    goog.dom.classlist.add(elem, Css.TOOLTIP);
  }
  this.wrapper_ = dom.createDom('div', {
    'class': Css.CANDIDATE_INTERNAL_WRAPPER
  }, this.candidate[Name.CANDIDATE]);

  dom.appendChild(elem, this.wrapper_);

  this.setSize(this.width, this.height);

  if (this.isDefault) {
    goog.dom.classlist.add(elem, Css.CANDIDATE_DEFAULT);
  }
  if (!!this.candidate[Name.IS_AUTOCORRECT]) {
    goog.dom.classlist.add(elem, Css.CANDIDATE_AUTOCORRECT);
  }
};


/**
 * Sets the candidate size.
 *
 * @param {number=} opt_width .
 * @param {number=} opt_height .
 */
Candidate.prototype.setSize = function(opt_width, opt_height) {
  var elem = this.getElement();
  if (opt_width && opt_width > 0) {
    this.width = opt_width;
    this.wrapper_.style.width = opt_width + 'px';
    elem.style.width = opt_width + 'px';
  }
  if (opt_height && opt_height > 0) {
    this.height = opt_height;
    elem.style.height = opt_height + 'px';
  }
};


/** @override */
Candidate.prototype.setHighlighted = function(highlight) {
  if (highlight) {
    goog.dom.classlist.add(this.getElement(), Css.CANDIDATE_HIGHLIGHT);
  } else {
    goog.dom.classlist.remove(this.getElement(), Css.CANDIDATE_HIGHLIGHT);
  }
};
});  // goog.scope
