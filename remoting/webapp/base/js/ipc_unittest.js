// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {base.Ipc} */
var ipc_;

QUnit.module('base.Ipc', {
  beforeEach: function() {
    ipc_ = base.Ipc.getInstance();
  },
  afterEach: function() {
    base.Ipc.deleteInstance();
    ipc_ = null;
  }
});

QUnit.test(
  'register() should return false if the request type was already registered',
  function(assert) {
    var handler1 = function() {};
    var handler2 = function() {};
    assert.equal(true, ipc_.register('foo', handler1));
    assert.equal(false, ipc_.register('foo', handler2));
});

QUnit.test(
  'send() should invoke a registered handler with the correct arguments',
  function(assert) {
    var handler = sinon.spy();
    var argArray = [1, 2, 3];
    var argDict = {
      key1: 'value1',
      key2: false
    };

    ipc_.register('foo', handler);
    return base.Ipc.invoke('foo', 1, false, 'string', argArray, argDict).then(
      function() {
        sinon.assert.calledWith(handler, 1, false, 'string', argArray, argDict);
    });
});

QUnit.test(
  'send() should not invoke a handler that is unregistered',
  function(assert) {
    var handler = sinon.spy();
    ipc_.register('foo', handler);
    ipc_.unregister('foo');
    return base.Ipc.invoke('foo', 'hello', 'world').then(function() {
      assert.ok(false, 'Invoking an unregistered handler should fail.');
    }).catch(function(error) {
      sinon.assert.notCalled(handler);
      assert.equal(error, base.Ipc.Error.UNSUPPORTED_REQUEST_TYPE);
    });
});

QUnit.test(
  'send() should raise exceptions on unknown request types',
  function(assert) {
    var handler = sinon.spy();
    ipc_.register('foo', handler);
    return base.Ipc.invoke('bar', 'hello', 'world').then(function() {
      assert.ok(false, 'Invoking unknown request types should fail.');
    }).catch(function(error) {
      assert.equal(error, base.Ipc.Error.UNSUPPORTED_REQUEST_TYPE);
    });
});

QUnit.test(
  'send() should raise exceptions on request from another extension',
  function(assert) {
    var handler = sinon.spy();
    var oldId = chrome.runtime.id;
    ipc_.register('foo', handler);
    chrome.runtime.id = 'foreign-extension';
    var promise = base.Ipc.invoke('foo', 'hello', 'world').then(function() {
      assert.ok(false, 'Requests from another extension should fail.');
    }).catch(function(error) {
      assert.equal(error, base.Ipc.Error.UNAUTHORIZED_REQUEST_ORIGIN);
    });
    chrome.runtime.id = oldId;
    return promise;
});

QUnit.test(
  'send() should not raise exceptions for externally-accessible methods',
  function(assert) {
    var handler = function(request) { return request; };
    var oldId = chrome.runtime.id;
    ipc_.register('foo', handler, true);
    chrome.runtime.id = 'foreign-extension';
    var promise = base.Ipc.invoke('foo', 'payload').then(function(response) {
      assert.equal(response, 'payload');
    });
    chrome.runtime.id = oldId;
    return promise;
});

QUnit.test(
  'send() should pass exceptions raised by the handler to the caller',
  function(assert) {
    var handler = function() {
      throw new Error('Whatever can go wrong, will go wrong.');
    };
    ipc_.register('foo', handler);
    return base.Ipc.invoke('foo').then(function() {
      assert.ok(false, 'Exceptions expected.');
    }).catch(function(error) {
      assert.equal(error, 'Whatever can go wrong, will go wrong.');
    });
});

QUnit.test(
  'send() should pass the return value of the handler to the caller',
  function(assert) {
    var handlers = {
      'boolean': function() { return false; },
      'number': function() { return 12; },
      'string': function() { return 'string'; },
      'array': function() { return [1, 2]; },
      'dict': function() { return {key1: 'value1', key2: 'value2'}; }
    };

    var testCases = [];
    for (var ipcName in handlers) {
      ipc_.register(ipcName, handlers[ipcName]);
      testCases.push(base.Ipc.invoke(ipcName));
    }

    return Promise.all(testCases).then(function(results){
      assert.equal(results[0], false);
      assert.equal(results[1], 12);
      assert.equal(results[2], 'string');
      assert.deepEqual(results[3], [1,2]);
      assert.deepEqual(results[4], {key1: 'value1', key2: 'value2'});
    });
});

QUnit.test(
  'send() supports asynchronous handlers',
  function(assert) {
    var success = function() {
      return new Promise(function(resolve) { resolve('result'); });
    };
    var failure = function() {
      return new Promise(function() {
        throw new Error('Whatever can go wrong, will go wrong.');
      });
    };
    ipc_.register('foo', success);
    ipc_.register('bar', failure);
    var testCases = [];
    testCases.push(base.Ipc.invoke('foo').then(function(response) {
      assert.equal(response, 'result');
    }));
    testCases.push(base.Ipc.invoke('bar').then(function() {
      assert.ok(false, 'bar method expected to fail.');
    }).catch(function(error) {
      assert.equal(error, 'Whatever can go wrong, will go wrong.');
    }))
    return Promise.all(testCases);
});

})();
