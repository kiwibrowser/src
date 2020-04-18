// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// <include src="keyboard_overlay_data.js">

var BASE_KEYBOARD = {top: 0, left: 0, width: 1237, height: 514};

var BASE_INSTRUCTIONS = {top: 194, left: 370, width: 498, height: 142};

var MODIFIER_TO_CLASS = {
  'SHIFT': 'modifier-shift',
  'CTRL': 'modifier-ctrl',
  'ALT': 'modifier-alt',
  'SEARCH': 'modifier-search'
};

var IDENTIFIER_TO_CLASS = {
  '2A': 'is-shift',
  '36': 'is-shift',
  '1D': 'is-ctrl',
  'E0 1D': 'is-ctrl',
  '38': 'is-alt',
  'E0 38': 'is-alt',
  'E0 5B': 'is-search'
};

var LABEL_TO_IDENTIFIER = {
  'search': 'E0 5B',
  'ctrl': '1D',
  'alt': '38',
  'caps lock': '3A',
  'esc': '01',
  'backspace': '0E',
  'disabled': 'DISABLED'
};

// For KeyboardOverlayUIBrowserTest.
var KEYCODE_TO_LABEL = {
  8: 'backspace',
  9: 'tab',
  10: 'shift',
  13: 'enter',
  27: 'esc',
  32: 'space',
  33: 'pageup',
  34: 'pagedown',
  35: 'end',
  36: 'home',
  37: 'left',
  38: 'up',
  39: 'right',
  40: 'down',
  46: 'delete',
  91: 'search',
  92: 'search',
  96: '0',
  97: '1',
  98: '2',
  99: '3',
  100: '4',
  101: '5',
  102: '6',
  103: '7',
  104: '8',
  105: '9',
  106: '*',
  107: '+',
  109: '-',
  110: '.',
  111: '/',
  112: 'back',
  113: 'forward',
  114: 'reload',
  115: 'full screen',
  116: 'switch window',
  117: 'bright down',
  118: 'bright up',
  119: 'mute',
  120: 'vol. down',
  121: 'vol. up',
  152: 'power',
  166: 'back',
  167: 'forward',
  168: 'reload',
  173: 'mute',
  174: 'vol. down',
  175: 'vol. up',
  183: 'full screen',
  182: 'switch window',
  186: ';',
  187: '+',
  188: ',',
  189: '-',
  190: '.',
  191: '/',
  192: '`',
  216: 'bright down',
  217: 'bright up',
  218: 'bright down',
  219: '[',
  220: '\\',
  221: ']',
  222: '\'',
  232: 'bright up',
};

/**
 * When the top row keys setting is changed so that they're treated as function
 * keys, their labels should change as well.
 */
var TOP_ROW_KEY_LABEL_TO_FUNCTION_LABEL = {
  'back': 'f1',
  'forward': 'f2',
  'reload': 'f3',
  'full screen': 'f4',
  'switch window': 'f5',
  'bright down': 'f6',
  'bright up': 'f7',
  'mute': 'f8',
  'vol. down': 'f9',
  'vol. up': 'f10',
};

/**
 * The top row key labels are modified in the new keyboard layout.
 */
var TOP_ROW_KEY_LABEL_TO_FUNCTION_LABEL_LAYOUT_2 = {
  'back': 'f1',
  'reload': 'f2',
  'full screen': 'f3',
  'switch window': 'f4',
  'bright down': 'f5',
  'bright up': 'f6',
  'play / pause': 'f7',
  'mute': 'f8',
  'vol. down': 'f9',
  'vol. up': 'f10',
};

/**
 * Some key labels define actions (like for example 'vol. up' or 'mute').
 * These labels should be localized. (crbug.com/471025).
 */
