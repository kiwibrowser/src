/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @fileoverview This file contains various event related functions for
 * o3d.  It puts them in the 'event' module on the o3djs object.
 *
 * TODO Add selenium tests.
 *
 *
 */
o3djs.provide('o3djs.event');

/**
 * A Module for handling events related to o3d and various browsers.
 * @namespace
 */
o3djs.event = o3djs.event || {};

/**
  * @param {string} inStr base string.
  * @param {string} extraStr string to append.
  * @return {string} inStr + ' ' + extraStr, or just inStr if extraStr is ''.
  */
o3djs.event.appendWithSpace = function(inStr, extraStr) {
  return (inStr.length == 0) ? extraStr : inStr + ' ' + extraStr;
};

/**
  * @param {boolean} state whether to append or not.
  * @param {string} inStr base string.
  * @param {string} extraStr string to append.
  * @return {string} inStr + ' ' + extraStr, or just inStr if state is false.
  */
o3djs.event.appendWithSpaceIf = function(state, inStr, extraStr) {
  return (state) ? o3djs.event.appendWithSpace(inStr, extraStr) : inStr;
};


/**
 * Builds a DOM-level 3 modifier string for a KeyboardEvent - see
 * http://www.w3.org/TR/DOM-Level-3-Events/events.html
 * #Events-KeyboardEvents-Interfaces.
 * @param {boolean} control whether the control key is down.
 * @param {boolean} alt whether the alt/option key is down.
 * @param {boolean} shift whether the shift key is down.
 * @param {boolean} meta whether the meta/command key is down.
 * @return {string} space delimited list of keys that are down.
 */
o3djs.event.getModifierString = function(control, alt, shift, meta) {
  var modStr = o3djs.event.appendWithSpaceIf(control, '', 'Control');
  modStr = o3djs.event.appendWithSpaceIf(alt, modStr, 'Alt');
  modStr = o3djs.event.appendWithSpaceIf(shift, modStr, 'Shift');
  return o3djs.event.appendWithSpaceIf(meta, modStr, 'Meta');
};


/**
 * Pad a string with leading zeroes if needed until it is the length desired.
 * @param {string} str The input string, probably representing a number.
 * @param {number} to_length The desired minimum length of string with padding.
 * @return {string} A string padded with leading zeroes as needed to be the
 * length desired.
 */
o3djs.event.padWithLeadingZeroes = function(str, to_length) {
  while (str.length < to_length)
    str = '0' + str;
  return str;
};

/**
 * Creates a keyIdentifer string for a given keystroke as specified in the w3c
 * spec on http://www.w3.org/TR/DOM-Level-3-Events/events.html.
 * @param {number} charCode numeric unicode code point as reported by the OS.
 * @param {number} keyCode numeric keyCode as reported by the OS, currently
 * unused but will probably be necessary in the future.
 * @return {string} eg 'Left' or 'U+0040'.
 */
o3djs.event.getKeyIdentifier = function(charCode, keyCode) {
  if (!charCode) {
    // TODO: This works for webkit for keydown and keyup, for basic
    // alphanumeric keys, at least.  Likely it needs lots of work to handle
    // accented characters, various keyboards, etc., as does the rest of our
    // keyboard event code.
    charCode = keyCode;
  }
  switch (charCode) {
    case 3: case 13: return 'Enter';  // spec merges these.
    case 37: return 'Left';
    case 39: return 'Right';
    case 38: return 'Up';
    case 40: return 'Down';
  }
  charCode = (charCode >= 97 && charCode <= 122) ? charCode - 32 : charCode;
  var keyStr = charCode.toString(16).toUpperCase();
  return 'U+' + o3djs.event.padWithLeadingZeroes(keyStr, 4);
};


/** Takes a keyIdentifier string and remaps it to an ASCII/Unicode value
 *  suitable for javascript event handling.
 * @param {string} keyIdent a keyIdentifier string as generated above.
 * @return {number} the numeric Unicode code point represented.
 */
