// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {remoting.SessionLogger} */
var logger = null;

/** @type {function(Object)} */
var logWriter;

/** @type {sinon.Spy} */
var logWriterSpy = null;

/** @type {sinon.TestStub} */
var userAgentStub;

/** @type {remoting.Host} */
var fakeHost;

QUnit.module('SessionLogger', {
  beforeEach: function() {
    userAgentStub = sinon.stub(remoting, 'getUserAgent');
    userAgentStub.returns(
      'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_5) AppleWebKit/537.36' +
      ' (KHTML, like Gecko) Chrome/43.0.2357.81 Safari/537.36,gzip(gfe)');

    var spy = sinon.spy();
    logWriterSpy = /** @type {sinon.Spy} */ (spy);
    logWriter = /** @type {function(Object)} */ (spy);
    fakeHost = new remoting.Host('host_id');
    fakeHost.hostVersion = 'host_version';
    fakeHost.hostOs = remoting.ChromotingEvent.Os.OTHER;
    fakeHost.hostOsVersion = 'host_os_version';
    fakeHost.updatedTime = '2015-12-31T01:23:00';
  },
  afterEach: function() {
    userAgentStub.restore();
    logger = null;
  }
});

/**
 *  @param {QUnit.Assert} assert
 *  @param {number} index
 *  @param {Object} expected
 */
function verifyEvent(assert, index, expected) {
  var event = /** @type {Object} */ (logWriterSpy.getCall(index).args[0]);

  for (var key in expected) {
    assert.deepEqual(event[key], expected[key],
                     'Verifying ChromotingEvent.' + key);
  }
}

QUnit.test('logSignalStrategyProgress()', function(assert) {
  var Event = remoting.ChromotingEvent;
  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);

  logger.logSignalStrategyProgress(
      remoting.SignalStrategy.Type.WCS,
      remoting.FallbackSignalStrategy.Progress.TIMED_OUT);

  logger.logSignalStrategyProgress(
      remoting.SignalStrategy.Type.XMPP,
      remoting.FallbackSignalStrategy.Progress.SUCCEEDED);

  verifyEvent(assert, 0, {
    type: Event.Type.SIGNAL_STRATEGY_PROGRESS,
    signal_strategy_type: Event.SignalStrategyType.WCS,
    signal_strategy_progress: Event.SignalStrategyProgress.TIMED_OUT
  });

  verifyEvent(assert, 1, {
    type: Event.Type.SIGNAL_STRATEGY_PROGRESS,
    signal_strategy_type: Event.SignalStrategyType.XMPP,
    signal_strategy_progress: Event.SignalStrategyProgress.SUCCEEDED
  });
});

QUnit.test('logSessionStateChange()', function(assert){
  var clock = sinon.useFakeTimers(new Date('2015-12-31T01:23:05').getTime());
  var Event = remoting.ChromotingEvent;

  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);
  logger.setLogEntryMode(Event.Mode.ME2ME);
  logger.setConnectionType('stun');
  logger.setHost(fakeHost);

  logger.logSessionStateChange(
      remoting.ChromotingEvent.SessionState.CONNECTION_FAILED,
      new remoting.Error(remoting.Error.Tag.HOST_IS_OFFLINE));
  var sessionId = logger.getSessionId();

  assert.ok(sessionId !== null);

  verifyEvent(assert, 0, {
    type: Event.Type.SESSION_STATE,
    session_state: Event.SessionState.CONNECTION_FAILED,
    connection_error: Event.ConnectionError.HOST_OFFLINE,
    os: Event.Os.MAC,
    os_version: '10.9.5',
    cpu: 'Intel',
    browser_version: '43.0.2357.81',
    application_id: 'extensionId',
    role: Event.Role.CLIENT,
    mode: Event.Mode.ME2ME,
    connection_type: Event.ConnectionType.STUN,
    host_version: 'host_version',
    host_os: remoting.ChromotingEvent.Os.OTHER,
    host_os_version: 'host_os_version',
    host_last_heartbeat_elapsed_time: 5000,
    session_id: sessionId
  });
});