var LABEL_TO_LOCALIZED_LABEL_ID = {
  'esc': 'keyboardOverlayEscKeyLabel',
  'back': 'keyboardOverlayBackKeyLabel',
  'forward': 'keyboardOverlayForwardKeyLabel',
  'reload': 'keyboardOverlayReloadKeyLabel',
  'full screen': 'keyboardOverlayFullScreenKeyLabel',
  'switch window': 'keyboardOverlaySwitchWinKeyLabel',
  'bright down': 'keyboardOverlayBrightDownKeyLabel',
  'bright up': 'keyboardOverlayBrightUpKeyLabel',
  'mute': 'keyboardOverlayMuteKeyLabel',
  'vol. down': 'keyboardOverlayVolDownKeyLabel',
  'vol. up': 'keyboardOverlayVolUpKeyLabel',
  'power': 'keyboardOverlayPowerKeyLabel',
  'backspace': 'keyboardOverlayBackspaceKeyLabel',
  'tab': 'keyboardOverlayTabKeyLabel',
  'search': 'keyboardOverlaySearchKeyLabel',
  'assistant': 'keyboardOverlayAssistantKeyLabel',
  'play / pause': 'keyboardOverlayPlayPauseKeyLabel',
  'system menu': 'keyboardOverlaySystemMenuKeyLabel',
  'launcher': 'keyboardOverlayLauncherKeyLabel',
  'enter': 'keyboardOverlayEnterKeyLabel',
  'shift': 'keyboardOverlayShiftKeyLabel',
  'ctrl': 'keyboardOverlayCtrlKeyLabel',
  'alt': 'keyboardOverlayAltKeyLabel',
  'left': 'keyboardOverlayLeftKeyLabel',
  'right': 'keyboardOverlayRightKeyLabel',
  'up': 'keyboardOverlayUpKeyLabel',
  'down': 'keyboardOverlayDownKeyLabel',
  'f1': 'keyboardOverlayF1',
  'f2': 'keyboardOverlayF2',
  'f3': 'keyboardOverlayF3',
  'f4': 'keyboardOverlayF4',
  'f5': 'keyboardOverlayF5',
  'f6': 'keyboardOverlayF6',
  'f7': 'keyboardOverlayF7',
  'f8': 'keyboardOverlayF8',
  'f9': 'keyboardOverlayF9',
  'f10': 'keyboardOverlayF10',
};

var COMPOUND_ENTER_KEY_DATA = [815, 107, 60, 120];
var COMPOUND_ENTER_KEY_CLIP_PATH =
    'polygon(0% 0%, 100% 0%, 100% 100%, 28% 100%, 28% 47%, 0% 47%)';
var COMPOUND_ENTER_KEY_OVERLAY_DIV_CLIP_PATH =
    'polygon(12% 0%, 100% 0%, 100% 97%, 12% 97%)';

var IME_ID_PREFIX = '_comp_ime_';
var EXTENSION_ID_LEN = 32;

var keyboardOverlayId = 'en_US';
var identifierMap = {};

/**
 * True after at least one keydown event has been received.
 */
var gotKeyDown = false;

/**
 * Returns the actual key label based on the treatment of the top row setting,
 * as well as whether this device has the new keyboard layout.
 * @param {string} label The unmodified key label as read from
 * keyboard_overlay_data.js.
 */
function getTopRowKeyActualLabel(label) {
  if (!topRowKeysAreFunctionKeys())
    return label;

  var topRowMap = hasKeyboardLayout2() ?
      TOP_ROW_KEY_LABEL_TO_FUNCTION_LABEL_LAYOUT_2 :
      TOP_ROW_KEY_LABEL_TO_FUNCTION_LABEL;

  return topRowMap[label] || label;
}

/**
 * Returns the layout name.
 * @return {string} layout name.
 */
function getLayoutName() {
  return getKeyboardGlyphData().layoutName;
}

/**
 * Returns layout data.
 * @return {Array} Keyboard layout data.
 */
function getLayout() {
  return keyboardOverlayData['layouts'][getLayoutName()];
}

// Cache the shortcut data after it is constructed.
var shortcutDataCache;

/**
 * Returns shortcut data.
 * @return {Object} Keyboard shortcut data.
 */
function getShortcutData() {
  if (shortcutDataCache)
    return shortcutDataCache;

  shortcutDataCache = keyboardOverlayData['shortcut'];

  if (!isDisplayUIScalingEnabled()) {
    // Zoom screen in
    delete shortcutDataCache['+<>CTRL<>SHIFT'];
    // Zoom screen out
    delete shortcutDataCache['-<>CTRL<>SHIFT'];
    // Reset screen zoom
    delete shortcutDataCache['0<>CTRL<>SHIFT'];
  }

  return shortcutDataCache;
}

/**
 * Returns the keyboard overlay ID.
 * @return {string} Keyboard overlay ID.
 */
function getKeyboardOverlayId() {
  return keyboardOverlayId;
}

/**
 * Returns keyboard glyph data.
 * @return {Object} Keyboard glyph data.
 */
function getKeyboardGlyphData() {
  return keyboardOverlayData['keyboardGlyph'][getKeyboardOverlayId()];
}

/**
 * Tests if the top row keys are set to be treated as function keys.
 * @return {boolean} True if the top row keys are set to be function keys.
 */
function topRowKeysAreFunctionKeys() {
  return loadTimeData.getBoolean('keyboardOverlayTopRowKeysAreFunctionKeys');
}

/**
 * Tests if voice interaction is enabled.
 * @return {boolean} True if voice interaction feature is enabled.
 */
function isVoiceInteractionEnabled() {
  return loadTimeData.getBoolean('voiceInteractionEnabled');
}

/**
 * Tests if accelerators for moving window between displays are enabled.
 * @return {boolean} True if accelerators for moving window between displays
 * feature is enabled.
 */
