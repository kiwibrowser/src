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
goog.provide('i18n.input.chrome.inputview.elements.content.EmojiView');

goog.require('goog.array');
goog.require('goog.dom.classlist');
goog.require('goog.positioning.AnchoredViewportPosition');
goog.require('goog.positioning.Corner');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.elements.content.KeysetView');
goog.require('i18n.input.chrome.inputview.elements.content.PageIndicator');
goog.require('i18n.input.chrome.inputview.events.EventType');
goog.require('i18n.input.chrome.inputview.handler.PointerHandler');


goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;
var EventType = i18n.input.chrome.inputview.events.EventType;
var KeysetView = i18n.input.chrome.inputview.elements.content.KeysetView;
var PointerHandler = i18n.input.chrome.inputview.handler.PointerHandler;
var Css = i18n.input.chrome.inputview.Css;
var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;
var PageIndicator = i18n.input.chrome.inputview.elements.content.PageIndicator;



/**
 * The emoji view.
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
i18n.input.chrome.inputview.elements.content.EmojiView = function(keyData,
    layoutData, keyboardCode, languageCode, model, name, opt_eventTarget,
    opt_adapter) {
  i18n.input.chrome.inputview.elements.content.EmojiView.base(this,
      'constructor', keyData, layoutData, keyboardCode, languageCode, model,
      name, opt_eventTarget, opt_adapter);

  /**
   * The number of keys per emoji page.
   *
   * @private {number}
   */
  this.keysPerPage_ = 27;

  /**
   * The number of tabbar keys.
   *
   * @private {number}
   */
  this.totalTabbars_ = keyData[SpecNodeName.TEXT].length;

  /**
   * The first page offset of each category.
   *
   * @private {!Array.<number>}
   */
  this.pageOffsets_ = [];

  /**
   * The number of pages for each category.
   *
   * @private {!Array.<number>}
   */
  this.pagesInCategory_ = [];

  // Calculate the emojiPageoffset_ and totalPages_ according to keydata.
  var pageNum = 0;
  for (var i = 0, len = keyData[SpecNodeName.TEXT].length; i < len; ++i) {
    this.pageOffsets_.push(pageNum);
    pageNum += Math.ceil(
        keyData[SpecNodeName.TEXT][i].length / this.keysPerPage_);
  }

  /**
   * The category of each emoji page.
   *
   * @private {!Array.<number>}
   */
  this.pageToCategory_ = [];

  // Calculate the pageToCategory_ according to keydata.
  for (var i = 0, len = keyData[SpecNodeName.TEXT].length; i < len; ++i) {
    var lenJ = Math.ceil(
        keyData[SpecNodeName.TEXT][i].length / this.keysPerPage_);
    for (var j = 0; j < lenJ; ++j) {
      this.pageToCategory_.push(i);
    }
    this.pagesInCategory_.push(lenJ);
  }

  /**
   * The list of recent used emoji.
   *
   * @private {Array.<string>}
   */
  this.recentEmojiList_ = [];

  /**
   * The emoji keys on the recent page.
   *
   * @private {Array.<i18n.input.chrome.inputview.elements.content.EmojiKey>}
   */
  this.recentEmojiKeys_ = [];

  /**
   * The tabbars of the emoji view.
   *
   * @private {Array.<i18n.input.chrome.inputview.elements.content.TabBarKey>}
   */
  this.tabbarKeys_ = [];
};
var EmojiView = i18n.input.chrome.inputview.elements.content.EmojiView;
goog.inherits(EmojiView, KeysetView);


/**
 * The emoji rows of the emoji slider.
 *
 * @private {!i18n.input.chrome.inputview.elements.layout.ExtendedLayout}
 */
EmojiView.prototype.emojiRows_;


/**
 * The emoji slider of the emoji slider.
 *
 * @private {!i18n.input.chrome.inputview.elements.layout.VerticalLayout}
 */
EmojiView.prototype.emojiSlider_;


/**
 * The indicator of the emoji page index.
 *
 * @private {!i18n.input.chrome.inputview.elements.content.PageIndicator}
 */
EmojiView.prototype.pageIndicator_;


/**
 * Whether it is a drag event.
 *
 * @type {boolean}
 */
