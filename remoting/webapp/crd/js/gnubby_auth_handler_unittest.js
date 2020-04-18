// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 */

(function() {

'use strict';

/** @const {string} */
var EXTENSION_TYPE = 'gnubby-auth';

/** @const {string} */
var GNUBBY_DEV_EXTENSION_ID = 'dlfcjilkjfhdnfiecknlnddkmmiofjbg';

/** @const {string} */
var GNUBBY_STABLE_EXTENSION_ID = 'beknehfpfkghjoafdifaflglpjkojoco';

/** @private @const {string} */
var GNUBBY_DEV_CORP_EXTENSION_ID = 'klnjmillfildbbimkincljmfoepfhjjj';

/** @private @const {string} */
var GNUBBY_STABLE_CORP_EXTENSION_ID = 'lkjlajklkdhaneeelolkfgbpikkgnkpk';

/** @type {sinon.TestStub} */
var sendMessageStub = null;

/** @type {remoting.GnubbyAuthHandler} */
var gnubbyAuthHandler = null;

var sendMessageHandler = null;

/** @const */
var messageData = {
  'type': 'data',
  'data': ['Blergh!', 'Blargh!', 'Bleh!'],
  'connectionId': 42,
};

/** @const {string} */
var messageJSON = JSON.stringify(messageData);

QUnit.module('GnubbyAuthHandler', {
  beforeEach: function(/** QUnit.Assert */ assert) {
    // Configurable handler used for test customization.
    sendMessageStub = sinon.stub(chromeMocks.runtime, 'sendMessage',
        function(extensionId, message, responseCallback) {
          sendMessageHandler(extensionId, message, responseCallback);
        }
    );
  },
  afterEach: function(/** QUnit.Assert */ assert) {
    gnubbyAuthHandler = null;
    sendMessageStub.restore();
  }
});

QUnit.test(
  'isGnubbyExtensionInstalled() with no gnubby extensions installed',
  function(assert) {
    assert.expect(1);
    var done = assert.async();

    gnubbyAuthHandler = new remoting.GnubbyAuthHandler();
    sendMessageHandler = function(extensionId, message, responseCallback) {
      Promise.resolve().then(function() { responseCallback(null); });
    }

    gnubbyAuthHandler.isGnubbyExtensionInstalled().then(
      function(isInstalled) {
        assert.ok(!isInstalled);
        done();
      }
    );
});

QUnit.test(
  'isGnubbyExtensionInstalled() with Dev gnubby extensions installed',
  function(assert) {
    assert.expect(1);
    var done = assert.async();

    sendMessageHandler = function(extensionId, message, responseCallback) {
      var response = null;
      if (extensionId === GNUBBY_DEV_EXTENSION_ID) {
        response = { data: "W00t!" };
      }
      Promise.resolve().then(function() { responseCallback(response); });
    };

    gnubbyAuthHandler = new remoting.GnubbyAuthHandler();
    gnubbyAuthHandler.isGnubbyExtensionInstalled().then(
      function(isInstalled) {
        assert.ok(isInstalled);
        done();
      }
    );
});

QUnit.test(
  'isGnubbyExtensionInstalled() with Stable gnubby extensions installed',
  function(assert) {
    assert.expect(1);
    var done = assert.async();

    sendMessageHandler = function(extensionId, message, responseCallback) {
      var response = null;
      if (extensionId === GNUBBY_STABLE_EXTENSION_ID) {
        response = { data: "W00t!" };
      }
      Promise.resolve().then(function() { responseCallback(response); });
    };

    gnubbyAuthHandler = new remoting.GnubbyAuthHandler();
    gnubbyAuthHandler.isGnubbyExtensionInstalled().then(
      function(isInstalled) {
        assert.ok(isInstalled);
        done();
      }
    );
});

QUnit.test(
  'isGnubbyExtensionInstalled() with Dev corp gnubby extensions installed',
  function(assert) {
    assert.expect(1);
    var done = assert.async();

    sendMessageHandler = function(extensionId, message, responseCallback) {
      var response = null;
      if (extensionId === GNUBBY_DEV_CORP_EXTENSION_ID) {
        response = { data: "W00t!" };
      }
      Promise.resolve().then(function() { responseCallback(response); });
    };

    gnubbyAuthHandler = new remoting.GnubbyAuthHandler();
    gnubbyAuthHandler.isGnubbyExtensionInstalled().then(
      function(isInstalled) {
        assert.ok(isInstalled);
        done();
      }
    );
});

QUnit.test(
  'isGnubbyExtensionInstalled() with Stable corp gnubby extensions installed',
  function(assert) {
    assert.expect(1);
    var done = assert.async();

    sendMessageHandler = function(extensionId, message, responseCallback) {
      var response = null;
      if (extensionId === GNUBBY_STABLE_CORP_EXTENSION_ID) {
        response = { data: "W00t!" };
      }
      Promise.resolve().then(function() { responseCallback(response); });
    };

    gnubbyAuthHandler = new remoting.GnubbyAuthHandler();
    gnubbyAuthHandler.isGnubbyExtensionInstalled().then(
      function(isInstalled) {
        assert.ok(isInstalled);
        done();
      }
    );
});

QUnit.test('startExtension() sends message to host.',
  function(assert) {
    assert.expect(3);
    var done = assert.async();

    var sendMessageToHostCallback = function(extensionType, message) {
      assert.equal(extensionType, EXTENSION_TYPE);
      var messageObject = JSON.parse(message);
      assert.equal('control', messageObject['type']);
      assert.equal('auth-v1', messageObject['option']);
      done();
    }
    gnubbyAuthHandler = new remoting.GnubbyAuthHandler();
    gnubbyAuthHandler.startExtension(sendMessageToHostCallback);
});

QUnit.test(
  'onExtensionMessage() sends message to stable gnubby extension.',
  function(assert) {
    assert.expect(6);
    var done1 = assert.async();
    var done2 = assert.async();

    var isGnubbyExtensionInstalledHandler =
        function(extensionId, message, responseCallback) {
          var response = null;
          if (extensionId === GNUBBY_STABLE_EXTENSION_ID) {
            response = { data: "W00t!" };
          }
          Promise.resolve().then(function() { responseCallback(response); });
        };

    var onExtensionMessageHandler =
        function(extensionId, message, responseCallback) {
          assert.equal(extensionId, GNUBBY_STABLE_EXTENSION_ID,
              'Expected the stable gnubby extension ID.');
          Promise.resolve().then(function() { responseCallback(message); });
        };

    var sendMessageToHostCallback = function(extensionType, message) {
      assert.equal(extensionType, EXTENSION_TYPE);
      var messageObject = JSON.parse(message);
      var messageType = messageObject['type'];
      if (messageType === 'control') {
        // This is the first control message sent to the host.
        done1();
      } else if (messageType === 'data') {
        // Response from gnubby extension.
        assert.equal(messageData.connectionId, messageObject['connectionId']);
        assert.deepEqual(messageData.data, messageObject['data']);
        done2();
      } else {
        assert.ok(false);
      }
    };

    gnubbyAuthHandler = new remoting.GnubbyAuthHandler();
    gnubbyAuthHandler.startExtension(sendMessageToHostCallback);

    sendMessageHandler = isGnubbyExtensionInstalledHandler;
    return gnubbyAuthHandler.isGnubbyExtensionInstalled().then(
      function(isInstalled) {
        assert.ok(isInstalled);
        // The promise has completed for isGnubbyExtensionInstalled() so now we
        // can switch the handler out to work with the onExtensionMessage call.
        sendMessageHandler = onExtensionMessageHandler;
        return Promise.resolve().then(function() {
            gnubbyAuthHandler.onExtensionMessage(EXTENSION_TYPE, messageData);
        });
      }
    );
});

})();
