// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/** @suppress {duplicate} */
var remoting = remoting || {};

(function() {

'use strict';

/**
 * @constructor
 *
 * The |extends| annotation is used to make JSCompile happy.  The mock object
 * should never extends from the actual HostList as all its implementation
 * should be mocked out.  The caller of this class is responsible for ensuring
 * the methods that they need are implemented either here or via sinon.stub().
 *
 * @extends {remoting.HostList}
 */
remoting.MockHostList = function() {};

/** @override */
remoting.MockHostList.prototype.refreshAndDisplay = function() {
  return Promise.resolve();
};

/** @override */
remoting.MockHostList.prototype.getHostForId = function(hostId) {
  var host = new remoting.Host(hostId);
  host.jabberId = 'fake_jabber_id';
  host.status = 'ONLINE';
  return host;
};

/** @override */
remoting.MockHostList.prototype.getHostStatusUpdateElapsedTime = function() {
  return 2000;
};

/**
 * @constructor
 * @extends {remoting.DesktopConnectedView}
 */
var MockDesktopConnectedView = function() {};
/** @override */
MockDesktopConnectedView.prototype.dispose = function() {};
/** @override */
MockDesktopConnectedView.prototype.setRemapKeys = function() {};

/**
 * @constructor
 * @extends {remoting.NetworkConnectivityDetector}
 */
var MockNetworkConnectivityDetector = function() {};
/** @override */
MockNetworkConnectivityDetector.prototype.waitForOnline = function() {
  return Promise.resolve();
};

/** @override */
MockNetworkConnectivityDetector.prototype.cancel = function() {};
/** @override */
MockNetworkConnectivityDetector.prototype.dispose = function() {};

/**
 * A test driver that mocks out the UI components that are required by the
 * DesktopRemotingActivity.
 *
 * @param {string} mockHTML
 *
 * @constructor
 * @implements {base.Disposable}
 */
remoting.BaseTestDriver = function(mockHTML) {
  /** @private */
  this.deferred_ = new base.Deferred();
  /** @protected */
  this.mockConnection_ = new remoting.MockConnection();
  /** @private */
  this.originalDialogFactory_ = remoting.modalDialogFactory;
  /** @protected */
  this.mockDialogFactory_ = new remoting.MockModalDialogFactory();
  /** @private */
  this.desktopConnectedViewCreateStub_ =
      sinon.stub(remoting.DesktopConnectedView, 'create');
  /** @private */
  this.eventWriterMock_ = sinon.mock(remoting.TelemetryEventWriter.Client);
  /** @private */
  this.setModeStub_ = sinon.stub(remoting, 'setMode');
  /** @private */
  this.createNetworkConnectivityDetectorStub_ =
      sinon.stub(remoting.NetworkConnectivityDetector, 'create', function(){
        return new MockNetworkConnectivityDetector();
      });
  /** @private */
  this.isGnubbyExtensionInstalledStub_ = sinon.stub(
      remoting.GnubbyAuthHandler.prototype, 'isGnubbyExtensionInstalled',
      function() { return Promise.resolve(false); });

  /**
   * Use fake timers to prevent the generation of session ID expiration events.
   * @private
   */
  this.clock_ = sinon.useFakeTimers();

  this.init_(mockHTML);
};

/**
 * @param {string} mockHTML
 */
remoting.BaseTestDriver.prototype.init_ = function(mockHTML) {
  document.getElementById('qunit-fixture').innerHTML = mockHTML;
  // Return a token to pretend that we are signed-in.
  chromeMocks.identity.mock$setToken('fake_token');
  this.desktopConnectedViewCreateStub_.returns(new MockDesktopConnectedView());
  remoting.modalDialogFactory = this.mockDialogFactory_;
};

remoting.BaseTestDriver.prototype.dispose = function() {
  this.clock_.restore();
  remoting.modalDialogFactory = this.originalDialogFactory_;
  this.setModeStub_.restore();
  this.eventWriterMock_.restore();
  this.desktopConnectedViewCreateStub_.restore();
  this.createNetworkConnectivityDetectorStub_.restore();
  this.isGnubbyExtensionInstalledStub_.restore();
  if (Boolean(this.mockConnection_)) {
    this.mockConnection_.restore();
    this.mockConnection_ = null;
  }
};

/** @return {remoting.MockConnection} */
remoting.BaseTestDriver.prototype.mockConnection = function() {
  return this.mockConnection_;
};

/** @return {remoting.MockModalDialogFactory} */
remoting.BaseTestDriver.prototype.mockDialogFactory = function() {
  return this.mockDialogFactory_;
};

/** @param {Array<Object>} events */
remoting.BaseTestDriver.prototype.expectEvents = function(events) {
  var that = this;
  events.forEach(function(/** Object */ event){
    that.eventWriterMock_.expects('write').withArgs(sinon.match(event));
  });
};

/**
 * @return {Promise} A promise that will be resolved when endTest() is called.
 */
remoting.BaseTestDriver.prototype.startTest = function() {
  return this.deferred_.promise();
};

/**
 * Resolves the promise that is returned by startTest().
 */
remoting.BaseTestDriver.prototype.endTest = function() {
  try {
    this.eventWriterMock_.verify();
    this.deferred_.resolve();
  } catch (/** @type {*} */ reason) {
    this.deferred_.reject(reason);
  }
};

/**
 * The Me2MeTest Driver mocks out the UI components that are required by the
 * Me2MeActivity.  It provides test hooks for the caller to fake behavior of
 * those components.
 *
 * @constructor
 * @extends {remoting.BaseTestDriver}
 */
remoting.Me2MeTestDriver = function() {
  base.inherits(this, remoting.BaseTestDriver,
                remoting.Me2MeTestDriver.FIXTURE);
  /** @private */
  this.mockHostList_ = new remoting.MockHostList();
  /** @private {?remoting.Me2MeActivity} */
  this.me2meActivity_ = null;
};

/** @override */
remoting.Me2MeTestDriver.prototype.dispose = function() {
  base.dispose(this.me2meActivity_);
  this.me2meActivity_ = null;

  remoting.BaseTestDriver.prototype.dispose.call(this);
};

remoting.Me2MeTestDriver.prototype.enterPinWhenPrompted = function() {
  this.mockDialogFactory().inputDialog.show = function() {
    return Promise.resolve('fake_pin');
  };
};

remoting.Me2MeTestDriver.prototype.cancelWhenPinPrompted = function() {
  this.mockDialogFactory().inputDialog.show = function() {
    return Promise.reject(new remoting.Error(remoting.Error.Tag.CANCELLED));
  };
};

remoting.Me2MeTestDriver.prototype.clickOkWhenFinished = function() {
  this.mockDialogFactory().messageDialog.show = function() {
    return Promise.resolve(remoting.MessageDialog.Result.PRIMARY);
  };
};

remoting.Me2MeTestDriver.prototype.clickReconnectWhenFinished = function() {
  this.mockDialogFactory().messageDialog.show = function() {
    return Promise.resolve(remoting.MessageDialog.Result.SECONDARY);
  };
};

/** @return {remoting.MockHostList} */
remoting.Me2MeTestDriver.prototype.mockHostList = function() {
  return this.mockHostList_;
};

/** @return {remoting.Me2MeActivity} */
remoting.Me2MeTestDriver.prototype.me2meActivity = function() {
  return this.me2meActivity_;
};

remoting.Me2MeTestDriver.prototype.mockOnline = function() {
};

/** @return {Promise} */
remoting.Me2MeTestDriver.prototype.startTest = function() {
  var host = new remoting.Host('fake_host_id');

  // Default behavior.
  this.enterPinWhenPrompted();
  this.clickOkWhenFinished();

  this.me2meActivity_ = new remoting.Me2MeActivity(host, this.mockHostList_);
  this.me2meActivity_.start();
  return remoting.BaseTestDriver.prototype.startTest.call(this);
};

remoting.Me2MeTestDriver.FIXTURE =
  '<div id="connect-error-message"></div>' +
  '<div id="client-container">' +
    '<div class="client-plugin-container">' +
  '</div>' +
  '<div id="pin-dialog">' +
    '<form>' +
      '<input type="password" class="pin-inputField" />' +
      '<button class="cancel-button"></button>' +
    '</form>' +
    '<div class="pairing-section">' +
      '<input type="checkbox" class="pairing-checkbox" />' +
      '<div class="pin-message"></div>' +
    '</div>' +
  '</div>' +
  '<div id="host-needs-update-dialog">' +
    '<input class="connect-button" />' +
    '<input class="cancel-button" />' +
    '<div class="host-needs-update-message"></div>' +
  '</div>';

})();