function isDisplayMoveWindowAccelsEnabled() {
  return loadTimeData.getBoolean('displayMoveWindowAccelsEnabled');
}

/**
 * Converts a single hex number to a character.
 * @param {string} hex Hexadecimal string.
 * @return {string} Unicode values of hexadecimal string.
 */
function hex2char(hex) {
  if (!hex) {
    return '';
  }
  var result = '';
  var n = parseInt(hex, 16);
  if (n <= 0xFFFF) {
    result += String.fromCharCode(n);
  } else if (n <= 0x10FFFF) {
    n -= 0x10000;
    result +=
        (String.fromCharCode(0xD800 | (n >> 10)) +
         String.fromCharCode(0xDC00 | (n & 0x3FF)));
  } else {
    console.error('hex2Char error: Code point out of range :' + hex);
  }
  return result;
}

/**
 * Returns a list of modifiers normalized to ignore the distinction between
 * right or left keys.
 * @param {Array} modifiers List of modifiers with distinction between right
 *        and left keys.
 * @return {Array} List of normalized modifiers ignoring the difference between
 *         right or left keys.
 */
function normalizeModifiers(modifiers) {
  var result = [];
  if (contains(modifiers, 'L_SHIFT') || contains(modifiers, 'R_SHIFT')) {
    result.push('SHIFT');
  }
  if (contains(modifiers, 'L_CTRL') || contains(modifiers, 'R_CTRL')) {
    result.push('CTRL');
  }
  if (contains(modifiers, 'L_ALT') || contains(modifiers, 'R_ALT')) {
    result.push('ALT');
  }
  if (contains(modifiers, 'SEARCH')) {
    result.push('SEARCH');
  }
  return result.sort();
}

/**
 * This table will contain the status of the modifiers.
 */
var isPressed = {
  'L_SHIFT': false,
  'R_SHIFT': false,
  'L_CTRL': false,
  'R_CTRL': false,
  'L_ALT': false,
  'R_ALT': false,
  'SEARCH': false,
};

/**
 * Returns a list of modifiers from the key event distinguishing right and left
 * keys.
 * @param {Event} e The key event.
 * @return {Array} List of modifiers based on key event.
 */
function getModifiers(e) {
  if (!e)
    return [];

  var keyCodeToModifier = {
    16: 'SHIFT',
    17: 'CTRL',
    18: 'ALT',
    91: 'SEARCH',
  };
  var modifierWithKeyCode = keyCodeToModifier[e.keyCode];
  /** @const */ var DOM_KEY_LOCATION_LEFT = 1;
  var side = (e.location == DOM_KEY_LOCATION_LEFT) ? 'L_' : 'R_';
  var isKeyDown = (e.type == 'keydown');

  if (modifierWithKeyCode == 'SEARCH') {
    isPressed['SEARCH'] = isKeyDown;
  } else {
    isPressed[side + modifierWithKeyCode] = isKeyDown;
  }

  // make the result array
  return result =
             [
               'L_SHIFT', 'R_SHIFT', 'L_CTRL', 'R_CTRL', 'L_ALT', 'R_ALT',
               'SEARCH'
             ]
                 .filter(function(modifier) {
                   return isPressed[modifier];
                 })
                 .sort();
}

/**
 * Returns an ID of the key.
 * @param {string} identifier Key identifier.
 * @param {number} i Key number.
 * @return {string} Key ID.
 */
function keyId(identifier, i) {
  return identifier + '-key-' + i;
}

/**
 * Returns an ID of the text on the key.
 * @param {string} identifier Key identifier.
 * @param {number} i Key number.
 * @return {string} Key text ID.
 */
function keyTextId(identifier, i) {
  return identifier + '-key-text-' + i;
}

/**
 * Returns an ID of the shortcut text.
 * @param {string} identifier Key identifier.
 * @param {number} i Key number.
 * @return {string} Key shortcut text ID.
 */
function shortcutTextId(identifier, i) {
  return identifier + '-shortcut-text-' + i;
}

/**
 * Returns true if |list| contains |e|.
 * @param {Array} list Container list.
 * @param {string} e Element string.
 * @return {boolean} Returns true if the list contains the element.
 */
function contains(list, e) {
  return list.indexOf(e) != -1;
}

/**
 * Returns a list of the class names corresponding to the identifier and
 * modifiers.
 * @param {string} identifier Key identifier.
 * @param {Array} modifiers List of key modifiers (with distinction between
 *                right and left keys).
 * @param {Array} normalizedModifiers List of key modifiers (without distinction
 *                between right or left keys).
 * @return {Array} List of class names corresponding to specified params.
 */
