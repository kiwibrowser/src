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
goog.provide('i18n.input.chrome.inputview.elements.content.VoiceView');

goog.require('goog.array');
goog.require('goog.asserts');
goog.require('goog.async.Delay');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.content.SpanElement');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');
goog.require('i18n.input.chrome.sounds.Sounds');


goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;
var ElementType = i18n.input.chrome.ElementType;
var FunctionalKey = i18n.input.chrome.inputview.elements.content.FunctionalKey;
var Name = i18n.input.chrome.message.Name;
var Sounds = i18n.input.chrome.sounds.Sounds;
var SpanElement = i18n.input.chrome.inputview.elements.content.SpanElement;
var TagName = goog.dom.TagName;
var Type = i18n.input.chrome.message.Type;



/**
 * The voice input view.
 *
 * @param {goog.events.EventTarget=} opt_eventTarget The parent event target.
 * @param {i18n.input.chrome.inputview.Adapter=} opt_adapter .
 * @param {i18n.input.chrome.sounds.SoundController=} opt_soundController .
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.VoiceView =
    function(opt_eventTarget, opt_adapter, opt_soundController) {
  VoiceView.base(this, 'constructor', '', ElementType.VOICE_VIEW,
      opt_eventTarget);

  /**
   * The bus channel to communicate with background.
   *
   * @private {!i18n.input.chrome.inputview.Adapter}
   */
  this.adapter_ = goog.asserts.assertObject(opt_adapter);

  /**
   * The sound controller.
   *
   * @private {!i18n.input.chrome.sounds.SoundController}
   */
  this.soundController_ = goog.asserts.assertObject(opt_soundController);

  /** @private {!goog.async.Delay} */
  this.animator_ = new goog.async.Delay(this.animateMicrophoneLevel_, 0, this);
};
var VoiceView = i18n.input.chrome.inputview.elements.content.VoiceView;
goog.inherits(VoiceView, i18n.input.chrome.inputview.elements.Element);


/** @private {number} */
VoiceView.WIDTH_ = 150;


/** @private {boolean} */
VoiceView.prototype.isPrivacyAllowed_ = false;


/** @private {Element} */
VoiceView.prototype.maskElem_ = null;


/** @private {Element} */
VoiceView.prototype.voiceMicElem_ = null;


/** @private {Element} */
VoiceView.prototype.levelElement_ = null;


/** @private {Element} */
VoiceView.prototype.voicePanel_ = null;


/**
 * The div to show privacy information message.
 *
 * @type {!Element}
 * @private
 */
VoiceView.prototype.privacyDiv_;


/** @private {boolean} */
VoiceView.prototype.visible_ = false;


/** @override */
VoiceView.prototype.createDom = function() {
  goog.base(this, 'createDom');
  var dom = this.getDomHelper();
  var elem = this.getElement();
  goog.dom.classlist.add(elem, Css.VOICE_VIEW);
  this.voicePanel_ = dom.createDom(TagName.DIV, Css.VOICE_PANEL);
  this.voiceMicElem_ = dom.createDom(TagName.DIV,
      Css.VOICE_OPACITY + ' ' + Css.VOICE_MIC_ING);

  this.levelElement_ = dom.createDom(
      TagName.DIV, Css.VOICE_LEVEL);
  dom.append(/** @type {!Node} */ (this.voicePanel_),
      this.voiceMicElem_, this.levelElement_);

  this.maskElem_ = dom.createDom(TagName.DIV,
      [Css.VOICE_MASK, Css.VOICE_OPACITY_NONE]);
  dom.append(/** @type {!Node} */ (elem), this.maskElem_, this.voicePanel_);

  this.privacyDiv_ = dom.createDom(TagName.DIV,
      Css.VOICE_PRIVACY_INFO);

  var textDiv = dom.createDom(TagName.SPAN);
  dom.setTextContent(textDiv,
      chrome.i18n.getMessage('VOICE_PRIVACY_INFO'));
  dom.appendChild(this.privacyDiv_, textDiv);
  var spanView = new SpanElement('', ElementType.VOICE_PRIVACY_GOT_IT);
  spanView.render(this.privacyDiv_);
  var spanElement = spanView.getElement();
  goog.dom.classlist.add(spanElement, Css.VOICE_GOT_IT);
  dom.setTextContent(spanElement, chrome.i18n.getMessage('GOT_IT'));
  dom.appendChild(elem, this.privacyDiv_);

  // Shows or hides the privacy information.
  this.isPrivacyAllowed_ = !!localStorage.getItem(Name.VOICE_PRIVACY_INFO);
  if (this.isPrivacyAllowed_) {
    goog.dom.classlist.add(this.privacyDiv_,
        Css.HANDWRITING_PRIVACY_INFO_HIDDEN);
    goog.dom.classlist.remove(this.maskElem_, Css.VOICE_OPACITY_NONE);
  }
};


