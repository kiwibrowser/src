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
goog.provide('i18n.input.chrome.inputview.elements.content.CandidateView');

goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.object');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.content.Candidate');
goog.require('i18n.input.chrome.inputview.elements.content.CandidateButton');
goog.require('i18n.input.chrome.inputview.elements.content.DragButton');
goog.require('i18n.input.chrome.inputview.elements.content.ToolbarButton');
goog.require('i18n.input.chrome.inputview.util');
goog.require('i18n.input.chrome.message.Name');



goog.scope(function() {
var Css = i18n.input.chrome.inputview.Css;
var TagName = goog.dom.TagName;
var Candidate = i18n.input.chrome.inputview.elements.content.Candidate;
var Type = i18n.input.chrome.inputview.elements.content.Candidate.Type;
var ElementType = i18n.input.chrome.ElementType;
var content = i18n.input.chrome.inputview.elements.content;
var Name = i18n.input.chrome.message.Name;
var util = i18n.input.chrome.inputview.util;



/**
 * The candidate view.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.inputview.Adapter} adapter .
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.CandidateView = function(id,
    adapter, opt_eventTarget) {
  goog.base(this, id, ElementType.CANDIDATE_VIEW, opt_eventTarget);

  /**
   * The bus channel to communicate with background.
   *
   * @private {!i18n.input.chrome.inputview.Adapter}
   */
  this.adapter_ = adapter;

  /**
   * The icons.
   *
   * @private {!Array.<!i18n.input.chrome.inputview.elements.Element>}
   */
  this.iconButtons_ = [];

  this.iconButtons_[IconType.BACK] = new content.CandidateButton(
      '', ElementType.BACK_BUTTON, '',
      chrome.i18n.getMessage('HANDWRITING_BACK'), this);
  this.iconButtons_[IconType.SHRINK_CANDIDATES] = new content.
      CandidateButton('', ElementType.SHRINK_CANDIDATES,
          Css.SHRINK_CANDIDATES_ICON, '', this);
  this.iconButtons_[IconType.EXPAND_CANDIDATES] = new content.
      CandidateButton('', ElementType.EXPAND_CANDIDATES,
          Css.EXPAND_CANDIDATES_ICON, '', this);
  this.iconButtons_[IconType.VOICE] = new content.CandidateButton('',
      ElementType.VOICE_BTN, Css.VOICE_MIC_BAR, '', this, true);

  /**
   * The floating virtual keyboard buttons..
   *
   * @private {!Array.<!i18n.input.chrome.inputview.elements.Element>}
   */
  this.fvkButtons_ = [];

  this.fvkButtons_.push(new content.
      DragButton('', ElementType.DRAG, Css.DRAG_BUTTON, this));

  /**
   * Toolbar buttons.
   *
   * @private {!Array.<!i18n.input.chrome.inputview.elements.Element>}
   */
  this.toolbarButtons_ = [];

  this.toolbarButtons_.push(new content.
      ToolbarButton('', ElementType.UNDO, Css.UNDO_ICON, '', this, true));
  this.toolbarButtons_.push(new content.
      ToolbarButton('', ElementType.REDO, Css.REDO_ICON, '', this, true));
  this.toolbarButtons_.push(new content.
      ToolbarButton('', ElementType.BOLD, Css.BOLD_ICON, '', this, true));
  this.toolbarButtons_.push(new content.
      ToolbarButton('', ElementType.ITALICS, Css.ITALICS_ICON, '', this, true));
  this.toolbarButtons_.push(new content.ToolbarButton(
      '', ElementType.UNDERLINE, Css.UNDERLINE_ICON, '', this, true));
  this.toolbarButtons_.push(new content.
      ToolbarButton('', ElementType.CUT, Css.CUT_ICON, '', this));
  this.toolbarButtons_.push(new content.
      ToolbarButton('', ElementType.COPY, Css.COPY_ICON, '', this));
  this.toolbarButtons_.push(new content.
      ToolbarButton('', ElementType.PASTE, Css.PASTE_ICON, '', this));
  this.toolbarButtons_.push(new content.
      ToolbarButton('', ElementType.SELECT_ALL, Css.SELECT_ALL_ICON, '', this));
};
goog.inherits(i18n.input.chrome.inputview.elements.content.CandidateView,
    i18n.input.chrome.inputview.elements.Element);
var CandidateView = i18n.input.chrome.inputview.elements.content.CandidateView;


/**
 * The icon type at the right of the candidate view.
 *
 * @enum {number}
 */
CandidateView.IconType = {
  BACK: 0,
  SHRINK_CANDIDATES: 1,
  EXPAND_CANDIDATES: 2,
  VOICE: 3
};
var IconType = CandidateView.IconType;


/**
 * The current use of the candidate view.
 *
 * @enum {number}
 */
CandidateView.CandidateViewType = {
  NONE: 0,
  CANDIDATES: 1,
  NUMBER_ROW: 2,
  TOOLTIP: 3
};
var CandidateViewType = CandidateView.CandidateViewType;


/**
 * The padding between candidates.
 *
 * @const {number}
 * @private
 */
CandidateView.PADDING_ = 50;


/**
 * The width for a candidate when showing in THREE_CANDIDATE mode.
 *
 * @const {number}
 * @private
 */
CandidateView.WIDTH_FOR_THREE_CANDIDATES_ = 200;


/**
 * The width for a candidate when showing in small resolution virtual keyboard.
 *
 * @const {number}
 * @private
 */
CandidateView.SMALL_WIDTH_FOR_THREE_CANDIDATES_ = 75;


/**
 * The width of icons in the toolbar.
 *
 * @const {number}
 * @private
 */
CandidateView.TOOLBAR_ICON_WIDTH_ = 40;


/**
 * The handwriting keyset code.
 *
 * @const {string}
 * @private
 */
CandidateView.HANDWRITING_VIEW_CODE_ = 'hwt';


/**
 * The emoji keyset code.
 *
 * @const {string}
 * @private
 */
CandidateView.EMOJI_VIEW_CODE_ = 'emoji';


/**
 * How many candidates in this view.
 *
 * @type {number}
 */
CandidateView.prototype.candidateCount = 0;


/**
 * The width in weight which stands for the entire row. It is used for the
 * alignment of the number row.
 *
 * @private {number}
 */
CandidateView.prototype.widthInWeight_ = 0;


/**
 * The width in weight of the backspace key.
 *
 * @private {number}
 */
CandidateView.prototype.backspaceWeight_ = 0;


/**
 * The width of an icon e.g voice/down/up arrow.
 *
 * @private {number}
 */
CandidateView.prototype.iconWidth_ = 120;


/**
 * The current candidate view type.
 *
 * @private {CandidateViewType}
 */
CandidateView.prototype.candidateViewType_ = CandidateViewType.NONE;


/**
 * True if trying to show the toolbar row.
 *
 * @type {boolean}
 */
CandidateView.prototype.tryShowingToolbar = false;


/** @private {string} */
CandidateView.prototype.keyset_ = '';


/** @private {boolean} */
CandidateView.prototype.isPasswordBox_ = false;


/** @private {boolean} */
CandidateView.prototype.isRTL_ = false;


/**
 * The width of the inter container.
 *
 * @private {number}
 */
CandidateView.prototype.interContainerWidth_ = 0;


/** @private {boolean} */
CandidateView.prototype.navigation_ = false;


/** @private {number} */
CandidateView.prototype.sumOfCandidates_ = 0;


/** @override */
CandidateView.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var dom = this.getDomHelper();
  var elem = this.getElement();
  goog.dom.classlist.add(elem, Css.CANDIDATE_VIEW);

  for (var i = 0; i < this.toolbarButtons_.length; i++) {
    var button = this.toolbarButtons_[i];
    button.render(elem);
    button.setVisible(false);
  }

  for (var i = 0; i < this.fvkButtons_.length; i++) {
    var button = this.fvkButtons_[i];
    button.render(elem);
    button.setVisible(false);
  }

  this.interContainer_ = dom.createDom(TagName.DIV,
      Css.CANDIDATE_INTER_CONTAINER);
  dom.appendChild(elem, this.interContainer_);

  for (var i = 0; i < this.iconButtons_.length; i++) {
    var button = this.iconButtons_[i];
    button.render(elem);
    button.setVisible(false);
    if (button.type == ElementType.VOICE_BTN) {
      goog.dom.classlist.add(button.getElement(), Css.VOICE_BUTTON);
    }
  }

  goog.a11y.aria.setState(/** @type {!Element} */
      (this.iconButtons_[IconType.SHRINK_CANDIDATES].getElement()),
      goog.a11y.aria.State.LABEL,
      chrome.i18n.getMessage('SHRINK_CANDIDATES'));

  goog.a11y.aria.setState(/** @type {!Element} */
      (this.iconButtons_[IconType.EXPAND_CANDIDATES].getElement()),
      goog.a11y.aria.State.LABEL,
      chrome.i18n.getMessage('EXPAND_CANDIDATES'));
};