function getKeyClasses(identifier, modifiers, normalizedModifiers) {
  var classes = ['keyboard-overlay-key'];
  for (var i = 0; i < normalizedModifiers.length; ++i) {
    classes.push(MODIFIER_TO_CLASS[normalizedModifiers[i]]);
  }

  if ((identifier == '2A' && contains(modifiers, 'L_SHIFT')) ||
      (identifier == '36' && contains(modifiers, 'R_SHIFT')) ||
      (identifier == '1D' && contains(modifiers, 'L_CTRL')) ||
      (identifier == 'E0 1D' && contains(modifiers, 'R_CTRL')) ||
      (identifier == '38' && contains(modifiers, 'L_ALT')) ||
      (identifier == 'E0 38' && contains(modifiers, 'R_ALT')) ||
      (identifier == 'E0 5B' && contains(modifiers, 'SEARCH'))) {
    classes.push('pressed');
    classes.push(IDENTIFIER_TO_CLASS[identifier]);
  }
  return classes;
}

/**
 * Returns true if a character is a ASCII character.
 * @param {string} c A character to be checked.
 * @return {boolean} True if the character is an ASCII character.
 */
function isAscii(c) {
  var charCode = c.charCodeAt(0);
  return 0x00 <= charCode && charCode <= 0x7F;
}

/**
 * Returns a remapped identiifer based on the preference.
 * @param {string} identifier Key identifier.
 * @return {string} Remapped identifier.
 */
function remapIdentifier(identifier) {
  return identifierMap[identifier] || identifier;
}

/**
 * Returns a label of the key.
 * @param {string} keyData Key glyph data.
 * @param {Array} modifiers Key Modifier list.
 * @return {string} Label of the key.
 */
function getKeyLabel(keyData, modifiers) {
  if (!keyData)
    return '';

  if (keyData.label)
    return getTopRowKeyActualLabel(keyData.label);

  var keyLabel = '';
  for (var j = 1; j <= 9; j++) {
    var pos = keyData['p' + j];
    if (!pos) {
      continue;
    }
    keyLabel = hex2char(pos);
    if (!keyLabel) {
      continue;
    }
    if (isAscii(keyLabel) &&
        getShortcutData()[getAction(keyLabel, modifiers)]) {
      break;
    }
  }
  return keyLabel;
}

/**
 * Returns a normalized string used for a key of shortcutData.
 *
 * Examples:
 *   keyCode: 'd', modifiers: ['CTRL', 'SHIFT'] => 'd<>CTRL<>SHIFT'
 *   keyCode: 'alt', modifiers: ['ALT', 'SHIFT'] => 'ALT<>SHIFT'
 *
 * @param {string} keyCode Key code.
 * @param {Array} modifiers Key Modifier list.
 * @return {string} Normalized key shortcut data string.
 */
function getAction(keyCode, modifiers) {
  /** @const */ var separatorStr = '<>';
  if (keyCode.toUpperCase() in MODIFIER_TO_CLASS) {
    keyCode = keyCode.toUpperCase();
    if (keyCode in modifiers) {
      return modifiers.join(separatorStr);
    } else {
      var action = [keyCode].concat(modifiers);
      action.sort();
      return action.join(separatorStr);
    }
  }
  return [keyCode].concat(modifiers).join(separatorStr);
}

/**
 * Returns a text which displayed on a key.
 * @param {string} keyData Key glyph data.
 * @return {string} Key text value.
 */
function getKeyTextValue(keyData) {
  if (keyData.label) {
    // Do not show text on the space key.
    if (keyData.label == 'space') {
      return '';
    }

    // some key labels define actions such as 'mute' or 'vol. up'. Those actions
    // should be localized (crbug.com/471025).
    // If this is a top row key label, we need to convert that label to
    // function-keys label (i.e. mute --> f8), and then use that label to get
    // a localized one.
    var labelToBeLocalized = getTopRowKeyActualLabel(keyData.label);
    var localizedLabelId = LABEL_TO_LOCALIZED_LABEL_ID[labelToBeLocalized];
    if (localizedLabelId)
      return loadTimeData.getString(localizedLabelId);

    return keyData.label;
  }

  var chars = [];
  for (var j = 1; j <= 9; ++j) {
    var pos = keyData['p' + j];
    if (pos && pos.length > 0) {
      chars.push(hex2char(pos));
    }
  }
  return chars.join(' ');
}

/**
 * Updates the whole keyboard.
 * @param {Array} modifiers Key Modifier list.
 * @param {Array} normModifiers Key Modifier list ignoring the distinction
 *                between right and left keys.
 */
