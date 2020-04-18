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
goog.provide('i18n.input.chrome.inputview.elements.content.KeysetView');

goog.require('goog.array');
goog.require('goog.dom.classlist');
goog.require('goog.i18n.bidi');
goog.require('goog.style');
goog.require('goog.ui.Container');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.ConditionName');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.elements.content.BackspaceKey');
goog.require('i18n.input.chrome.inputview.elements.content.CandidateButton');
goog.require('i18n.input.chrome.inputview.elements.content.CanvasView');
goog.require('i18n.input.chrome.inputview.elements.content.CharacterKey');
goog.require('i18n.input.chrome.inputview.elements.content.CompactKey');
goog.require('i18n.input.chrome.inputview.elements.content.CompactKeyModel');
goog.require('i18n.input.chrome.inputview.elements.content.EmojiKey');
goog.require('i18n.input.chrome.inputview.elements.content.EnSwitcherKey');
goog.require('i18n.input.chrome.inputview.elements.content.EnterKey');
goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');
goog.require('i18n.input.chrome.inputview.elements.content.KeyboardView');
goog.require('i18n.input.chrome.inputview.elements.content.MenuKey');
goog.require('i18n.input.chrome.inputview.elements.content.ModifierKey');
goog.require('i18n.input.chrome.inputview.elements.content.PageIndicator');
goog.require('i18n.input.chrome.inputview.elements.content.SpaceKey');
goog.require('i18n.input.chrome.inputview.elements.content.SwitcherKey');
goog.require('i18n.input.chrome.inputview.elements.content.TabBarKey');
goog.require('i18n.input.chrome.inputview.elements.layout.ExtendedLayout');
goog.require('i18n.input.chrome.inputview.elements.layout.HandwritingLayout');
goog.require('i18n.input.chrome.inputview.elements.layout.LinearLayout');
goog.require('i18n.input.chrome.inputview.elements.layout.SoftKeyView');
goog.require('i18n.input.chrome.inputview.elements.layout.VerticalLayout');
goog.require('i18n.input.chrome.inputview.util');



