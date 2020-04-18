// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/** @type {sinon.FakeXhr} */
var fakeXhr;

/** @type {remoting.gcd.Client} */
var client;

/** @const */
var FAKE_REGISTRATION_TICKET = {
  kind: 'clouddevices#registrationTicket',
  id: 'fake_ticket_id',
  robotAccountEmail: 'fake@robotaccounts.com',
  robotAccountAuthorizationCode: 'fake_robot_auth_code',
  deviceDraft: {
    id: 'fake_device_id'
  }
};

/** @const */
var FAKE_DEVICE = {
  kind: 'clouddevices#device',
  id: 'fake_device_id'
};

/** @const */
var FAKE_DEVICE_PATCH = {
  fake_patch: true
};

/** @const */
var FAKE_DEVICE_LIST = {
  kind: 'clouddevices#devicesListResponse',
  devices: [FAKE_DEVICE]
};

/** @type {?function():void} */
var queuedResponse = null;

QUnit.module('gcd_client', {
  setup: function() {
    sinon.useFakeXMLHttpRequest().onCreate =
        function(/** sinon.FakeXhr */ xhr) {
          fakeXhr = xhr;
          xhr.addEventListener('loadstart', function() {
            if (queuedResponse) {
              queuedResponse();
            }
          });
        };
    remoting.identity = new remoting.Identity();
    chromeMocks.identity.mock$setToken('fake_token');
    client = new remoting.gcd.Client({
      apiBaseUrl: 'https://fake.api',
      apiKey: 'fake_key'
    });
  },
  teardown: function() {
    fakeXhr = null;
    queuedResponse = null;
    remoting.identity = null;
  }
});

/**
 * @param {number} status
 * @param {!Object<string>} headers
 * @param {string} body
 * @param {function():void=} opt_preconditions
 */
function queueResponse(status, headers, body, opt_preconditions) {
  console.assert(queuedResponse == null, '|queuedResponse| is null.');
  queuedResponse = function() {
    if (opt_preconditions) {
      opt_preconditions();
    }
    fakeXhr.respond(status, headers, body);
  };
};

QUnit.test('insertRegistrationTicket', function(assert) {
  queueResponse(
      200, {'Content-type': 'application/json'},
      JSON.stringify(FAKE_REGISTRATION_TICKET),
      function() {
        assert.equal(fakeXhr.method, 'POST');
        assert.equal(fakeXhr.url, 'https://fake.api/registrationTickets');
        assert.equal(fakeXhr.requestHeaders['Authorization'],
                     'Bearer fake_token');
        assert.deepEqual(
            JSON.parse(fakeXhr.requestBody || ''),
            { userEmail: 'me' });
      });
  return client.insertRegistrationTicket().then(function(ticket) {
    assert.deepEqual(ticket, FAKE_REGISTRATION_TICKET);
  });
});

QUnit.test('patchRegistrationTicket', function(assert) {
  queueResponse(
      200, {'Content-type': 'application/json'},
      JSON.stringify(FAKE_REGISTRATION_TICKET),
      function() {
        assert.equal(fakeXhr.method, 'PATCH');
        assert.equal(
            fakeXhr.url,
            'https://fake.api/registrationTickets/fake_ticket_id?key=fake_key');
        assert.deepEqual(
            JSON.parse(fakeXhr.requestBody || ''), {
              deviceDraft: { 'fake_device_draft': true },
              oauthClientId: 'fake_client_id'
            });
      });
  return client.patchRegistrationTicket('fake_ticket_id', {
    'fake_device_draft': true
  }, 'fake_client_id').then(function(ticket) {
    assert.deepEqual(ticket, FAKE_REGISTRATION_TICKET);
  });
});

QUnit.test('finalizeRegistrationTicket', function(assert) {
  queueResponse(
      200, {'Content-type': 'application/json'},
      JSON.stringify(FAKE_REGISTRATION_TICKET),
      function() {
        assert.equal(fakeXhr.method, 'POST');
        assert.equal(
            fakeXhr.url,
            'https://fake.api/registrationTickets/fake_ticket_id/finalize' +
              '?key=fake_key');
        assert.equal(fakeXhr.requestBody, null);
      });
  return client.finalizeRegistrationTicket('fake_ticket_id').
      then(function(ticket) {
        assert.deepEqual(ticket, FAKE_REGISTRATION_TICKET);
      });
});

QUnit.test('listDevices', function(assert) {
  queueResponse(
      200, {'Content-type': 'application/json'},
      JSON.stringify(FAKE_DEVICE_LIST),
      function() {
        assert.equal(fakeXhr.method, 'GET');
        assert.equal(fakeXhr.url, 'https://fake.api/devices');
        assert.equal(fakeXhr.requestHeaders['Authorization'],
                     'Bearer fake_token');
      });
  return client.listDevices().then(function(devices) {
    assert.deepEqual(devices, [FAKE_DEVICE]);
  });
});

QUnit.test('patchDevice', function(assert) {
  queueResponse(
      200, {'Content-type': 'application/json'},
      JSON.stringify(FAKE_DEVICE),
      function() {
        assert.equal(fakeXhr.method, 'PATCH');
        assert.equal(fakeXhr.url, 'https://fake.api/devices/fake_device_id');
        assert.equal(fakeXhr.requestHeaders['Authorization'],
                     'Bearer fake_token');
        assert.deepEqual(
            JSON.parse(fakeXhr.requestBody || ''), FAKE_DEVICE_PATCH);
      });
  return client.patchDevice('fake_device_id', FAKE_DEVICE_PATCH).
      then(function(device) {
        assert.deepEqual(device, FAKE_DEVICE);
      });
});

})();