function update(modifiers, normModifiers) {
  var keyboardGlyphData = getKeyboardGlyphData();
  var shortcutData = getShortcutData();
  var layout = getLayout();
  for (var i = 0; i < layout.length; ++i) {
    var identifier = remapIdentifier(layout[i][0]);
    var keyData = keyboardGlyphData.keys[identifier];
    var classes = getKeyClasses(identifier, modifiers, normModifiers);
    var keyLabel = getKeyLabel(keyData, normModifiers);
    var shortcutId = shortcutData[getAction(keyLabel, normModifiers)];
    if (modifiers.length == 0 && (identifier == '2A' || identifier == '36')) {
      // Either the right or left shift keys are used to disable the caps lock
      // if it was enabled. To fix crbug.com/453623.
      shortcutId = 'keyboardOverlayDisableCapsLock';
    }

    classes.push('keyboard-overlay-key-background');

    if (shortcutId == 'keyboardOverlayVoiceInteraction' &&
        (!isVoiceInteractionEnabled() || hasKeyboardLayout2())) {
      // The shortcut should be disabled either voice interaction is disabled or
      // keyboard layout 2 is in use (in which case a dedicated key for voice
      // interaction exists).
      continue;
    }

    // Currently hidden behind experimental accessibility features flag.
    if (shortcutId == 'keyboardOverlayToggleDictation')
      continue;

    if ((shortcutId == 'keyboardOverlayMoveWindowToBelowDisplay' ||
         shortcutId == 'keyboardOverlayMoveWindowToLeftDisplay' ||
         shortcutId == 'keyboardOverlayMoveWindowToRightDisplay' ||
         shortcutId == 'keyboardOverlayMoveWindowToAboveDisplay') &&
        !isDisplayMoveWindowAccelsEnabled()) {
      continue;
    }

    if (shortcutId) {
      classes.push('is-shortcut');
      classes.push('keyboard-overlay-shortcut-key-background');
    }

    var key = $(keyId(identifier, i));
    key.className = classes.join(' ');

    if (!keyData) {
      continue;
    }

    var keyText = $(keyTextId(identifier, i));
    var keyTextValue = getKeyTextValue(keyData);
    if (keyTextValue) {
      keyText.style.visibility = 'visible';
    } else {
      keyText.style.visibility = 'hidden';
    }
    keyText.textContent = keyTextValue;

    var shortcutText = $(shortcutTextId(identifier, i));
    if (shortcutId) {
      shortcutText.style.visibility = 'visible';
      shortcutText.textContent = loadTimeData.getString(shortcutId);
    } else {
      shortcutText.style.visibility = 'hidden';
    }

    if (layout[i][1] == 'COMPOUND_ENTER_KEY') {
      var overlayDivClasses =
          getKeyClasses(identifier, modifiers, normModifiers);
      if (shortcutId)
        overlayDivClasses.push('is-shortcut');
      $(keyId(identifier, i) + '-sub').className = overlayDivClasses.join(' ');
    }

    var format = keyboardGlyphData.keys[layout[i][0]].format;
    if (format) {
      if (format == 'left' || format == 'right') {
        shortcutText.style.textAlign = format;
        keyText.style.textAlign = format;
      }
    }
  }
}

/**
 * A callback function for onkeydown and onkeyup events.
 * @param {Event} e Key event.
 */
function handleKeyEvent(e) {
  if (!getKeyboardOverlayId()) {
    return;
  }

  var modifiers = getModifiers(e);

  // To avoid flickering as the user releases the modifier keys that were held
  // to trigger the overlay, avoid updating in response to keyup events until at
  // least one keydown event has been received.
  if (!gotKeyDown) {
    if (e.type == 'keyup') {
      return;
    } else if (e.type == 'keydown') {
      gotKeyDown = true;
    }
  }

  var normModifiers = normalizeModifiers(modifiers);
  update(modifiers, normModifiers);

  var instructions = $('instructions');
  if (modifiers.length == 0) {
    instructions.style.visibility = 'visible';
  } else {
    instructions.style.visibility = 'hidden';
  }

  e.preventDefault();
}

/**
 * Initializes the layout of the keys.
 */
