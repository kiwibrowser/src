// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

QUnit.module('HostOptions', {
  beforeEach: function() {
    sinon.stub(remoting, 'platformIsChromeOS');
  },
  afterEach: function() {
    $testStub(remoting.platformIsChromeOS).restore();
    chrome.storage.local.clear();
  },
});

/**
 * @param {!QUnit.Assert} assert
 * @param {remoting.HostOptions} options
 * @param {boolean} isChromeOS
*/
function assertDefaults(assert, options, isChromeOS) {
  var pairingInfo = {clientId: '', sharedSecret: ''};
  var chromeOsMap = {0x0700e4: 0x0700e7};
  var nonChromeOsMap = {};
  assert.equal(options.getShrinkToFit(), true);
  assert.equal(options.getResizeToClient(), true);
  assert.equal(options.getDesktopScale(), 1);
  assert.deepEqual(options.getPairingInfo(), pairingInfo);
  assert.deepEqual(options.getRemapKeys(),
                   isChromeOS ? chromeOsMap : nonChromeOsMap);
}

QUnit.test('Per-platform defaults are set correctly',
  function(assert) {

    $testStub(remoting.platformIsChromeOS).returns(false);
    var options = new remoting.HostOptions('host-id');
    assertDefaults(assert, options, false);

    $testStub(remoting.platformIsChromeOS).returns(true);
    options = new remoting.HostOptions('host-id');
    assertDefaults(assert, options, true);
});

QUnit.test('Loading a non-existent host yields default values',
  function(assert) {
    $testStub(remoting.platformIsChromeOS).returns(true);
    var options = new remoting.HostOptions('host-id');
    return options.load().then(
        function() {
          assertDefaults(assert, options, true);
        });
});

QUnit.test('Saving and loading a host preserves the saved values',
  function(assert) {
    var options = new remoting.HostOptions('host-id');
    var optionsLoaded = new remoting.HostOptions('host-id');
    options.setShrinkToFit(false);
    options.setResizeToClient(false);
    options.setDesktopScale(2);
    options.setRemapKeys({2: 1, 1: 2});
    options.setPairingInfo({
      clientId: 'client-id',
      sharedSecret: 'shared-secret'
    });
    var optionsCopy = base.deepCopy(options);

    return options.save().then(function() {
      return optionsLoaded.load();
    }).then(function() {
      assert.deepEqual(optionsCopy, base.deepCopy(optionsLoaded));
    });
});

QUnit.test('Saving a host ignores unset values',
  function(assert) {
    var options = new remoting.HostOptions('host-id');
    options.setShrinkToFit(false);
    var optionsCopy = base.deepCopy(options);
    var optionsEmpty = new remoting.HostOptions('host-id');

    return optionsEmpty.save().then(function() {
      return options.load();
    }).then(function() {
      assert.deepEqual(optionsCopy, base.deepCopy(options));
    });
});

QUnit.test('Old-style (string-formatted) key remappings are parsed correctly',
  function(assert) {
    var options1 = new remoting.HostOptions('host-id');
    options1.setRemapKeys({2: 1, 1: 2});
    var savedHostOptions = {
      'host-id': {
        'remapKeys': '2>1,1>2'
      }
    };
    var savedAllOptions = {
      'remoting-host-options': JSON.stringify(savedHostOptions)
    };
    chrome.storage.local.set(savedAllOptions);  // Mock storage is synchronous
    var options2 = new remoting.HostOptions('host-id');
    return options2.load().then(
        function() {
          assert.deepEqual(options1, options2);
        });
});

QUnit.test('New options are loaded and saved without updating the code',
  function(assert) {
    var options = new remoting.HostOptions('host-id');
    var optionsLoaded = new remoting.HostOptions('host-id');
    options['undefined-option'] = 42;

    return options.save().then(function() {
      return optionsLoaded.load();
    }).then(function() {
      assert.equal(optionsLoaded['undefined-option'], 42);
    });
});
