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
goog.provide('i18n.input.chrome.inputview.content.Constants');

goog.require('i18n.input.chrome.ElementType');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.StateType');

goog.scope(function() {

var ElementType = i18n.input.chrome.ElementType;


/**
 * The non letter keys.
 *
 * @const
 * @enum {Object}
 */
i18n.input.chrome.inputview.content.Constants.NON_LETTER_KEYS = {
  BACKSPACE: {
    'iconCssClass': i18n.input.chrome.inputview.Css.BACKSPACE_ICON,
    'type': ElementType.BACKSPACE_KEY,
    'id': 'Backspace'
  },
  ENTER: {
    'iconCssClass': i18n.input.chrome.inputview.Css.ENTER_ICON,
    'type': ElementType.ENTER_KEY,
    'id': 'Enter'
  },
  HIDE: {
    'iconCssClass': i18n.input.chrome.inputview.Css.HIDE_KEYBOARD_ICON,
    'type': ElementType.HIDE_KEYBOARD_KEY,
    'id': 'HideKeyboard'
  },
  LEFT_SHIFT: {
    'toState': i18n.input.chrome.inputview.StateType.SHIFT,
    'iconCssClass': i18n.input.chrome.inputview.Css.SHIFT_ICON,
    'type': ElementType.MODIFIER_KEY,
    'id': 'ShiftLeft',
    'supportSticky': true
  },
  RIGHT_SHIFT: {
    'toState': i18n.input.chrome.inputview.StateType.SHIFT,
    'iconCssClass': i18n.input.chrome.inputview.Css.SHIFT_ICON,
    'type': ElementType.MODIFIER_KEY,
    'id': 'ShiftRight',
    'supportSticky': true
  },
  SPACE: {
    'name': ' ',
    'type': ElementType.SPACE_KEY,
    'id': 'Space'
  },
  SWITCHER: {
    'type': ElementType.SWITCHER_KEY
  },
  MENU: {
    'iconCssClass': i18n.input.chrome.inputview.Css.MENU_ICON,
    'type': ElementType.MENU_KEY,
    'id': 'Menu'
  },
  GLOBE: {
    'iconCssClass': i18n.input.chrome.inputview.Css.GLOBE_ICON,
    'type': ElementType.GLOBE_KEY,
    'id': 'Globe'
  },
  HOTROD_SWITCHER: {
    'iconCssClass': i18n.input.chrome.inputview.Css.BACK_TO_KEYBOARD_ICON,
    'type': ElementType.HOTROD_SWITCHER_KEY,
    'id': 'HotrodSwitch'
  }
};


/**
 * The place holder for hint text in accents(more keys).
 *
 * @const
 * @type {string}
 */
i18n.input.chrome.inputview.content.Constants.HINT_TEXT_PLACE_HOLDER =
    '%hinttext%';

});  // goog.scope
