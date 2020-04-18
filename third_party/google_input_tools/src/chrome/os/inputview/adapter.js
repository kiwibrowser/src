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
goog.provide('i18n.input.chrome.inputview.Adapter');

goog.require('goog.events.Event');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('goog.events.EventType');
goog.require('goog.object');
goog.require('i18n.input.chrome.DataSource');
goog.require('i18n.input.chrome.FeatureName');
goog.require('i18n.input.chrome.FeatureTracker');
goog.require('i18n.input.chrome.inputview.ReadyState');
goog.require('i18n.input.chrome.inputview.StateType');
goog.require('i18n.input.chrome.inputview.events.EventType');
goog.require('i18n.input.chrome.inputview.events.MessageEvent');
goog.require('i18n.input.chrome.inputview.events.SurroundingTextChangedEvent');
goog.require('i18n.input.chrome.message');
goog.require('i18n.input.chrome.message.ContextType');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');

goog.scope(function() {
var CandidatesBackEvent = i18n.input.chrome.DataSource.CandidatesBackEvent;
var ContextType = i18n.input.chrome.message.ContextType;
var FeatureTracker = i18n.input.chrome.FeatureTracker;
var FeatureName = i18n.input.chrome.FeatureName;
var GesturesBackEvent = i18n.input.chrome.DataSource.GesturesBackEvent;
var Name = i18n.input.chrome.message.Name;
var State = i18n.input.chrome.inputview.ReadyState.State;
var Type = i18n.input.chrome.message.Type;
var events = i18n.input.chrome.inputview.events;



/**
 * The adapter for interview.
 *
 * @param {!i18n.input.chrome.inputview.ReadyState} readyState .
 * @extends {goog.events.EventTarget}
 * @constructor
 */
i18n.input.chrome.inputview.Adapter = function(readyState) {
  goog.base(this);

  /**
   * Whether the keyboard is visible.
   *
   * @type {boolean}
   */
  this.isVisible = !document.webkitHidden;

  /**
   * The modifier state map.
   *
   * @type {!Object.<i18n.input.chrome.inputview.StateType, boolean>}
   * @private
   */
  this.modifierState_ = {};


  /**
   * Tracker for which FeatureName are enabled.
   *
   * @type {!FeatureTracker};
   */
  this.features = new FeatureTracker();


  /**
   * The system ready state.
   *
   * @private {!i18n.input.chrome.inputview.ReadyState}
   */
  this.readyState_ = readyState;

  chrome.runtime.onMessage.addListener(this.onMessage_.bind(this));

  /** @private {!goog.events.EventHandler} */
  this.handler_ = new goog.events.EventHandler(this);
  this.handler_.
      listen(document, 'webkitvisibilitychange', this.onVisibilityChange_).
      // When screen rotate, will trigger resize event.
      listen(window, goog.events.EventType.RESIZE, this.onVisibilityChange_);

  // Notifies the initial visibility change message to background.
  this.onVisibilityChange_();
};
goog.inherits(i18n.input.chrome.inputview.Adapter,
    goog.events.EventTarget);
var Adapter = i18n.input.chrome.inputview.Adapter;


/**
 * URL prefixes of common Google sites.
 *
 * @enum {string}
 */
Adapter.GoogleSites = {
  // TODO: Add support for spreadsheets.
  DOCS: 'https://docs.google.com/document/d',
  MAIL: 'https://mail.google.com'
};


/**
 * The minimal inches of display which enables floating virtual keyboard.
 *
 * @private {number}
 */
Adapter.ENABLE_FLOATING_THRESHOLD_INCHES_ = 15;


/** @type {boolean} */
Adapter.prototype.isA11yMode = false;


/** @type {boolean} */
Adapter.prototype.isHotrod = false;


/** @type {boolean} */
Adapter.prototype.isVoiceInputEnabled = true;


/** @type {boolean} */
Adapter.prototype.showGlobeKey = false;


/** @type {number} */
Adapter.prototype.displayInInches = 0;


/** @type {string} */
Adapter.prototype.contextType = ContextType.DEFAULT;


/** @type {string} */
Adapter.prototype.screen = '';


/** @type {boolean} */
Adapter.prototype.isChromeVoxOn = false;


/** @type {string} */
Adapter.prototype.textBeforeCursor = '';


/**
  * Whether the background controller is on switching.
  *
  * @private {boolean}
  */
Adapter.prototype.isBgControllerSwitching_ = false;


/**
 * Callback for updating settings.
 *
 * @param {!Object} message .
 * @private
 */
Adapter.prototype.onUpdateSettings_ = function(message) {
  this.screen = message[Name.SCREEN];
  this.queryCurrentSite();
  this.contextType = /** @type {string} */ (message[Name.CONTEXT_TYPE]);
  // Resets the flag, since when inputview receive the update setting response,
  // it means the background switching is done.
  this.isBgControllerSwitching_ = false;
  this.dispatchEvent(
      new events.MessageEvent(events.EventType.UPDATE_SETTINGS, message));
};


/**
 * Sets the modifier states.
 *
 * @param {i18n.input.chrome.inputview.StateType} stateType .
 * @param {boolean} enable True to enable the state, false otherwise.
 */
Adapter.prototype.setModifierState = function(stateType, enable) {
  this.modifierState_[stateType] = enable;
};


/**
 * Clears the modifier states.
 */
Adapter.prototype.clearModifierStates = function() {
  this.modifierState_ = {};
};


/**
 * Simulates to send 'keydown' and 'keyup' event.
 *
 * @param {string} key
 * @param {string} code
 * @param {number=} opt_keyCode The key code.
 * @param {!Object=} opt_spatialData .
 * @param {!Object.<{ctrl: boolean, shift: boolean}>=} opt_modifiers .
 */
Adapter.prototype.sendKeyDownAndUpEvent = function(key, code, opt_keyCode,
    opt_spatialData, opt_modifiers) {
  this.sendKeyEvent_([
    this.generateKeyboardEvent_(
        goog.events.EventType.KEYDOWN,
        key,
        code,
        opt_keyCode,
        opt_spatialData,
        opt_modifiers),
    this.generateKeyboardEvent_(
        goog.events.EventType.KEYUP,
        key,
        code,
        opt_keyCode,
        opt_spatialData,
        opt_modifiers)
  ]);
};


/**
 * Simulates to send 'keydown' event.
 *
 * @param {string} key
 * @param {string} code
 * @param {number=} opt_keyCode The key code.
 * @param {!Object=} opt_spatialData .
 */
Adapter.prototype.sendKeyDownEvent = function(key, code, opt_keyCode,
    opt_spatialData) {
  this.sendKeyEvent_([this.generateKeyboardEvent_(
      goog.events.EventType.KEYDOWN, key, code, opt_keyCode,
      opt_spatialData)]);
};


/**
 * Sets whether gesture editing is in progress.
 *
 * @param {boolean} inProgress
 * @param {?boolean=} opt_isSwipe Whether it was triggered by a swipe.
 */
Adapter.prototype.setGestureEditingInProgress = function(inProgress,
    opt_isSwipe) {
  chrome.runtime.sendMessage(
      goog.object.create(
          Name.TYPE, Type.SET_GESTURE_EDITING,
          Name.IN_PROGRESS, inProgress,
          Name.IS_SWIPE, opt_isSwipe));
};


/**
 * Sends a gesture typing event to the backend for decoding.
 *
 * @param {!Array.<!i18n.input.chrome.inputview.elements.content.Point>}
 *     gestureData The gesture data (trail) to send.
 */
Adapter.prototype.sendGestureEvent = function(gestureData) {
  chrome.runtime.sendMessage(
      goog.object.create(Name.TYPE, Type.SEND_GESTURE_EVENT, Name.GESTURE_DATA,
          gestureData));
};


/**
 * Simulates to send 'keyup' event.
 *
 * @param {string} key
 * @param {string} code
 * @param {number=} opt_keyCode The key code.
 * @param {!Object=} opt_spatialData .
 */
Adapter.prototype.sendKeyUpEvent = function(key, code, opt_keyCode,
    opt_spatialData) {
  this.sendKeyEvent_([this.generateKeyboardEvent_(
      goog.events.EventType.KEYUP, key, code, opt_keyCode, opt_spatialData)]);
};


/**
 * Use {@code chrome.input.ime.sendKeyEvents} to simulate key events.
 *
 * @param {!Array.<!Object.<string, string|boolean>>} keyData .
 * @private
 */
Adapter.prototype.sendKeyEvent_ = function(keyData) {
  chrome.runtime.sendMessage(
      goog.object.create(Name.TYPE, Type.SEND_KEY_EVENT, Name.KEY_DATA,
          keyData));
};


/**
 * Sends an updated keyboard layout to the backend gesture typing decoder.
 *
 * @param {?Object} keyboardLayout The keyboard layout object to send.
 */
Adapter.prototype.sendKeyboardLayout = function(keyboardLayout) {
  chrome.runtime.sendMessage(
      goog.object.create(Name.TYPE, Type.SEND_KEYBOARD_LAYOUT,
          Name.KEYBOARD_LAYOUT, keyboardLayout));
};


/**
 * Generates a {@code ChromeKeyboardEvent} by given values.
 *
 * @param {string} type .
 * @param {string} key The key.
 * @param {string} code The code.
 * @param {number=} opt_keyCode The key code.
 * @param {!Object=} opt_spatialData .
 * @param {!Object.<{ctrl: boolean, shift: boolean}>=} opt_modifiers .
 * @return {!Object.<string, string|boolean>}
 * @private
 */
Adapter.prototype.generateKeyboardEvent_ = function(
    type, key, code, opt_keyCode, opt_spatialData, opt_modifiers) {
  var StateType = i18n.input.chrome.inputview.StateType;
  var ctrl = !!this.modifierState_[StateType.CTRL];
  var alt = !!this.modifierState_[StateType.ALT];
  var shift = !!this.modifierState_[StateType.SHIFT];

  if (opt_modifiers) {
    if (opt_modifiers.ctrl != undefined)
      ctrl = opt_modifiers.ctrl;
    if (opt_modifiers.shift != undefined)
      shift = opt_modifiers.shift;
  }

  if (ctrl || alt) {
    key = '';
  }
  var result = {
    'type': type,
    'key': key,
    'code': code,
    'keyCode': opt_keyCode || 0,
    'spatialData': opt_spatialData
  };

  result['altKey'] = alt;
  result['ctrlKey'] = ctrl;
  result['shiftKey'] = shift;
  result['capsLock'] = !!this.modifierState_[StateType.CAPSLOCK];

  return result;
};


/**
 * Callback when surrounding text is changed.
 *
 * @param {string} textBeforeCursor .
 * @param {number} anchor .
 * @param {number} focus .
 * @param {number} offset .
 * @private
 */
Adapter.prototype.onSurroundingTextChanged_ = function(textBeforeCursor,
    anchor, focus, offset) {
  this.textBeforeCursor = textBeforeCursor;
  this.dispatchEvent(new i18n.input.chrome.inputview.events.
      SurroundingTextChangedEvent(textBeforeCursor, anchor, focus,
          offset));
};


/**
 * Gets the context.
 *
 * @return {string} .
 */
Adapter.prototype.getContext = function() {
  var matches = this.textBeforeCursor.match(/([a-zA-Z'-Ḁ-ỹÀ-ȳ]+)\s+$/);
  var text = matches ? matches[1] : '';
  return text;
};


/**
 * Sends request for handwriting.
 *
 * @param {!Object} payload .
 */
Adapter.prototype.sendHwtRequest = function(payload) {
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.HWT_REQUEST, Name.MSG, payload
      ));
};


/**
 * True if it is a password box.
 *
 * @return {boolean} .
 */
Adapter.prototype.isPasswordBox = function() {
  return this.contextType == 'password';
};


/**
 * Whether the floating virtual keyboard feature is enabled.
 *
 * @return {boolean}
 */
Adapter.prototype.isFloatingVirtualKeyboardEnabled = function() {
  // This feature depends on setMode API. The api is a private API and may not
  // be available all the time.
  if (!inputview || !inputview.setMode) {
    return false;
  }

  if (!this.readyState_.isReady(State.DISPLAY_SIZE_READY))
    console.error('Display size is not ready yet.');

  // Disable floating virtual keyboard if the screen is smaller than
  // ENABLE_FLOATING_THRESHOLD_PX.
  if (this.displayInInches < Adapter.ENABLE_FLOATING_THRESHOLD_INCHES_) {
    return false;
  }
  return this.features.isEnabled(FeatureName.FLOATING_VIRTUAL_KEYBOARD);
};


/**
 * Callback when blurs in the context.
 *
 * @private
 */
Adapter.prototype.onContextBlur_ = function() {
  this.contextType = '';
  this.dispatchEvent(new goog.events.Event(i18n.input.chrome.inputview.events.
      EventType.CONTEXT_BLUR));
};


/**
 * Asynchronously queries the current site.
 */
Adapter.prototype.queryCurrentSite = function() {
  var adapter = this;
  var criteria = {'active': true, 'lastFocusedWindow': true};
  if (chrome && chrome.tabs) {
    chrome.tabs.query(criteria, function(tabs) {
      tabs[0] && adapter.setCurrentSite_(tabs[0].url);
    });
  }
};


/**
 * Sets the current context URL.
 *
 * @param {string} url .
 * @private
 */
Adapter.prototype.setCurrentSite_ = function(url) {
  if (url != this.currentSite_) {
    this.currentSite_ = url;
    this.dispatchEvent(new goog.events.Event(
        i18n.input.chrome.inputview.events.EventType.URL_CHANGED));
  }
};


/**
 * Returns whether the current context is Google Documents.
 *
 * @return {boolean} .
 */
Adapter.prototype.isGoogleDocument = function() {
  return this.currentSite_ &&
      this.currentSite_.lastIndexOf(Adapter.GoogleSites.DOCS) === 0;
};


/**
 * Returns whether the current context is Google Mail.
 *
 * @return {boolean}
 */
Adapter.prototype.isGoogleMail = function() {
  return this.currentSite_ &&
      this.currentSite_.lastIndexOf(Adapter.GoogleSites.MAIL) === 0;
};


/**
 * Callback when focus on a context.
 *
 * @param {!Object<string, *>} message .
 * @private
 */
Adapter.prototype.onContextFocus_ = function(message) {
  // URL might have changed.
  this.queryCurrentSite();

  this.contextType = /** @type {string} */ (message[Name.CONTEXT_TYPE]);
  this.dispatchEvent(new goog.events.Event(
      i18n.input.chrome.inputview.events.EventType.CONTEXT_FOCUS));
};


/**
 * Initializes the communication to background page.
 *
 * @private
 */
Adapter.prototype.initBackground_ = function() {
  chrome.runtime.getBackgroundPage((function() {
    this.isBgControllerSwitching_ = true;
    chrome.runtime.sendMessage(goog.object.create(
        Name.TYPE, Type.CONNECT,
        Name.VISIBILITY, this.isVisible));
  }).bind(this));
};


/**
 * Loads the keyboard settings.
 */
Adapter.prototype.initialize = function() {
  if (chrome.accessibilityFeatures &&
      chrome.accessibilityFeatures.spokenFeedback) {
    chrome.accessibilityFeatures.spokenFeedback.get({}, (function(details) {
      this.isChromeVoxOn = details['value'];
    }).bind(this));
    chrome.accessibilityFeatures.spokenFeedback.onChange.addListener((function(
        details) {
          if (!this.isChromeVoxOn && details['value']) {
            this.dispatchEvent(new goog.events.Event(
                i18n.input.chrome.inputview.events.EventType.REFRESH));
          }
          this.isChromeVoxOn = details['value'];
        }).bind(this));
  }

  this.initBackground_();

  if (window.inputview) {
    inputview.getKeyboardConfig((function(config) {
      this.isA11yMode = !!config['a11ymode'];
      this.isHotrod = !!config['hotrodmode'];
      this.features.initialize(config);
      this.isVoiceInputEnabled =
          this.features.isEnabled(FeatureName.VOICE_INPUT);
      this.readyState_.markStateReady(State.KEYBOARD_CONFIG_READY);
      this.maybeDispatchSettingsReadyEvent_();
    }).bind(this));
    inputview.getInputMethods((function(inputMethods) {
      // Globe key is disabled if either condition is met:
      // - Only one IME presents.
      // - In hotrod mode.
      this.showGlobeKey = inputMethods.length > 1 && !this.isHotrod;
      this.readyState_.markStateReady(State.IME_LIST_READY);
      this.maybeDispatchSettingsReadyEvent_();
    }).bind(this));
    inputview.getDisplayInInches((function(inches) {
      this.displayInInches = inches;
      this.readyState_.markStateReady(State.DISPLAY_SIZE_READY);
      this.maybeDispatchSettingsReadyEvent_();
    }).bind(this));
  } else {
    this.readyState_.markStateReady(State.IME_LIST_READY);
    this.readyState_.markStateReady(State.KEYBOARD_CONFIG_READY);
    this.readyState_.markStateReady(State.DISPLAY_SIZE_READY);
  }

  this.maybeDispatchSettingsReadyEvent_();
};


/**
 * Dispatch event SETTINGS_READY if all required bits are flipped.
 *
 * @private
 */
Adapter.prototype.maybeDispatchSettingsReadyEvent_ = function() {
  var states = [
    State.KEYBOARD_CONFIG_READY,
    State.IME_LIST_READY,
    State.DISPLAY_SIZE_READY];
  var ready = true;
  for (var i = 0; i < states.length; i++) {
    ready = ready && this.readyState_.isReady(states[i]);
  }
  if (ready) {
    window.setTimeout((function() {
      this.dispatchEvent(new goog.events.Event(
          i18n.input.chrome.inputview.events.EventType.SETTINGS_READY));
    }).bind(this), 0);
  }
};


/**
 * Gets the currently activated input method.
 *
 * @param {function(string)} callback .
 */
Adapter.prototype.getCurrentInputMethod = function(callback) {
  if (window.inputview && inputview.getCurrentInputMethod) {
    inputview.getCurrentInputMethod(callback);
  } else {
    callback('DU');
  }
};


/**
 * Gets the list of all activated input methods.
 *
 * @param {function(Array.<Object>)} callback .
 */
Adapter.prototype.getInputMethods = function(callback) {
  if (window.inputview && inputview.getInputMethods) {
    inputview.getInputMethods(callback);
  } else {
    // Provides a dummy IME item to enable IME switcher UI.
    callback([
      {'indicator': 'DU', 'id': 'DU', 'name': 'Dummy IME', 'command': 1}]);
  }
};


/**
 * Switches to the input method with id equals |inputMethodId|.
 *
 * @param {!string} inputMethodId .
 */
Adapter.prototype.switchToInputMethod = function(inputMethodId) {
  if (window.inputview && inputview.switchToInputMethod) {
    inputview.switchToInputMethod(inputMethodId);
  }
};


/**
 * Callback for visibility change on the input view window.
 *
 * @private
 */
Adapter.prototype.onVisibilityChange_ = function() {
  this.isVisible = !document.webkitHidden;
  this.dispatchEvent(new goog.events.Event(i18n.input.chrome.inputview.
      events.EventType.VISIBILITY_CHANGE));
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.VISIBILITY_CHANGE,
      Name.VISIBILITY, !document.webkitHidden));
};


/**
 * Sends request for completion.
 *
 * @param {string} query .
 * @param {!Object=} opt_spatialData .
 */
Adapter.prototype.sendCompletionRequest = function(query, opt_spatialData) {
  var spatialData = {};
  if (opt_spatialData) {
    spatialData[Name.SOURCES] = opt_spatialData.sources;
    spatialData[Name.POSSIBILITIES] = opt_spatialData.possibilities;
  }
  chrome.runtime.sendMessage(goog.object.create(Name.TYPE,
      Type.COMPLETION, Name.TEXT, query, Name.SPATIAL_DATA, spatialData));
};


/**
 * Selects the candidate.
 *
 * @param {!Object} candidate .
 */
Adapter.prototype.selectCandidate = function(candidate) {
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.SELECT_CANDIDATE, Name.CANDIDATE, candidate));
};


