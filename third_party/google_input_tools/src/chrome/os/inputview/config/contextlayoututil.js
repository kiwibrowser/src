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
goog.provide('i18n.input.chrome.inputview.content.ContextlayoutUtil');

goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.content.compact.util');
goog.require('i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec');
goog.require('i18n.input.chrome.message.ContextType');

goog.scope(function() {
var ContextType = i18n.input.chrome.message.ContextType;
var util = i18n.input.chrome.inputview.content.ContextlayoutUtil;
var compact = i18n.input.chrome.inputview.content.compact.util;
var keysetSpecNode = compact.CompactKeysetSpec;
var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;


/**
 * Generates custom layouts to be displayed on specified input type contexts.
 *
 *
 * @param {Object.<ContextType, !Object>} inputTypeToKeysetSpecMap Map from
 *     input types to context-specific keysets.
 * @param {!function(!Object): void} onLoaded The function to call once a keyset
 *     data is ready.
 */
util.generateContextLayouts = function(inputTypeToKeysetSpecMap, onLoaded) {
  // Creates context-specific keysets.
  if (inputTypeToKeysetSpecMap) {
    for (var inputType in inputTypeToKeysetSpecMap) {
      var spec = inputTypeToKeysetSpecMap[inputType];
      var data = compact.createCompactData(
          spec, 'compactkbd-k-', 'compactkbd-k-key-');
      data[SpecNodeName.ID] = spec[keysetSpecNode.ID];
      data[SpecNodeName.SHOW_MENU_KEY] = false;
      data[SpecNodeName.NO_SHIFT] = true;
      data[SpecNodeName.ON_CONTEXT] = inputType;
      onLoaded(data);
    }
  }
};

});  // goog.scope
