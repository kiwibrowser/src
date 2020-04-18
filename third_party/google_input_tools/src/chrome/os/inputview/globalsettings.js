// Copyright 2014 The Cloud Input Tools Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Defines the global settings variables for ITA.
 */

goog.provide('i18n.input.common.GlobalSettings');

goog.require('goog.positioning.Corner');
goog.require('goog.userAgent');


/**
 * The application name.
 *
 * @type {string}
 */
i18n.input.common.GlobalSettings.ApplicationName = 'jsapi';


/**
 * The help url for the help button on keyboard. If empty string, the help
 * button will not be showed.
 *
 * @type {string}
 */
i18n.input.common.GlobalSettings.KeyboardHelpUrl = '';


/**
 * Whether show min/max button on keyboard.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.KeyboardShowMinMax = false;


/**
 * Whether to show status bar.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.ShowStatusBar = true;


/**
 * Whether to show google logo.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.showGoogleLogo = false;


/**
 * The shortcut key definition for statusbar toggle language command.
 *
 * @type {string}
 */
i18n.input.common.GlobalSettings.StatusBarToggleLanguageShortcut = 'shift';


/**
 * The shortcut key definition for statusbar toggle sbc/dbc command.
 *
 * @type {string}
 */
i18n.input.common.GlobalSettings.StatusBarToggleSbcShortcut = 'shift+space';


/**
 * The shortcut key definition for statusbar punctuation command.
 *
 * @type {string}
 */
i18n.input.common.GlobalSettings.StatusBarPunctuationShortcut = 'ctrl+.';


/**
 * Keyboard default location.
 *
 * @type {!goog.positioning.Corner}
 */
i18n.input.common.GlobalSettings.KeyboardDefaultLocation =
    goog.positioning.Corner.BOTTOM_END;


/**
 * Handwriting panel default location.
 *
 * @type {!goog.positioning.Corner}
 */
i18n.input.common.GlobalSettings.HandwritingDefaultLocation =
    goog.positioning.Corner.BOTTOM_END;


/**
 * Whether is offline mode. If true, IME will be switched to offline and all
 * tracking (server ping, ga, csi, etc.) are disabled.
 * TODO(shuchen): Later we will use this flag to switch to ITA offline decoder.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.isOfflineMode = false;


/**
 * Whether to sends the fake events when input box value is changed.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.canSendFakeEvents = true;


/**
 * No need to register handler in capture phase for IE8.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.canListenInCaptureForIE8 =
    !goog.userAgent.IE || goog.userAgent.isVersionOrHigher(9);


/**
 * The chrome extension settings.
 *
 * @enum {string}
 */
i18n.input.common.GlobalSettings.chromeExtension = {
  ACT_FLAG: 'IS_INPUT_ACTIVE',
  ACTIVE_UI_IFRAME_ID: 'GOOGLE_INPUT_ACTIVE_UI',
  APP_FLAG: 'GOOGLE_INPUT_NON_CHEXT_FLAG',
  CHEXT_FLAG: 'GOOGLE_INPUT_CHEXT_FLAG',
  INPUTTOOL: 'input',
  INPUTTOOL_STAT: 'input_stat',
  STATUS_BAR_IFRAME_ID: 'GOOGLE_INPUT_STATUS_BAR'
};


/**
 * @define {string} The name of the product which uses Google Input Tools API.
 */
i18n.input.common.GlobalSettings.BUILD_SOURCE = 'jsapi';


/**
 * @define {boolean} Whether uses XMLHttpRequest or not.
 */
i18n.input.common.GlobalSettings.ENABLE_XHR = false;


/**
 * Whether enables the statistics for IME's status bar.
 * TODO(shuchen): Investigates how to make sure status bar won't send
 * duplicated metrics requests. And then we can remove this flag.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.enableStatusBarMetrics = false;


/**
 * Whether to show on-screen keyboard.
 * false: Hides on-screen keyboard.
 * true: Shows on-screen keyboard.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.onScreenKeyboard = true;


/**
 * Whether to enable personal dictionary or not.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.enableUserDict = false;


/**
 * Defines the max int value.
 *
 * @type {number}
 */
i18n.input.common.GlobalSettings.MAX_INT = 2147483647;


/**
 * Whether enables the user prefs.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.enableUserPrefs = true;


/**
 * @define {boolean} UI wrapper by iframe.
 */
i18n.input.common.GlobalSettings.IFRAME_WRAPPER = false;


/**
 * The CSS string. When create a new iframe wrapper, need to install css style
 * in the iframe document.
 *
 * @type {string}
 *
 */
i18n.input.common.GlobalSettings.css = '';


/**
 * Alternative image URL in CSS (e.g. Chrome extension needs a local path).
 *
 * @type {string}
 */
i18n.input.common.GlobalSettings.alternativeImageUrl = '';


/**
 * @define {boolean} Whether to enable Voice input.
 */
i18n.input.common.GlobalSettings.enableVoice = false;


/**
 * Whether to enable global event delegate.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.enableGlobalEventDelegate = true;


/**
 * Whether to adapter in mobile devices.
 *
 * @type {boolean}
 */
i18n.input.common.GlobalSettings.mobile = goog.userAgent.MOBILE;


/**
 * Whether to enable free simple easy accents mode.
 */
i18n.input.common.GlobalSettings.simpleEasyAccents = false;

