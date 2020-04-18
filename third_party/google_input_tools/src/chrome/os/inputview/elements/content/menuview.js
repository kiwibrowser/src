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
goog.provide('i18n.input.chrome.inputview.elements.content.MenuView');

goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.array');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.log');
goog.require('goog.style');
goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.content.MenuItem');
goog.require('i18n.input.chrome.inputview.util');



goog.scope(function() {
var ElementType = i18n.input.chrome.ElementType;
var MenuItem = i18n.input.chrome.inputview.elements.content.MenuItem;
var Css = i18n.input.chrome.inputview.Css;


/**
 * The view for IME switcher, layout switcher and settings menu popup.
 * TODO(bshe): Refactor MenuView to extend AltDataView.
 *
 * @param {goog.events.EventTarget=} opt_eventTarget The parent event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.MenuView = function(
    opt_eventTarget) {
  goog.base(this, '', ElementType.MENU_VIEW, opt_eventTarget);

  /**
   * The cover element.
   * Note: The reason we use a separate cover element instead of the view is
   * because of the opacity. We can not reassign the opacity in child element.
   *
   * @private {!Element}
   */
  this.coverElement_ = this.getDomHelper().createDom(goog.dom.TagName.DIV,
      Css.ALTDATA_COVER);

  /**
   * The key triggered this menu view.
   *
   * @private {i18n.input.chrome.inputview.elements.content.SoftKey}
   */
  this.triggeredBy_ = null;

  /**
   * Logger for MenuView.
   * @private {goog.log.Logger}
   */
  this.logger_ = goog.log.getLogger(
      'i18n.input.chrome.inputview.elements.content.MenuView');

  this.pointerConfig.stopEventPropagation = false;
  this.pointerConfig.preventDefault = false;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.MenuView,
    i18n.input.chrome.inputview.elements.Element);
var MenuView = i18n.input.chrome.inputview.elements.content.MenuView;


/**
 * The commands which items in MenuView may send to inputview/controller.js.
 *
 * @enum {number}
 */
MenuView.Command = {
  SWITCH_IME: 0,
  SWITCH_KEYSET: 1,
  OPEN_EMOJI: 2,
  OPEN_HANDWRITING: 3,
  OPEN_SETTING: 4
};


/**
 * The maximal number of visible input methods in the view. If more than 3 input
 * methods are enabled, only 3 of them will show and others can be scrolled into
 * view. The reason we have this limitation is because menu view can not be
 * higher than the keyboard view.
 *
 * @private {number}
 */
MenuView.MAX_VISIBLE_ITEMS_ = 4;


/**
 * Same as MAX_VISIBLE_ITEM_ but for a11y keyboard. A11y virtual keyboard is
 * shorter than normal keyboard so 3 is used.
 *
 * @private {number}
 */
MenuView.MAX_VISIBLE_ITEMS_A11Y_ = 3;


/**
 * The width in px of the popup menu.
 * The total width including padding is 300px, the left padding is 41px.
 *
 * @private {number}
 */
MenuView.WIDTH_PX_ = 275;


/**
 * The left padding in px of the menu item.
 *
 * @private {number}
 */
MenuView.PADDING_LEFT_PX_ = 25;


/**
 * The height in px of the popup menu item.
 *
 * @private {number}
 */
MenuView.LIST_ITEM_HEIGHT_PX_ = 45;


/** @override */
MenuView.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var dom = this.getDomHelper();
  var elem = this.getElement();
  goog.dom.classlist.add(elem, Css.MENU_VIEW);
  goog.a11y.aria.setState(this.coverElement_, goog.a11y.aria.State.LABEL,
      chrome.i18n.getMessage('DISMISS_MENU'));
  dom.appendChild(document.body, this.coverElement_);
  goog.style.setElementShown(this.coverElement_, false);

  this.coverElement_['view'] = this;
};


/**
 * Shows the menu view.
 * @param {!i18n.input.chrome.inputview.elements.content.SoftKey} key The key
 *     triggered this menu view.
 * @param {!string} currentKeysetId The keyset ID that this menu key belongs to.
 * @param {boolean} isCompact True if the keyset that owns the menu key is a
 *     compact layout.
 * @param {boolean} enableCompactLayout True if the keyset that owns the menu
 *     key enabled compact layout.
 * @param {!string} currentInputMethod The current input method ID.
 * @param {?Array.<!Object<string, string, string>>} inputMethods The list of
 *     activated input methods.
 * @param {boolean} hasHwt Whether to add handwriting button.
 * @param {boolean} enableSettings Whether to add a link to settings page.
 * @param {boolean} hasEmoji Whether to enable emoji.
 * @param {boolean} isA11y Whether the current keyboard is a11y-optimized.
 */