EmojiView.prototype.isDragging = false;


/**
 * The width percent to used inside the emoji panel.
 *
 * @private {number}
 */
EmojiView.prototype.emojiWidthPercent_ = 1;


/**
 * The ID of the selected emoji category.
 *
 * @private {number}
 */
EmojiView.prototype.categoryID_ = 0;


/**
 * The timestamp of the last pointer down event.
 *
 * @private {number}
 */
EmojiView.prototype.pointerDownTimeStamp_ = 0;


/**
 * The drag distance of a drag event.
 *
 * @private {number}
 */
EmojiView.prototype.dragDistance_ = 0;


/**
 * The maximal required time interval for quick emoji page swipe in ms.
 *
 * @private {number}
 */
EmojiView.EMOJI_DRAG_INTERVAL_ = 300;


/**
 * The minimal required drag distance for scrolling emoji page swipe in px.
 *
 * @private {number}
 */
EmojiView.EMOJI_DRAG_SCROLL_DISTANCE_ = 60;


/**
 * The minimal distance in px to determine a drag action.
 *
 * @private {number}
 */
EmojiView.EMOJI_DRAG_START_DISTANCE_ = 10;


/**
 * The default emoji category ID.
 *
 * @private {number}
 */
EmojiView.EMOJI_DEFAULT_CATEGORY_ID_ = 2;


/** @private {!PointerHandler} */
EmojiView.prototype.pointerHandler_;


/** @override */
EmojiView.prototype.createDom = function() {
  goog.base(this, 'createDom');
  this.pointerHandler_ = new PointerHandler();
  this.getHandler().
      listen(this.pointerHandler_, EventType.POINTER_DOWN,
      this.onPointerDown_).
      listen(this.pointerHandler_, EventType.POINTER_UP, this.onPointerUp_).
      listen(this.pointerHandler_, EventType.POINTER_OUT, this.onPointerOut_).
      listen(this.pointerHandler_, EventType.DRAG, this.onDragEvent_);
  this.emojiRows_ =
      /** @type {!i18n.input.chrome.inputview.elements.layout.ExtendedLayout} */
      (this.getChildViewById('emojiRows'));
  this.emojiSlider_ =
      /** @type {!i18n.input.chrome.inputview.elements.layout.VerticalLayout} */
      (this.getChildViewById('emojiSlider'));
  for (var i = 0; i < this.keysPerPage_; i++) {
    this.recentEmojiKeys_.push(
        /** @type {!i18n.input.chrome.inputview.elements.content.EmojiKey} */
        (this.getChildViewById('emojikey' + i)));
  }
  for (var i = 0; i < this.totalTabbars_; i++) {
    this.tabbarKeys_.push(
        /** @type {!i18n.input.chrome.inputview.elements.content.TabBarKey} */
        (this.getChildViewById('Tabbar' + i)));
  }
  this.pageIndicator_ = new PageIndicator(
      'indicator-background', ElementType.PAGE_INDICATOR);
  this.pageIndicator_.render();
};


/** @override */
EmojiView.prototype.enterDocument = function() {
  this.pageIndicator_.setVisible(false);
};


/** @override */
EmojiView.prototype.resize = function(outerWidth, outerHeight, widthPercent,
    opt_force) {
  if (this.getElement() && (!!opt_force || this.outerHeight != outerHeight ||
      this.outerWidth != outerWidth ||
      this.emojiWidthPercent_ != widthPercent)) {
    this.outerHeight = outerHeight;
    this.outerWidth = outerWidth;
    goog.style.setSize(this.getElement(), outerWidth, outerHeight);
    this.emojiWidthPercent_ = widthPercent;
    var marginOrPadding = Math.round((outerWidth -
        outerWidth * widthPercent) / 2);
    var tabBar = /** @type {!Element} */ (
        this.getChildViewById('tabBar').getElement());
    tabBar.style.paddingLeft = tabBar.style.paddingRight =
        marginOrPadding + 'px';
    var rowsAndKeys = /** @type {!Element} */ (
        this.getChildViewById('rowsAndSideKeys').getElement());
    rowsAndKeys.style.marginLeft = rowsAndKeys.style.marginRight =
        marginOrPadding + 'px';
    var spaceRow = /**@type {!Element} */ (
        this.getChildViewById('emojiSpaceRow').getElement());
    spaceRow.style.marginLeft = spaceRow.style.marginRight =
        marginOrPadding + 'px';
    this.resizeRows(outerWidth, outerHeight);
  }
  // Reposition must happen before clear because it will set the width.
  this.repositionIndicator_();
  this.clearEmojiStates();
};


