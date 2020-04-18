// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

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

QUnit.module('gcd_client with mock_xhr', {
  beforeEach: function() {
    remoting.MockXhr.activate();
    remoting.identity = new remoting.Identity();
    chromeMocks.identity.mock$setToken('fake_token');
    client = new remoting.gcd.Client({
      apiBaseUrl: 'https://fake.api',
      apiKey: 'fake_key'
    });
  },
  afterEach: function() {
    remoting.identity = null;
    remoting.MockXhr.restore();
  }
});

QUnit.test('insertRegistrationTicket', function(assert) {
  remoting.MockXhr.setResponseFor(
      'POST', 'https://fake.api/registrationTickets',
      function(/** remoting.MockXhr */ xhr) {
        assert.equal(xhr.params.useIdentity, true);
        assert.deepEqual(
            xhr.params.jsonContent,
            { userEmail: 'me' });
        xhr.setJsonResponse(200, FAKE_REGISTRATION_TICKET);
      });
  return client.insertRegistrationTicket().then(function(ticket) {
    assert.deepEqual(ticket, FAKE_REGISTRATION_TICKET);
  });
});

QUnit.test('patchRegistrationTicket', function(assert) {
  remoting.MockXhr.setResponseFor(
      'PATCH', 'https://fake.api/registrationTickets/fake_ticket_id',
      function(/** remoting.MockXhr */ xhr) {
        assert.equal(xhr.params.useIdentity, false);
        assert.deepEqual(
            xhr.params.urlParams, {
              key: 'fake_key'
            });
        assert.deepEqual(
            xhr.params.jsonContent, {
              deviceDraft: { 'fake_device_draft': true },
              oauthClientId: 'fake_client_id'
            });
        xhr.setJsonResponse(200, FAKE_REGISTRATION_TICKET);
      });
  return client.patchRegistrationTicket('fake_ticket_id', {
    'fake_device_draft': true
  }, 'fake_client_id').then(function(ticket) {
    assert.deepEqual(ticket, FAKE_REGISTRATION_TICKET);
  });
});

QUnit.test('finalizeRegistrationTicket', function(assert) {
  remoting.MockXhr.setResponseFor(
      'POST', 'https://fake.api/registrationTickets/fake_ticket_id/finalize',
      function(/** remoting.MockXhr */ xhr) {
        assert.equal(xhr.params.urlParams['key'], 'fake_key');
        assert.equal(xhr.params.useIdentity, false);
        xhr.setJsonResponse(200, FAKE_REGISTRATION_TICKET);
      });
  return client.finalizeRegistrationTicket('fake_ticket_id').
      then(function(ticket) {
        assert.deepEqual(ticket, FAKE_REGISTRATION_TICKET);
      });
});

QUnit.test('listDevices', function(assert) {
  remoting.MockXhr.setResponseFor(
      'GET', 'https://fake.api/devices',
      function(/** remoting.MockXhr */ xhr) {
        assert.equal(xhr.params.useIdentity, true);
        xhr.setJsonResponse(200, FAKE_DEVICE_LIST);
      });
  return client.listDevices().then(function(devices) {
    assert.deepEqual(devices, [FAKE_DEVICE]);
  });
});

QUnit.test('patchDevice', function(assert) {
  remoting.MockXhr.setResponseFor(
      'PATCH', 'https://fake.api/devices/fake_device_id',
      function(/** remoting.MockXhr */ xhr) {
        assert.equal(xhr.params.useIdentity, true);
        assert.deepEqual(xhr.params.jsonContent, FAKE_DEVICE_PATCH);
        xhr.setJsonResponse(200, FAKE_DEVICE);
      });
  return client.patchDevice('fake_device_id', FAKE_DEVICE_PATCH).
      then(function(device) {
        assert.deepEqual(device, FAKE_DEVICE);
      });
});

})();
