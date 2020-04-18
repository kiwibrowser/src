// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {remoting.TelemetryEventWriter.Service} */
var service;
/** @type {remoting.XhrEventWriter} */
var eventWriter = null;

/** @type {remoting.SessionLogger} */
var logger = null;

/**
 *  @param {sinon.TestStub} stub
 *  @param {QUnit.Assert} assert
 *  @param {number} index
 *  @param {Object} expected
 */
function verifyEventAtIndex(stub, assert, index, expected) {
  var event = /** @type {Object} */ (stub.getCall(index).args[0]);
  for (var key in expected) {
    if (event[key] != expected[key]) {
      return false;
    }
  }
  return true;
}

function verifyEvent(stub, assert, expected) {
  var match = false;
  for (var index = 0; index < stub.callCount; ++ index) {
    if (verifyEventAtIndex(stub, assert, index, expected)) {
      match = true;
      break;
    }
  }
  assert.ok(match, 'No logged event matches expected.');
}

QUnit.module('TelemetryEventWriter', {
  beforeEach: function() {
    remoting.MockXhr.activate();
    var ipc = base.Ipc.getInstance();
    eventWriter =
        new remoting.XhrEventWriter('URL', chrome.storage.local, 'fake-key');
    service = new remoting.TelemetryEventWriter.Service(ipc, eventWriter);
    logger = new remoting.SessionLogger(
      remoting.ChromotingEvent.Role.CLIENT,
      remoting.TelemetryEventWriter.Client.write);
    logger.setLogEntryMode(remoting.ChromotingEvent.Mode.ME2ME);
    var fakeHost = new remoting.Host('fake_id');
    fakeHost.hostOs = remoting.ChromotingEvent.Os.OTHER;
    fakeHost.hostOsVersion = 'host_os_version';
    fakeHost.hostVersion = 'host_version';
    logger.setHost(fakeHost);
    logger.setConnectionType('stun');
    chrome.storage = {
      sync: {
        get: sinon.stub(),
      }
    };
    chrome.storage.sync.get.callsArgWith(1, 0);
  },
  afterEach: function() {
    base.dispose(service);
    service = null;
    base.Ipc.deleteInstance();
    remoting.MockXhr.restore();
  }
});

QUnit.test('Client.write() should write request.', function(assert){
  var mockEventWriter = sinon.mock(eventWriter);
  return service.init().then(function(){
    mockEventWriter.expects('write').once().withArgs({
      id: '1',
      website_and_app_user: false
    });
    return remoting.TelemetryEventWriter.Client.write({id: '1'});
  }).then(function(){
    mockEventWriter.verify();
  });
});

QUnit.test('should save log requests on suspension.', function(assert){
  var mockEventWriter = sinon.mock(eventWriter);
  mockEventWriter.expects('writeToStorage').once();
  return service.init().then(function(){
    var mockSuspendEvent =
        /** @type {chromeMocks.Event} */ (chrome.runtime.onSuspend);
    mockSuspendEvent.mock$fire();
  }).then(function() {
    mockEventWriter.verify();
  });
});

QUnit.test('should flush log requests when online.', function(assert) {
  var mockEventWriter = sinon.mock(eventWriter);
  mockEventWriter.expects('flush').once();
  return service.init().then(function() {
    mockEventWriter.verify();
    mockEventWriter.expects('flush').once();
    window.dispatchEvent(new CustomEvent('online'));
  }).then(function() {
    mockEventWriter.verify();
  });
});

QUnit.test('should send CANCELED event when window is closed while started.',
  function(assert) {
  var writeStub = sinon.stub(eventWriter, 'write');
  return service.init().then(function() {
    chrome.app.window.current().id = 'fake-window-id';
  }).then(function() {
    logger.logSessionStateChange(
        remoting.ChromotingEvent.SessionState.STARTED);
  }).then(function() {
    return service.unbindSession('fake-window-id');
  }).then(function() {
    var Event = remoting.ChromotingEvent;
    verifyEvent(writeStub, assert, {
      type: Event.Type.SESSION_STATE,
      session_state: Event.SessionState.CONNECTION_CANCELED,
      connection_error: Event.ConnectionError.NONE,
      application_id: 'extensionId',
      role: Event.Role.CLIENT,
      mode: Event.Mode.ME2ME,
      connection_type: Event.ConnectionType.STUN,
      host_version: 'host_version',
      host_os: remoting.ChromotingEvent.Os.OTHER,
      host_os_version: 'host_os_version'
    });
  });
});

QUnit.test('should send CANCELED event when window is closed while connecting.',
  function(assert) {
  var writeStub = sinon.stub(eventWriter, 'write');
  return service.init().then(function() {
    chrome.app.window.current().id = 'fake-window-id';
  }).then(function() {
    logger.logSessionStateChange(
        remoting.ChromotingEvent.SessionState.CONNECTING);
  }).then(function() {
    return service.unbindSession('fake-window-id');
  }).then(function() {
    var Event = remoting.ChromotingEvent;
    verifyEvent(writeStub, assert, {
      type: Event.Type.SESSION_STATE,
      session_state: Event.SessionState.CONNECTION_CANCELED,
      connection_error: Event.ConnectionError.NONE,
      application_id: 'extensionId',
      role: Event.Role.CLIENT,
      mode: Event.Mode.ME2ME,
      connection_type: Event.ConnectionType.STUN,
      host_version: 'host_version',
      host_os: remoting.ChromotingEvent.Os.OTHER,
      host_os_version: 'host_os_version'
    });
  });
});

QUnit.test('should send CLOSED event when window is closed while connected.',
  function(assert) {
  var writeStub = sinon.stub(eventWriter, 'write');

  return service.init().then(function() {
    chrome.app.window.current().id = 'fake-window-id';
  }).then(function() {
    logger.logSessionStateChange(
        remoting.ChromotingEvent.SessionState.CONNECTING);
  }).then(function() {
    logger.logSessionStateChange(
        remoting.ChromotingEvent.SessionState.CONNECTED);
  }).then(function() {
    return service.unbindSession('fake-window-id');
  }).then(function() {
    var Event = remoting.ChromotingEvent;
    verifyEvent(writeStub, assert, {
      type: Event.Type.SESSION_STATE,
      session_state: Event.SessionState.CLOSED,
      connection_error: Event.ConnectionError.NONE,
      application_id: 'extensionId',
      role: Event.Role.CLIENT,
      mode: Event.Mode.ME2ME,
      connection_type: Event.ConnectionType.STUN,
      host_version: 'host_version',
      host_os: remoting.ChromotingEvent.Os.OTHER,
      host_os_version: 'host_os_version'
    });
  });
});

QUnit.test('should not send CLOSED event when window is closed unconnected.',
  function(assert) {

  var mockEventWriter = sinon.mock(eventWriter);
  mockEventWriter.expects('write').exactly(2);

  return service.init().then(function() {
    chrome.app.window.current().id = 'fake-window-id';
  }).then(function() {
    logger.logSessionStateChange(
        remoting.ChromotingEvent.SessionState.CONNECTING);
  }).then(function() {
    logger.logSessionStateChange(
        remoting.ChromotingEvent.SessionState.CONNECTION_FAILED);
  }).then(function() {
    return service.unbindSession('fake-window-id');
  }).then(function() {
    mockEventWriter.verify();
  });
});

})();
