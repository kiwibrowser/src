// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @const */
chrome.webstoreWidgetPrivate = {};

/**
 * @enum {string}
 */
chrome.webstoreWidgetPrivate.Type = {
  PRINTER_PROVIDER: 'PRINTER_PROVIDER'
};

/**
 * @typedef {{vendorId: number, productId: number}}
 */
chrome.webstoreWidgetPrivate.UsbId;

/**
 * @typedef {{
 *   options: chrome.webstoreWidgetPrivate.Type,
 *   usbId: (chrome.webstoreWidgetPrivate.UsbId|undefined)
 * }}
 */
chrome.webstoreWidgetPrivate.Options;

/**
 * @constructor
 */
chrome.webstoreWidgetPrivate.ShowWidgetEvent;

/**
 * @param {function(!chrome.webstoreWidgetPrivate.Options)} callback
 */
chrome.webstoreWidgetPrivate.ShowWidgetEvent.prototype.addListener = function(
    callback) {
};

/**
 * @param {function(!chrome.webstoreWidgetPrivate.Options)} callback
 */
chrome.webstoreWidgetPrivate.ShowWidgetEvent.prototype.removeListener =
    function(callback) {};

/**
 * @param {function(!chrome.webstoreWidgetPrivate.Options)} callback
 * @return {boolean}
 */
chrome.webstoreWidgetPrivate.ShowWidgetEvent.prototype.hasListener = function(
    callback) {
};

/**
 * @return {boolean}
 */
chrome.webstoreWidgetPrivate.ShowWidgetEvent.prototype.hasListeners =
    function() {};

/**
 * @typedef {!chrome.webstoreWidgetPrivate.ShowWidgetEvent}
 */
chrome.webstoreWidgetPrivate.onShowWidget;

/**
 * @param {function(Object<string>)} callback
 */
chrome.webstoreWidgetPrivate.getStrings = function(callback) {};

/**
 * Installs the app with ID {@code itemId}.
 * @param {string} itemId
 * @param {boolean} silentInstall
 * @param {function()} callback
 */
chrome.webstoreWidgetPrivate.installWebstoreItem = function(
    itemId, silentInstall, callback) {
};