function initLayout() {
  // Add data for the caps lock key
  var keys = getKeyboardGlyphData().keys;
  if (!('3A' in keys)) {
    keys['3A'] = {label: 'caps lock', format: 'left'};
  }
  // Add data for the special key representing a disabled key
  keys['DISABLED'] = {label: 'disabled', format: 'left'};

  var layout = getLayout();
  var keyboard = document.body;
  var minX = window.innerWidth;
  var maxX = 0;
  var minY = window.innerHeight;
  var maxY = 0;
  var multiplier = 1.38 * window.innerWidth / BASE_KEYBOARD.width;
  var keyMargin = 7;
  var offsetX = 10;
  var offsetY = 7;

  var instructions = document.createElement('div');
  instructions.id = 'instructions';
  instructions.className = 'keyboard-overlay-instructions';
  instructions.setAttribute('aria-live', 'polite');
  // Postpone showing the instructions a delay after the widget has been shown,
  // so that chrome vox behaves correctly.
  instructions.style.visibility = 'hidden';
  instructions.tabIndex = '0';
  var instructionsText = document.createElement('div');
  instructionsText.id = 'instructions-text';
  instructionsText.className = 'keyboard-overlay-instructions-text';
  instructionsText.innerHTML =
      loadTimeData.getString('keyboardOverlayInstructions');
  instructions.appendChild(instructionsText);
  var instructionsHideText = document.createElement('div');
  instructionsHideText.id = 'instructions-hide-text';
  instructionsHideText.className = 'keyboard-overlay-instructions-hide-text';
  instructionsHideText.innerHTML =
      loadTimeData.getString('keyboardOverlayInstructionsHide');
  instructions.appendChild(instructionsHideText);
  var learnMoreLinkText = document.createElement('div');
  learnMoreLinkText.id = 'learn-more-text';
  learnMoreLinkText.className = 'keyboard-overlay-learn-more-text';
  learnMoreLinkText.addEventListener('click', learnMoreClicked);
  var learnMoreLinkAnchor = document.createElement('a');
  learnMoreLinkAnchor.href =
      loadTimeData.getString('keyboardOverlayLearnMoreURL');
  learnMoreLinkAnchor.textContent =
      loadTimeData.getString('keyboardOverlayLearnMore');
  learnMoreLinkText.appendChild(learnMoreLinkAnchor);
  instructions.appendChild(learnMoreLinkText);
  keyboard.appendChild(instructions);

  for (var i = 0; i < layout.length; i++) {
    var array = layout[i];
    var identifier = remapIdentifier(array[0]);

    var keyDataX = 0;
    var keyDataY = 0;
    var keyDataW = 0;
    var keyDataH = 0;
    var isCompoundEnterKey = false;
    if (array[1] == 'COMPOUND_ENTER_KEY') {
      keyDataX = COMPOUND_ENTER_KEY_DATA[0];
      keyDataY = COMPOUND_ENTER_KEY_DATA[1];
      keyDataW = COMPOUND_ENTER_KEY_DATA[2];
      keyDataH = COMPOUND_ENTER_KEY_DATA[3];
      isCompoundEnterKey = true;
    } else {
      keyDataX = array[1];
      keyDataY = array[2];
      keyDataW = array[3];
      keyDataH = array[4];
    }

    var x = Math.round((keyDataX + offsetX) * multiplier);
    var y = Math.round((keyDataY + offsetY) * multiplier);
    var w = Math.round((keyDataW - keyMargin) * multiplier);
    var h = Math.round((keyDataH - keyMargin) * multiplier);

    var key = document.createElement('div');
    key.id = keyId(identifier, i);
    key.className = 'keyboard-overlay-key';
    key.style.left = x + 'px';
    key.style.top = y + 'px';
    key.style.width = w + 'px';
    key.style.height = h + 'px';

    var keyText = document.createElement('div');
    keyText.id = keyTextId(identifier, i);
    keyText.className = 'keyboard-overlay-key-text';
    keyText.style.visibility = 'hidden';
    key.appendChild(keyText);

    var shortcutText = document.createElement('div');
    shortcutText.id = shortcutTextId(identifier, i);
    shortcutText.className = 'keyboard-overlay-shortcut-text';
    shortcutText.style.visilibity = 'hidden';
    key.appendChild(shortcutText);

    if (isCompoundEnterKey) {
      key.style.webkitClipPath = COMPOUND_ENTER_KEY_CLIP_PATH;
      keyText.style.webkitClipPath = COMPOUND_ENTER_KEY_CLIP_PATH;
      shortcutText.style.webkitClipPath = COMPOUND_ENTER_KEY_CLIP_PATH;

      // Add an overlay div to account for clipping and show the borders.
      var overlayDiv = document.createElement('div');
      overlayDiv.id = keyId(identifier, i) + '-sub';
      overlayDiv.className = 'keyboard-overlay-key';
      var overlayDivX = x - 3;
      var overlayDivY = y + Math.round(h * 0.47) + 2;
      var overlayDivW = Math.round(w * 0.28);
      var overlayDivH = Math.round(h * (1 - 0.47)) + 1;

      overlayDiv.style.left = overlayDivX + 'px';
      overlayDiv.style.top = overlayDivY + 'px';
      overlayDiv.style.width = overlayDivW + 'px';
      overlayDiv.style.height = overlayDivH + 'px';
      overlayDiv.style.webkitClipPath =
          COMPOUND_ENTER_KEY_OVERLAY_DIV_CLIP_PATH;
      keyboard.appendChild(overlayDiv);
    }

    keyboard.appendChild(key);

    minX = Math.min(minX, x);
    maxX = Math.max(maxX, x + w);
    minY = Math.min(minY, y);
    maxY = Math.max(maxY, y + h);
  }

  var width = maxX - minX + 1;
  var height = maxY - minY + 1;
  keyboard.style.width = (width + 2 * (minX + 1)) + 'px';
  keyboard.style.height = (height + 2 * (minY + 1)) + 'px';

  instructions.style.left = ((BASE_INSTRUCTIONS.left - BASE_KEYBOARD.left) *
                                 width / BASE_KEYBOARD.width +
                             minX) +
      'px';
  instructions.style.top = ((BASE_INSTRUCTIONS.top - BASE_KEYBOARD.top) *
                                height / BASE_KEYBOARD.height +
                            minY) +
      'px';
  instructions.style.width =
      (width * BASE_INSTRUCTIONS.width / BASE_KEYBOARD.width) + 'px';
  instructions.style.height =
      (height * BASE_INSTRUCTIONS.height / BASE_KEYBOARD.height) + 'px';
}