/**
 * Hides the number row.
 */
CandidateView.prototype.hideNumberRow = function() {
  if (this.candidateViewType_ == CandidateViewType.NUMBER_ROW) {
    this.candidateViewType_ = CandidateViewType.NONE;
    this.getDomHelper().removeChildren(this.interContainer_);
  }
};


/**
 * Shows the number row.
 */
CandidateView.prototype.showNumberRow = function() {
  this.candidateViewType_ = CandidateViewType.NUMBER_ROW;
  goog.dom.classlist.remove(this.getElement(),
      i18n.input.chrome.inputview.Css.THREE_CANDIDATES);
  var dom = this.getDomHelper();
  dom.removeChildren(this.interContainer_);
  var weightArray = [];
  for (var i = 0; i < 10; i++) {
    weightArray.push(1);
  }
  weightArray.push(this.widthInWeight_ - 10);
  var values = util.splitValue(weightArray, this.width);
  for (var i = 0; i < 10; i++) {
    var candidateElem = new Candidate(String(i), goog.object.create(
        Name.CANDIDATE, String((i + 1) % 10)),
        Type.NUMBER, this.height, false, values[i], this);
    candidateElem.render(this.interContainer_);
  }
};


/**
 * Shows a tooltop in place of candidates.
 *
 * @param {string=} opt_text The text to display.
 */
