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
goog.provide('i18n.input.chrome.MessageKey');
goog.provide('i18n.input.chrome.TriggerType');


/**
 * The message keys for communicating with the native client.
 *
 * @enum {string}
 */
i18n.input.chrome.MessageKey = {
  APPEND: 'append',
  APPEND_TOKENS: 'append_tokens',
  CLEAR: 'clear',
  COMMIT: 'commit',
  COMMIT_MARK: '@',
  COMMIT_POS: 'commit_pos',
  COMMIT_WORD: 'commit_word',
  CONTEXT: 'context',
  CORRECTION: 'correction',
  DECODE_GESTURE: 'decode_gesture',
  DELETE: 'delete',
  ENABLE_USER_DICT: 'enable_user_dict',
  ENTRIES: 'entries',
  FREQUENCY: 'frequency',
  FUZZY_PAIRS: 'fuzzy_pairs',
  KEYBOARD_LAYOUT: 'keyboard_layout',
  HIGHLIGHT: 'highlight',
  HIGHLIGHT_INDEX: 'highlight_index',
  IME: 'itc',
  MIN_SCORE: 'min_score',
  MULTI: 'multi',
  MULTI_APPEND: 'multi_append',
  PRECEDING_TEXT: 'preceding_text',
  PREDICT: 'predict',
  REVERT: 'revert',
  SELECT: 'select',
  SELECT_HIGHLIGHT: 'select_highlight',
  SOURCE: 'source',
  TARGET: 'target',
  UPDATE_ALL: 'update_all',
  USER_DICT: 'user_dict'
};


/**
 * The trigger types for committing text.
 *
 * @enum {number}
 */
i18n.input.chrome.TriggerType = {
  SPACE: 0,
  RESET: 1,
  CANDIDATE: 2,
  SYMBOL_OR_NUMBER: 3,
  DOUBLE_SPACE_TO_PERIOD: 4,
  REVERT: 5,
  VOICE: 6,
  COMPOSITION_DISABLED: 7,
  GESTURE: 8,
  UNKNOWN: -1
};