/**
 * Commits the text.
 *
 * @param {string} text .
 */
Adapter.prototype.commitText = function(text) {
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.COMMIT_TEXT, Name.TEXT, text));
};


/**
 * Commits the gesture result, possibly with a force space before the result.
 *
 * @param {string} text .
 * @param {boolean} forceAutoSpace .
 */
Adapter.prototype.commitGestureResult = function(text, forceAutoSpace) {
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.CONFIRM_GESTURE_RESULT, Name.TEXT, text,
      Name.FORCE_AUTO_SPACE, forceAutoSpace));
};


/**
 * Sets the language.
 *
 * @param {string} language .
 */
Adapter.prototype.setLanguage = function(language) {
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.SET_LANGUAGE, Name.LANGUAGE, language));
};


/**
 * Callbck when completion is back.
 *
 * @param {!Object} message .
 * @private
 */
Adapter.prototype.onCandidatesBack_ = function(message) {
  var source = message['source'] || '';
  var candidates = message['candidates'] || [];
  this.dispatchEvent(new CandidatesBackEvent(source, candidates));
};


/**
 * Callback when completion is back.
 *
 * @param {!Object} message .
 * @private
 */
Adapter.prototype.onGesturesBack_ = function(message) {
  var results = message[Name.GESTURE_RESULTS];
  this.dispatchEvent(new GesturesBackEvent(results));
};