goog.scope(function() {

var ConditionName = i18n.input.chrome.inputview.ConditionName;
var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;
var ElementType = i18n.input.chrome.ElementType;
var content = i18n.input.chrome.inputview.elements.content;
var layout = i18n.input.chrome.inputview.elements.layout;
var Css = i18n.input.chrome.inputview.Css;
var util = i18n.input.chrome.inputview.util;
var CompactKeyModel =
    i18n.input.chrome.inputview.elements.content.CompactKeyModel;



/**
 * The keyboard.
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
 * @extends {goog.ui.Container}
 */
i18n.input.chrome.inputview.elements.content.KeysetView = function(keyData,
    layoutData, keyboardCode, languageCode, model, name, opt_eventTarget,
    opt_adapter) {
  goog.base(this);
  this.setParentEventTarget(opt_eventTarget || null);

  /**
   * The key configuration data.
   *
   * @type {!Object}
   * @const
   * @private
   */
  this.keyData_ = keyData;

  /**
   * The layout definition.
   *
   * @type {!Object}
   * @const
   * @private
   */
  this.layoutData_ = layoutData;

  /**
   * The keyboard code.
   *
   * @type {string}
   * @private
   */
  this.keyboardCode_ = keyboardCode;

  /**
   * The language code.
   *
   * @protected {string}
   */
  this.languageCode = languageCode;

  /**
   * The model, the reason use dataModel as its name because model_ will
   * conflict with the one in goog.ui.Container.
   *
   * @type {!i18n.input.chrome.inputview.Model}
   * @private
   */
  this.dataModel_ = model;

  /**
   * The rows in this view, the reason we don't use getChild is that container
   * only accepts control as its child, so we have to use
   * row.render(this.getElement()) style.
   *
   * @type {!Array.<layout.LinearLayout>}
   * @private
   */
  this.rows_ = [];

  /**
   * The maps of all the soft key view.
   *
   * @type {!Object.<string, !layout.SoftKeyView>}
   * @private
   */
  this.softKeyViewMap_ = {};

  /**
   * The map from the condition to the soft key view.
   *
   * @type {!Object.<string, !layout.SoftKeyView>}
   * @private
   */
  this.softKeyConditionMap_ = {};

  /**
   * The on-screen keyboard title.
   *
   * @type {string}
   * @private
   */
  this.title_ = name;

  /**
   * The bus channel to communicate with background.
   *
   * @protected {i18n.input.chrome.inputview.Adapter}
   */
  this.adapter = opt_adapter || null;

  /**
   * The conditions.
   *
   * @private {!Object.<string, boolean>}
   */
  this.conditions_ = {};

  /**
   * whether to display the candidate view or not.
   *
   * @type {boolean}
   */
  this.disableCandidateView =
      goog.isDef(this.layoutData_['disableCandidateView']) ?
      this.layoutData_['disableCandidateView'] : false;

  /**
   * The map of the child views.
   * Key: The id of the child element.
   * Value: The element.
   *
   * @private {!Object.<string, !i18n.input.chrome.inputview.elements.Element>}
   */
  this.childMap_ = {};
};
var KeysetView = i18n.input.chrome.inputview.elements.content.KeysetView;
goog.inherits(KeysetView, goog.ui.Container);


/**
 * True if the keyset view has shift state.
 *
 * @type {boolean}
 */
KeysetView.prototype.hasShift = true;


/**
 * The keyboard.
 *
 * @type {!content.KeyboardView}
 * @private
 */
KeysetView.prototype.keyboardView_;


/**
 * The keyset code from which jumps to this keyset view.
 *
 * @type {string}
 */
KeysetView.prototype.fromKeyset = '';


/**
 * The handwriting canvas view.
 *
 * @protected {!content.CanvasView}
 */
KeysetView.prototype.canvasView;


/**
 * The space key.
 *
 * @type {!content.SpaceKey}
 */
KeysetView.prototype.spaceKey;


/**
 * The backspace key.
 *
 * @type {!content.BackspaceKey}
 */
KeysetView.prototype.backspaceKey;


/**
 * The outer height of the view.
 *
 * @protected {number}
 */
KeysetView.prototype.outerHeight = 0;


/**
 * The outer width of the view.
 *
 * @protected {number}
 */
KeysetView.prototype.outerWidth = 0;


/**
 * The width percentage.
 *
 * @private {number}
 */
KeysetView.prototype.widthPercent_ = 1;


/** @override */
KeysetView.prototype.createDom = function() {
  goog.base(this, 'createDom');

  this.hasShift = !this.keyData_[SpecNodeName.NO_SHIFT];
  var elem = this.getElement();
  elem.id = this.keyboardCode_.replace(/\./g, '-');
  goog.dom.classlist.add(elem, i18n.input.chrome.inputview.Css.VIEW);

  var children = this.layoutData_['children'];
  for (var i = 0; i < children.length; i++) {
    var child = children[i];
    var layoutElem = /** @type {!layout.LinearLayout} */
        (this.createLayoutElement_(child[i18n.input.chrome.inputview.
            SpecNodeName.SPEC], this));
    // Can't use addChild here, because container only allow control as its
    // child.
    if (layoutElem) {
      layoutElem.render(elem);
      this.rows_.push(layoutElem);
    }
  }

  var softKeyList = [];
  var keySpecs = this.keyData_[SpecNodeName.KEY_LIST];
  var hasAltGrCharacterInTheKeyset = this.hasAltGrCharacterInTheKeyset_(
      keySpecs);
  for (var i = 0; i < keySpecs.length; i++) {
    var softKey = this.createKey_(keySpecs[i][SpecNodeName.SPEC],
        hasAltGrCharacterInTheKeyset);
    if (softKey) {
      softKeyList.push(softKey);
    }
  }
  var mapping = this.keyData_[
      SpecNodeName.MAPPING];
  this.keyboardView_.setUp(softKeyList, this.softKeyViewMap_, mapping);
};


/**
 * Returns an object describing the KeysetView and soft keys in the format
 * necessary for gesture typing decoding. The returned object is only meaningful
 * when this is called on a KeysetView for compact alphanumeric layouts.
 *
 * @return {?Object} A representation of this keyset for gesture typing
 *     decoding, or null if there are no soft keys.
 */
KeysetView.prototype.getKeyboardLayoutForGesture = function() {
  var softKeyViewMap = this.softKeyViewMap_;
  // Filter down to the compact character soft keys.
  var softKeyViews = [];
  for (var key in softKeyViewMap) {
    var skv = softKeyViewMap[key];
    if (skv.softKey && skv.softKey.type == ElementType.COMPACT_KEY &&
        this.isCharacter_(skv.softKey.text)) {
      softKeyViews.push(skv);
    }
  }

  if (softKeyViews.length == 0) {
    return null;
  }

  // Find the common key width/height. We assume that all the keys have the same
  // width and height, so use an arbitrary key to determine this.
  // TODO: Update this code to actually calculate the common dimensions.
  // Technically in QWERTY the A key is wider than other keys, though in most
  // cases this should not matter since we provide explicit dimensions for
  // every key.
  // TODO: Define this keyboard layout as an actual data type.
  var ret = {};
  var softKeyView = softKeyViews[0];
  var common_width = softKeyView.softKey.getElement().clientWidth;
  var common_height = softKeyView.softKey.getElement().clientHeight;
  ret['most_common_key_width'] = common_width;
  ret['most_common_key_height'] = common_height;

  // Determine the necessary attributes for each key and write it into the
  // object as an array.
  var keys = [];
  for (var i = 0; i < softKeyViews.length; i++) {
    softKeyView = softKeyViews[i];
    var codepoint = softKeyView.softKey.text.toLowerCase().charCodeAt(0);
    var width = softKeyView.softKey.getElement().clientWidth;
    var height = softKeyView.softKey.getElement().clientHeight;
    // Return the x, y positions relative to the viewport, as this is the same
    // convention that gesture points follow. Note that these are the center
    // points of the keys and not the top-left corner.
    var rect = softKeyView.softKey.getElement().getBoundingClientRect();
    var x = rect.left + (width / 2.0);
    var y = rect.top + (height / 2.0);

    keys.push({
      'codepoint': codepoint,
      'width': width,
      'height': height,
      'x': x,
      'y': y
    });
  }
  ret['keys'] = keys;

  // Determine the remaining parameters specific to the keyboard. For the width,
  // the KeysetView width is OK because it is known to span the document. For
  // the height, the viewport height must be used in order to include the offset
  // from the top of the viewport.
  ret['keyboard_width'] = this.getElement().clientWidth;
  ret['keyboard_height'] = window.innerHeight;
  return ret;
};


/**
 * Updates the view.
 */
KeysetView.prototype.update = function() {
  this.keyboardView_.update();
};


/**
 * @param {number} outerWidth .
 * @param {number} outerHeight .
 * @param {number} widthPercent .
 * @param {boolean} force .
 * @return {boolean} .
 */
KeysetView.prototype.shouldResize = function(outerWidth, outerHeight,
    widthPercent, force) {
  var needResize = force || (this.outerHeight != outerHeight ||
      this.outerWidth != outerWidth || this.widthPercent_ != widthPercent);
  return !!this.getElement() && needResize;
};


/**
 * Resizes the view.
 *
 * @param {number} outerWidth The width of the outer space.
 * @param {number} outerHeight The height of the outer space.
 * @param {number} widthPercent The percentage of the width.
 * @param {boolean=} opt_force Forces to resize the view.
 */
KeysetView.prototype.resize = function(outerWidth, outerHeight, widthPercent,
    opt_force) {
  if (this.shouldResize(outerWidth, outerHeight, widthPercent, !!opt_force)) {
    this.outerHeight = outerHeight;
    this.outerWidth = outerWidth;
    this.widthPercent_ = widthPercent;
    var elem = this.getElement();
    var margin = Math.round((outerWidth - outerWidth * widthPercent) / 2);
    // Keyset view has 1px border.
    var w = outerWidth - 2 * margin - 2;
    elem.style.marginLeft = elem.style.marginRight = margin + 'px';
    goog.style.setSize(elem, w, outerHeight);

    this.resizeRows(w, outerHeight);
  }
};


/**
 * Resizes the rows inside the keyset.
 *
 * @param {number} width .
 * @param {number} height .
 */
KeysetView.prototype.resizeRows = function(width, height) {
  var weightArray = [];
  for (var i = 0; i < this.rows_.length; i++) {
    var row = this.rows_[i];
    weightArray.push(row.getHeightInWeight());
  }

  var splitedHeight = i18n.input.chrome.inputview.util.splitValue(weightArray,
      height);
  for (var i = 0; i < this.rows_.length; i++) {
    var row = this.rows_[i];
    row.resize(width, splitedHeight[i]);
  }
};


/**
 * Gets the total height in weight.
 *
 * @return {number} The total height in weight.
 */
KeysetView.prototype.getHeightInWeight = function() {
  var heightInWeight = 0;
  for (var i = 0; i < this.rows_.length; i++) {
    var row = this.rows_[i];
    heightInWeight += row.getHeightInWeight();
  }
  return heightInWeight;
};


/**
 * Apply conditions.
 *
 * @param {!Object.<string, boolean>} conditions The conditions.
 */
KeysetView.prototype.applyConditions = function(conditions) {
  this.conditions_ = conditions;
  for (var condition in conditions) {
    var softKeyView = this.softKeyConditionMap_[condition];
    var isConditionEnabled = conditions[condition];
    if (softKeyView) {
      softKeyView.setVisible(isConditionEnabled);
      var softKeyViewGetWeight = this.softKeyViewMap_[softKeyView.
          giveWeightTo];
      if (softKeyViewGetWeight) {
        // Only supports horizontal weight transfer now.
        softKeyViewGetWeight.dynamicaGrantedWeight += isConditionEnabled ?
            0 : softKeyView.widthInWeight;
      }
    }
  }

  // Adjusts the width of globe key and menu key according to the mock when they
  // both show up.
  // TODO: This is hacky. Remove the hack once figure out a better way.
  var showGlobeKey = conditions[ConditionName.SHOW_GLOBE_OR_SYMBOL];
  var showMenuKey = conditions[ConditionName.SHOW_MENU];
  var menuKeyView = this.softKeyConditionMap_[ConditionName.SHOW_MENU];
  var globeKeyView =
      this.softKeyConditionMap_[ConditionName.SHOW_GLOBE_OR_SYMBOL];
  if (menuKeyView && globeKeyView) {
    var softKeyViewGetWeight =
        this.softKeyViewMap_[menuKeyView.giveWeightTo];
    if (softKeyViewGetWeight) {
      if (showGlobeKey && showMenuKey) {
        globeKeyView.dynamicaGrantedWeight = -0.1;
        menuKeyView.dynamicaGrantedWeight = -0.4;
        softKeyViewGetWeight.dynamicaGrantedWeight += 0.5;
      }
    }
  }
};


/**
 * Updates the condition.
 *
 * @param {string} name .
 * @param {boolean} value .
 */
KeysetView.prototype.updateCondition = function(name, value) {
  if (this.conditions_[name] === value) {
    // No need to update.
    return;
  }
  for (var id in this.softKeyViewMap_) {
    var skv = this.softKeyViewMap_[id];
    skv.dynamicaGrantedWeight = 0;
  }
  this.conditions_[name] = value;
  this.applyConditions(this.conditions_);
  this.resize(this.outerWidth, this.outerHeight, this.widthPercent_, true);
  this.update();
};


/**
 * Returns whether c is a character between a-z or A-Z.
 *
 * @param {string} c The character to test.
 * @return {boolean} .
 * @private
 */
KeysetView.prototype.isCharacter_ = function(c) {
  return c.length === 1 && c.match(/^[a-z]$/i) != null;
};


/**
 * Creates the element according to its type.
 *
 * @param {!Object} spec The specification.
 * @param {!goog.events.EventTarget=} opt_eventTarget The event target.
 * @return {i18n.input.chrome.inputview.elements.Element} The element.
 * @private
 */
KeysetView.prototype.createElement_ = function(spec, opt_eventTarget) {
  var type = spec[SpecNodeName.TYPE];
  var id = spec[SpecNodeName.ID];
  var widthInWeight = spec[
      SpecNodeName.WIDTH_IN_WEIGHT];
  var heightInWeight = spec[
      SpecNodeName.HEIGHT_IN_WEIGHT];
  var elem = null;
  switch (type) {
    case ElementType.SOFT_KEY_VIEW:
      var condition = spec[SpecNodeName.CONDITION];
      var giveWeightTo = spec[SpecNodeName.GIVE_WEIGHT_TO];
      elem = new layout.SoftKeyView(id, widthInWeight,
          heightInWeight, condition, giveWeightTo, opt_eventTarget);
      this.softKeyConditionMap_[condition] = elem;
      break;
    case ElementType.LINEAR_LAYOUT:
      var opt_iconCssClass = spec[SpecNodeName.ICON_CSS_CLASS];
      elem = new layout.LinearLayout(id, opt_eventTarget, opt_iconCssClass);
      break;
    case ElementType.EXTENDED_LAYOUT:
      elem = new layout.ExtendedLayout(id, opt_eventTarget);
      break;
    case ElementType.VERTICAL_LAYOUT:
      elem = new layout.VerticalLayout(id, opt_eventTarget);
      break;
    case ElementType.LAYOUT_VIEW:
      this.keyboardView_ = new content.KeyboardView(id, opt_eventTarget);
      elem = this.keyboardView_;
      break;
    case ElementType.CANVAS_VIEW:
      this.canvasView = new content.CanvasView(id, widthInWeight,
          heightInWeight, opt_eventTarget, this.adapter);
      elem = this.canvasView;
      break;
    case ElementType.HANDWRITING_LAYOUT:
      elem = new layout.HandwritingLayout(id, opt_eventTarget);
      break;
  }
  if (elem) {
    this.childMap_[id] = elem;
  }
  return elem;
};


/**
 * Creates the layout element.
 *
 * @param {!Object} spec The specification for the element.
 * @param {!goog.events.EventTarget=} opt_parentEventTarget The parent event
 *     target.
 * @return {i18n.input.chrome.inputview.elements.Element} The element.
 * @private
 */
KeysetView.prototype.createLayoutElement_ = function(spec,
    opt_parentEventTarget) {
  var element = this.createElement_(spec, opt_parentEventTarget);
  if (!element) {
    return null;
  }

  var children = spec[SpecNodeName.CHILDREN];
  if (children) {
    children = goog.array.flatten(children);
    for (var i = 0; i < children.length; i++) {
      var child = children[i];
      var childElem = this.createLayoutElement_(
          child[SpecNodeName.SPEC], element);
      if (childElem) {
        element.addChild(childElem, true);
      }
    }
  }
  if (element.type == ElementType.SOFT_KEY_VIEW) {
    this.softKeyViewMap_[element.id] =
        /** @type {!layout.SoftKeyView} */ (element);
  }
  return element;
};


/**
 * Checks if there is altgr character.
 *
 * @param {!Array.<!Object>} keySpecs The list of key specs.
 * @return {!Array<boolean>} A list with two boolean values, the first is
 *    for whether there is altgr character of letter keys, the second is for
 *    symbol keys.
 * @private
 */
KeysetView.prototype.hasAltGrCharacterInTheKeyset_ = function(keySpecs) {
  var result = [false, false];
  for (var i = 0; i < keySpecs.length; i++) {
    var spec = keySpecs[i];
    var characters = spec[SpecNodeName.CHARACTERS];
    if (characters && (!!characters[2] || !!characters[3])) {
      var index = i18n.input.chrome.inputview.util.isLetterKey(
          characters) ? 0 : 1;
      result[index] = true;
    }
  }
  return result;
};


/**
 * Creates a soft key.
 *
 * @param {Object} spec The specification.
 * @param {!Array.<boolean, boolean>} hasAltGrCharacterInTheKeyset The list
 *     of results for whether there is altgr character, the first for letter
 *     key, the second for symbol key.
 * @return {i18n.input.chrome.inputview.elements.Element} The soft key.
 * @private
 */
KeysetView.prototype.createKey_ = function(spec, hasAltGrCharacterInTheKeyset) {
  var type = spec[SpecNodeName.TYPE];
  var id = spec[SpecNodeName.ID];
  var keyCode = spec[SpecNodeName.KEY_CODE]; // Could be undefined.
  var name = spec[SpecNodeName.NAME];
  var characters = spec[SpecNodeName.CHARACTERS];
  var iconCssClass = spec[SpecNodeName.ICON_CSS_CLASS];
  var textCssClass = spec[SpecNodeName.TEXT_CSS_CLASS];
  var toKeyset = spec[SpecNodeName.TO_KEYSET];
  var toKeysetName = spec[SpecNodeName.TO_KEYSET_NAME];
  var elem = null;
  switch (type) {
    case ElementType.MODIFIER_KEY:
      var toState = spec[SpecNodeName.TO_STATE];
      var supportSticky = spec[SpecNodeName.SUPPORT_STICKY];
      elem = new content.ModifierKey(id, name, iconCssClass, toState,
          this.dataModel_.stateManager, supportSticky);
      break;
    case ElementType.SPACE_KEY:
      this.spaceKey = new content.SpaceKey(id,
          this.dataModel_.stateManager, this.title_, characters,
          undefined, iconCssClass, toKeyset);
      elem = this.spaceKey;
      break;
    case ElementType.EN_SWITCHER:
      elem = new content.EnSwitcherKey(id, type, name, iconCssClass,
          this.dataModel_.stateManager, Css.EN_SWITCHER_DEFAULT,
          Css.EN_SWITCHER_ENGLISH);
      break;
    case ElementType.BACKSPACE_KEY:
      elem = new content.BackspaceKey(id, type, name, iconCssClass);
      this.backspaceKey = elem;
      break;
    case ElementType.ENTER_KEY:
      elem = new content.EnterKey(id, iconCssClass);
      break;
    case ElementType.TAB_KEY:
    case ElementType.ARROW_UP:
    case ElementType.ARROW_DOWN:
    case ElementType.ARROW_LEFT:
    case ElementType.ARROW_RIGHT:
    case ElementType.HIDE_KEYBOARD_KEY:
    case ElementType.GLOBE_KEY:
    case ElementType.BACK_TO_KEYBOARD:
    case ElementType.HOTROD_SWITCHER_KEY:
      elem = new content.FunctionalKey(id, type, name, iconCssClass);
      break;
    case ElementType.TAB_BAR_KEY:
      elem = new content.TabBarKey(id, type, name, iconCssClass,
          toKeyset, this.dataModel_.stateManager);
      break;
    case ElementType.EMOJI_KEY:
      var text = spec[SpecNodeName.TEXT];
      var isEmoticon = spec[SpecNodeName.IS_EMOTICON];
      elem = new content.EmojiKey(id, type, text, isEmoticon);
      break;
    case ElementType.PAGE_INDICATOR:
      elem = new content.PageIndicator(id, type);
      break;
    case ElementType.IME_SWITCH:
      elem = new content.FunctionalKey(id, type, name, iconCssClass, undefined,
          textCssClass);
      break;
    case ElementType.MENU_KEY:
      elem = new content.MenuKey(id, type, name, iconCssClass, toKeyset);
      break;
    case ElementType.SWITCHER_KEY:
      var record = spec[SpecNodeName.RECORD];
      elem = new content.SwitcherKey(id, type, name, iconCssClass, toKeyset,
          toKeysetName, record);
      break;
    case ElementType.COMPACT_KEY:
      var hintText = spec[SpecNodeName.HINT_TEXT];
      var text = spec[SpecNodeName.TEXT];
      var marginLeftPercent = spec[SpecNodeName.MARGIN_LEFT_PERCENT];
      var marginRightPercent = spec[SpecNodeName.MARGIN_RIGHT_PERCENT];
      var isGrey = spec[SpecNodeName.IS_GREY];
      var moreKeys = spec[SpecNodeName.MORE_KEYS];
      var moreKeysCharacters =
          moreKeys ? moreKeys[SpecNodeName.CHARACTERS] : undefined;
      var fixedColumns =
          moreKeys ? moreKeys[SpecNodeName.FIXED_COLUMN_NUMBER] : undefined;
      var contextMap = spec[SpecNodeName.ON_CONTEXT];
      var title = spec[SpecNodeName.TITLE];
      var onShift = spec[SpecNodeName.ON_SHIFT];
      var moreKeysShiftType = spec[SpecNodeName.MORE_KEYS_SHIFT_OPERATION];
      var compactKeyModel = new CompactKeyModel(marginLeftPercent,
          marginRightPercent, isGrey, moreKeysCharacters, moreKeysShiftType,
          onShift, contextMap, textCssClass, title, fixedColumns);
      elem = new content.CompactKey(
          id, text, hintText, this.dataModel_.stateManager, this.hasShift,
          compactKeyModel, undefined);
      break;
    case ElementType.CHARACTER_KEY:
      if (characters.length == 1) {
        // If there is no character for shift state, just make the character of
        // default state to be that one.
        characters.push(characters[0]);
      }
      var isLetterKey = i18n.input.chrome.inputview.util.isLetterKey(
          characters);
      var enableShiftRendering = !!spec[SpecNodeName.ENABLE_SHIFT_RENDERING];
      elem = new content.CharacterKey(id, keyCode || 0,
          characters, isLetterKey,
          hasAltGrCharacterInTheKeyset[isLetterKey ? 1 : 0],
          this.dataModel_.settings.alwaysRenderAltGrCharacter,
          this.dataModel_.stateManager,
          goog.i18n.bidi.isRtlLanguage(this.languageCode),
          enableShiftRendering);
      break;

    case ElementType.BACK_BUTTON:
      elem = new content.CandidateButton(
          id, ElementType.BACK_BUTTON, iconCssClass,
          chrome.i18n.getMessage('HANDWRITING_BACK'), this);
      break;
  }
  if (elem) {
    this.childMap_[id] = elem;
  }
  return elem;
};


/**
 * Gets the view for the key.
 *
 * @param {string} code The code of the key.
 * @return {i18n.input.chrome.inputview.elements.content.SoftKey} The soft key.
 */
KeysetView.prototype.getViewForKey = function(code) {
  return this.keyboardView_.getViewForKey(code);
};


/**
 * Gets the width in weight for a entire row.
 *
 * @return {number} .
 */
KeysetView.prototype.getWidthInWeight = function() {
  if (this.rows_.length > 0) {
    return this.rows_[0].getWidthInWeight();
  }

  return 0;
};


/**
 * Whether there are strokes on canvas.
 *
 * @return {boolean} Whether there are strokes on canvas.
 */
KeysetView.prototype.hasStrokesOnCanvas = function() {
  if (this.canvasView) {
    return this.canvasView.hasStrokesOnCanvas();
  } else {
    return false;
  }
};


/**
 * Cleans the stokes.
 */
KeysetView.prototype.cleanStroke = function() {
  if (this.canvasView) {
    this.canvasView.reset();
  }
};


/**
 * Checks the view whether is handwriting panel.
 *
 * @return {boolean} Whether is handwriting panel.
 */
KeysetView.prototype.isHandwriting = function() {
  return this.keyboardCode_ == 'hwt';
};


/**
 * Get the subview of the keysetview according to the id.
 *
 * @param {string} id The id.
 * @return {i18n.input.chrome.inputview.elements.Element}
 */
KeysetView.prototype.getChildViewById = function(id) {
  return this.childMap_[id];
};


/**
 * Activate the current keyset view instance.
 *
 * @param {string} rawKeyset The raw keyset.
 */
KeysetView.prototype.activate = function(rawKeyset) {
  var haveEnSwitcher =
      goog.array.contains(util.KEYSETS_HAVE_EN_SWTICHER, rawKeyset);
  this.updateCondition(ConditionName.SHOW_EN_SWITCHER_KEY, haveEnSwitcher);
  if (haveEnSwitcher) {
    goog.dom.classlist.add(this.getElement(), Css.PINYIN);
  } else {
    goog.dom.classlist.remove(this.getElement(), Css.PINYIN);
  }
};


/**
 * Deactivate the current keyset view instance.
 *
 * @param {string} rawKeyset The raw keyset id map to the instance keyset id.
 */
KeysetView.prototype.deactivate = goog.nullFunction;
});  // goog.scope
