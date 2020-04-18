// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains various hacks needed to inform JSCompiler of various
// test-specific properties and methods. It is used only with JSCompiler to
// verify the type-correctness of our code.

/** @suppress {duplicate} */
var browserTest = browserTest || {};

/** @interface */
browserTest.TestableClass = function() {};

/** @param {*} data */
browserTest.TestableClass.prototype.run = function(data) {};


/** @constructor */
window.DomAutomationControllerMessage = function() {
  /** @type {boolean} */
  this.succeeded = false;
  /** @type {string} */
  this.error_message = '';
  /** @type {string} */
  this.stack_trace = '';
};

/** @constructor */
window.DomAutomationController = function() {};

/** @param {string} json A stringified DomAutomationControllerMessage. */
window.DomAutomationController.prototype.send = function(json) {};
