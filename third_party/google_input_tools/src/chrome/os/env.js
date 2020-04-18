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
goog.provide('i18n.input.chrome.Env');

goog.require('goog.array');
goog.require('i18n.input.chrome.Constant');
goog.require('i18n.input.chrome.FeatureTracker');
goog.require('i18n.input.lang.InputTool');
goog.require('i18n.input.lang.InputToolType');

goog.scope(function() {
var Constant = i18n.input.chrome.Constant;
var FeatureTracker = i18n.input.chrome.FeatureTracker;
var InputTool = i18n.input.lang.InputTool;
var InputToolType = i18n.input.lang.InputToolType;



/**
 * The Environment class which holds some status' of ChromeOS for input methods.
 * e.g. the current input field context, the current input method engine ID,
 * the flags, etc.
 *
 * @constructor
 */
i18n.input.chrome.Env = function() {

  /**
   * Tracker for which FeatureName are enabled.
   *
   * @type {!FeatureTracker};
   */
  this.featureTracker = new FeatureTracker();

  /** @private {!Function} */
  this.compositionBoundsChangedHandler_ =
      this.onBoundsChanged_.bind(this);

  /**
   * The current initial bounds.
   * @type {!BoundSize}
   */
  this.currentBounds = /** @type {!BoundSize} */ ({x: 0, y: 0, w: 0, h: 0});


  /**
   * The current bounds list.
   * @type {!Array.<!BoundSize>}
   */
  this.currentBoundsList = [
    /** @type {!BoundSize} */ ({x: 0, y: 0, w: 0, h: 0})];


  if (chrome.accessibilityFeatures &&
      chrome.accessibilityFeatures.spokenFeedback) {
    chrome.accessibilityFeatures.spokenFeedback.get({}, (function(details) {
      this.isChromeVoxOn = details['value'];
    }).bind(this));

    chrome.accessibilityFeatures.spokenFeedback.onChange.addListener(
        function(details) {
          this.isChromeVoxOn = details['value'];
        }.bind(this));
  }

  if (window.inputview && window.inputview.getKeyboardConfig) {
    window.inputview.getKeyboardConfig((function(config) {
      this.featureTracker.initialize(config);
    }).bind(this));
  }
};
var Env = i18n.input.chrome.Env;
goog.addSingletonGetter(Env);


/** @type {string} */
Env.prototype.engineId = '';


/** @type {InputContext} */
Env.prototype.context = null;


/** @type {string} */
Env.prototype.screenType = 'normal';


/** @type {boolean} */
Env.prototype.isPhysicalKeyboardAutocorrectEnabled = false;


/** @type {string} */
Env.prototype.textBeforeCursor = '';


/**
 * @type {Object.<{text: string, focus: number, anchor: number,
 *     offset: number}>}
 */
Env.prototype.surroundingInfo = null;


/** @type {boolean} */
Env.prototype.isChromeVoxOn = false;


/** @type {boolean} */
Env.prototype.isOnScreenKeyboardShown = false;


/**
 * Handler for onCompositionBoundsChanged event.
 *
 * @param {!BoundSize} bounds The bounds of the composition text.
 * @param {!Array.<!BoundSize>} boundsList The list of bounds of each
 *     composition character.
 * @private
 */
Env.prototype.onBoundsChanged_ = function(bounds, boundsList) {
  this.currentBounds = bounds;
  this.currentBoundsList = boundsList;
};


/**
 * Let Env listen "onCompositionBoundsChanged" event.
 */
Env.prototype.listenCompositionBoundsChanged = function() {
  if (chrome.inputMethodPrivate &&
      chrome.inputMethodPrivate.onCompositionBoundsChanged) {
    chrome.inputMethodPrivate.onCompositionBoundsChanged.addListener(
        this.compositionBoundsChangedHandler_);
  }
};


/**
 * Let Env unlisten "onCompositionBoundsChanged" event.
 */
Env.prototype.unlistenCompositionBoundsChanged = function() {
  if (chrome.inputMethodPrivate &&
      chrome.inputMethodPrivate.onCompositionBoundsChanged) {
    chrome.inputMethodPrivate.onCompositionBoundsChanged.removeListener(
        this.compositionBoundsChangedHandler_);
  }
};


/**
 * Returns whether the XKB engine id supports NACL.
 *
 * @param {string} xkbEngineId The XKB engine Id.
 * @return {boolean} supported or not.
 */
Env.isXkbAndNaclEnabled = function(xkbEngineId) {
  var inputTool = InputTool.get(xkbEngineId);
  return !!inputTool && inputTool.type == InputToolType.XKB &&
      goog.array.contains(Constant.NACL_LANGUAGES, inputTool.languageCode);
};
});  // goog.scope