CandidateView.prototype.showTooltip = function(opt_text) {
  this.clearCandidates();
  this.candidateViewType_ = CandidateViewType.TOOLTIP;
  goog.dom.classlist.remove(this.getElement(),
      i18n.input.chrome.inputview.Css.THREE_CANDIDATES);
  var candidateElem = new Candidate('tooltip', goog.object.create(
      Name.CANDIDATE, opt_text || ''),
      Type.TOOLTIP, this.height, false, this.width, this);
  candidateElem.render(this.interContainer_);
  this.switchToIcon(IconType.VOICE, false);
};


/**
 * Hides the tooltip.
 */
CandidateView.prototype.hideTooltip = function() {
  if (this.candidateViewType_ == CandidateViewType.TOOLTIP) {
    this.candidateViewType_ = CandidateViewType.NONE;
    this.getDomHelper().removeChildren(this.interContainer_);
    // Restore previous settings.
    this.updateByKeyset(this.keyset_, this.isPasswordBox_, this.isRTL_);
  }
};


/**
 * Shows the candidates.
 *
 * @param {!Array.<!Object>} candidates The candidate list.
 * @param {boolean} showThreeCandidates .
 * @param {boolean=} opt_expandable True if the candidates would be shown
 *     in expanded view.
 */
CandidateView.prototype.showCandidates = function(candidates,
    showThreeCandidates, opt_expandable) {
  this.clearCandidates();
  this.sumOfCandidates_ = candidates.length;
  if (candidates.length > 0) {
    this.candidateViewType_ = CandidateViewType.CANDIDATES;
    if (showThreeCandidates) {
      this.addThreeCandidates_(candidates);
    } else {
      this.addFullCandidates_(candidates);
      if (!this.iconButtons_[IconType.BACK].isVisible()) {
        this.switchToIcon(IconType.EXPAND_CANDIDATES,
            !!opt_expandable && this.candidateCount < candidates.length);
      }
    }
  }
};


/**
 * Adds the candidates in THREE-CANDIDATE mode.
 *
 * @param {!Array.<!Object>} candidates The candidate list.
 * @private
 */
CandidateView.prototype.addThreeCandidates_ = function(candidates) {
  goog.dom.classlist.add(this.getElement(),
      i18n.input.chrome.inputview.Css.THREE_CANDIDATES);
  this.interContainer_.style.width = 'auto';
  var num = Math.min(3, candidates.length);
  var width = CandidateView.WIDTH_FOR_THREE_CANDIDATES_;
  if (this.adapter_.isFloatingVirtualKeyboardEnabled()) {
    //TODO: large size floating virtual keyboard may still use the regular
    //width. Add an enum to distinguish small size from large and middle size
    //for floating virtual keyboard.
    width = CandidateView.SMALL_WIDTH_FOR_THREE_CANDIDATES_;
  }
  if (this.tryShowingToolbar && this.hasEnoughSpaceForToolbar_()) {
    width -= this.iconWidth_ / 3;
  }
  for (var i = 0; i < num; i++) {
    var candidateElem = new Candidate(String(i), candidates[i], Type.CANDIDATE,
        this.height, i == 1 || num == 1, width, this);
    candidateElem.render(this.interContainer_);
  }
  this.candidateCount = num;
};


/**
 * Clears the candidates.
 */