/**
 * Handles the pointer down event.
 *
 * @param {!i18n.input.chrome.inputview.events.PointerEvent} e .
 * @private
 */
EmojiView.prototype.onPointerDown_ = function(e) {
  var view = e.view;
  if (!view) {
    return;
  }

  if (view.type == ElementType.EMOJI_KEY) {
    this.pointerDownTimeStamp_ = e.timestamp;
    this.dragDistance_ = 0;
    view.setHighlighted(true);
  } else if (view.type == ElementType.PAGE_INDICATOR) {
    this.dragDistance_ = 0;
  }
};


/**
 * Tidy ups the UI after the dragging is ended.
 *
 * @param {number} timestamp .
 * @private
 */
EmojiView.prototype.onDragEnd_ = function(timestamp) {
  var interval = timestamp - this.pointerDownTimeStamp_;
  if (interval < EmojiView.EMOJI_DRAG_INTERVAL_ &&
      Math.abs(this.dragDistance_) >=
      EmojiView.EMOJI_DRAG_SCROLL_DISTANCE_) {
    this.adjustXPosition_(this.dragDistance_);
  } else {
    this.adjustXPosition_();
  }
};


/**
 * Handles POINTER_OUT event.
 *
 * @param {!i18n.input.chrome.inputview.events.PointerEvent} e .
 * @private
 */
EmojiView.prototype.onPointerOut_ = function(e) {
  if (e.view && e.view.type == ElementType.EMOJI_KEY) {
    e.view.setHighlighted(false);
  }
};


/**
 * Handles the pointer up event.
 *
 * @param {!i18n.input.chrome.inputview.events.PointerEvent} e .
 * @private
 */
EmojiView.prototype.onPointerUp_ = function(e) {
  var view = e.view;
  if (!view) {
    this.onDragEnd_(e.timestamp);
    return;
  }

  switch (view.type) {
    case ElementType.EMOJI_KEY:
      if (this.pointerDownTimeStamp_ > 0) {
        if (this.isDragging) {
          this.onDragEnd_(e.timestamp);
        } else if (view.text != '') {
          this.setRecentEmoji_(view.text, view.isEmoticon);
        }
        this.update();
      }
      break;
    case ElementType.TAB_BAR_KEY:
      this.updateCategory_(view.toCategory);
      break;
    case ElementType.PAGE_INDICATOR:
      if (this.isDragging) {
        this.adjustXPosition_();
        this.update();
      }
      break;
  }
  this.isDragging = false;
};


/**
 * Handles the drag event.
 *
 * @param {!i18n.input.chrome.inputview.events.PointerEvent} e .
 * @private
 */
EmojiView.prototype.onDragEvent_ = function(e) {
  var view = e.view;
  if (view.type == ElementType.EMOJI_KEY) {
    this.setEmojiMarginLeft_(e.deltaX);
    this.dragDistance_ += e.deltaX;
  } else if (view.type == ElementType.PAGE_INDICATOR) {
    this.pageIndicator_.slide(e.deltaX,
        this.pagesInCategory_[this.categoryID_]);
    this.setEmojiMarginLeft_(-e.deltaX *
        this.pagesInCategory_[this.categoryID_]);
    this.dragDistance_ -= e.deltaX * this.pagesInCategory_[this.categoryID_];
  }
  if (!this.isDragging) {
    this.isDragging = Math.abs(this.dragDistance_) >
        EmojiView.EMOJI_DRAG_START_DISTANCE_;
  }
};


/** @override */
EmojiView.prototype.disposeInternal = function() {
  goog.dispose(this.pointerHandler_);

  goog.base(this, 'disposeInternal');
};


/**
 * Set the margin left of the emoji slider.
 *
 * @param {number} deltaX The margin left value.
 * @private
 */
