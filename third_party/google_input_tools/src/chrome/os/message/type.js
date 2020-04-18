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
goog.provide('i18n.input.chrome.message');
goog.provide('i18n.input.chrome.message.Type');

goog.require('i18n.input.chrome.message.Source');


goog.scope(function() {
var Source = i18n.input.chrome.message.Source;


/**
 * The message type. "Background->Inputview" don't allow to share the same
 * message type with "Inputview->Background".
 *
 * @enum {string}
 */
i18n.input.chrome.message.Type = {
  // Background -> Inputview
  CANDIDATES_BACK: 'candidates_back',
  CONTEXT_BLUR: 'context_blur',
  CONTEXT_FOCUS: 'context_focus',
  FRONT_TOGGLE_LANGUAGE_STATE: 'front_toggle_language_state',
  GESTURES_BACK: 'gestures_back',
  HWT_NETWORK_ERROR: 'hwt_network_error',
  SURROUNDING_TEXT_CHANGED: 'surrounding_text_changed',
  UPDATE_SETTINGS: 'update_settings',
  VOICE_STATE_CHANGE: 'voice_state_change',

  // Inputview -> Background
  COMMIT_TEXT: 'commit_text',
  COMPLETION: 'completion',
  CONFIRM_GESTURE_RESULT: 'confirm_gesture_result',
  CONNECT: 'connect',
  DATASOURCE_READY: 'datasource_ready',
  DISCONNECT: 'disconnect',
  DOUBLE_CLICK_ON_SPACE_KEY: 'double_click_on_space_key',
  EXEC_ALL: 'exec_all',
  HWT_REQUEST: 'hwt_request',
  KEY_CLICK: 'key_click',
  KEY_EVENT: 'key_event',
  OPTION_CHANGE: 'option_change',
  PREDICTION: 'prediction',
  SELECT_CANDIDATE: 'select_candidate',
  SEND_GESTURE_EVENT: 'send_gesture_event',
  SEND_KEY_EVENT: 'send_key_event',
  SEND_KEYBOARD_LAYOUT: 'send_keyboard_layout',
  SET_COMPOSITION: 'set_composition',
  SET_GESTURE_EDITING: 'set_gesture_editing',
  SET_LANGUAGE: 'set_language',
  SWITCH_KEYSET: 'switch_keyset',
  TOGGLE_LANGUAGE_STATE: 'toggle_language_state',
  VISIBILITY_CHANGE: 'visibility_change',
  SET_CONTROLLER: 'set_controller',
  UNSET_CONTROLLER: 'unset_controller',
  VOICE_VIEW_STATE_CHANGE: 'voice_view_state_change',


  // Inputview -> Elements
  HWT_PRIVACY_GOT_IT: 'hwt_privacy_got_it',
  VOICE_PRIVACY_GOT_IT: 'voice_privacy_got_it',

  // Options -> Background
  USER_DICT_ADD_ENTRY: 'user_dict_add_entry',
  USER_DICT_CLEAR: 'user_dict_clear',
  USER_DICT_LIST: 'user_dict_list',
  USER_DICT_SET_THRESHOLD: 'user_dict_set_threshold',
  USER_DICT_START: 'user_dict_start',
  USER_DICT_STOP: 'user_dict_stop',
  USER_DICT_REMOVE_ENTRY: 'user_dict_remove_entry',

  // Background -> Options
  USER_DICT_ENTRIES: 'user_dict_entries',

  // Background->Background
  HEARTBEAT: 'heart_beat'
};
var Type = i18n.input.chrome.message.Type;


/**
 * Returns whether the message type belong to "Background->Inputview" group;
 *
 * @param {string} type The message type.
 * @return {boolean} .
 */
i18n.input.chrome.message.isFromBackground = function(type) {
  var source = i18n.input.chrome.message.getMessageSource(type);
  return source == Source.BG_BG || source == Source.BG_OP ||
      source == Source.BG_VK;
};


/**
 * Returns whether the message type belong to "Background->Inputview" group;
 *
 * @param {string} type The message type.
 * @return {i18n.input.chrome.message.Source} The source.
 */
i18n.input.chrome.message.getMessageSource = function(type) {
  switch (type) {
    // Background -> Inputview
    case Type.CANDIDATES_BACK:
    case Type.CONTEXT_BLUR:
    case Type.CONTEXT_FOCUS:
    case Type.FRONT_TOGGLE_LANGUAGE_STATE:
    case Type.GESTURES_BACK:
    case Type.HWT_NETWORK_ERROR:
    case Type.SURROUNDING_TEXT_CHANGED:
    case Type.UPDATE_SETTINGS:
    case Type.VOICE_STATE_CHANGE:
      return Source.BG_VK;

    // Inputview -> Background
    case Type.COMMIT_TEXT:
    case Type.COMPLETION:
    case Type.CONFIRM_GESTURE_RESULT:
    case Type.CONNECT:
    case Type.DATASOURCE_READY:
    case Type.DISCONNECT:
    case Type.DOUBLE_CLICK_ON_SPACE_KEY:
    case Type.EXEC_ALL:
    case Type.HWT_REQUEST:
    case Type.KEY_CLICK:
    case Type.KEY_EVENT:
    case Type.OPTION_CHANGE:
    case Type.PREDICTION:
    case Type.SELECT_CANDIDATE:
    case Type.SEND_GESTURE_EVENT:
    case Type.SEND_KEY_EVENT:
    case Type.SEND_KEYBOARD_LAYOUT:
    case Type.SET_COMPOSITION:
    case Type.SET_GESTURE_EDITING:
    case Type.SET_LANGUAGE:
    case Type.SWITCH_KEYSET:
    case Type.TOGGLE_LANGUAGE_STATE:
    case Type.VISIBILITY_CHANGE:
    case Type.SET_CONTROLLER:
    case Type.UNSET_CONTROLLER:
    case Type.VOICE_VIEW_STATE_CHANGE:
      return Source.VK_BG;

    // Inputview -> Elements
    case Type.HWT_PRIVACY_GOT_IT:
    case Type.VOICE_PRIVACY_GOT_IT:
      return Source.VK_VK;

    // Options -> Background
    case Type.USER_DICT_ADD_ENTRY:
    case Type.USER_DICT_CLEAR:
    case Type.USER_DICT_LIST:
    case Type.USER_DICT_SET_THRESHOLD:
    case Type.USER_DICT_START:
    case Type.USER_DICT_STOP:
    case Type.USER_DICT_REMOVE_ENTRY:
      return Source.OP_BG;

    // Background -> Options
    case Type.USER_DICT_ENTRIES:
      return Source.BG_OP;

    // Background->Background
    case Type.HEARTBEAT:
      return Source.BG_BG;
    default:
      return Source.UNKNOWN;
  }
};
});  // goog.scope
