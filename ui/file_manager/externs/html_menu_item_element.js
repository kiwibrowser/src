// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @extends {HTMLElement}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#the-menuitem-element
 */
function HTMLMenuItemElement() {}

/**
 * @type {string}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#dom-menuitem-type
 */
HTMLMenuItemElement.prototype.type;

/**
 * @type {string}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#dom-menuitem-label
 */
HTMLMenuItemElement.prototype.label;

/**
 * @type {string}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#dom-menuitem-icon
 */
HTMLMenuItemElement.prototype.icon;

/**
 * @type {boolean}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#dom-menuitem-disabled
 */
HTMLMenuItemElement.prototype.disabled;

/**
 * @type {boolean}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#dom-menuitem-checked
 */
HTMLMenuItemElement.prototype.checked;

/**
 * @type {string}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#dom-menuitem-radiogroup
 */
HTMLMenuItemElement.prototype.radiogroup;

/**
 * @type {boolean}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#dom-menuitem-default
 */
HTMLMenuItemElement.prototype.default;

/**
 * @type {HTMLElement|undefined}
 * @see http://www.w3.org/html/wg/drafts/html/master/interactive-elements.html#dom-menuitem-command
 */
HTMLMenuItemElement.prototype.command;
