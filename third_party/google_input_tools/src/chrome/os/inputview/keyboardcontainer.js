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
goog.provide('i18n.input.chrome.inputview.KeyboardContainer');

goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.i18n.bidi');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.WindowUtil');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.GesturePreviewWindow');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.content.AltDataView');
goog.require('i18n.input.chrome.inputview.elements.content.CandidateView');
goog.require('i18n.input.chrome.inputview.elements.content.EmojiView');
goog.require('i18n.input.chrome.inputview.elements.content.ExpandedCandidateView');
goog.require('i18n.input.chrome.inputview.elements.content.FloatingView');
goog.require('i18n.input.chrome.inputview.elements.content.GestureCanvasView');
goog.require('i18n.input.chrome.inputview.elements.content.HandwritingView');
goog.require('i18n.input.chrome.inputview.elements.content.KeysetView');
goog.require('i18n.input.chrome.inputview.elements.content.MenuView');
goog.require('i18n.input.chrome.inputview.elements.content.SelectView');
goog.require('i18n.input.chrome.inputview.elements.content.SwipeView');
goog.require('i18n.input.chrome.inputview.elements.content.VoiceView');



goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;
var EmojiView = i18n.input.chrome.inputview.elements.content.EmojiView;
var GesturePreviewWindow = i18n.input.chrome.inputview.GesturePreviewWindow;
var HandwritingView = i18n.input.chrome.inputview.elements.content.
    HandwritingView;
var KeysetView = i18n.input.chrome.inputview.elements.content.KeysetView;
var content = i18n.input.chrome.inputview.elements.content;
var ElementType = i18n.input.chrome.ElementType;
var WindowUtil = i18n.input.chrome.WindowUtil;



/**
 * The keyboard container.
 *
 * @param {!i18n.input.chrome.inputview.Adapter} adapter .
 * @param {!i18n.input.chrome.sounds.SoundController} soundController .
 * @param {goog.events.EventTarget=} opt_eventTarget The parent event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
// TODO(bshe): Move this file to elements/content
i18n.input.chrome.inputview.KeyboardContainer =
    function(adapter, soundController, opt_eventTarget) {
  goog.base(this, '', ElementType.KEYBOARD_CONTAINER_VIEW, opt_eventTarget);

  /** @type {!content.CandidateView} */
  this.candidateView = new content.CandidateView(
      'candidateView', adapter, this);

  /** @type {!content.AltDataView} */
  this.altDataView = new content.AltDataView(this);

  // Create a new window and construct a GesturePreviewWindow with it
  // once it's ready.
  WindowUtil.createWindow(
      function(newWindow) {
        /** @type {!GesturePreviewWindow} */
        this.gesturePreviewWindow = new GesturePreviewWindow(newWindow);
      }.bind(this));

  /** @type {!content.SwipeView} */
  this.swipeView = new content.SwipeView(adapter, this.candidateView, this);

  /** @type {!content.SelectView} */
  this.selectView = new content.SelectView(this);

  /** @type {!content.MenuView} */
  this.menuView = new content.MenuView(this);

  /** @type {!content.VoiceView} */
  this.voiceView = new content.VoiceView(this, adapter, soundController);

  /** @type {!content.ExpandedCandidateView} */
  this.expandedCandidateView = new content.ExpandedCandidateView(this);

  /**
   * The map of the KeysetViews.
   * Key: keyboard code.
   * Value: The view object.
   *
   * @type {!Object.<string, !KeysetView>}
   */
  this.keysetViewMap = {};

  /**
   * The bus channel to communicate with background.
   *
   * @private {!i18n.input.chrome.inputview.Adapter}
   */
  this.adapter_ = adapter;
};
goog.inherits(i18n.input.chrome.inputview.KeyboardContainer,
    i18n.input.chrome.inputview.elements.Element);
var KeyboardContainer = i18n.input.chrome.inputview.KeyboardContainer;


/** @type {!KeysetView} */
KeyboardContainer.prototype.currentKeysetView;


/**
 * The padding bottom of the whole keyboard.
 *
 * @private {number}
 */
KeyboardContainer.PADDING_BOTTOM_ = 7;


/**
 * The margin of the tab style keyset.
 *
 * @type {number}
 * @private
 */
KeyboardContainer.TAB_MARGIN_ = 11;


/**
 * An div to wrapper candidate view and keyboard set view.
 *
 * @private {Element}
 */