MenuView.prototype.show = function(key, currentKeysetId, isCompact,
    enableCompactLayout, currentInputMethod, inputMethods, hasHwt,
    enableSettings, hasEmoji, isA11y) {
  var ElementType = i18n.input.chrome.ElementType;
  var dom = this.getDomHelper();
  if (key.type != ElementType.MENU_KEY) {
    goog.log.warning(this.logger_,
        'Unexpected key triggered the menu view. Key type = ' + key.type + '.');
    return;
  }
  this.triggeredBy_ = key;
  var coordinate = goog.style.getClientPosition(key.getElement());
  var x = coordinate.x;
  // y is the maximal height that menu view can have.
  var y = coordinate.y;

  goog.style.setElementShown(this.getElement(), true);
  // TODO(bshe): May not need to remove child.
  dom.removeChildren(this.getElement());

  var totalHeight = 0;
  totalHeight += this.addInputMethodItems_(currentInputMethod, inputMethods,
      isA11y);
  totalHeight += this.addLayoutSwitcherItem_(key, currentKeysetId, isCompact,
      enableCompactLayout);
  if (hasHwt || enableSettings || hasEmoji) {
    totalHeight += this.addFooterItems_(hasHwt, enableSettings, hasEmoji);
  }


  var left = x;
  // TODO(bshe): Take care of elemTop < 0. A scrollable view is probably needed.
  var elemTop = y - totalHeight;

  goog.style.setPosition(this.getElement(), left, elemTop);
  goog.style.setElementShown(this.coverElement_, true);
  this.triggeredBy_.setHighlighted(true);
};


/** Hides the menu view. */
MenuView.prototype.hide = function() {
  goog.style.setElementShown(this.getElement(), false);
  goog.style.setElementShown(this.coverElement_, false);
  if (this.triggeredBy_) {
    this.triggeredBy_.setHighlighted(false);
  }
};


/**
 * Adds the list of activated input methods.
 *
 * @param {!string} currentInputMethod The current input method ID.
 * @param {?Array.<!Object<string, string, string>>} inputMethods The list of
 *     activated input methods.
 * @param {boolean} isA11y Whether the current keyboard is a11y-optimized.
 * @return {number} The height of the ime list container.
 * @private
 */
MenuView.prototype.addInputMethodItems_ = function(currentInputMethod,
    inputMethods, isA11y) {
  var dom = this.getDomHelper();
  var container = dom.createDom(goog.dom.TagName.DIV,
      Css.IME_LIST_CONTAINER);
  var visibleItems = isA11y ?
      MenuView.MAX_VISIBLE_ITEMS_A11Y_ : MenuView.MAX_VISIBLE_ITEMS_;

  for (var i = 0; i < inputMethods.length; i++) {
    var inputMethod = inputMethods[i];
    var listItem = {
      'indicator': inputMethod['indicator'],
      'name': inputMethod['name'],
      'command':
        [MenuView.Command.SWITCH_IME, inputMethod['id']]
    };
    var ariaLabel = chrome.i18n.getMessage('SWITCH_TO_KEYBOARD_PREFIX') +
        inputMethod['name'];
    if (currentInputMethod == inputMethod['id']) {
      ariaLabel = chrome.i18n.getMessage('CURRENT_KEYBOARD_PREFIX') +
          inputMethod['name'];
    }
    var imeItem = new MenuItem(String(i), listItem, MenuItem.Type.LIST_ITEM,
        ariaLabel);
    imeItem.render(container);
    if (currentInputMethod == inputMethod['id']) {
      imeItem.check();
    }
    goog.style.setSize(imeItem.getElement(),
        MenuView.WIDTH_PX_ + MenuView.PADDING_LEFT_PX_,
        MenuView.LIST_ITEM_HEIGHT_PX_);
  }

  var containerHeight = inputMethods.length > visibleItems ?
      MenuView.LIST_ITEM_HEIGHT_PX_ * visibleItems :
      MenuView.LIST_ITEM_HEIGHT_PX_ * inputMethods.length;
  goog.style.setSize(container, MenuView.WIDTH_PX_ + MenuView.PADDING_LEFT_PX_,
      containerHeight);

  dom.appendChild(this.getElement(), container);
  return containerHeight;
};


/**
 * Add the full/compact layout switcher item. If a full layout does not have or
 * disabled compact layout, this is a noop.
 *
 * @param {!i18n.input.chrome.inputview.elements.content.SoftKey} key The key
 *     triggerred this menu view.
 * @param {!string} currentKeysetId The keyset ID that this menu key belongs to.
 * @param {boolean} isCompact True if the keyset that owns the menu key is a
 *     compact layout.
 * @param {boolean} enableCompactLayout True if the keyset that owns the menu
 *     key enabled compact layout.
 * @return {number} The height of the layout switcher item.
 * @private
 */
