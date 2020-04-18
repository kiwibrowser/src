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
goog.provide('i18n.input.chrome.inputview.M17nModel');

goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.content.util');
goog.require('i18n.input.chrome.inputview.events.ConfigLoadedEvent');
goog.require('i18n.input.chrome.vk.KeyCode');
goog.require('i18n.input.chrome.vk.Model');



/**
 * The model to legacy cloud vk configuration.
 *
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.inputview.M17nModel = function() {
  goog.base(this);

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.handler_ = new goog.events.EventHandler(this);

  /**
   * The model for cloud vk.
   *
   * @type {!i18n.input.chrome.vk.Model}
   * @private
   */
  this.model_ = new i18n.input.chrome.vk.Model();
  this.handler_.listen(this.model_,
      i18n.input.chrome.vk.EventType.LAYOUT_LOADED,
      this.onLayoutLoaded_);
};
goog.inherits(i18n.input.chrome.inputview.M17nModel,
    goog.events.EventTarget);


/**
 * The active layout view.
 *
 * @type {!i18n.input.chrome.vk.ParsedLayout}
 * @private
 */
i18n.input.chrome.inputview.M17nModel.prototype.layoutView_;


/**
 * Loads the configuration.
 *
 * @param {string} lang The m17n keyboard layout code (with 'm17n:' prefix).
 */
i18n.input.chrome.inputview.M17nModel.prototype.loadConfig = function(lang) {
  var m17nMatches = lang.match(/^m17n:(.*)/);
  if (m17nMatches && m17nMatches[1]) {
    this.model_.loadLayout(m17nMatches[1]);
  }
};


/**
 * Callback when legacy model is loaded.
 *
 * @param {!i18n.input.chrome.vk.LayoutEvent} e The event.
 * @private
 */
i18n.input.chrome.inputview.M17nModel.prototype.onLayoutLoaded_ = function(
    e) {
  var layoutView = /** @type {!i18n.input.chrome.vk.ParsedLayout} */
      (e.layoutView);
  this.layoutView_ = layoutView;
  var is102 = layoutView.view.is102;
  var codes = is102 ? i18n.input.chrome.vk.KeyCode.CODES102 :
      i18n.input.chrome.vk.KeyCode.CODES101;
  var keyCount = is102 ? 48 : 47;
  var keyCharacters = [];
  for (var i = 0; i < keyCount; i++) {
    var characters = this.findCharacters_(layoutView.view.mappings,
        codes[i]);
    keyCharacters.push(characters);
  }
  keyCharacters.push(['\u0020', '\u0020']);
  var hasAltGrKey = !!layoutView.view.mappings['c'] &&
      layoutView.view.mappings['c'] != layoutView.view.mappings[''];
  var skvPrefix = is102 ? '102kbd-k-' : '101kbd-k-';
  var skPrefix = layoutView.view.id + '-k-';
  var data = i18n.input.chrome.inputview.content.util.createData(keyCharacters,
      skvPrefix, is102, hasAltGrKey);
  if (data) {
    data[i18n.input.chrome.inputview.SpecNodeName.TITLE] =
        layoutView.view.title;
    data[i18n.input.chrome.inputview.SpecNodeName.ID] =
        'm17n:' + e.layoutCode;
    this.dispatchEvent(new i18n.input.chrome.inputview.events.
        ConfigLoadedEvent(data));
  }
};


/**
 * Finds out the characters for the key.
 *
 * @param {!Object} mappings The mappings.
 * @param {string} code The code.
 * @return {!Array.<string>} The characters for the code.
 * @private
 */
i18n.input.chrome.inputview.M17nModel.prototype.findCharacters_ = function(
    mappings, code) {
  var characters = [];
  var states = [
    '',
    's',
    'c',
    'sc',
    'l',
    'sl',
    'cl',
    'scl'
  ];
  for (var i = 0; i < states.length; i++) {
    if (mappings[states[i]] && mappings[states[i]][code]) {
      characters[i] = mappings[states[i]][code][1];
    } else if (code == '\u0020') {
      characters[i] = '\u0020';
    } else {
      characters[i] = '';
    }
  }
  return characters;
};


/** @override */
i18n.input.chrome.inputview.M17nModel.prototype.disposeInternal = function() {
  goog.dispose(this.handler_);

  goog.base(this, 'disposeInternal');
};