/**
 * Returns true if the device has a diamond key.
 * @return {boolean} Returns true if the device has a diamond key.
 */
function hasDiamondKey() {
  return loadTimeData.getBoolean('keyboardOverlayHasChromeOSDiamondKey');
}

/**
 * @return {boolean} True if the internal keyboard of the device has the new
 * keyboard layout.
 */
function hasKeyboardLayout2() {
  return loadTimeData.getBoolean('keyboardOverlayUsesLayout2');
}

/**
 * Returns true if display scaling feature is enabled.
 * @return {boolean} True if display scaling feature is enabled.
 */
function isDisplayUIScalingEnabled() {
  return loadTimeData.getBoolean('keyboardOverlayIsDisplayUIScalingEnabled');
}

/**
 * Modifies the current layout based on the given parameters.
 *
 * @param {Object<string, Array<number>>} newLayoutData The new or updated key
 * layout positions and sizes. The keys are the key IDs, and the values are
 * bounds of the corresponding keys.
 * @param {Object<string, Object<string, string>>} newKeyData The updated key
 * data such as labels and formats. The keys are the key IDs, and the values
 * are the key data.
 * @param {Array<string>} keyIdsToRemove The key IDs to remove from the
 * current layout.
 */
function modifyLayoutAndKeyData(newLayoutData, newKeyData, keyIdsToRemove) {
  var layout = getLayout();
  var indicesToRemove = [];
  for (var i = 0; i < layout.length; i++) {
    var keyId = layout[i][0];
    if (keyId in newLayoutData) {
      layout[i] = [keyId].concat(newLayoutData[keyId]);
      delete newLayoutData[keyId];
    }

    if (keyIdsToRemove.includes(keyId))
      indicesToRemove.push(i);
  }

  // Remove the desired keys.
  for (var i in indicesToRemove)
    layout.splice(indicesToRemove[i], 1);

  // Add the new ones.
  for (var keyId in newLayoutData)
    layout.push([keyId].concat(newLayoutData[keyId]));

  // Update the new key data.
  var keyData = getKeyboardGlyphData()['keys'];
  for (var keyId in newKeyData)
    keyData[keyId] = newKeyData[keyId];
}

/**
 * Initializes the layout and the key labels for the keyboard that has a diamond
 * key.
 */
function initDiamondKey() {
  var newLayoutData = {
    '1D': [65.0, 287.0, 60.0, 60.0],      // left Ctrl
    '38': [185.0, 287.0, 60.0, 60.0],     // left Alt
    'E0 5B': [125.0, 287.0, 60.0, 60.0],  // search
    '3A': [5.0, 167.0, 105.0, 60.0],      // caps lock
    '5B': [803.0, 6.0, 72.0, 35.0],       // lock key
    '5D': [5.0, 287.0, 60.0, 60.0]        // diamond key
  };
  var newKeyData = {
    '3A': {'label': 'caps lock', 'format': 'left'},
    '5B': {'label': 'lock'},
    '5D': {'label': 'diamond', 'format': 'left'}
  };
  var keysToRemove = [
    '00',  // The power key.
  ];
  modifyLayoutAndKeyData(newLayoutData, newKeyData, keysToRemove);
}

/**
 * Initializes the layout for devices which have the new keyboard layout.
 */