QUnit.test('logSessionStateChange() should handle XMPP error',
    function(assert){
  var Event = remoting.ChromotingEvent;

  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);
  logger.setLogEntryMode(Event.Mode.ME2ME);
  logger.setConnectionType('stun');
  logger.setHost(fakeHost);

  logger.logSessionStateChange(
      remoting.ChromotingEvent.SessionState.CONNECTION_FAILED,
      new remoting.Error(remoting.Error.Tag.HOST_IS_OFFLINE, '<fake-stanza/>'));
  var sessionId = logger.getSessionId();

  assert.ok(sessionId !== null);

  verifyEvent(assert, 0, {
    type: Event.Type.SESSION_STATE,
    session_state: Event.SessionState.CONNECTION_FAILED,
    connection_error: Event.ConnectionError.HOST_OFFLINE,
    os: Event.Os.MAC,
    os_version: '10.9.5',
    cpu: 'Intel',
    browser_version: '43.0.2357.81',
    application_id: 'extensionId',
    role: Event.Role.CLIENT,
    mode: Event.Mode.ME2ME,
    connection_type: Event.ConnectionType.STUN,
    host_version: 'host_version',
    host_os: remoting.ChromotingEvent.Os.OTHER,
    host_os_version: 'host_os_version',
    session_id: sessionId,
    xmpp_error: {
      raw_stanza: '<fake-stanza/>'
    }
  });
});

QUnit.test('logSessionStateChange() should handle sessionId change.',
  function(assert){
  var clock = sinon.useFakeTimers(new Date('2015-12-31T01:23:05').getTime());
  var Event = remoting.ChromotingEvent;

  // Creates the logger.
  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);
  logger.setLogEntryMode(Event.Mode.ME2ME);
  logger.setConnectionType('relay');
  logger.setHost(fakeHost);
  var oldSessionId = logger.getSessionId();

  // Expires the session id.
  clock.tick(remoting.SessionLogger.MAX_SESSION_ID_AGE + 100);

  // Logs the event.
  logger.logSessionStateChange(
      remoting.ChromotingEvent.SessionState.AUTHENTICATED);

  var newSessionId = logger.getSessionId();
  verifyEvent(assert, 0, {
    type: Event.Type.SESSION_ID_OLD,
    session_id: oldSessionId,
    os: Event.Os.MAC,
    os_version: '10.9.5',
    cpu: 'Intel',
    browser_version: '43.0.2357.81',
    application_id: 'extensionId',
    role: Event.Role.CLIENT,
    mode: Event.Mode.ME2ME,
    connection_type: Event.ConnectionType.RELAY,
    host_version: 'host_version',
    host_os: remoting.ChromotingEvent.Os.OTHER,
    host_os_version: 'host_os_version',
    host_last_heartbeat_elapsed_time: 5000
  });

  verifyEvent(assert, 1, {
    type: Event.Type.SESSION_ID_NEW,
    session_id: newSessionId,
    os: Event.Os.MAC,
    os_version: '10.9.5',
    cpu: 'Intel',
    browser_version: '43.0.2357.81',
    application_id: 'extensionId',
    role: Event.Role.CLIENT,
    mode: Event.Mode.ME2ME,
    connection_type: Event.ConnectionType.RELAY,
    host_version: 'host_version',
    host_os: remoting.ChromotingEvent.Os.OTHER,
    host_os_version: 'host_os_version',
    host_last_heartbeat_elapsed_time: 5000
  });

  verifyEvent(assert, 2, {
    type: Event.Type.SESSION_STATE,
    session_state: Event.SessionState.AUTHENTICATED,
    connection_error: Event.ConnectionError.NONE,
    os: Event.Os.MAC,
    os_version: '10.9.5',
    cpu: 'Intel',
    browser_version: '43.0.2357.81',
    application_id: 'extensionId',
    role: Event.Role.CLIENT,
    mode: Event.Mode.ME2ME,
    connection_type: Event.ConnectionType.RELAY,
    host_version: 'host_version',
    host_os: remoting.ChromotingEvent.Os.OTHER,
    host_os_version: 'host_os_version',
    host_last_heartbeat_elapsed_time: 5000,
    session_id: newSessionId
  });
});