EmojiView.prototype.setEmojiMarginLeft_ = function(deltaX) {
  this.emojiRows_.slide(deltaX);
  this.pageIndicator_.slide(-deltaX,
      this.pagesInCategory_[this.categoryID_]);
};


/**
 * Update the current emoji category.
 *
 * @param {number} categoryID .
 * @private
 */
EmojiView.prototype.updateCategory_ = function(categoryID) {
  this.categoryID_ = categoryID;
  this.emojiRows_.updateCategory(this.pageOffsets_[this.categoryID_]);
  this.pageIndicator_.gotoPage(0,
      this.pagesInCategory_[this.categoryID_]);
  this.updateTabbarBorder_();
};


/**
 * Adjust the margin left to the nearest page.
 *
 * @param {number=} opt_distance The distance to adjust to.
 * @private
 */
EmojiView.prototype.adjustXPosition_ = function(opt_distance) {
  var pageNum = this.emojiRows_.adjustXPosition(opt_distance);
  this.categoryID_ = this.pageToCategory_[pageNum];
  this.pageIndicator_.gotoPage(
      pageNum - this.pageOffsets_[this.categoryID_],
      this.pagesInCategory_[this.categoryID_]);
  this.updateTabbarBorder_();

};


/**
 * Clear all the states for the emoji.
 *
 */
EmojiView.prototype.clearEmojiStates = function() {
  this.updateCategory_(this.recentEmojiList_.length > 0 ?
      0 : EmojiView.EMOJI_DEFAULT_CATEGORY_ID_);
};


/**
 * Sets the recent emoji.
 *
 * @param {string} text The recent emoji text.
 * @param {boolean} isEmoticon .
 * @private
 */
EmojiView.prototype.setRecentEmoji_ = function(text, isEmoticon) {
  if (goog.array.contains(this.recentEmojiList_, text)) {
    return;
  }
  goog.array.insertAt(this.recentEmojiList_, text, 0);
  var lastRecentEmojiKeyIndex = Math.min(this.keysPerPage_ - 1,
      this.recentEmojiKeys_.length);
  for (var i = lastRecentEmojiKeyIndex; i > 0; i--) {
    var recentEmojiKey = this.recentEmojiKeys_[i];
    var previousKey = this.recentEmojiKeys_[i - 1];
    recentEmojiKey.updateText(previousKey.text, previousKey.isEmoticon);
  }
  this.recentEmojiKeys_[0].updateText(text, isEmoticon);
};


/**
 * Update the tabbar's border.
 *
 * @private
 */
EmojiView.prototype.updateTabbarBorder_ = function() {
  for (var i = 0, len = this.totalTabbars_; i < len; i++) {
    this.tabbarKeys_[i].updateBorder(this.categoryID_);
  }
};


/** @override */
EmojiView.prototype.activate = function(rawKeyset) {
  this.adapter.setController('emoji', this.languageCode);
  goog.dom.classlist.add(this.getElement().parentElement.parentElement,
      Css.EMOJI);
  this.clearEmojiStates();
  this.pageIndicator_.setVisible(true);
};


/** @override */
EmojiView.prototype.deactivate = function(rawKeyset) {
  this.adapter.unsetController();
  this.pointerDownTimeStamp_ = 0;
  goog.dom.classlist.remove(this.getElement().parentElement.parentElement,
      Css.EMOJI);
  this.pageIndicator_.setVisible(false);
};


/**
 * Set the position of the indicator.
 *
 * @private
 */
EmojiView.prototype.repositionIndicator_ = function() {
  var emojiElement = this.emojiSlider_.getElement();
  var elem = this.pageIndicator_.getElement();
  elem.style.width = goog.style.getSize(emojiElement).width + 'px';
  var rowsAndSideKeys = /** @type {!Element} */ (
      this.getChildViewById('rowsAndSideKeys').getElement());
  var position = new goog.positioning.AnchoredViewportPosition(
      rowsAndSideKeys, goog.positioning.Corner.BOTTOM_START, true);
  position.reposition(this.pageIndicator_.getElement(),
      goog.positioning.Corner.BOTTOM_START);
};
});  // goog.scope
