// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview JSCompiler externs for QUnit.
 * @externs
 */

/**
 * namespace
 * @const
 */
var QUnit = {};

/** @interface */
QUnit.Test = function() {};

/** @type {QUnit.Clock} */
QUnit.Test.prototype.clock;

/**
 */
QUnit.start = function() {};


/**
 */
QUnit.stop = function() {};


/**
 * @param {string} name
 * @param {function(this:QUnit.Test, !QUnit.Assert)} testFunction
 */
QUnit.test = function(name, testFunction) {};



/**
 * @constructor
 */
QUnit.Assert = function() {};


/**
 * @param {number} assertionCount
 */
QUnit.Assert.prototype.expect = function(assertionCount) {};


/** @constructor */
QUnit.Clock = function() {};


/** @param {number} ticks */
QUnit.Clock.prototype.tick = function(ticks) {};


/**
 * @param {*} a
 * @param {*} b
 * @param {string=} opt_desc
 */
QUnit.Assert.prototype.notEqual = function(a, b, opt_desc) {};


/**
 * @param {*} a
 * @param {*} b
 * @param {string=} opt_message
 */
QUnit.Assert.prototype.strictEqual = function(a, b, opt_message) {};


/**
 * @param {boolean} condition
 * @param {string=} opt_message
 */
QUnit.Assert.prototype.ok = function(condition, opt_message) {};


/**
 * @return {function():void}
 */
QUnit.Assert.prototype.async = function() {};


/**
 * @param {*} a
 * @param {*} b
 * @param {string=} opt_message
 */
QUnit.Assert.prototype.deepEqual = function(a, b, opt_message) {};


/**
 * @param {function()} a
 * @param {*=} opt_b
 * @param {string=} opt_message
 */
QUnit.Assert.prototype.throws = function(a, opt_b, opt_message) {};


/**
 * @param {*} a
 * @param {*} b
 * @param {string=} opt_message
 */
QUnit.Assert.prototype.equal = function(a, b, opt_message) {};


/** @param {Function} f */
QUnit.testStart = function(f) {};


/** @param {Function} f */
QUnit.testDone = function(f) {};


/**
 * @typedef {{
 *   beforeEach: (function(!QUnit.Assert) | undefined),
 *   afterEach: (function(!QUnit.Assert) | undefined)
 * }}
 */
QUnit.ModuleArgs;


/**
 * @param {string} desc
 * @param {QUnit.ModuleArgs=} opt_args=
 */
QUnit.module = function(desc, opt_args) {};
