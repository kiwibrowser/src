// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/** @suppress {duplicate} */
var remoting = remoting || {};

(function() {

'use strict';

/** @type {remoting.Me2MeTestDriver} */
var testDriver;

QUnit.module('Me2Me Telemetry Integration', {
  // We rely on our telemetry data to gain insight into the reliability and
  // the durability on our connections.  This test verifies the integrity of our
  // telemetry by ensuring that certain well known connection sequences will
  // generate the correct sequence of telemetry data.
  beforeEach: function() {
    testDriver = new remoting.Me2MeTestDriver();
  },
  afterEach: function() {
    base.dispose(testDriver);
    testDriver = null;
  }
});

/**
 * @param {remoting.Me2MeTestDriver} testDriver
 * @param {Object} baseEvent
 * @param {Array<remoting.ChromotingEvent.SessionState>} sequence
 */
var expectSequence = function(testDriver, baseEvent, sequence) {
  var expectedEvents = sequence.map(
    /**
     * @param {remoting.ChromotingEvent.SessionState} state
     * @param {number} i
     */
    function(state, i) {
      var event = /** @type {Object} */ (base.deepCopy(baseEvent));
      event.session_state = state;
      if (i > 0) {
        event.previous_session_state = sequence[i -1];
      }
      return event;
  });
  testDriver.expectEvents(expectedEvents);
};

/**
 * @param {remoting.Me2MeTestDriver} testDriver
 * @param {Object} baseEvent
 */
var expectSucceeded = function(testDriver, baseEvent) {
  var ChromotingEvent = remoting.ChromotingEvent;
  expectSequence(testDriver, baseEvent, [
    ChromotingEvent.SessionState.STARTED,
    ChromotingEvent.SessionState.SIGNALING,
    ChromotingEvent.SessionState.CREATING_PLUGIN,
    ChromotingEvent.SessionState.CONNECTING,
    ChromotingEvent.SessionState.AUTHENTICATED,
    ChromotingEvent.SessionState.CONNECTED,
    ChromotingEvent.SessionState.CLOSED
  ]);
};

/**
 * @param {remoting.Me2MeTestDriver} testDriver
 * @param {Object} baseEvent
 */
var expectCanceled = function(testDriver, baseEvent) {
  var ChromotingEvent = remoting.ChromotingEvent;
  expectSequence(testDriver, baseEvent, [
    ChromotingEvent.SessionState.STARTED,
    ChromotingEvent.SessionState.SIGNALING,
    ChromotingEvent.SessionState.CREATING_PLUGIN,
    ChromotingEvent.SessionState.CONNECTING,
    ChromotingEvent.SessionState.CONNECTION_CANCELED
  ]);
};

/**
 * @param {remoting.Me2MeTestDriver} testDriver
 * @param {Object} baseEvent
 * @param {remoting.ChromotingEvent.ConnectionError} error
 */
var expectFailed = function(testDriver, baseEvent, error) {
  var ChromotingEvent = remoting.ChromotingEvent;
  expectSequence(testDriver, baseEvent, [
    ChromotingEvent.SessionState.STARTED,
    ChromotingEvent.SessionState.SIGNALING,
    ChromotingEvent.SessionState.CREATING_PLUGIN,
    ChromotingEvent.SessionState.CONNECTING,
  ]);

  var failedEvent = /** @type {Object} */ (base.deepCopy(baseEvent));
  failedEvent.session_state = ChromotingEvent.SessionState.CONNECTION_FAILED;
  failedEvent.previous_session_state = ChromotingEvent.SessionState.CONNECTING;
  failedEvent.connection_error = error;
  testDriver.expectEvents([failedEvent]);
};

QUnit.test('Connection succeeded', function() {
  expectSucceeded(testDriver, {
    session_entry_point:
        remoting.ChromotingEvent.SessionEntryPoint.CONNECT_BUTTON,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
  });

  testDriver.expectEvents([{
    feature_tracker: {}
  }]);

  /**
   * @param {remoting.MockClientPlugin} plugin
   * @param {remoting.ClientSession.State} state
   */
  function onStatusChanged(plugin, state) {
    if (state == remoting.ClientSession.State.CONNECTED) {
      testDriver.me2meActivity().stop();
      testDriver.endTest();
    }
  }

  testDriver.mockConnection().pluginFactory().mock$setPluginStatusChanged(
      onStatusChanged);

  return testDriver.startTest();
});

QUnit.test('Connection canceled - Pin prompt', function() {
   expectCanceled(testDriver, {
     session_entry_point:
         remoting.ChromotingEvent.SessionEntryPoint.CONNECT_BUTTON,
     role: remoting.ChromotingEvent.Role.CLIENT,
     mode: remoting.ChromotingEvent.Mode.ME2ME,
   });

  /**
   * @param {remoting.MockClientPlugin} plugin
   * @param {remoting.ClientSession.State} state
   */
  function onStatusChanged(plugin, state) {
    if (state == remoting.ClientSession.State.CONNECTING) {
      testDriver.cancelWhenPinPrompted();
      plugin.mock$onDisposed().then(function(){
        testDriver.endTest();
      });
    }
  }

  testDriver.mockConnection().pluginFactory().mock$setPluginStatusChanged(
      onStatusChanged);

  return testDriver.startTest();
});

QUnit.test('Connection failed - Signal strategy', function() {
  var EntryPoint = remoting.ChromotingEvent.SessionEntryPoint;
  testDriver.expectEvents([{
    session_entry_point: EntryPoint.CONNECT_BUTTON,
    session_state: remoting.ChromotingEvent.SessionState.STARTED
  },{
    session_entry_point: EntryPoint.CONNECT_BUTTON,
    session_state: remoting.ChromotingEvent.SessionState.SIGNALING,
    previous_session_state: remoting.ChromotingEvent.SessionState.STARTED
  },{
    session_entry_point: EntryPoint.CONNECT_BUTTON,
    previous_session_state: remoting.ChromotingEvent.SessionState.SIGNALING,
    session_state: remoting.ChromotingEvent.SessionState.CONNECTION_FAILED,
    connection_error: remoting.ChromotingEvent.ConnectionError.UNEXPECTED
  }]);

  var promise = testDriver.startTest();

  // The message dialog is shown when the connection fails.
  testDriver.mockDialogFactory().messageDialog.show = function() {
    testDriver.endTest();
    return Promise.resolve(remoting.MessageDialog.Result.PRIMARY);
  };

  var signalStrategy = testDriver.mockConnection().signalStrategy();
  signalStrategy.connect = function() {
    Promise.resolve().then(function(){
      signalStrategy.setStateForTesting(remoting.SignalStrategy.State.FAILED);
    });
  };

  return promise;
});

QUnit.test('Reconnect', function() {
  var EntryPoint = remoting.ChromotingEvent.SessionEntryPoint;
  expectSucceeded(testDriver, {
    session_entry_point: EntryPoint.CONNECT_BUTTON,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
  });
  testDriver.expectEvents([{
    feature_tracker: {}
  }]);
  expectSucceeded(testDriver, {
    session_entry_point: EntryPoint.RECONNECT_BUTTON,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
    previous_session: {
      last_state: remoting.ChromotingEvent.SessionState.CLOSED,
      last_error: remoting.ChromotingEvent.ConnectionError.NONE,
      entry_point: EntryPoint.CONNECT_BUTTON
    }
  });
  testDriver.expectEvents([{
    feature_tracker: {}
  }]);

  var count = 0;
  /**
   * @param {remoting.MockClientPlugin} plugin
   * @param {remoting.ClientSession.State} state
   */
  function onStatusChanged(plugin, state) {
    if (state == remoting.ClientSession.State.CONNECTED) {
      count++;
      if (count == 1) {
        testDriver.clickReconnectWhenFinished();
        testDriver.me2meActivity().stop();
      } else if (count == 2) {
        testDriver.clickOkWhenFinished();
        testDriver.me2meActivity().stop();
        testDriver.endTest();
      }
    }
  }

  testDriver.mockConnection().pluginFactory().mock$setPluginStatusChanged(
      onStatusChanged);

  return testDriver.startTest();
});

QUnit.test('HOST_OFFLINE - JID refresh failed', function() {
  var EntryPoint = remoting.ChromotingEvent.SessionEntryPoint;
  // Expects the first connection to fail with HOST_OFFLINE
  expectFailed(testDriver, {
    session_entry_point:EntryPoint.CONNECT_BUTTON,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
  }, remoting.ChromotingEvent.ConnectionError.HOST_OFFLINE);

  function onPluginCreated(/** remoting.MockClientPlugin */ plugin) {
    plugin.mock$returnErrorOnConnect(
        remoting.ClientSession.ConnectionError.HOST_IS_OFFLINE);
  }

  /**
   * @param {remoting.MockClientPlugin} plugin
   * @param {remoting.ClientSession.State} state
   */
  function onStatusChanged(plugin, state) {
    if (state == remoting.ClientSession.State.FAILED) {
      testDriver.endTest();
    }
  }

  testDriver.mockConnection().pluginFactory().mock$setPluginCreated(
      onPluginCreated);
  testDriver.mockConnection().pluginFactory().mock$setPluginStatusChanged(
      onStatusChanged);
  sinon.stub(testDriver.mockHostList(), 'refreshAndDisplay', function(callback){
      return Promise.reject();
  });

  return testDriver.startTest();
});

QUnit.test('HOST_OFFLINE - JID refresh succeeded', function() {
  var EntryPoint = remoting.ChromotingEvent.SessionEntryPoint;
  // Expects the first connection to fail with HOST_OFFLINE
  expectFailed(testDriver, {
    session_entry_point:EntryPoint.CONNECT_BUTTON,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
  }, remoting.ChromotingEvent.ConnectionError.HOST_OFFLINE);
  // Expects the second connection to succeed with RECONNECT
  expectSucceeded(testDriver, {
    session_entry_point: EntryPoint.AUTO_RECONNECT_ON_HOST_OFFLINE,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
    previous_session: {
      last_state: remoting.ChromotingEvent.SessionState.CONNECTION_FAILED,
      last_error: remoting.ChromotingEvent.ConnectionError.HOST_OFFLINE,
      entry_point: EntryPoint.CONNECT_BUTTON
    }
  });
  testDriver.expectEvents([{
    feature_tracker: {}
  }]);

  var count = 0;
  function onPluginCreated(/** remoting.MockClientPlugin */ plugin) {
    count++;
    if (count == 1) {
      plugin.mock$returnErrorOnConnect(
          remoting.ClientSession.ConnectionError.HOST_IS_OFFLINE);
    } else if (count == 2) {
      plugin.mock$useDefaultBehavior(
          remoting.ChromotingEvent.AuthMethod.PIN);
    }
  }

  /**
   * @param {remoting.MockClientPlugin} plugin
   * @param {remoting.ClientSession.State} state
   */
  function onStatusChanged(plugin, state) {
    if (state == remoting.ClientSession.State.CONNECTED) {
      testDriver.me2meActivity().stop();
      testDriver.endTest();
    }
  }

  testDriver.mockConnection().pluginFactory().mock$setPluginCreated(
      onPluginCreated);
  testDriver.mockConnection().pluginFactory().mock$setPluginStatusChanged(
      onStatusChanged);

  return testDriver.startTest();
});

QUnit.test('HOST_OFFLINE - Reconnect failed', function() {
  var EntryPoint = remoting.ChromotingEvent.SessionEntryPoint;
  // Expects the first connection to fail with HOST_OFFLINE
  expectFailed(testDriver, {
    session_entry_point:EntryPoint.CONNECT_BUTTON,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
  }, remoting.ChromotingEvent.ConnectionError.HOST_OFFLINE);
  // Expects the second connection to fail with HOST_OVERLOAD
  expectFailed(testDriver, {
    session_entry_point:EntryPoint.AUTO_RECONNECT_ON_HOST_OFFLINE,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
    previous_session: {
      last_state: remoting.ChromotingEvent.SessionState.CONNECTION_FAILED,
      last_error: remoting.ChromotingEvent.ConnectionError.HOST_OFFLINE,
      entry_point: EntryPoint.CONNECT_BUTTON
    }
  }, remoting.ChromotingEvent.ConnectionError.HOST_OVERLOAD);

  var count = 0;
  function onPluginCreated(/** remoting.MockClientPlugin */ plugin) {
    count++;
    if (count == 1) {
      plugin.mock$returnErrorOnConnect(
          remoting.ClientSession.ConnectionError.HOST_IS_OFFLINE);
    } else if (count == 2) {
      plugin.mock$returnErrorOnConnect(
          remoting.ClientSession.ConnectionError.HOST_OVERLOAD);
    }
  }

  var failureCount = 0;
  /**
   * @param {remoting.MockClientPlugin} plugin
   * @param {remoting.ClientSession.State} state
   */
  function onStatusChanged(plugin, state) {
    if (state == remoting.ClientSession.State.FAILED) {
      failureCount++;

      if (failureCount == 2) {
        testDriver.endTest();
      }
    }
  }
  testDriver.mockConnection().pluginFactory().mock$setPluginCreated(
      onPluginCreated);
  testDriver.mockConnection().pluginFactory().mock$setPluginStatusChanged(
      onStatusChanged);
  return testDriver.startTest();
});


QUnit.test('Connection dropped - Auto Reconnect', function() {
  var EntryPoint = remoting.ChromotingEvent.SessionEntryPoint;
  expectSucceeded(testDriver, {
    session_entry_point: EntryPoint.CONNECT_BUTTON,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
  });

  testDriver.expectEvents([{
    feature_tracker: {}
  }]);

  expectSucceeded(testDriver, {
    session_entry_point: EntryPoint.AUTO_RECONNECT_ON_CONNECTION_DROPPED,
    role: remoting.ChromotingEvent.Role.CLIENT,
    mode: remoting.ChromotingEvent.Mode.ME2ME,
    previous_session: {
      last_state: remoting.ChromotingEvent.SessionState.CLOSED,
      last_error: remoting.ChromotingEvent.ConnectionError.CLIENT_SUSPENDED,
      entry_point: EntryPoint.CONNECT_BUTTON
    }
  });

  testDriver.expectEvents([{
    feature_tracker: {}
  }]);

  var count = 0;

  /**
   * @param {remoting.MockClientPlugin} plugin
   * @param {remoting.ClientSession.State} state
   */
  function onStatusChanged(plugin, state) {
    if (state == remoting.ClientSession.State.CONNECTED) {
      count++;
      if (count == 1) {
        // On first CONNECTED, fake client suspension.
        testDriver.me2meActivity().getDesktopActivity().getSession().disconnect(
            new remoting.Error(remoting.Error.Tag.CLIENT_SUSPENDED));
      } else if (count == 2) {
        // On second CONNECTED, disconnect and finish the test.
        testDriver.me2meActivity().stop();
        testDriver.endTest();
      }
    }
  }

  testDriver.mockConnection().pluginFactory().mock$setPluginStatusChanged(
      onStatusChanged);

  return testDriver.startTest();
});


/**
 * @param {remoting.Me2MeTestDriver} testDriver
 * @param {remoting.ChromotingEvent.AuthMethod} authMethod
 * @param {boolean} connected
 * @return {Promise}
 */
function createAuthMethodTest(testDriver, authMethod, connected) {
  var ChromotingEvent = remoting.ChromotingEvent;
  var baseEvent = {
    session_entry_point: ChromotingEvent.SessionEntryPoint.CONNECT_BUTTON,
    role: ChromotingEvent.Role.CLIENT,
    mode: ChromotingEvent.Mode.ME2ME,
  };

  expectSequence(
      testDriver, baseEvent, [
        ChromotingEvent.SessionState.STARTED,
        ChromotingEvent.SessionState.SIGNALING,
        ChromotingEvent.SessionState.CREATING_PLUGIN,
        ChromotingEvent.SessionState.CONNECTING,
      ]);

  var copy = /** @type{Object} */ (base.deepCopy(baseEvent));
  copy.auth_method = authMethod;
  if (connected) {
    expectSequence(
        testDriver, copy, [
          ChromotingEvent.SessionState.AUTHENTICATED,
          ChromotingEvent.SessionState.CONNECTED,
          ChromotingEvent.SessionState.CLOSED
        ]);

    testDriver.expectEvents([{
      feature_tracker: {}
    }]);
  } else {
    testDriver.expectEvents([{
        session_entry_point: ChromotingEvent.SessionEntryPoint.CONNECT_BUTTON,
        role: ChromotingEvent.Role.CLIENT,
        mode: ChromotingEvent.Mode.ME2ME,
        auth_method: authMethod,
        session_state: ChromotingEvent.SessionState.CONNECTION_FAILED,
        connection_error: ChromotingEvent.ConnectionError.INVALID_ACCESS_CODE,
    }]);
  }

  /**
   * @param {remoting.MockClientPlugin} plugin
   * @param {remoting.ClientSession.State} state
   */
  function onStatusChanged(plugin, state) {
    if (state == remoting.ClientSession.State.CONNECTED  ||
        state == remoting.ClientSession.State.FAILED) {
      testDriver.me2meActivity().stop();
      testDriver.endTest();
    }
  }
  function onPluginCreated(/** remoting.MockClientPlugin */ plugin) {
    if (connected) {
      plugin.mock$useDefaultBehavior(authMethod);
    } else {
      plugin.mock$returnErrorOnConnect(
          remoting.ClientSession.ConnectionError.SESSION_REJECTED, authMethod);
    }
  }

  testDriver.mockConnection().pluginFactory().mock$setPluginCreated(
      onPluginCreated);
  testDriver.mockConnection().pluginFactory().mock$setPluginStatusChanged(
      onStatusChanged);

  return testDriver.startTest();
}

QUnit.test('Auth method - THIRD_PARTY (connected)', function() {
  return createAuthMethodTest(
      testDriver, remoting.ChromotingEvent.AuthMethod.THIRD_PARTY, true);
});

QUnit.test('Auth method - PINLESS (connected)', function() {
  return createAuthMethodTest(
      testDriver, remoting.ChromotingEvent.AuthMethod.PINLESS, true);
});

QUnit.test('Auth method - PIN (connected)', function() {
  return createAuthMethodTest(
      testDriver, remoting.ChromotingEvent.AuthMethod.PIN, true);
});

QUnit.test('Auth method - THIRD_PARTY (failed)', function() {
  return createAuthMethodTest(
      testDriver, remoting.ChromotingEvent.AuthMethod.THIRD_PARTY, false);
});

QUnit.test('Auth method - PINLESS (failed)', function() {
  return createAuthMethodTest(
      testDriver, remoting.ChromotingEvent.AuthMethod.PINLESS, false);
});

QUnit.test('Auth method - PIN (failed)', function() {
  return createAuthMethodTest(
      testDriver, remoting.ChromotingEvent.AuthMethod.PIN, false);
});

})();