CandidateView.prototype.clearCandidates = function() {
  this.sumOfCandidates_ = 0;
  if (this.candidateViewType_ == CandidateViewType.CANDIDATES) {
    this.candidateViewType_ = CandidateViewType.NONE;
    this.candidateCount = 0;
    this.getDomHelper().removeChildren(this.interContainer_);
  }
};


/**
 * Adds candidates into the view, as many as the candidate bar can support.
 *
 * @param {!Array.<!Object>} candidates The candidate list.
 * @private
 */
CandidateView.prototype.addFullCandidates_ = function(candidates) {
  goog.dom.classlist.remove(this.getElement(),
      i18n.input.chrome.inputview.Css.THREE_CANDIDATES);
  var totalWidth = Math.floor(this.width - this.iconWidth_);
  if (this.tryShowingToolbar && this.hasEnoughSpaceForToolbar_()) {
    totalWidth -=
        (CandidateView.TOOLBAR_ICON_WIDTH_ * this.toolbarButtons_.length);
  }
  var w = 0;
  var i;
  for (i = 0; i < candidates.length; i++) {
    var candidateElem = new Candidate(String(i), candidates[i], Type.CANDIDATE,
        this.height, false, undefined, this);
    candidateElem.render(this.interContainer_);
    var size = goog.style.getSize(candidateElem.getElement());
    var candidateWidth = size.width + CandidateView.PADDING_ * 2;
    // 1px is the width of the separator.
    w += candidateWidth + 1;

    if (w >= totalWidth) {
      if (i == 0) {
        // Make sure have one at least.
        candidateElem.setSize(totalWidth);
        ++i;
      } else {
        this.interContainer_.removeChild(candidateElem.getElement());
      }
      break;
    }
    candidateElem.setSize(candidateWidth);
  }
  this.candidateCount = i;
};


/**
 * Sets the widthInWeight which equals to a total line in the
 * keyset view and it is used for alignment of number row.
 *
 * @param {number} widthInWeight .
 * @param {number} backspaceWeight .
 */
CandidateView.prototype.setWidthInWeight = function(widthInWeight,
    backspaceWeight) {
  this.widthInWeight_ = widthInWeight;
  this.backspaceWeight_ = backspaceWeight;
};


/** @override */
CandidateView.prototype.resize = function(width, height) {
  goog.style.setSize(this.getElement(), width, height);

  var remainingWidth = width;
  if (this.backspaceWeight_ > 0) {
    var weightArray = [Math.round(this.widthInWeight_ - this.backspaceWeight_)];
    weightArray.push(this.backspaceWeight_);
    var values = util.splitValue(weightArray, width);
    this.iconWidth_ = values[values.length - 1];
  }
  for (var i = 0; i < this.iconButtons_.length; i++) {
    var button = this.iconButtons_[i];
    button.resize(this.iconWidth_, height);
  }
  // At most one icon in iconButtons can be visible and the space must be
  // reserved.
  remainingWidth = width - this.iconWidth_;


  for (var i = 0; i < this.fvkButtons_.length; i++) {
    var button = this.fvkButtons_[i];
    if (button.isVisible()) {
      // Uses square buttons
      button.resize(height, height);
      remainingWidth -= height;
    }
  }

  if (this.tryShowingToolbar) {
    if (this.hasEnoughSpaceForToolbar_()) {
      for (var i = 0; i < this.toolbarButtons_.length; i++) {
        var button = this.toolbarButtons_[i];
        button.resize(CandidateView.TOOLBAR_ICON_WIDTH_, height);
        // Button may be invisible previously due to not enough space.
        button.setVisible(true);
        remainingWidth -= CandidateView.TOOLBAR_ICON_WIDTH_;
      }
    } else {
      for (var i = 0; i < this.toolbarButtons_.length; i++) {
        this.toolbarButtons_[i].setVisible(false);
      }
    }
  }

  // Only show candidates which could fit into the remaining width.
  if (this.candidateCount > 0) {
    var w = 0;
    for (i = 0; i < this.candidateCount; i++) {
      if (w <= remainingWidth) {
        w += goog.style.getSize(this.interContainer_.children[i]).width;
      }
      goog.style.setElementShown(this.interContainer_.children[i],
          w <= remainingWidth);
    }
  }
  // Three candidates has width: auto.
  if (!goog.dom.classlist.contains(this.getElement(),
      i18n.input.chrome.inputview.Css.THREE_CANDIDATES)) {
    goog.style.setSize(this.interContainer_, remainingWidth, height);
  }

  goog.base(this, 'resize', width, height);

  if (this.candidateViewType_ == CandidateViewType.NUMBER_ROW) {
    this.showNumberRow();
  }
};


/**
 * Switches to the icon, or hide it.
 *
 * @param {number} type .
 * @param {boolean} visible The visibility of back button.
 */
