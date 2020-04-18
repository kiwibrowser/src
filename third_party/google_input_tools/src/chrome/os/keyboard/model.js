// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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

/**
 * @fileoverview Definition of Model class.
 *     It is responsible for dynamically loading the layout JS files. It
 *     interprets the layout info and provides the function of getting
 *     transformed chars and recording history states to Model.
 *     It notifies View via events when layout info changes.
 *     This is the Model of MVC pattern.
 */

goog.provide('i18n.input.chrome.vk.Model');

goog.require('goog.events.EventTarget');
goog.require('goog.net.jsloader');
goog.require('goog.object');
goog.require('goog.string');
goog.require('i18n.input.chrome.vk.EventType');
goog.require('i18n.input.chrome.vk.LayoutEvent');
goog.require('i18n.input.chrome.vk.ParsedLayout');



/**
 * Creates the Model object.
 *
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.vk.Model = function() {
  goog.base(this);

  /**
   * The registered layouts object.
   * Its format is {<layout code>: <parsed layout obj>}.
   *
   * @type {!Object.<!i18n.input.chrome.vk.ParsedLayout|boolean>}
   * @private
   */
  this.layouts_ = {};

  /**
   * The active layout code.
   *
   * @type {string}
   * @private
   */
  this.activeLayout_ = '';

  /**
   * The layout code of which the layout is "being activated" when the layout
   * hasn't been loaded yet.
   *
   * @type {string}
   * @private
   */
  this.delayActiveLayout_ = '';

  /**
   * History state used for ambiguous transforms.
   *
   * @type {!Object}
   * @private
   */
  this.historyState_ = {
    previous: {text: '', transat: -1},
    ambi: '',
    current: {text: '', transat: -1}
  };

  // Exponses the onLayoutLoaded so that the layout JS can call it back.
  goog.exportSymbol('cros_vk_loadme', goog.bind(this.onLayoutLoaded_, this));
};
goog.inherits(i18n.input.chrome.vk.Model, goog.events.EventTarget);


/**
 * Loads the layout in the background.
 *
 * @param {string} layoutCode The layout will be loaded.
 */
i18n.input.chrome.vk.Model.prototype.loadLayout = function(layoutCode) {
  if (!layoutCode) return;

  var parsedLayout = this.layouts_[layoutCode];
  // The layout is undefined means not loaded, false means loading.
  if (parsedLayout == undefined) {
    this.layouts_[layoutCode] = false;
    i18n.input.chrome.vk.Model.loadLayoutScript_(layoutCode);
  } else if (parsedLayout) {
    this.dispatchEvent(new i18n.input.chrome.vk.LayoutEvent(
        i18n.input.chrome.vk.EventType.LAYOUT_LOADED,
        /** @type {!Object} */ (parsedLayout)));
  }
};


/**
 * Activate layout by setting the current layout.
 *
 * @param {string} layoutCode The layout will be set as current layout.
 */
i18n.input.chrome.vk.Model.prototype.activateLayout = function(
    layoutCode) {
  if (!layoutCode) return;

  if (this.activeLayout_ != layoutCode) {
    var parsedLayout = this.layouts_[layoutCode];
    if (parsedLayout) {
      this.activeLayout_ = layoutCode;
      this.delayActiveLayout_ = '';
      this.clearHistory();
    } else if (parsedLayout == false) { // Layout being loaded?
      this.delayActiveLayout_ = layoutCode;
    }
  }
};


/**
 * Gets the current layout.
 *
 * @return {string} The current layout code.
 */
i18n.input.chrome.vk.Model.prototype.getCurrentLayout = function() {
  return this.activeLayout_;
};


/**
 * Predicts whether there would be future transforms for the history text.
 *
 * @return {number} The matched position. Returns -1 for no match.
 */
