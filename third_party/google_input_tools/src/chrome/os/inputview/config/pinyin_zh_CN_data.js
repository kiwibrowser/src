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
goog.require('i18n.input.chrome.inputview.content.compact.letter');
goog.require('i18n.input.chrome.inputview.content.compact.more');
goog.require('i18n.input.chrome.inputview.content.compact.symbol');
goog.require('i18n.input.chrome.inputview.content.compact.util');
goog.require('i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec');

(function() {
  var keysetSpecNode =
      i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec;
  var letter =
      i18n.input.chrome.inputview.content.compact.letter;
  var symbol =
      i18n.input.chrome.inputview.content.compact.symbol;
  var more =
      i18n.input.chrome.inputview.content.compact.more;
  var letterKeysetSpec = {};
  letterKeysetSpec[keysetSpecNode.ID] = 'pinyin-zh-CN.compact.qwerty';
  letterKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  letterKeysetSpec[keysetSpecNode.DATA] = letter.keyPinyinCharacters();

  var enKeysetSpecNode =
      i18n.input.chrome.inputview.content.compact.util.CompactKeysetSpec;
  var enLetterKeysetSpec = {};
  enLetterKeysetSpec[enKeysetSpecNode.ID] = 'pinyin-zh-CN.en.compact.qwerty';
  enLetterKeysetSpec[enKeysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  enLetterKeysetSpec[enKeysetSpecNode.DATA] = letter.keyEnCharacters();

  var symbolKeysetSpec = {};
  symbolKeysetSpec[keysetSpecNode.ID] = 'pinyin-zh-CN.compact.symbol';
  symbolKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  symbolKeysetSpec[keysetSpecNode.DATA] = symbol.keyPinyinSymbolCharacters();

  var enSymbolKeysetSpec = {};
  enSymbolKeysetSpec[keysetSpecNode.ID] = 'pinyin-zh-CN.en.compact.symbol';
  enSymbolKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  enSymbolKeysetSpec[keysetSpecNode.DATA] = symbol.keyNASymbolCharacters();

  var moreKeysetSpec = {};
  moreKeysetSpec[keysetSpecNode.ID] = 'pinyin-zh-CN.compact.more';
  moreKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  moreKeysetSpec[keysetSpecNode.DATA] = more.keyPinyinMoreCharacters();

  var enMoreKeysetSpec = {};
  enMoreKeysetSpec[keysetSpecNode.ID] = 'pinyin-zh-CN.en.compact.more';
  enMoreKeysetSpec[keysetSpecNode.LAYOUT] = 'compactkbd-qwerty';
  enMoreKeysetSpec[keysetSpecNode.DATA] = more.keyNAMoreCharacters();

  i18n.input.chrome.inputview.content.compact.util.
      generatePinyinCompactKeyboard(
      letterKeysetSpec, enLetterKeysetSpec, symbolKeysetSpec,
      enSymbolKeysetSpec, moreKeysetSpec, enMoreKeysetSpec,
      google.ime.chrome.inputview.onConfigLoaded);
}) ();