function initKeyboardLayout2() {
  var newLayoutData = {
    '1D': [5.0, 287.0, 80.0, 60.0],    // left Ctrl
    'D8': [85.0, 287.0, 80.0, 60.0],   // Assistant key
    '38': [165.0, 287.0, 80.0, 60.0],  // left Alt
    '5D': [803.0, 6.0, 72.0, 35.0],    // System menu
  };
  var newKeyData = {
    'D8': {'label': 'assistant', 'format': 'left'},
    '5D': {'label': 'system menu'},
    'E0 5B': {'label': 'launcher', 'format': 'left'},
    '3B': {'label': 'back'},
    '3C': {'label': 'reload'},
    '3D': {'label': 'full screen'},
    '3E': {'label': 'switch window'},
    '3F': {'label': 'bright down'},
    '40': {'label': 'bright up'},
    '41': {'label': 'play / pause'},
    '42': {'label': 'mute'},
    '43': {'label': 'vol. down'},
    '44': {'label': 'vol. up'},
  };
  var keysToRemove = [
    '00',  // The power key.
  ];
  modifyLayoutAndKeyData(newLayoutData, newKeyData, keysToRemove);

  // Modify the Search + top row keys shortcuts:
  var newShortcuts = {
    'bright down<>SEARCH': 'keyboardOverlayF5',
    'bright up<>SEARCH': 'keyboardOverlayF6',
    'f2<>SEARCH': 'keyboardOverlayReloadKeyLabel',
    'f3<>SEARCH': 'keyboardOverlayFullScreenKeyLabel',
    'f4<>SEARCH': 'keyboardOverlaySwitchWinKeyLabel',
    'f5<>SEARCH': 'keyboardOverlayBrightDownKeyLabel',
    'f6<>SEARCH': 'keyboardOverlayBrightUpKeyLabel',
    'f7<>SEARCH': 'keyboardOverlayPlayPauseKeyLabel',
    'full screen<>SEARCH': 'keyboardOverlayF3',
    'play / pause<>SEARCH': 'keyboardOverlayF7',
    'reload<>SEARCH': 'keyboardOverlayF2',
    'switch window<>SEARCH': 'keyboardOverlayF4',
  };
  var shortcutDataCache = keyboardOverlayData['shortcut'];
  for (var shortcutId in newShortcuts) {
    shortcutDataCache[shortcutId] = newShortcuts[shortcutId];
    delete newShortcuts[shortcutId];
  }
  for (var remainingId in newShortcuts)
    shortcutDataCache[remainingId] = newShortcuts[remainingId];
}

/**
 * A callback function for the onload event of the body element.
 */
function init() {
  document.addEventListener('keydown', handleKeyEvent);
  document.addEventListener('keyup', handleKeyEvent);
  chrome.send('getLabelMap');
}

/**
 * Initializes the global map for remapping identifiers of modifier keys based
 * on the preference.
 * Called after sending the 'getLabelMap' message.
 * @param {Object} remap Identifier map.
 */
function initIdentifierMap(remap) {
  for (var key in remap) {
    var val = remap[key];
    if ((key in LABEL_TO_IDENTIFIER) && (val in LABEL_TO_IDENTIFIER)) {
      identifierMap[LABEL_TO_IDENTIFIER[key]] = LABEL_TO_IDENTIFIER[val];
    } else {
      console.error('Invalid label map element: ' + key + ', ' + val);
    }
  }
  chrome.send('getInputMethodId');
}

/**
 * Initializes the global keyboad overlay ID and the layout of keys.
 * Called after sending the 'getInputMethodId' message.
 * @param {inputMethodId} inputMethodId Input Method Identifier.
 */
function initKeyboardOverlayId(inputMethodId) {
  // Libcros returns an empty string when it cannot find the keyboard overlay ID
  // corresponding to the current input method.
  // In such a case, fallback to the default ID (en_US).
  var inputMethodIdToOverlayId =
      keyboardOverlayData['inputMethodIdToOverlayId'];
  if (inputMethodId) {
    if (inputMethodId.startsWith(IME_ID_PREFIX)) {
      // If the input method is a component extension IME, remove the prefix:
      //   _comp_ime_<ext_id>
      // The extension id is a hash value with 32 characters.
      inputMethodId =
          inputMethodId.slice(IME_ID_PREFIX.length + EXTENSION_ID_LEN);
    }
    keyboardOverlayId = inputMethodIdToOverlayId[inputMethodId];
  }
  if (!keyboardOverlayId) {
    console.error('No keyboard overlay ID for ' + inputMethodId);
    keyboardOverlayId = 'en_US';
  }
  while (document.body.firstChild) {
    document.body.removeChild(document.body.firstChild);
  }
  // We show Japanese layout as-is because the user has chosen the layout
  // that is quite diffrent from the physical layout that has a diamond key.
  if (hasDiamondKey() && getLayoutName() != 'J')
    initDiamondKey();
  else if (hasKeyboardLayout2())
    initKeyboardLayout2();

  initLayout();
  update([], []);
  window.webkitRequestAnimationFrame(function() {
    chrome.send('didPaint');
  });
}

/**
 * Handles click events of the learn more link.
 * @param {Event} e Mouse click event.
 */
function learnMoreClicked(e) {
  chrome.send('openLearnMorePage');
  chrome.send('dialogClose');
  e.preventDefault();
}

/**
 * Called from C++ when the widget is shown.
 */
function onWidgetShown() {
  setTimeout(function() {
    // Show and focus the instructions div after a delay so that chrome vox
    // speaks it correctly.
    $('instructions').style.visibility = 'visible';
    $('instructions').focus();
  }, 500);
}

document.addEventListener('DOMContentLoaded', init);