/** @override */
VoiceView.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');
  this.getHandler().listen(this.adapter_, Type.VOICE_PRIVACY_GOT_IT,
      this.onConfirmPrivacyInfo_);
};


/**
 * Start recognition.
 */
VoiceView.prototype.start = function() {
  // visible -> invisible
  if (!this.isVisible()) {
    this.soundController_.playSound(Sounds.VOICE_RECOG_START, true);
  }
  if (this.isPrivacyAllowed_) {
    this.adapter_.sendVoiceViewStateChange(true);
    this.animator_.start(600);
  }
  this.setVisible(true);
};


/**
 * Stop recognition.
 */
VoiceView.prototype.stop = function() {
  // invisible -> visible
  if (this.isVisible()) {
    this.soundController_.playSound(Sounds.VOICE_RECOG_END, true);
  }
  this.animator_.stop();
  this.setVisible(false);
};


/** @override */
VoiceView.prototype.setVisible = function(visible) {
  VoiceView.base(this, 'setVisible', visible);
  this.visible_ = visible;
  var elem = this.getElement();
  goog.style.setElementShown(elem, true);
  elem.style.visibility = visible ? 'visible' : 'hidden';
  elem.style.transition = visible ? '' : 'visibility 0.1s ease 0.4s';
  if (visible) {
    this.voicePanel_.style.transform = 'scale(1)';
    this.voicePanel_.style.transition = 'transform 0.4s ease';
    goog.dom.classlist.add(this.maskElem_, Css.VOICE_MASK_OPACITY);
    goog.style.setElementShown(this.privacyDiv_, true);
  } else {
    goog.dom.classlist.remove(this.maskElem_, Css.VOICE_MASK_OPACITY);
    this.voicePanel_.style.transform = 'scale(0)';
    this.voicePanel_.style.transition = 'transform 0.4s ease';
    this.levelElement_.style.transform = 'scale(0)';
    goog.style.setElementShown(this.privacyDiv_, false);
  }
  var enterKeys = this.getDomHelper().getElementsByClass(Css.ENTER_ICON);
  var stylestr = 'grayscale(' + (visible ? 1 : 0) + ')';
  goog.array.forEach(enterKeys, function(key) {
    key.style.webkitFilter = stylestr;
  });
  this.resize(this.width, this.height);
};


/** @override */
VoiceView.prototype.resize = function(width, height) {
  VoiceView.base(this, 'resize', width, height);
  this.voicePanel_.style.left = (width - VoiceView.WIDTH_) + 'px';

  var elem = this.getElement();
  var size = goog.style.getSize(this.privacyDiv_);
  this.privacyDiv_.style.top = elem.offsetTop +
      Math.round((height - size.height) / 2) + 'px';
  this.privacyDiv_.style.left = elem.offsetLeft +
      Math.round((width - size.width) / 2) + 'px';
};


/**
 * The voice recognition animation.
 *
 * @private
 */
VoiceView.prototype.animateMicrophoneLevel_ = function() {
  var scale = 1 + 1.5 * Math.random();
  var timeStep = Math.round(110 + Math.random() * 10);
  var transitionInterval = timeStep / 1000;

  this.levelElement_.style.transition = 'transform' + ' ' +
      transitionInterval + 's ease-in-out';
  this.levelElement_.style.transform = 'scale(' + scale + ')';
  this.animator_.start(timeStep);
};


/**
 * Handler on user confirming the privacy information.
 *
 * @private
 */
VoiceView.prototype.onConfirmPrivacyInfo_ = function() {
  // Stores the handwriting privacy permission value.
  localStorage.setItem(Name.VOICE_PRIVACY_INFO, 'true');
  this.isPrivacyAllowed_ = true;
  this.adapter_.sendVoiceViewStateChange(true);
  this.animator_.start(200);
  this.soundController_.playSound(Sounds.VOICE_RECOG_START, true);
  goog.dom.classlist.add(this.privacyDiv_, Css.HANDWRITING_PRIVACY_INFO_HIDDEN);
  goog.dom.classlist.remove(this.maskElem_, Css.VOICE_OPACITY_NONE);
};


/** @override */
VoiceView.prototype.isVisible = function() {
  return this.visible_;
};
});  // goog.scope
