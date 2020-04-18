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
goog.provide('i18n.input.chrome.inputview.Settings');


goog.scope(function() {



/**
 * The settings.
 *
 * @constructor
 */
i18n.input.chrome.inputview.Settings = function() {};
var Settings = i18n.input.chrome.inputview.Settings;


/**
 * True to always render the altgr character in the soft key.
 *
 * @type {boolean}
 */
Settings.prototype.alwaysRenderAltGrCharacter = false;


/** @type {boolean} */
Settings.prototype.autoSpace = false;


/** @type {boolean} */
Settings.prototype.autoCapital = false;


/** @type {boolean} */
Settings.prototype.autoCorrection = false;


/** @type {boolean} */
Settings.prototype.enableLongPress = true;


/** @type {boolean} */
Settings.prototype.doubleSpacePeriod = false;


/** @type {boolean} */
Settings.prototype.soundOnKeypress = false;


/** @type {boolean} */
Settings.prototype.gestureEditing = false;


/** @type {boolean} */
Settings.prototype.gestureTyping = false;


/**
 * The flag to control whether candidates naviagation feature is enabled.
 *
 * @type {boolean}
 */
Settings.prototype.candidatesNavigation = false;


/**
 * Saves the preferences.
 *
 * @param {string} preference The name of the preference.
 * @param {*} value The preference value.
 */
Settings.prototype.savePreference = function(preference, value) {
  window.localStorage.setItem(preference, /** @type {string} */(value));
};


/**
 * Gets the preference value.
 *
 * @param {string} preference The name of the preference.
 * @return {*} The value.
 */
Settings.prototype.getPreference = function(preference) {
  return window.localStorage.getItem(preference);
};

});  // goog.scope