KeyboardContainer.prototype.wrapperDiv_ = null;


/**
 * The gesture canvas view.
 *
 * @type {content.GestureCanvasView}
 */
KeyboardContainer.prototype.gestureCanvasView = null;


/**
 * The gesture canvas view.
 *
 * @type {content.FloatingView}
 */
KeyboardContainer.prototype.floatingView = null;


/** @override */
KeyboardContainer.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var elem = this.getElement();
  var dom = this.getDomHelper();
  this.wrapperDiv_ = dom.createDom(goog.dom.TagName.DIV, Css.WRAPPER);
  this.candidateView.render(this.wrapperDiv_);
  dom.appendChild(elem, this.wrapperDiv_);
  this.altDataView.render();
  this.swipeView.render();
  this.selectView.render();
  this.menuView.render();
  this.voiceView.render();
  this.voiceView.setVisible(false);
  this.expandedCandidateView.render(this.wrapperDiv_);
  this.expandedCandidateView.setVisible(false);
  if (this.adapter_.isFloatingVirtualKeyboardEnabled()) {
    inputview.setMode('FLOATING');
    this.floatingView = new content.FloatingView(this);
    this.floatingView.render();
    this.candidateView.setFloatingVKButtonsVisible(true);
  }
  this.gestureCanvasView = new content.GestureCanvasView(this);
  this.gestureCanvasView.render(this.wrapperDiv_);
  goog.dom.classlist.add(elem, Css.CONTAINER);
};


/** @override */
KeyboardContainer.prototype.update = function() {
  this.currentKeysetView && this.currentKeysetView.update();
};


/**
 * Adds a keyset view.
 *
 * @param {!Object} keysetData .
 * @param {!Object} layoutData .
 * @param {string} keyset .
 * @param {string} languageCode .
 * @param {!i18n.input.chrome.inputview.Model} model .
 * @param {string} inputToolName .
 * @param {!Object.<string, boolean>} conditions .
 */
KeyboardContainer.prototype.addKeysetView = function(keysetData, layoutData,
    keyset, languageCode, model, inputToolName, conditions) {
  var view;
  if (keyset == 'emoji') {
    view = new EmojiView(keysetData, layoutData, keyset, languageCode, model,
        inputToolName, this, this.adapter_);
  } else if (keyset == 'hwt') {
    view = new HandwritingView(keysetData, layoutData, keyset, languageCode,
        model, inputToolName, this, this.adapter_);
  } else {
    view = new KeysetView(keysetData, layoutData, keyset, languageCode, model,
        inputToolName, this, this.adapter_);
  }
  view.render(this.wrapperDiv_);
  view.applyConditions(conditions);
  view.setVisible(false);
  this.keysetViewMap[keyset] = view;
};


/**
 * Switches to a keyset.
 *
 * @param {string} keyset .
 * @param {string} title .
 * @param {boolean} isPasswordBox .
 * @param {boolean} isA11yMode .
 * @param {string} rawKeyset The raw keyset id will switch to.
 * @param {string} lastRawkeyset .
 * @param {string} languageCode .
 * @return {boolean} True if switched successfully.
 */
KeyboardContainer.prototype.switchToKeyset = function(keyset, title,
    isPasswordBox, isA11yMode, rawKeyset, lastRawkeyset, languageCode) {
  if (!this.keysetViewMap[keyset]) {
    return false;
  }

  for (var name in this.keysetViewMap) {
    var view = this.keysetViewMap[name];
    if (name == keyset) {
      this.candidateView.setVisible(!view.disableCandidateView);
      // Before setting view visible, activate it first, since activation may
      // change view keys.
      view.activate(rawKeyset);
      view.setVisible(true);
      view.update();
      if (view.spaceKey) {
        view.spaceKey.updateTitle(title, !isPasswordBox &&
            keyset != 'hwt' && keyset != 'emoji');
      }
      if (isA11yMode) {
        goog.dom.classlist.add(this.getElement(), Css.A11Y);
      }
      // If current raw keyset is changed, record it.
      if (lastRawkeyset != rawKeyset) {
        view.fromKeyset = lastRawkeyset;
      }
      if (view instanceof HandwritingView) {
        view.setLanguagecode(languageCode);
      }
      // Deactivate the last keyset view instance.
      if (this.currentKeysetView != view) {
        if (this.currentKeysetView) {
          this.currentKeysetView.deactivate(lastRawkeyset);
        }
        this.currentKeysetView = view;
      }
      this.candidateView.updateByKeyset(rawKeyset, isPasswordBox,
          goog.i18n.bidi.isRtlLanguage(languageCode));
    } else {
      view.setVisible(false);
    }
  }
  var isCompact = keyset.indexOf('compact') >= 0;
  this.selectView.setKeysetSupported(isCompact);
  this.swipeView.setKeysetSupported(isCompact);
  return true;
};


