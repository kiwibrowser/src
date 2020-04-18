// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains various hacks needed to inform JSCompiler of various
// WebKit- and Chrome-specific properties and methods. It is used only with
// JSCompiler to verify the type-correctness of our code.

/** @type {Array<string>} */
ClipboardData.prototype.types;

/** @type {HTMLElement} */
Document.prototype.activeElement;

/** @type {Array<HTMLElement>} */
Document.prototype.all;

/** @type {boolean} */
Document.prototype.hidden;

/** @return {void} Nothing. */
Document.prototype.exitPointerLock = function() {};

/** @type {boolean} */
Document.prototype.webkitIsFullScreen;

/** @type {boolean} */
Document.prototype.webkitHidden;

/** @type {Element} */
Document.prototype.firstElementChild;

/** @return {void} Nothing. */
Element.prototype.requestPointerLock = function() {};

/** @type {boolean} */
Element.prototype.disabled;

/** @type {boolean} */
Element.prototype.hidden;

/** @type {string} */
Element.prototype.innerText;

/** @type {string} */
Element.prototype.localName;

/** @type {number} */
Element.prototype.offsetRight;

/** @type {number} */
Element.prototype.offsetBottom;

/** @type {string} */
Element.prototype.textContent;

/** @type {boolean} */
Element.prototype.checked;

/** @type {Window} */
HTMLIFrameElement.prototype.contentWindow;

/**
 * @param {string} selector
 * @return {?HTMLElement}.
 */
HTMLElement.prototype.querySelector = function(selector) {};

/**
 * @param {string} name
 * @return {string}
 */
Node.prototype.getAttribute = function(name) { };

/** @type {string} */
Node.prototype.value;

/** @type {{top: string, left: string, bottom: string, right: string}} */
Node.prototype.style;

/** @type {boolean} */
Node.prototype.hidden;


/** @type {{getRandomValues: function(!ArrayBufferView):!ArrayBufferView}} */
Window.prototype.crypto;


/**
 * @type {DataTransfer}
 */
Event.prototype.dataTransfer = null;

/**
 * @type {number}
 */
Event.prototype.movementX = 0;

/**
 * @type {number}
 */
Event.prototype.movementY = 0;

/**
 * @param {string} type
 * @param {boolean} canBubble
 * @param {boolean} cancelable
 * @param {Window} view
 * @param {number} detail
 * @param {number} screenX
 * @param {number} screenY
 * @param {number} clientX
 * @param {number} clientY
 * @param {boolean} ctrlKey
 * @param {boolean} altKey
 * @param {boolean} shiftKey
 * @param {boolean} metaKey
 * @param {number} button
 * @param {EventTarget} relatedTarget
 */
Event.prototype.initMouseEvent = function(
    type, canBubble, cancelable, view, detail,
    screenX, screenY, clientX, clientY,
    ctrlKey, altKey, shiftKey, metaKey,
    button, relatedTarget) {};

/** @type {Object} */
Event.prototype.data = {};

// Chrome implements XMLHttpRequest.responseURL starting from Chrome 37.
/** @type {string} */
XMLHttpRequest.prototype.responseURL = "";


/*******************************************************************************
 * Webview and related declarations
 ******************************************************************************/

/**
 * Like chrome.webRequest, but for webview tags.
 *
 * chrome.webRequest defined in chrome_extensions.js can't be
 * used because it's not a type.
 *
 * @constructor
 */
function WebviewWebRequest() {}

/** @type {WebRequestEvent} */
WebviewWebRequest.prototype.onBeforeSendHeaders;

/** @type {WebRequestEvent} */
WebviewWebRequest.prototype.onCompleted;

/** @type {WebRequestOnErrorOccurredEvent} */
WebviewWebRequest.prototype.onErrorOccurred;

/**
 * Enable access to special APIs of webview DOM element.
 *
 * @constructor
 * @extends {HTMLElement}
 */
function Webview() {}

/** @type {!WebviewWebRequest} */
Webview.prototype.request;

/** @type {function(InjectDetails, function(Object))} */
Webview.prototype.executeScript;

/** @type {Window} */
Webview.prototype.contentWindow;

/**
 * See https://developer.chrome.com/apps/tags/webview#type-InjectDetails
 *
 * @typedef {{
 *   file: (string|undefined),
 *   code: (string|undefined)
 * }}
 */
var InjectDetails;

/*******************************************************************************
 * ConsoleMessage event
 ******************************************************************************/

/**
 * The consolemessage BrowserEvent contains these fields:
 * e.level: int32, log severity level (for exception/info etc)
 * e.line: int32, line number
 * e.message: string, the console message
 * e.sourceId: string, source identifier (the ones seen in devtools)
 *
 * @constructor
 * @extends {Event}
 */
chrome.ConsoleMessageBrowserEvent = function() {};

/** @type {number} */
chrome.ConsoleMessageBrowserEvent.prototype.level;

/** @type {number} */
chrome.ConsoleMessageBrowserEvent.prototype.line;

/** @type {string} */
chrome.ConsoleMessageBrowserEvent.prototype.message;

/** @type {string} */
chrome.ConsoleMessageBrowserEvent.prototype.sourceId;

/**
 * The last error of the NaCL embed element.
 * https://developer.chrome.com/native-client/devguide/coding/progress-events
 *
 * @type {string}
 */
HTMLEmbedElement.prototype.lastError;