MenuView.prototype.addLayoutSwitcherItem_ = function(key, currentKeysetId,
    isCompact, enableCompactLayout) {
  // Some full layouts either disabled compact layout (CJK) or do not have one.
  // Do not add a layout switcher item in this case.
  if (!isCompact && !enableCompactLayout) {
    return 0;
  }
  // Adds layout switcher.
  var layoutSwitcherItem;
  if (isCompact) {
    var fullLayoutId = currentKeysetId.split('.')[0];
    layoutSwitcherItem = new MenuItem('MenuLayoutSwitcher',
        {
          'iconURL': 'images/regular_size.png',
          'name': chrome.i18n.getMessage('SWITCH_TO_FULL_LAYOUT'),
          'command': [MenuView.Command.SWITCH_KEYSET, fullLayoutId]
        },
        MenuItem.Type.LIST_ITEM,
        chrome.i18n.getMessage('SWITCH_TO_FULL_LAYOUT'));
  } else {
    if (goog.array.contains(i18n.input.chrome.inputview.util.KEYSETS_USE_US,
        currentKeysetId)) {
      key.toKeyset = currentKeysetId + '.compact.qwerty';
    }
    layoutSwitcherItem = new MenuItem('MenuLayoutSwitcher',
        {
          'iconURL': 'images/compact.png',
          'name': chrome.i18n.getMessage('SWITCH_TO_COMPACT_LAYOUT'),
          'command': [MenuView.Command.SWITCH_KEYSET, key.toKeyset]
        },
        MenuItem.Type.LIST_ITEM,
        chrome.i18n.getMessage('SWITCH_TO_COMPACT_LAYOUT'));
  }
  layoutSwitcherItem.render(this.getElement());
  goog.style.setSize(layoutSwitcherItem.getElement(), MenuView.WIDTH_PX_,
      MenuView.LIST_ITEM_HEIGHT_PX_);

  return MenuView.LIST_ITEM_HEIGHT_PX_;
};


/**
 * Adds a few items into the menu view. We only support handwriting and settings
 * now.
 *
 * @param {boolean} hasHwt Whether to add handwriting button.
 * @param {boolean} enableSettings Whether to add settings button.
 * @param {boolean} hasEmoji Whether to add emoji button.
 * @return {number} The height of the footer.
 * @private
 */
MenuView.prototype.addFooterItems_ = function(hasHwt, enableSettings,
    hasEmoji) {
  var dom = this.getDomHelper();
  var footer = dom.createDom(goog.dom.TagName.DIV, Css.MENU_FOOTER);
  if (hasEmoji) {
    var emoji = {
      'iconCssClass': Css.MENU_FOOTER_EMOJI_BUTTON,
      'command': [MenuView.Command.OPEN_EMOJI]
    };
    var emojiFooter = new MenuItem('emoji', emoji,
        MenuItem.Type.FOOTER_ITEM,
        chrome.i18n.getMessage('FOOTER_EMOJI_BUTTON'));
    emojiFooter.render(footer);
  }

  if (hasHwt) {
    var handWriting = {
      'iconCssClass': Css.MENU_FOOTER_HANDWRITING_BUTTON,
      'command': [MenuView.Command.OPEN_HANDWRITING]
    };
    var handWritingFooter = new MenuItem('handwriting', handWriting,
        MenuItem.Type.FOOTER_ITEM,
        chrome.i18n.getMessage('FOOTER_HANDWRITING_BUTTON'));
    handWritingFooter.render(footer);
  }

  if (enableSettings) {
    var setting = {
      'iconCssClass': Css.MENU_FOOTER_SETTING_BUTTON,
      'command': [MenuView.Command.OPEN_SETTING]
    };
    var settingFooter = new MenuItem('setting', setting,
        MenuItem.Type.FOOTER_ITEM,
        chrome.i18n.getMessage('FOOTER_SETTINGS_BUTTON'));
    settingFooter.render(footer);
  }

  // Sets footer itmes' width.
  var elems = dom.getChildren(footer);
  var len = elems.length;
  var subWidth =
      Math.ceil((MenuView.WIDTH_PX_ + MenuView.PADDING_LEFT_PX_) / len);
  for (var i = 0; i < len - 1; i++) {
    elems[i].style.width = subWidth + 'px';
  }
  elems[len - 1].style.width = (MenuView.WIDTH_PX_ + MenuView.PADDING_LEFT_PX_ -
      subWidth * (len - 1)) + 'px';

  dom.appendChild(this.getElement(), footer);
  goog.style.setSize(footer, (MenuView.WIDTH_PX_ + MenuView.PADDING_LEFT_PX_),
      MenuView.LIST_ITEM_HEIGHT_PX_);

  return MenuView.LIST_ITEM_HEIGHT_PX_;
};


/** @return {!Element} The cover element. */
MenuView.prototype.getCoverElement = function() {
  return this.coverElement_;
};


/** @override */
MenuView.prototype.resize = function(width, height) {
  goog.base(this, 'resize', width, height);

  goog.style.setSize(this.coverElement_, width, height);
  // Hides the menu view directly after resize.
  this.hide();
};


/** @override */
MenuView.prototype.disposeInternal = function() {
  this.coverElement_['view'] = null;

  goog.base(this, 'disposeInternal');
};
});  // goog.scope