QUnit.test('logSessionStateChange() should log session_duration.',
  function(assert){
  var clock = sinon.useFakeTimers();
  var Event = remoting.ChromotingEvent;

  // Creates the logger.
  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);
  logger.setLogEntryMode(Event.Mode.ME2ME);
  logger.setConnectionType('direct');
  logger.setHost(fakeHost);
  logger.setAuthTotalTime(1000);
  clock.tick(2500);

  // Logs the event.
  logger.logSessionStateChange(
    remoting.ChromotingEvent.SessionState.CONNECTED);

  verifyEvent(assert, 0, {
    type: Event.Type.SESSION_STATE,
    session_state: Event.SessionState.CONNECTED,
    connection_error: Event.ConnectionError.NONE,
    os: Event.Os.MAC,
    os_version: '10.9.5',
    cpu: 'Intel',
    browser_version: '43.0.2357.81',
    application_id: 'extensionId',
    role: Event.Role.CLIENT,
    mode: Event.Mode.ME2ME,
    connection_type: Event.ConnectionType.DIRECT,
    host_version: 'host_version',
    host_os: remoting.ChromotingEvent.Os.OTHER,
    host_os_version: 'host_os_version',
    session_id: logger.getSessionId(),
    session_duration: 1.5
  });
});

QUnit.test('logStatistics()', function(assert) {
  var clock = sinon.useFakeTimers();
  var Event = remoting.ChromotingEvent;

  // Creates the logger.
  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);
  logger.setLogEntryMode(Event.Mode.LGAPP);
  logger.setConnectionType('direct');
  logger.setHost(fakeHost);

  // Log the statistics
  logger.logStatistics({
    videoBandwidth: 3.0,
    captureLatency: 1.0,
    maxCaptureLatency: 3.0,
    encodeLatency: 2.0,
    maxEncodeLatency: 4.0,
    decodeLatency: 3.0,
    maxDecodeLatency: 5.0,
    renderLatency: 4.0,
    maxRenderLatency: 6.0,
    roundtripLatency: 5.0,
    maxRoundtripLatency: 7.0
  });

  verifyEvent(assert, 0, {
    type: Event.Type.CONNECTION_STATISTICS,
    os: Event.Os.MAC,
    os_version: '10.9.5',
    cpu: 'Intel',
    browser_version: '43.0.2357.81',
    application_id: 'extensionId',
    role: Event.Role.CLIENT,
    mode: Event.Mode.LGAPP,
    connection_type: Event.ConnectionType.DIRECT,
    host_version: 'host_version',
    host_os: remoting.ChromotingEvent.Os.OTHER,
    host_os_version: 'host_os_version',
    session_id: logger.getSessionId(),
    video_bandwidth: 3.0,
    capture_latency: 1.0,
    encode_latency: 2.0,
    decode_latency: 3.0,
    render_latency: 4.0,
    roundtrip_latency: 5.0,
    max_capture_latency: 3.0,
    max_encode_latency: 4.0,
    max_decode_latency: 5.0,
    max_render_latency: 6.0,
    max_roundtrip_latency: 7.0
  });
});

QUnit.test('logStatistics() should not log if all stats are zeros ',
    function(assert) {
  var clock = sinon.useFakeTimers();
  var Event = remoting.ChromotingEvent;

  // Creates the logger.
  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);

  logger.logStatistics({
    videoBandwidth: 0.0,
    captureLatency: 0.0,
    encodeLatency: 0.0,
    decodeLatency: 0.0,
    renderLatency: 0.0,
    roundtripLatency: 0.0
  });

  sinon.assert.notCalled(logWriterSpy);

});

QUnit.test('incrementFeatureUsage() and flushFeatureTracker()',
    function(assert) {
  var Event = remoting.ChromotingEvent;

  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);

  logger.incrementFeatureUsage('fullscreen_esc_count');

  sinon.assert.notCalled(logWriterSpy);

  logger.flushFeatureTracker();
  verifyEvent(assert, 0, {
    type: Event.Type.FEATURE_TRACKING,
    feature_tracker: {fullscreen_esc_count: 1}
  });
});

QUnit.test('flushFeatureTracker() with default tracker',
    function(assert) {
  var Event = remoting.ChromotingEvent;

  logger = new remoting.SessionLogger(Event.Role.CLIENT, logWriter);

  sinon.assert.notCalled(logWriterSpy);

  logger.flushFeatureTracker();
  verifyEvent(assert, 0, {
    type: Event.Type.FEATURE_TRACKING,
    feature_tracker: {fullscreen_esc_count: 0}
  });
});

})();
