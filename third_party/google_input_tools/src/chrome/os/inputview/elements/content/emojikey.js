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
goog.provide('i18n.input.chrome.inputview.elements.content.EmojiKey');
goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.content.SoftKey');

goog.scope(function() {

var EMOJI_KEY_MARGIN = 6;



/**
 * The emoji key
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.ElementType} type The element
 *     type.
 * @param {string} text The text.
 * @param {boolean} isEmoticon Wether it is an emoticon.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.SoftKey}
 */
i18n.input.chrome.inputview.elements.content.EmojiKey = function(id, type,
    text, isEmoticon, opt_eventTarget) {
  i18n.input.chrome.inputview.elements.content.EmojiKey.base(
      this, 'constructor', id, type, opt_eventTarget);
  /**
   * The text in the key.
   *
   * @type {string}
   */
  this.text = text;


  /**
   * Wether it is an emoticon.
   *
   * @type {boolean}
   */
  this.isEmoticon = isEmoticon;

  this.pointerConfig.stopEventPropagation = false;
  this.pointerConfig.dblClick = true;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.EmojiKey,
    i18n.input.chrome.inputview.elements.content.SoftKey);
var EmojiKey = i18n.input.chrome.inputview.elements.content.EmojiKey;


/** @override */
EmojiKey.prototype.createDom = function() {
  goog.base(this, 'createDom');
  var dom = this.getDomHelper();
  var elem = this.getElement();
  if (!this.textElem) {
    this.textElem = dom.createDom(goog.dom.TagName.DIV,
        i18n.input.chrome.inputview.Css.SPECIAL_KEY_NAME, this.text);
  }
  goog.dom.classlist.add(elem, i18n.input.chrome.inputview.Css.EMOJI_KEY);
  elem.style.margin = EMOJI_KEY_MARGIN + 'px';
  this.updateText(this.text, this.isEmoticon);
  dom.appendChild(elem, this.textElem);
  // Sets aria label after UI is ready.
  setTimeout(this.setAriaLable_.bind(this), 0);
};


/** @override */
EmojiKey.prototype.resize = function(width, height) {
  var elem = this.getElement();
  var w = width - EMOJI_KEY_MARGIN * 2;
  var h = height - EMOJI_KEY_MARGIN * 2;
  elem.style.width = w + 'px';
  elem.style.height = h + 'px';
  this.availableWidth = w;
  this.availableHeight = h;
};


/** @override */
EmojiKey.prototype.setHighlighted = function(highlight) {
  if (this.text == '') {
    return;
  }
  goog.base(this, 'setHighlighted', highlight);
};


/** @override */
EmojiKey.prototype.update = function() {
  goog.base(this, 'update');
  goog.dom.setTextContent(this.textElem, this.text);
};


/**
 * Update the emoji's text
 *
 * @param {string} text The new text.
 * @param {boolean} isEmoticon .
 */
EmojiKey.prototype.updateText = function(text, isEmoticon) {
  this.text = text;
  this.isEmoticon = isEmoticon;
  goog.dom.setTextContent(this.textElem, text);
  var elem = this.getElement();
  if (isEmoticon) {
    goog.dom.classlist.add(elem, i18n.input.chrome.inputview.Css.EMOTICON);
  } else {
    goog.dom.classlist.remove(elem, i18n.input.chrome.inputview.Css.EMOTICON);
  }
};


/**
 * Sets the aria label for this emoji key if any.
 *
 * @private
 */
EmojiKey.prototype.setAriaLable_ = function() {
  var emojiName = '';
  var lead = this.text.charCodeAt(0);
  if (this.isEmoticon) {
    emojiName = chrome.i18n.getMessage('smiley');
  } else if (lead) {
    var trail = this.text.charCodeAt(1);
    var msgName = lead.toString(16);
    if (!!trail) {
      msgName += '_' + trail.toString(16);
    }
    emojiName = chrome.i18n.getMessage(msgName.toLowerCase());
  }
  if (emojiName) {
    goog.a11y.aria.setState(/** @type {!Element} */ (this.getElement()),
        goog.a11y.aria.State.LABEL, emojiName);
  }
};
});  // goog.scope