i18n.input.chrome.vk.Model.prototype.predictHistory = function() {
  if (!this.activeLayout_ || !this.layouts_[this.activeLayout_]) {
    return -1;
  }
  var parsedLayout = this.layouts_[this.activeLayout_];
  var history = this.historyState_;
  var text, transat;
  if (history.ambi) {
    text = history.previous.text;
    transat = history.previous.transat;
    // Tries to predict transform for previous history.
    if (transat > 0) {
      text = text.slice(0, transat) + '\u001d' + text.slice(transat) +
          history.ambi;
    } else {
      text += history.ambi;
    }
    if (parsedLayout.predictTransform(text) >= 0) {
      // If matched previous history, always return 0 because outside will use
      // this to keep the composition text.
      return 0;
    }
  }
  // Tries to predict transform for current history.
  text = history.current.text;
  transat = history.current.transat;
  if (transat >= 0) {
    text = text.slice(0, transat) + '\u001d' + text.slice(transat);
  }
  var pos = parsedLayout.predictTransform(text);
  if (transat >= 0 && pos > transat) {
    // Adjusts the pos for removing the temporary \u001d character.
    pos--;
  }
  return pos;
};


/**
 * Translates the key code into the chars to put into the active input box.
 *
 * @param {string} chars The key commit chars.
 * @param {string} charsBeforeCaret The chars before the caret in the active
 *     input box. This will be used to compare with the history states.
 * @return {Object} The replace chars object whose 'back' means delete how many
 *     chars back from the caret, and 'chars' means the string insert after the
 *     deletion. Returns null if no result.
 */
i18n.input.chrome.vk.Model.prototype.translate = function(
    chars, charsBeforeCaret) {
  if (!this.activeLayout_ || !chars) {
    return null;
  }
  var parsedLayout = this.layouts_[this.activeLayout_];
  if (!parsedLayout) {
    return null;
  }

  this.matchHistory_(charsBeforeCaret);
  var result, history = this.historyState_;
  if (history.ambi) {
    // If ambi is not empty, it means some ambi chars has been typed
    // before. e.g. ka->k, kaa->K, typed 'ka', and now typing 'a':
    //   history.previous == 'k',1
    //   history.current == 'k',1
    //   history.ambi == 'a'
    // So now we should get transform of 'k\u001d' + 'aa'.
    result = parsedLayout.transform(
        history.previous.text, history.previous.transat,
        history.ambi + chars);
    // Note: result.back could be negative number. In such case, we should give
    // up the transform result. This is to be compatible the old vk behaviors.
    if (result && result.back < 0) {
      result = null;
    }
  }
  if (result) {
    // Because the result is related to previous history, adjust the result so
    // that it is related to current history.
    var prev = history.previous.text;
    prev = prev.slice(0, prev.length - result.back);
    prev += result.chars;
    result.back = history.current.text.length;
    result.chars = prev;
  } else {
    // If no ambi chars or no transforms for ambi chars, try to match the
    // regular transforms. In above case, if now typing 'b', we should get
    // transform of 'k\u001d' + 'b'.
    result = parsedLayout.transform(
        history.current.text, history.current.transat, chars);
  }
  // Updates the history state.
  if (parsedLayout.isAmbiChars(history.ambi + chars)) {
    if (!history.ambi) {
      // Empty ambi means chars should be the first ambi chars.
      // So now we should set the previous.
      history.previous = goog.object.clone(history.current);
    }
    history.ambi += chars;
  } else if (parsedLayout.isAmbiChars(chars)) {
    // chars could match ambi regex when ambi+chars cannot.
    // In this case, record the current history to previous, and set ambi as
    // chars.
    history.previous = goog.object.clone(history.current);
    history.ambi = chars;
  } else {
    history.previous.text = '';
    history.previous.transat = -1;
    history.ambi = '';
  }
  // Updates the history text per transform result.
  var text = history.current.text;
  var transat = history.current.transat;
  if (result) {
    text = text.slice(0, text.length - result.back);
    text += result.chars;
    transat = text.length;
  } else {
    text += chars;
    // This function doesn't return null. So if result is null, fill it.
    result = {back: 0, chars: chars};
  }
  // The history text cannot cannot contain SPACE!
  var spacePos = text.lastIndexOf(' ');
  if (spacePos >= 0) {
    text = text.slice(spacePos + 1);
    if (transat > spacePos) {
      transat -= spacePos + 1;
    } else {
      transat = -1;
    }
  }
  history.current.text = text;
  history.current.transat = transat;

  return result;
};