o3djs.event.keyIdentifierToChar = function(keyIdent) {
  if (keyIdent && typeof(keyIdent) == 'string') {
    switch (keyIdent) {
      case 'Enter': return 13;
      case 'Left': return 37;
      case 'Right': return 39;
      case 'Up': return 38;
      case 'Down': return 40;
    }
    if (keyIdent.indexOf('U+') == 0)
      return parseInt(keyIdent.substr(2).toUpperCase(), 16);
  }
  return 0;
};

/**
 *  Extracts the key char in number form from the event, in a cross-browser
 *  manner.
 * @param {!Event} event .
 * @return {number} unicode code point for the key.
 */
o3djs.event.getEventKeyChar = function(event) {
  if (!event) {
    event = window.event;
  }
  var charCode = 0;
  if (event.keyIdentifier) 
    charCode = o3djs.event.keyIdentifierToChar(event.keyIdentifier);
  if (!charCode)
    charCode = (window.event) ? window.event.keyCode : event.charCode;
  if (!charCode)
    charCode = event.keyCode;
  return charCode;
};


/**
 * Cancel an event we've handled so it stops propagating upwards.
 * The cancelBubble is for IE, stopPropagation is for all other browsers.
 * preventDefault ensures that the default action is also canceled.
 * @param {!Event} event - the event to cancel.
 */
o3djs.event.cancel = function(event) {
  if (!event)
    event = window.event;
  event.cancelBubble = true;
  if (event.stopPropagation)
    event.stopPropagation();
  if (event.preventDefault)
    event.preventDefault();
};

/**
 * Convenience function to setup synthesizing and dispatching of keyboard events
 * whenever the focussed plug-in calls Javascript to report a keyboard action.
 * @param {!Element} pluginObject the <object> where the o3d plugin lives,
 * which the caller probably obtained by calling getElementById.
 */
o3djs.event.startKeyboardEventSynthesis = function(pluginObject) {
  var handler = function(event) {
    o3djs.event.onKey(event, pluginObject);
  };

  o3djs.event.addEventListener(pluginObject, 'keypress', handler);
  o3djs.event.addEventListener(pluginObject, 'keydown', handler);
  o3djs.event.addEventListener(pluginObject, 'keyup', handler);
};

/**
 * Dispatches a DOM-level 3 KeyboardEvent when called back by the plugin.
 * see http://www.w3.org/TR/DOM-Level-3-Events/events.html
 * #Events-KeyboardEvents-Interfaces
 * see http://developer.mozilla.org/en/DOM/event.initKeyEvent
 * @param {!Event} event an O3D event object.
 * @param {!Element} pluginObject the plugin object on the page.
 */
o3djs.event.onKey = function(event, pluginObject) {
  var k_evt = o3djs.event.createKeyEvent(event.type, event.charCode,
      event.keyCode, event.ctrlKey, event.altKey, event.shiftKey,
      event.metaKey);
  if (k_evt) {
    if (pluginObject.parentNode.dispatchEvent) {
      // Using the pluginObject itself fails for non-capturing event listeners
      // on keypress events on Firefox only, as far as I've been able to
      // determine.  I have no idea why.
      pluginObject.parentNode.dispatchEvent(k_evt);
    } else if (pluginObject.fireEvent) {
      pluginObject.fireEvent('on' + event.type, k_evt);
    }
  }
};

/**
 * Creates a DOM-level 3 KeyboardEvent.
 * see http://www.w3.org/TR/DOM-Level-3-Events/events.html
 * #Events-KeyboardEvents-Interfaces.
 * see http://developer.mozilla.org/en/DOM/event.initKeyEvent
 * @param {string} eventName one of 'keypress', 'keydown' or 'keyup'.
 * @param {number} charCode the character code for the key.
 * @param {number} keyCode the key code for the key.
 * @param {boolean} control whether the control key is down.
 * @param {boolean} alt whether the alt/option key is down.
 * @param {boolean} shift whether the shift key is down.
 * @param {boolean} meta whether the meta/command key is down.
 */