/**
 * Resizes the whole keyboard.
 *
 * @param {number} width .
 * @param {number} height .
 * @param {number} widthPercent .
 * @param {number} candidateViewHeight .
 */
KeyboardContainer.prototype.setContainerSize = function(width, height,
    widthPercent, candidateViewHeight) {
  if (!this.currentKeysetView) {
    return;
  }
  var elem = this.getElement();

  var h = height;
  var wrapperMargin = 0;
  this.wrapperDiv_.style.marginTop = this.wrapperDiv_.style.marginBottom =
      wrapperMargin + 'px';
  h -= KeyboardContainer.PADDING_BOTTOM_;
  elem.style.paddingBottom = KeyboardContainer.PADDING_BOTTOM_ + 'px';

  var padding = Math.round((width - width * widthPercent) / 2);
  var w = width - 2 * padding;

  // Reduce height if candidate view is enabled
  h = this.currentKeysetView.disableCandidateView ? h :
      h - candidateViewHeight;

  var backspaceWeight = this.currentKeysetView.backspaceKey ?
      this.currentKeysetView.backspaceKey.getParent().getWidthInWeight() : 0;
  this.candidateView.setWidthInWeight(
      this.currentKeysetView.getWidthInWeight(), backspaceWeight);
  this.candidateView.resize(w, candidateViewHeight);
  this.expandedCandidateView.setWidthInWeight(
      this.currentKeysetView.getWidthInWeight(), backspaceWeight);
  this.expandedCandidateView.resize(w, h);
  var candidateElem = this.candidateView.getElement();
  candidateElem.style.paddingLeft = candidateElem.style.paddingRight =
      padding + 'px';
  this.currentKeysetView.resize(width, h, widthPercent);
  var expandViewElem = this.expandedCandidateView.getElement();
  expandViewElem.style.marginLeft = expandViewElem.style.marginRight =
      padding + 'px';
  if (this.expandedCandidateView.isVisible()) {
    // Closes the expanded candidate view if it's visible.
    // This is to avoid mis-layout issue for the expanded candidate when screen
    // is rotated.
    this.expandedCandidateView.state = content.ExpandedCandidateView.State.NONE;
    this.candidateView.switchToIcon(
        content.CandidateView.IconType.EXPAND_CANDIDATES, true);
    this.expandedCandidateView.setVisible(false);
    this.currentKeysetView.setVisible(true);
  }
  this.altDataView.resize(width, height);
  this.swipeView.resize(width, height);
  this.selectView.resize(width, height);
  this.menuView.resize(width, height);
  this.voiceView.resize(w + padding, height);
  if (this.floatingView) {
    this.floatingView.resize(width, height);
  }
  this.gestureCanvasView.resize(width, height);
};


/** @override */
KeyboardContainer.prototype.disposeInternal = function() {
  goog.dispose(this.candidateView);
  goog.dispose(this.altDataView);
  goog.dispose(this.gesturePreviewWindow);
  goog.dispose(this.swipeView);
  goog.dispose(this.selectView);
  goog.dispose(this.menuView);
  goog.dispose(this.voiceView);
  if (this.floatingView) {
    goog.dispose(this.floatingView);
  }
  goog.dispose(this.gestureCanvasView);
  for (var key in this.keysetViewMap) {
    goog.dispose(this.keysetViewMap[key]);
  }

  goog.base(this, 'disposeInternal');
};


/**
 * Whether there are strokes on canvas.
 *
 * @return {boolean} Whether there are strokes on canvas.
 */
KeyboardContainer.prototype.hasStrokesOnCanvas = function() {
  if (this.currentKeysetView) {
    return this.currentKeysetView.hasStrokesOnCanvas();
  } else {
    return false;
  }
};


/**
 * Cleans the stokes.
 */
KeyboardContainer.prototype.cleanStroke = function() {
  if (this.currentKeysetView) {
    this.currentKeysetView.cleanStroke();
  }
};

});  // goog.scope
