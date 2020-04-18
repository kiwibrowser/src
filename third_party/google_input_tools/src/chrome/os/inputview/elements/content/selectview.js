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
goog.provide('i18n.input.chrome.inputview.elements.content.SelectView');

goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');


goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;
var ElementType = i18n.input.chrome.ElementType;
var TagName = goog.dom.TagName;



/**
 * The view for triggering select mode.
 *
 * @param {goog.events.EventTarget=} opt_eventTarget The parent event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.SelectView = function(
    opt_eventTarget) {
  goog.base(this, '', ElementType.SELECT_VIEW, opt_eventTarget);

  /**
   * Whether swipe selection is enabled by the settings page.
   * @private {boolean}
   */
  this.enabled_ = false;

  /**
   * Whether the current keyset supports swipe selection.
   *
   * @private {boolean}
   */
  this.isKeysetSupported_ = false;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.SelectView,
    i18n.input.chrome.inputview.elements.Element);
var SelectView = i18n.input.chrome.inputview.elements.content.SelectView;


/**
 * The window that shows the left knob.
 *
 * @private {!Element}
 */
SelectView.prototype.left_;


/**
 * The window that shows the right knob.
 *
 * @private {!Element}
 */
SelectView.prototype.right_;


/**
 * The distance between finger to track view which will cancel the track
 * view.
 *
 * @private {number}
 */
SelectView.WIDTH_ = 37;


/**
 * Width when in portrait mode.
 *
 * @private {number}
 */
SelectView.PORTRAIT_WIDTH_ = 15;


/** @override */
SelectView.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var dom = this.getDomHelper();
  var elem = this.getElement();
  var knob = dom.createDom(TagName.DIV);
  var knobContainer = dom.createDom(TagName.DIV, undefined, knob);
  this.left_ = dom.createDom(TagName.DIV, Css.SELECT_KNOB_LEFT, knobContainer);
  dom.appendChild(this.getElement(), this.left_);

  knob = dom.createDom(TagName.DIV);
  knobContainer = dom.createDom(TagName.DIV, undefined, knob);
  this.right_ =
      dom.createDom(TagName.DIV, Css.SELECT_KNOB_RIGHT, knobContainer);
  dom.appendChild(this.getElement(), this.right_);
};


/** @override */
SelectView.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);
  var isLandscape = screen.width > screen.height;
  var affordanceWidth = isLandscape ?
      SelectView.WIDTH_ : SelectView.PORTRAIT_WIDTH_;
  if (isLandscape) {
    goog.dom.classlist.addRemove(this.getElement(),
        Css.PORTRAIT, Css.LANDSCAPE);
  } else {
    goog.dom.classlist.addRemove(this.getElement(),
        Css.LANDSCAPE, Css.PORTRAIT);
  }
  this.left_ && goog.style.setStyle(this.left_, {
    'height': height + 'px',
    'width': affordanceWidth + 'px'
  });
  this.right_ && goog.style.setStyle(this.right_, {
    'height': height + 'px',
    'width': affordanceWidth + 'px',
    'left': width - affordanceWidth + 'px'
  });
};


/**
 * Sets whether swipe selection is enabled by the settings page.
 *
 * @param {boolean} enabled .
 */
SelectView.prototype.setSettingsEnabled = function(enabled) {
  this.enabled_ = enabled;
  this.update_();
};


/**
 * Sets whether the current keyset supports swipe selection.
 *
 * @param {boolean} supported .
 */
SelectView.prototype.setKeysetSupported = function(supported) {
  this.isKeysetSupported_ = supported;
  this.update_();
};


/**
 * Update elements visibility.
 *
 * @private
 */
SelectView.prototype.update_ = function() {
  this.setVisible(this.enabled_ && this.isKeysetSupported_);
};


/** @override */
SelectView.prototype.disposeInternal = function() {
  goog.dispose(this.left_);
  goog.dispose(this.right_);

  goog.base(this, 'disposeInternal');
};


/** @override */
SelectView.prototype.setVisible = function(visible) {
  SelectView.base(this, 'setVisible', visible);
  if (this.left_) {
    goog.style.setElementShown(this.left_, visible);
  }
  if (this.right_) {
    goog.style.setElementShown(this.right_, visible);
  }
};
});  // goog.scope