o3djs.event.createKeyEvent = function(eventName, charCode, keyCode,
                                      control, alt, shift, meta) {
  var k_evt;
  var keyIdentifier = o3djs.event.getKeyIdentifier(charCode, keyCode);
  if (document.createEvent) {
    k_evt = document.createEvent('KeyboardEvent');
    if (k_evt.initKeyboardEvent) {  // WebKit.
      k_evt.initKeyboardEvent(eventName, true, true, window,
                   keyIdentifier, 0,
                   control, alt, shift, meta);
      // TODO: These actually fail to do anything in Chrome; those are
      // read-only fields, and it's not setting them in initKeyboardEvent.
      k_evt.charCode = charCode;
      if (eventName == 'keypress')
        k_evt.keyCode = charCode;
      else
        k_evt.keyCode = keyCode;
    } else if (k_evt.initKeyEvent) {  // FF.
      k_evt.initKeyEvent(eventName, true, true, window,
                         control, alt, shift, meta, keyCode, charCode);
      k_evt.keyIdentifier = keyIdentifier;
    }
  } else if (document.createEventObject) {
    k_evt = document.createEventObject();
    k_evt.ctrlKey = control;
    k_evt.altKey = alt;
    k_evt.shiftKey = shift;
    k_evt.metaKey = meta;
    k_evt.keyCode = charCode;  // Emulate IE charcode-in-the-keycode onkeypress.
    k_evt.keyIdentifier = keyIdentifier;
  }
  k_evt.synthetic = true;
  return k_evt;
};

/*
 * Function to create a closure that will call each event handler in an array
 * whenever it gets called, passing its single argument through to the
 * sub-handlers.  The sub-handlers may either be functions or EventListeners.
 * This is generally expected to be used only through
 * o3djs.event.addEventListener.
 * @param {!Array.<*>} listenerSet an array of handlers.
 * @return {!function(*): void} a closure to be used to multiplex out
 *     event-handling.
 */
o3djs.event.createEventHandler = function(listenerSet) {
  return function(event) {
    var length = listenerSet.length;
    for (var index = 0; index < length; ++index) {
      var handler = listenerSet[index];
      if (typeof(handler.handleEvent) == 'function') {
        handler.handleEvent(event);
      } else {
        handler(event);
      }
    }
  }
};

/**
 * Convenience function to manage event listeners on the o3d plugin object,
 * intended as a drop-in replacement for the DOM addEventListener [with slightly
 * different arguments, but the same effect].
 * @param {!Element} pluginObject the html object where the o3d plugin lives,
 * which the caller probably obtained by calling getElementById or makeClients.
 * @param {string} type the event type on which to trigger, e.g. 'mousedown',
 * 'mousemove', etc.
 * @param {!Object} handler either a function or an EventListener object.
 */
o3djs.event.addEventListener = function(pluginObject, type, handler) {
  if (!handler || typeof(type) != 'string' ||
      (typeof(handler) != 'function' &&
       typeof(handler.handleEvent) != 'function')) {
    throw new Error('Invalid argument.');
  }
  pluginObject.o3d_eventRegistry = pluginObject.o3d_eventRegistry || [];
  var registry = pluginObject.o3d_eventRegistry;
  var listenerSet = registry[type];
  if (!listenerSet || listenerSet.length == 0) {
    listenerSet = registry[type] = [];
    pluginObject.client.setEventCallback(type,
        o3djs.event.createEventHandler(listenerSet));
  } else {
    for (var index in listenerSet) {
      if (listenerSet[index] == handler) {
        return;  // We're idempotent.
      }
    }
  }
  listenerSet.push(handler);
};


/**
 * Convenience function to manage event listeners on the o3d plugin object,
 * intended as a drop-in replacement for the DOM removeEventListener [with
 * slightly different arguments, but the same effect].
 * @param {!Element} pluginObject the <object> where the o3d plugin lives,
 * which the caller probably obtained by calling getElementById.
 * @param {string} type the event type on which the handler to be removed was to
 * trigger, e.g. 'mousedown', 'mousemove', etc.
 * @param {!Object} handler either a function or an EventListener object.
 */
o3djs.event.removeEventListener = function(pluginObject, type, handler) {
  var registry = pluginObject.o3d_eventRegistry;
  if (!registry) {
    return;
  }
  var listenerSet = registry[type];
  if (!listenerSet) {
    return;
  }
  for (var index in listenerSet) {
    if (listenerSet[index] == handler) {
      if (listenerSet.length == 1) {
        pluginObject.client.clearEventCallback(type);
      }
      listenerSet.splice(index, 1);
      break;
    }
  }
};