CandidateView.prototype.switchToIcon = function(type, visible) {
  if (visible) {
    for (var i = 0; i < this.iconButtons_.length; i++) {
      if (type != IconType.VOICE) {
        this.iconButtons_[i].setVisible(i == type);
      } else {
        this.iconButtons_[i].setVisible(i == type &&
            this.needToShowVoiceIcon_());
      }
    }
  } else {
    this.iconButtons_[type].setVisible(false);
    // When some icon turn to invisible, need to show voice icon.
    if (type != IconType.VOICE && this.needToShowVoiceIcon_()) {
      this.iconButtons_[IconType.VOICE].setVisible(true);
    }
  }
};


/**
 * Changes the visibility of the floating virtual keyboard related buttons.
 *
 * @param {boolean} visible The drag button visibility.
 */
CandidateView.prototype.setFloatingVKButtonsVisible = function(visible) {
  for (var i = 0; i < this.fvkButtons_.length; i++) {
    this.fvkButtons_[i].setVisible(visible);
  }
};


/**
 * Changes the visibility of the toolbar and it's icons.
 *
 * @param {boolean} visible The target visibility.
 */
CandidateView.prototype.setToolbarVisible = function(visible) {
  this.tryShowingToolbar = visible;
  if (this.hasEnoughSpaceForToolbar_()) {
    for (var i = 0; i < this.toolbarButtons_.length; i++) {
      this.toolbarButtons_[i].setVisible(visible);
    }
  }
};


/**
 * Whether there is enough space to show toolbar in this view.
 *
 * @return {boolean}
 * @private
 */
CandidateView.prototype.hasEnoughSpaceForToolbar_ = function() {
  var toolbarSpace =
      CandidateView.TOOLBAR_ICON_WIDTH_ * this.toolbarButtons_.length;
  // Reserve space to display at least 3 candidates
  var candidatesSpace = CandidateView.WIDTH_FOR_THREE_CANDIDATES_ * 3;
  if (this.adapter_.isFloatingVirtualKeyboardEnabled()) {
    //TODO: large size floating virtual keyboard may still use the regular
    //width. Add an enum to distinguish small size from large and middle size
    //for floating virtual keyboard.
    candidatesSpace = CandidateView.SMALL_WIDTH_FOR_THREE_CANDIDATES_ * 3;
  }
  return toolbarSpace + candidatesSpace < this.width;
};


/**
 * Updates the candidate view by key set changing. Whether to show voice icon
 * or not.
 *
 * @param {string} keyset .
 * @param {boolean} isPasswordBox .
 * @param {boolean} isRTL .
 */
CandidateView.prototype.updateByKeyset = function(
    keyset, isPasswordBox, isRTL) {
  this.keyset_ = keyset;
  this.isPasswordBox_ = isPasswordBox;
  this.isRTL_ = isRTL;
  if (keyset == CandidateView.HANDWRITING_VIEW_CODE_ ||
      keyset == CandidateView.EMOJI_VIEW_CODE_) {
    // Handwriting and emoji keyset do not allow to show voice icon.
    this.switchToIcon(IconType.VOICE, false);
  } else {
    this.switchToIcon(IconType.VOICE, this.needToShowVoiceIcon_());
  }

  if (isPasswordBox && (keyset.indexOf('compact') != -1 &&
      keyset.indexOf('compact.symbol') == -1)) {
    this.showNumberRow();
  } else {
    this.hideNumberRow();
  }
  this.interContainer_.style.direction = isRTL ? 'rtl' : 'ltr';
};


/** @override */
CandidateView.prototype.disposeInternal = function() {
  goog.disposeAll(this.toolbarButtons_);
  goog.disposeAll(this.iconButtons_);

  goog.base(this, 'disposeInternal');
};


/**
 * Whether need to show voice icon on candidate view bar.
 *
 * @return {boolean}
 * @private
 */
CandidateView.prototype.needToShowVoiceIcon_ = function() {
  return this.adapter_.isVoiceInputEnabled &&
      this.adapter_.contextType != 'password' &&
      this.keyset_ != CandidateView.HANDWRITING_VIEW_CODE_ &&
      this.keyset_ != CandidateView.EMOJI_VIEW_CODE_ &&
      this.candidateViewType_ != CandidateViewType.TOOLTIP &&
      (!this.navigation_ || this.candidateCount == this.sumOfCandidates_);
};


/**
 * Sets the navigation value.
 *
 * @param {boolean} navigation .
 */
CandidateView.prototype.setNavigation = function(navigation) {
  this.navigation_ = navigation;
};
});  // goog.scope