/**
 * Hides the keyboard.
 */
Adapter.prototype.hideKeyboard = function() {
  chrome.input.ime.hideInputView();
};


/**
 * Sends DOUBLE_CLICK_ON_SPACE_KEY message.
 */
Adapter.prototype.doubleClickOnSpaceKey = function() {
  chrome.runtime.sendMessage(
      goog.object.create(
          Name.TYPE,
          Type.DOUBLE_CLICK_ON_SPACE_KEY));
};


/**
 * Sends message to the background when do internal inputtool switch.
 *
 * @param {boolean} inputToolValue The value of the language flag.
 */
Adapter.prototype.toggleLanguageState = function(inputToolValue) {
  chrome.runtime.sendMessage(
      goog.object.create(
          Name.TYPE,
          Type.TOGGLE_LANGUAGE_STATE,
          Name.MSG,
          inputToolValue));
};


/**
 * Processes incoming message from option page or inputview window.
 *
 * @param {*} request Message from option page or inputview window.
 * @param {*} sender Information about the script
 *     context that sent the message.
 * @param {function(*): void} sendResponse Function to call to send a response.
 * @return {boolean|undefined} {@code true} to keep the message channel open in
 *     order to send a response asynchronously.
 * @private
 */
Adapter.prototype.onMessage_ = function(request, sender, sendResponse) {
  var type = request[Name.TYPE];
  var msg = request[Name.MSG];
  if (!i18n.input.chrome.message.isFromBackground(type)) {
    return;
  }
  switch (type) {
    case Type.CANDIDATES_BACK:
      this.onCandidatesBack_(msg);
      break;
    case Type.CONTEXT_FOCUS:
      this.onContextFocus_(msg);
      break;
    case Type.CONTEXT_BLUR:
      this.onContextBlur_();
      break;
    case Type.GESTURES_BACK:
      this.onGesturesBack_(msg);
      break;
    case Type.SURROUNDING_TEXT_CHANGED:
      this.onSurroundingTextChanged_(request[Name.TEXT_BEFORE_CURSOR],
          request[Name.ANCHOR],
          request[Name.FOCUS],
          request[Name.OFFSET]);
      break;
    case Type.UPDATE_SETTINGS:
      this.onUpdateSettings_(msg);
      break;
    case Type.VOICE_STATE_CHANGE:
      this.dispatchEvent(
          new events.MessageEvent(events.EventType.VOICE_STATE_CHANGE,
              msg));
      break;
    case Type.HWT_NETWORK_ERROR:
      this.dispatchEvent(
          new events.MessageEvent(events.EventType.HWT_NETWORK_ERROR,
              msg));
      break;
    case Type.FRONT_TOGGLE_LANGUAGE_STATE:
      this.dispatchEvent(
          new events.MessageEvent(events.EventType.FRONT_TOGGLE_LANGUAGE_STATE,
              msg));
      break;
  }
};


/**
 * Sends the voice state to background.
 *
 * @param {boolean} state .
 */
Adapter.prototype.sendVoiceViewStateChange = function(state) {
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.VOICE_VIEW_STATE_CHANGE, Name.MSG, state));
};


/** @override */
Adapter.prototype.disposeInternal = function() {
  goog.dispose(this.handler_);

  goog.base(this, 'disposeInternal');
};


/**
 * Return the background IME switching state.
 *
 * @return {boolean}
 */
Adapter.prototype.isSwitching = function() {
  return this.isBgControllerSwitching_;
};


/**
 * Set the inputtool.
 *
 * @param {string} keyset The keyset.
 * @param {string} languageCode The language code.
 */
Adapter.prototype.setController = function(keyset, languageCode) {
  chrome.runtime.sendMessage(
      goog.object.create(
          Name.TYPE,
          Type.SET_CONTROLLER,
          Name.MSG,
          {'rawkeyset': keyset, 'languageCode': languageCode}));
};


/**
 * Unset the inputtool
 */
Adapter.prototype.unsetController = function() {
  chrome.runtime.sendMessage(
      goog.object.create(
          Name.TYPE,
          Type.UNSET_CONTROLLER));
};
});  // goog.scope
