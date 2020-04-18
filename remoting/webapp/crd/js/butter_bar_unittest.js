// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

var logWriterSpy;

function verifyLog(assert, index, expectedEvent, expectedPhase) {
  var actual = logWriterSpy.getCall(index).args[0];
  assert.equal(
      actual.type, remoting.ChromotingEvent.Type.CHROMOTING_DOT_COM_MIGRATION,
      'ChromotingEvent.type == CHROMOTING_DOT_COM_MIGRATION');
  assert.equal(
      actual.chromoting_dot_com_migration.event_type, expectedEvent,
      'ChromotingEvent.chromoting_dot_com_migration.event matched.');
  assert.equal(
      actual.chromoting_dot_com_migration.phase, expectedPhase,
      'ChromotingEvent.chromoting_dot_com_migration.phase matched');
}

QUnit.module('ButterBar', {
  beforeEach: function() {
    var fixture = document.getElementById('qunit-fixture');
    fixture.innerHTML =
        '<div id="butter-bar" hidden>' +
        '  <p>' +
        '    <span id="butter-bar-message"></span>' +
        '    <a id="butter-bar-dismiss" href="#" tabindex="0">' +
        '      <img src="icon_cross.webp" class="close-icon">' +
        '    </a>' +
        '  </p>' +
        '</div>';
    logWriterSpy = sinon.spy();
    this.butterBar = new remoting.ButterBar(logWriterSpy);
    chrome.storage = {
      sync: {
        get: sinon.stub(),
        set: sinon.stub(),
      }
    };
    this.currentMessage = -1;
    this.percent = 100;
    this.hash = 0;
    this.url = 'https://www.example.com';
    this.email = 'user@domain.com';

    this.fakeSinonXhr = sinon.useFakeXMLHttpRequest();
    this.fakeSinonXhr.onCreate =
        function(/** sinon.FakeXhr */ xhr) {
          Promise.resolve().then(function() {
            if (this.currentMessage === undefined) {
              xhr.respond(500, {}, 'Internal server error');
            } else {
              xhr.respond(200, {}, JSON.stringify({
                "index": this.currentMessage,
                "url": this.url,
                "percent": this.percent,
              }));
            }
          }.bind(this));
        }.bind(this);

    this.now = 0;
    sinon.stub(remoting.ButterBar, 'now_', () => this.now);
    sinon.stub(remoting.ButterBar, 'hash_', () => this.hash);
    sinon.stub(remoting.Identity.prototype, 'getEmail', () => {
      return Promise.resolve(this.email);
    });
    sinon.stub(l10n, 'localizeElementFromTag', (element, id, substitutions) => {
      element.innerHTML = substitutions[0] + 'link' + substitutions[1];
    });
    remoting.identity = new remoting.Identity();
  },
  afterEach: function() {
    remoting.ButterBar.now_.restore();
    remoting.ButterBar.hash_.restore();
    this.fakeSinonXhr.restore();
    remoting.identity.getEmail.restore();
    l10n.localizeElementFromTag.restore();
  }
});

QUnit.test('should stay hidden for google.com addresses', function(assert) {
  this.currentMessage = 0;
  this.percent = 100;
  this.email = 'uSeR@gOoGlE.cOm';
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
  });
});

QUnit.test('should stay hidden if XHR fails', function(assert) {
  this.currentMessage = undefined;
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
  });
});

QUnit.test('should stay hidden if index==-1', function(assert) {
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
  });
});

QUnit.test('should stay hidden if not selected by percentage',
           function(assert) {
  this.currentMessage = 0;
  this.percent = 50;
  this.hash = 64;
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
  });
});

QUnit.test('should be shown, yellow and dismissable if index==0',
           function(assert) {
  this.currentMessage = 0;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == false);
    assert.ok(this.butterBar.dismiss_.hidden == false);
    assert.ok(!this.butterBar.root_.classList.contains('red'));
  });
});

QUnit.test('should update storage when shown', function(assert) {
  this.currentMessage = 0;
  this.now = 123;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    assert.deepEqual(chrome.storage.sync.set.firstCall.args,
                     [{
                       "message-state": {
                         "hidden": false,
                         "index": 0,
                         "timestamp": 123,
                       }
                     }]);
  });
});

QUnit.test('should show the correct URL', function(assert) {
  this.currentMessage = 0;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.innerHTML.indexOf(this.url) != -1);
  });
});

QUnit.test('should escape dangerous URLs', function(assert) {
  this.currentMessage = 0;
  this.url = '<script>evil()</script>';
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.innerHTML.indexOf(this.url) == -1);
    assert.ok(this.butterBar.root_.innerHTML.indexOf(
        '&lt;script&gt;evil()&lt;/script&gt;') != -1);
  });
});

QUnit.test(
    'should be shown and should not update local storage if it has already ' +
    'shown, the timeout has not elapsed and it has not been dismissed',
    function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;
  this.currentMessage = 0;
  chrome.storage.sync.get.callsArgWith(1, {
    "message-state": {
      "hidden": false,
      "index": 0,
      "timestamp": 0,
    }
  });
  this.now = remoting.ButterBar.kTimeout_;
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == false);
    assert.ok(!chrome.storage.sync.set.called);
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.BETA)
  });
});

QUnit.test('should stay hidden if the timeout has elapsed', function(assert) {
  this.currentMessage = 0;
  chrome.storage.sync.get.callsArgWith(1, {
    "message-state": {
      "hidden": false,
      "index": 0,
      "timestamp": 0,
    }
  });
  this.now = remoting.ButterBar.kTimeout_+ 1;
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
    sinon.assert.notCalled(logWriterSpy);
  });
});


QUnit.test('should stay hidden if it was previously dismissed',
           function(assert) {
  this.currentMessage = 0;
  chrome.storage.sync.get.callsArgWith(1, {
    "message-state": {
      "hidden": true,
      "index": 0,
      "timestamp": 0,
    }
  });
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
    sinon.assert.notCalled(logWriterSpy);
  });
});


QUnit.test('should be shown if the index has increased', function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;
  this.currentMessage = 1;
  chrome.storage.sync.get.callsArgWith(1, {
    "message-state": {
      "hidden": true,
      "index": 0,
      "timestamp": 0,
    }
  });
  this.now = remoting.ButterBar.kTimeout_ + 1;
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == false);
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.STABLE)
  });
});

QUnit.test('should be red and not dismissable for the final message',
           function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;

  this.currentMessage = 3;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == false);
    assert.ok(this.butterBar.dismiss_.hidden == true);
    assert.ok(this.butterBar.root_.classList.contains('red'));
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.DEPRECATION_2)
  });
});

QUnit.test('dismiss button updates local storage', function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;

  this.currentMessage = 0;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    this.butterBar.dismiss_.click();
    // The first call is in response to showing the message; the second is in
    // response to dismissing the message.
    assert.deepEqual(chrome.storage.sync.set.secondCall.args,
                     [{
                       "message-state": {
                         "hidden": true,
                         "index": 0,
                         "timestamp": 0,
                       }
                     }]);
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.BETA)
    verifyLog(
        assert, 1, MigrationEvent.DEPRECATION_NOTICE_DISMISSAL,
        MigrationPhase.BETA)
  });
});

QUnit.test('clicks on upsell URL are reported', function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;

  this.currentMessage = 2;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    // Invoke the link.
    this.butterBar.root_.getElementsByTagName('a')[0].click();
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.DEPRECATION_1)
    verifyLog(
        assert, 1, MigrationEvent.DEPRECATION_NOTICE_CLICKED,
        MigrationPhase.DEPRECATION_1)
  });
});

}());
