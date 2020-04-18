// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var sinon = sinon || {};

/** @type {Object} */
sinon.assert = {};

/**
 * @param {(sinon.Spy|Function)} f
 */
sinon.assert.called = function(f) {};

/**
 * @param {(sinon.Spy|Function)} f
 */
sinon.assert.calledOnce = function(f) {};

/**
 * @param {(sinon.Spy|Function)} f
 * @param {...} data
 */
sinon.assert.calledWith = function(f, data) {};

/**
 * @param {*} value
 * @return {Object}
 */
sinon.match = function(value) {};
/**
 * @param {(sinon.Spy|Function)} f
 */
sinon.assert.notCalled = function(f) {};

/** @constructor */
sinon.Clock = function() {};

/** @param {number} ms */
sinon.Clock.prototype.tick = function(ms) {};

/** @return {void} */
sinon.Clock.prototype.restore = function() {};

/**
 * @param {number=} opt_now
 * @return {sinon.Clock}
 */
sinon.useFakeTimers = function(opt_now) {};

/** @constructor */
sinon.Expectation = function() {};

/** @return {sinon.Expectation} */
sinon.Expectation.prototype.once = function() {};

/** @return {sinon.Expectation} */
sinon.Expectation.prototype.never = function() {};

/**
 * @param {number} times
 * @return {sinon.Expectation}
 */
sinon.Expectation.prototype.exactly = function(times) {};

/**
 * @param {...} data
 * @return {sinon.Expectation}
 */
sinon.Expectation.prototype.withArgs = function(data) {};

/** @return {boolean} */
sinon.Expectation.prototype.verify = function() {};

/** @param {...} data */
sinon.Expectation.prototype.returns = function(data) {};

/**
 * @param {Object} obj
 * @return {sinon.Mock}
 */
sinon.mock = function(obj) {};

/** @constructor */
sinon.Mock = function() {};

/**
 * @param {string} method
 * @return {sinon.Expectation}
 */
sinon.Mock.prototype.expects = function(method) {};

/**
 * @return {void}
 */
sinon.Mock.prototype.restore = function() {};

/**
 * @return {boolean}
 */
sinon.Mock.prototype.verify = function() {};

/** @type {function(...):Function} */
sinon.spy = function() {};

/**
 * This is a jscompile type that can be OR'ed with the actual type to make
 * jscompile aware of the sinon.spy functions that are added to the base
 * type.
 * Example: Instead of specifying a type of
 *   {function():void}
 * the following can be used to add the sinon.spy functions:
 *   {(sinon.Spy|function():void)}
 *
 * @interface
 */
sinon.Spy = function() {};

/** @type {number} */
sinon.Spy.prototype.callCount;

/** @type {boolean} */
sinon.Spy.prototype.called;

/** @type {boolean} */
sinon.Spy.prototype.calledOnce;

/** @type {boolean} */
sinon.Spy.prototype.calledTwice;

/** @type {function(...):boolean} */
sinon.Spy.prototype.calledWith = function() {};

/** @type {function(number):{args:Array}} */
sinon.Spy.prototype.getCall = function(index) {};

sinon.Spy.prototype.reset = function() {};

sinon.Spy.prototype.restore = function() {};

/** @type {Array<Array<*>>} */
sinon.Spy.prototype.args;

/**
 * @param {Object=} opt_obj
 * @param {string=} opt_method
 * @param {Function=} opt_stubFunction
 * @return {sinon.TestStub}
 */
sinon.stub = function(opt_obj, opt_method, opt_stubFunction) {};

/**
* TODO(jrw): rename to |sinon.Stub| for consistency
 * @interface
 * @extends {sinon.Spy}
 */
sinon.TestStub = function() {};

/** @type {function(number):{args:Array}} */
sinon.TestStub.prototype.getCall = function(index) {};

sinon.TestStub.prototype.restore = function() {};

/** @param {*} a */
sinon.TestStub.prototype.returns = function(a) {};

/** @type {function(...):sinon.Expectation} */
sinon.TestStub.prototype.withArgs = function() {};

/** @type {function(...):sinon.Expectation} */
sinon.TestStub.prototype.onFirstCall = function() {};

/** @type {function(...):sinon.Expectation} */
sinon.TestStub.prototype.callsArgWith = function() {};

/** @returns {Object}  */
sinon.createStubInstance = function (/** * */ constructor) {};

/** @interface */
sinon.FakeXhrCtrl = function() {};

/**
 * @type {?function(!sinon.FakeXhr)}
 */
sinon.FakeXhrCtrl.prototype.onCreate;

/**
 * @type {function():void}
 */
sinon.FakeXhrCtrl.prototype.restore;

/** @return {sinon.FakeXhrCtrl} */
sinon.useFakeXMLHttpRequest = function() {};

/** @interface */
sinon.FakeXhr = function() {};

/** @type {number} */
sinon.FakeXhr.prototype.readyState;

/** @type {string} */
sinon.FakeXhr.prototype.method;

/** @type {string} */
sinon.FakeXhr.prototype.url;

/** @type {boolean} */
sinon.FakeXhr.prototype.withCredentials;

/** @type {?string} */
sinon.FakeXhr.prototype.requestBody;

/** @type {!Object<string>} */
sinon.FakeXhr.prototype.requestHeaders;

/**
 * @param {number} status
 * @param {!Object<string>} headers
 * @param {?string} content
 */
sinon.FakeXhr.prototype.respond;

/**
 * @param {string} event
 * @param {Function} handler
 */
sinon.FakeXhr.prototype.addEventListener;
