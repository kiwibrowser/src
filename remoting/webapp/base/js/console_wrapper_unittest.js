// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @suppress {checkTypes|reportUnknownTypes} */

(function() {

'use strict';

var oldLog = null;
var oldWarn = null;
var oldError = null;
var oldAssert = null;

var logSpy = null
var warnSpy = null;
var errorSpy = null;
var assertSpy = null;

var LOCATION_PREFIX = 'console_wrapper_unittest.js:'

QUnit.module('console_wrapper', {
  beforeEach: function() {
    oldLog = console.log;
    oldWarn = console.warn;
    oldError = console.error;
    oldAssert = console.assert;

    logSpy = sinon.spy();
    warnSpy = sinon.spy();
    errorSpy = sinon.spy();
    assertSpy = sinon.spy();

    console.log = logSpy;
    console.warn = warnSpy;
    console.error = errorSpy;
    console.assert = assertSpy;
  },

  afterEach: function() {
    remoting.ConsoleWrapper.getInstance().deactivate();

    console.log = oldLog;
    console.warn = oldWarn;
    console.error = oldError;
    console.assert = oldAssert;

    logSpy = null;
    warnSpy = null;
    errorSpy = null;
    assertSpy = null;
    oldLog = null;
    oldWarn = null;
    oldError = null;
    oldAssert = null;
  }
});

QUnit.test('calls to console methods are passed through when wrapped.',
           function(assert)
{
  remoting.ConsoleWrapper.getInstance().activate(
      0,  // No history
      remoting.ConsoleWrapper.LogType.LOG,
      remoting.ConsoleWrapper.LogType.WARN,
      remoting.ConsoleWrapper.LogType.ERROR,
      remoting.ConsoleWrapper.LogType.ASSERT);

  // Ignore any logging in the ctor or activate() methods.
  logSpy.reset();
  warnSpy.reset();
  errorSpy.reset();
  assertSpy.reset();

  console.log('log', 1, 2, 3);
  assert.ok(logSpy.calledWith('log', 1, 2, 3));

  console.warn('warn', 1, 2, 3);
  assert.ok(warnSpy.calledWith('warn', 1, 2, 3));

  console.error('error', 1, 2, 3);
  assert.ok(errorSpy.calledWith('error', 1, 2, 3));

  console.assert(false, 'assert', 1, 2, 3);
  assert.ok(assertSpy.calledWith(false, 'assert', 1, 2, 3));

  // Verify that all the above calls also include the call-site, and validate
  // it as far as is possible without making the test too flaky.
  assert.ok(typeof(logSpy.firstCall.args[4]) == 'string');
  assert.ok(logSpy.firstCall.args[4].startsWith(LOCATION_PREFIX));

  assert.ok(typeof(warnSpy.firstCall.args[4]) == 'string');
  assert.ok(warnSpy.firstCall.args[4].startsWith(LOCATION_PREFIX));

  assert.ok(typeof(errorSpy.firstCall.args[4]) == 'string');
  assert.ok(errorSpy.firstCall.args[4].startsWith(LOCATION_PREFIX));

  assert.ok(typeof(assertSpy.firstCall.args[5]) == 'string');
  assert.ok(assertSpy.firstCall.args[5].startsWith(LOCATION_PREFIX));

  // Verify that console methods are no longer wrapped after deactivate().
  remoting.ConsoleWrapper.getInstance().deactivate();
  logSpy.reset();
  console.log('Should not be intercepted.');
  assert.equal(logSpy.firstCall.args.length, 1);
});


QUnit.test('calls to console methods are saved when wrapped.',
           function(assert)
{
  remoting.ConsoleWrapper.getInstance().activate(
      3,
      remoting.ConsoleWrapper.LogType.LOG,
      remoting.ConsoleWrapper.LogType.WARN,
      remoting.ConsoleWrapper.LogType.ERROR);

  console.log('first', 1, 2, 3);
  assert.equal(remoting.ConsoleWrapper.getInstance().getHistory().length, 1);
  var entry = remoting.ConsoleWrapper.getInstance().getHistory()[0];
  assert.equal(entry.type,'log');
  assert.equal(entry.message, '["first",1,2,3]');
  assert.ok(entry.caller.startsWith(LOCATION_PREFIX));
  assert.equal(entry.timestamp.toGMTString(), 'Thu, 01 Jan 1970 00:00:00 GMT');

  this.clock.tick(1000);
  console.log('second', 1, 2, 3);
  assert.equal(remoting.ConsoleWrapper.getInstance().getHistory().length, 2);
  entry = remoting.ConsoleWrapper.getInstance().getHistory()[1];
  assert.equal(entry.type, 'log');
  assert.equal(entry.message, '["second",1,2,3]');
  assert.ok(entry.caller.startsWith(LOCATION_PREFIX));
  assert.equal(entry.timestamp.toGMTString(), 'Thu, 01 Jan 1970 00:00:01 GMT');

  this.clock.tick(1000);
  console.warn('third', 1, 2, 3);
  assert.equal(remoting.ConsoleWrapper.getInstance().getHistory().length, 3);
  entry = remoting.ConsoleWrapper.getInstance().getHistory()[2];
  assert.equal(entry.type, 'warn');
  assert.equal(entry.message, '["third",1,2,3]');
  assert.ok(entry.caller.startsWith(LOCATION_PREFIX));
  assert.equal(entry.timestamp.toGMTString(),'Thu, 01 Jan 1970 00:00:02 GMT');

  this.clock.tick(1000);
  console.error('fourth', 1, 2, 3);
  assert.equal(remoting.ConsoleWrapper.getInstance().getHistory().length, 3);
  entry = remoting.ConsoleWrapper.getInstance().getHistory()[2];
  assert.equal(entry.type, 'error');
  assert.equal(entry.message, '["fourth",1,2,3]');
  assert.ok(entry.caller.startsWith(LOCATION_PREFIX));
  assert.equal(entry.timestamp.toGMTString(), 'Thu, 01 Jan 1970 00:00:03 GMT');
});


QUnit.test('calls to assert are saved only for failures.',
           function(assert)
{
  remoting.ConsoleWrapper.getInstance().activate(
      2,
      remoting.ConsoleWrapper.LogType.ASSERT);
  console.assert(true, 'first', 1, 2, 3);
  this.clock.tick(1000);
  console.assert(false, 'second', 1, 2, 3);
  assert.equal(remoting.ConsoleWrapper.getInstance().getHistory().length, 1);
  var entry = remoting.ConsoleWrapper.getInstance().getHistory()[0];
  assert.equal(entry.type, 'assert');
  assert.equal(entry.message, '[false,"second",1,2,3]');
  assert.ok(entry.caller.startsWith(LOCATION_PREFIX));
  assert.equal(entry.timestamp.toGMTString(), 'Thu, 01 Jan 1970 00:00:01 GMT');
});

})();
