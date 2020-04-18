// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @fileoverview Element listener interface declaration
 */
goog.provide('ink.ElementListener');

/**
 * @interface
 */
ink.ElementListener = function() {};

/**
 * @param {string} uuid
 * @param {string} encodedElement
 * @param {string} encodedTransform
 */
ink.ElementListener.prototype.onElementCreated = function(
    uuid, encodedElement, encodedTransform) {};

/**
 * @param {Array.<string>} uuids
 * @param {Array.<string>} encodedTransforms
 */
ink.ElementListener.prototype.onElementsMutated = function(
    uuids, encodedTransforms) {};

/**
 * @param {Array.<string>} uuids
 */
ink.ElementListener.prototype.onElementsRemoved = function(uuids) {};