/**
 * Wether the active layout has transforms defined.
 *
 * @return {boolean} True if transforms defined, false otherwise.
 */
i18n.input.chrome.vk.Model.prototype.hasTransforms = function() {
  var parsedLayout = this.layouts_[this.activeLayout_];
  return !!parsedLayout && !!parsedLayout.transforms;
};


/**
 * Processes the backspace key. It affects the history state.
 *
 * @param {string} charsBeforeCaret The chars before the caret in the active
 *     input box. This will be used to compare with the history states.
 */
i18n.input.chrome.vk.Model.prototype.processBackspace = function(
    charsBeforeCaret) {
  this.matchHistory_(charsBeforeCaret);

  var history = this.historyState_;
  // Reverts the current history. If the backspace across over the transat pos,
  // clean it up.
  var text = history.current.text;
  if (text) {
    text = text.slice(0, text.length - 1);
    history.current.text = text;
    if (history.current.transat > text.length) {
      history.current.transat = text.length;
    }

    text = history.ambi;
    if (text) { // If there is ambi text, remove the last char in ambi.
      history.ambi = text.slice(0, text.length - 1);
    }
    // Prev history only exists when ambi is not empty.
    if (!history.ambi) {
      history.previous = {text: '', transat: -1};
    }
  } else {
    // Cleans up the previous history.
    history.previous = {text: '', transat: -1};
    history.ambi = '';
    // Cleans up the current history.
    history.current = goog.object.clone(history.previous);
  }
};


/**
 * Callback when layout loaded.
 *
 * @param {!Object} layout The layout object passed from the layout JS's loadme
 *     callback.
 * @private
 */
i18n.input.chrome.vk.Model.prototype.onLayoutLoaded_ = function(layout) {
  var parsedLayout = new i18n.input.chrome.vk.ParsedLayout(layout);
  if (parsedLayout.id) {
    this.layouts_[parsedLayout.id] = parsedLayout;
  }
  if (this.delayActiveLayout_ == layout.id) {
    this.activateLayout(this.delayActiveLayout_);
    this.delayActiveLayout_ = '';
  }
  this.dispatchEvent(new i18n.input.chrome.vk.LayoutEvent(
      i18n.input.chrome.vk.EventType.LAYOUT_LOADED, parsedLayout));
};


/**
 * Matches the given text to the last transformed text. Clears history if they
 * are not matched.
 *
 * @param {string} text The text to be matched.
 * @private
 */
i18n.input.chrome.vk.Model.prototype.matchHistory_ = function(text) {
  var hisText = this.historyState_.current.text;
  if (!hisText || !text || !(goog.string.endsWith(text, hisText) ||
      goog.string.endsWith(hisText, text))) {
    this.clearHistory();
  }
};


/**
 * Clears the history state.
 */
i18n.input.chrome.vk.Model.prototype.clearHistory = function() {
  this.historyState_.ambi = '';
  this.historyState_.previous = {text: '', transat: -1};
  this.historyState_.current = goog.object.clone(this.historyState_.previous);
};


/**
 * Prunes the history state to remove a number of chars at beginning.
 *
 * @param {number} count The count of chars to be removed.
 */
i18n.input.chrome.vk.Model.prototype.pruneHistory = function(count) {
  var pruneFunc = function(his) {
    his.text = his.text.slice(count);
    if (his.transat > 0) {
      his.transat -= count;
      if (his.transat <= 0) {
        his.transat = -1;
      }
    }
  };
  pruneFunc(this.historyState_.previous);
  pruneFunc(this.historyState_.current);
};


/**
 * Loads the script for a layout.
 *
 * @param {string} layoutCode The layout code.
 * @private
 */
i18n.input.chrome.vk.Model.loadLayoutScript_ = function(layoutCode) {
  goog.net.jsloader.load('layouts/' + layoutCode + '.js');
};
