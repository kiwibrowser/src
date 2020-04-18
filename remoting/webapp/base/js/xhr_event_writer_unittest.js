// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {remoting.XhrEventWriter} */
var eventWriter = null;
/** @type {!chromeMocks.StorageArea} */
var mockStorage;
/** @type {sinon.TestStub} */
var isOnlineStub;

/**
 * Flush the writer and ensure no outgoing requests is made.
 *
 * @param {QUnit.Assert} assert
 * @param {remoting.XhrEventWriter} writer
 * @return {Promise}
 */
function flushAndEnsureNoRequests(assert, writer) {
  var requestReceived = false;
  remoting.MockXhr.setResponseFor(
    'POST', 'fake_url', function(/** remoting.MockXhr */ xhr) {
      xhr.setTextResponse(200, '');
      requestReceived = true;
    });
  return writer.flush().then(function(){
    assert.ok(!requestReceived);
  });
}

/**
 * Flush the writer and ensure no outgoing requests is made.
 *
 * @param {QUnit.Assert} assert
 * @param {Array<string>} requestIds
 * @return {Map<string, boolean>}
 */
function expectRequests(assert, requestIds) {
  var expected = new Map();
  requestIds.forEach(function(/** string */ id){
    expected.set(id, true);
  });

  remoting.MockXhr.setResponseFor(
    'POST', 'fake_url', function(/** remoting.MockXhr */ xhr) {
    var jsonContent = /** @type {Object} */ (xhr.params.jsonContent);
    var requests = /** @type {Array} */ (jsonContent['event']);
    requests.forEach(function(/** Object */ request){
      var id = /** @type {string} */ (request['id']);
      assert.ok(expected.has(id), 'Unexpected request :=' + id);
      expected.delete(id);
    });
    xhr.setTextResponse(200, '');
  }, true /* reuse */);
  return expected;
}


QUnit.module('XhrEventWriter', {
  beforeEach: function() {
    remoting.MockXhr.activate();
    mockStorage = new chromeMocks.StorageArea();
    eventWriter = new remoting.XhrEventWriter(
        'fake_url', mockStorage, 'fake-storage-key');
    isOnlineStub = sinon.stub(base, 'isOnline');
  },
  afterEach: function() {
    isOnlineStub.restore();
    remoting.MockXhr.restore();
  }
});

QUnit.test('loadPendingRequests() handles empty storage.', function(assert){
  return eventWriter.loadPendingRequests().then(function(){
    return flushAndEnsureNoRequests(assert, eventWriter);
  });
});

QUnit.test('loadPendingRequests() handles corrupted data.', function(assert){
  var storage = mockStorage.mock$getStorage();
  storage['fake-storage-key'] = 'corrupted_data';
  return eventWriter.loadPendingRequests().then(function(){
    return flushAndEnsureNoRequests(assert, eventWriter);
  });
});

QUnit.test('write() should post XHR to server.', function(assert){
  isOnlineStub.returns(true);
  var outstanding = expectRequests(assert, ['1']);
  return eventWriter.write({ id: '1'}).then(function(){
    assert.equal(outstanding.size, 0, outstanding.toString());
  });
});

QUnit.test('flush() should retry requests if OFFLINE.', function(assert){
  isOnlineStub.returns(false);
  /** @type {Map<string, boolean>} */
  var outstanding = null;

  return eventWriter.write({ id: '1'}).then(function(){
    assert.ok(false, 'Expect to fail.');
  }).catch(function() {
    isOnlineStub.returns(true);
    outstanding = expectRequests(assert, ['1']);
    return eventWriter.flush();
  }).then(function() {
    assert.equal(outstanding.size, 0, outstanding.toString());
  });
});

QUnit.test('flush() should handle batch requests.', function(assert){
  /** @type {Map<string, boolean>} */
  var outstanding = null;
  isOnlineStub.returns(false);

  return Promise.all([
    eventWriter.write({id: '1'}),
    eventWriter.write({id: '2'}),
    eventWriter.write({id: '3'})
  ]).then(function() {
    assert.ok(false, 'Expect to fail.');
  }).catch(function() {
    isOnlineStub.returns(true);
    outstanding = expectRequests(assert, ['1', '2', '3']);
    return eventWriter.flush();
  }).then(function() {
    assert.equal(outstanding.size, 0, outstanding.toString());
  });
});

QUnit.test('flush() should not send duplicate requests.', function(assert){
  isOnlineStub.returns(true);
  var outstanding = expectRequests(assert, ['1', '2', '3', '4', '5']);

  return Promise.all([
    eventWriter.write({ id: '1'}),
    eventWriter.write({ id: '2'}),
    eventWriter.write({ id: '3'}),
    eventWriter.write({ id: '4'}),
    eventWriter.write({ id: '5'})
  ]).then(function(){
    assert.equal(outstanding.size, 0, outstanding.toString());
  });
});

QUnit.test('flush() should not retry on server error.', function(assert){
  isOnlineStub.returns(true);
  remoting.MockXhr.setResponseFor(
    'POST', 'fake_url', function(/** remoting.MockXhr */ xhr) {
    assert.deepEqual(xhr.params.jsonContent, {event: [{hello: 'world'}]});
    xhr.setTextResponse(500, '');
  });

  return eventWriter.write({ hello: 'world'}).then(function(){
    assert.ok(false, 'Expect to fail.');
  }).catch(function() {
    return flushAndEnsureNoRequests(assert, eventWriter);
  });
});

QUnit.test('writeToStorage() should save pending requests.', function(assert){
  var requestReceived = false;
  var newEventWriter = new remoting.XhrEventWriter(
      'fake_url', mockStorage, 'fake-storage-key');
  isOnlineStub.returns(false);

  return eventWriter.write({ hello: 'world'}).then(function(){
    assert.ok(false, 'Expect to fail.');
  }).catch(function(){
    return eventWriter.writeToStorage();
  }).then(function() {
  return newEventWriter.loadPendingRequests();
  }).then(function() {
    isOnlineStub.returns(true);
    remoting.MockXhr.setResponseFor(
      'POST', 'fake_url', function(/** remoting.MockXhr */ xhr) {
      assert.deepEqual(xhr.params.jsonContent, {event: [{hello: 'world'}]});
      requestReceived = true;
      xhr.setTextResponse(200, '');
    });
    return newEventWriter.flush();
  }).then(function(){
    assert.ok(requestReceived);
  });
});

})();
