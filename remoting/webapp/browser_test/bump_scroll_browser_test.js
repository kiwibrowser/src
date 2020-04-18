// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * @suppress {checkTypes}
 * Browser test for the scenario below:
 * 1. Enter full-screen mode
 * 2. Move the mouse to each edge; verify that the desktop bump-scrolls.
 */

'use strict';

/** @constructor */
browserTest.FakeDesktopViewport = function() {
  /** @private */
  this.pluginPosition_ = {
    top: 0,
    left: 0
  };
  /** @private */
  this.bumpScroller_ = new base.EventSourceImpl();
  this.bumpScroller_.defineEvents(Object.keys(remoting.BumpScroller.Events));
};

/**
 * @param {number} top
 * @param {number} left
 * @return {void} nothing.
 */
browserTest.FakeDesktopViewport.prototype.setPluginPositionForTesting =
    function(top, left) {
  this.pluginPosition_ = {
    top: top,
    left: left
  };
};

/**
 * @return {{top: number, left:number}} The top-left corner of the plugin.
 */
browserTest.FakeDesktopViewport.prototype.getPluginPositionForTesting =
    function() {
  return this.pluginPosition_;
};

/** @return {base.EventSource} */
browserTest.FakeDesktopViewport.prototype.getBumpScrollerForTesting =
    function() {
  return this.bumpScroller_;
};

/** @suppress {reportUnknownTypes} */
browserTest.FakeDesktopViewport.prototype.raiseEvent =
    function() {
  return this.bumpScroller_.raiseEvent.apply(this.bumpScroller_, arguments);
};

/** @return {remoting.DesktopViewport} */
function getViewportForTesting() {
  var desktopApp = /** @type {remoting.DesktopRemoting} */ (remoting.app);
  var view = desktopApp.getConnectedViewForTesting();
  if (view) {
    return view.getViewportForTesting();
  }
  return null;
}

/** @constructor */
browserTest.Bump_Scroll = function() {
  // To avoid dependencies on the actual host desktop size, we simulate a
  // desktop larger or smaller than the client window. The exact value is
  // arbitrary, but must be positive.
  /** @type {number} */
  this.kHostDesktopSizeDelta = 10;
};

/**
 * @param {{pin:string}} data
 */
browserTest.Bump_Scroll.prototype.run = function(data) {
  browserTest.expect(typeof data.pin == 'string');

  if (!base.isAppsV2()) {
    browserTest.fail(
        'Bump-scroll requires full-screen, which can only be activated ' +
        'programmatically in apps v2.');
  }

  var mockConnection = new remoting.MockConnection();

  function onPluginCreated(/** remoting.MockClientPlugin */ plugin) {
    plugin.mock$useDefaultBehavior(remoting.ChromotingEvent.AuthMethod.PIN);
  }
  mockConnection.pluginFactory().mock$setPluginCreated(onPluginCreated);


  function cleanup() {
    mockConnection.restore();
    browserTest.disconnect();
  }

  this.testVerifyScroll().then(function() {
    return browserTest.connectMe2Me();
  }).then(function() {
    return browserTest.enterPIN(data.pin);
  }).then(
    this.noScrollWindowed.bind(this)
  ).then(
    this.activateFullscreen.bind(this)
  ).then(
    this.noScrollSmaller.bind(this)
    // The order of these operations is important. Because the plugin starts
    // scrolled to the top-left, it needs to be scrolled right and down first.
  ).then(
    this.scrollDirection.bind(this, 1.0, 0.5)  // Right edge
  ).then(
    this.scrollDirection.bind(this, 0.5, 1.0)  // Bottom edge
  ).then(
    this.scrollDirection.bind(this, 0.0, 0.5)  // Left edge
  ).then(
    this.scrollDirection.bind(this, 0.5, 0.0)  // Top edge
  ).then(
    function(value) {
      cleanup();
      return browserTest.pass();
    },
    function(error) {
      cleanup();
      return browserTest.fail(error);
    }
  );
};

/**
 * @return {Promise}
 */
browserTest.Bump_Scroll.prototype.noScrollWindowed = function() {
  var viewport = getViewportForTesting();
  viewport.setPluginSizeForBumpScrollTesting(
      window.innerWidth + this.kHostDesktopSizeDelta,
      window.innerHeight + this.kHostDesktopSizeDelta);
  this.moveMouseTo(0, 0);
  return this.verifyNoScroll();
};

/**
 * @return {Promise}
 */
browserTest.Bump_Scroll.prototype.noScrollSmaller = function() {
  var viewport = getViewportForTesting();
  viewport.setPluginSizeForBumpScrollTesting(
      window.innerWidth - this.kHostDesktopSizeDelta,
      window.innerHeight - this.kHostDesktopSizeDelta);
  this.moveMouseTo(0, 0);
  return this.verifyNoScroll();
};

/**
 * @param {number} widthFraction
 * @param {number} heightFraction
 * @return {Promise}
 */
browserTest.Bump_Scroll.prototype.scrollDirection =
    function(widthFraction, heightFraction) {
  var viewport = getViewportForTesting();
  viewport.setPluginSizeForBumpScrollTesting(
      screen.width + this.kHostDesktopSizeDelta,
      screen.height + this.kHostDesktopSizeDelta);
  /** @type {number} */
  var expectedTop = heightFraction === 0.0 ? 0 :
                    heightFraction == 1.0 ? -this.kHostDesktopSizeDelta :
                    undefined;
  /** @type {number} */
  var expectedLeft = widthFraction === 0.0 ? 0 :
                     widthFraction === 1.0 ? -this.kHostDesktopSizeDelta :
                     undefined;
  var result = this.verifyScroll(expectedTop, expectedLeft);
  this.moveMouseTo(widthFraction * screen.width,
                   heightFraction * screen.height);
  return result;
};

/**
 * @return {Promise}
 */
browserTest.Bump_Scroll.prototype.activateFullscreen = function() {
  return new Promise(function(fulfill, reject) {
    remoting.fullscreen.activate(true, function() {
      // The onFullscreen callback is invoked before the window has
      // resized, so defer fulfilling the promise so that innerWidth
      // and innerHeight are correct.
      base.Promise.sleep(1000).then(fulfill);
    });
    base.Promise.sleep(5000).then(function(){
      reject('Timed out waiting for full-screen');
    });
  });
};

/**
 * @param {number} x
 * @param {number} y
 */
browserTest.Bump_Scroll.prototype.moveMouseTo = function(x, y) {
  var e = {
    bubbles: true,
    cancelable: false,
    view: window,
    detail: 0,
    screenX: x,
    screenY: y,
    clientX: x,
    clientY: y,
    ctrlKey: false,
    altKey: false,
    shiftKey: false,
    metaKey: false,
    button: 0,
    relatedTarget: undefined
  };
  var event = document.createEvent('MouseEvents');
  event.initMouseEvent('mousemove',
                       e.bubbles, e.cancelable, e.view, e.detail,
                       e.screenX, e.screenY, e.clientX, e.clientY,
                       e.ctrlKey, e.altKey, e.shiftKey, e.metaKey,
                       e.button, document.documentElement);
  document.documentElement.dispatchEvent(event);
};

/**
 * verifyScroll() is complicated enough to warrant a test.
 * @return {Promise}
 */
browserTest.Bump_Scroll.prototype.testVerifyScroll = function() {
  var STARTED = remoting.BumpScroller.Events.bumpScrollStarted;
  var STOPPED = remoting.BumpScroller.Events.bumpScrollStopped;
  var fakeViewport = new browserTest.FakeDesktopViewport;
  var that = this;

  // No events raised (e.g. windowed mode).
  var result = this.verifyNoScroll(fakeViewport)

  .then(function() {
    // Start and end events raised, but no scrolling (e.g. full-screen mode
    // with host desktop <= window size).
    fakeViewport = new browserTest.FakeDesktopViewport;
    var result = that.verifyNoScroll(fakeViewport);
    fakeViewport.raiseEvent(STARTED, {});
    fakeViewport.raiseEvent(STOPPED, {});
    return result;

  }).then(function() {
    // Start and end events raised, with incorrect scrolling.
    fakeViewport = new browserTest.FakeDesktopViewport;
    var result = base.Promise.negate(
        that.verifyScroll(2, 2, fakeViewport));
    fakeViewport.raiseEvent(STARTED, {});
    fakeViewport.setPluginPositionForTesting(1, 1);
    fakeViewport.raiseEvent(STOPPED, {});
    return result;

  }).then(function() {
    // Start event raised, but not end event.
    fakeViewport = new browserTest.FakeDesktopViewport;
    var result = base.Promise.negate(
        that.verifyScroll(2, 2, fakeViewport));
    fakeViewport.raiseEvent(STARTED, {});
    fakeViewport.setPluginPositionForTesting(2, 2);
    return result;

  }).then(function() {
    // Start and end events raised, with correct scrolling.
    fakeViewport = new browserTest.FakeDesktopViewport;
    var result = that.verifyScroll(2, 2, fakeViewport);
    fakeViewport.raiseEvent(STARTED, {});
    fakeViewport.setPluginPositionForTesting(2, 2);
    fakeViewport.raiseEvent(STOPPED, {});
    return result;
  });

  return result;
};

/**
 * Verify that a bump scroll operation takes place and that the top-left corner
 * of the plugin is as expected when it completes.
 * @param {number|undefined} expectedTop The expected vertical position of the
 *    plugin, or undefined if it is not expected to change.
 * @param {number|undefined} expectedLeft The expected horizontal position of
 *    the plugin, or undefined if it is not expected to change.
 * @param {browserTest.FakeDesktopViewport=} opt_desktopViewport
 *     DesktopViewport fake, for testing.
 * @return {Promise}
 */
browserTest.Bump_Scroll.prototype.verifyScroll =
    function (expectedTop, expectedLeft, opt_desktopViewport) {
  var desktopViewport = opt_desktopViewport || getViewportForTesting();
  console.assert(desktopViewport != null, '|desktopViewport| is null.');
  var STARTED = remoting.BumpScroller.Events.bumpScrollStarted;
  var STOPPED = remoting.BumpScroller.Events.bumpScrollStopped;

  var initialPosition = desktopViewport.getPluginPositionForTesting();
  var initialTop = initialPosition.top;
  var initialLeft = initialPosition.left;

  /** @return {Promise} */
  var verifyPluginPosition = function() {
    var position = desktopViewport.getPluginPositionForTesting();
    if (expectedLeft === undefined) {
      expectedLeft = initialLeft;
    }
    if (expectedTop === undefined) {
      expectedTop = initialTop;
    }
    if (position.top != expectedTop || position.left != expectedLeft) {
      return Promise.reject(
          new Error('No or incorrect scroll detected: (' +
                    position.left + ',' + position.top + ' instead of ' +
                    expectedLeft + ',' + expectedTop + ')'));
    } else {
      return Promise.resolve();
    }
  };

  var bumpScroller = desktopViewport.getBumpScrollerForTesting();
  var started = browserTest.expectEvent(bumpScroller, STARTED, 1000);
  var stopped = browserTest.expectEvent(bumpScroller, STOPPED, 5000);
  return started.then(function() {
    return stopped;
  }, function() {
    // If no started event is raised, the test might still pass if it asserted
    // no scrolling.
    if (expectedTop === undefined && expectedLeft === undefined) {
      return Promise.resolve();
    } else {
      return Promise.reject(
          new Error('Scroll expected but no start event fired.'));
    }
  }).then(function() {
    return verifyPluginPosition();
  });
};

/**
 * @param {browserTest.FakeDesktopViewport=} opt_desktopViewport
 *     DesktopViewport fake, for testing.
 *
 * @return {Promise<boolean>} A promise that resolves to true if no scrolling
 *   occurs within a timeout.
 */
browserTest.Bump_Scroll.prototype.verifyNoScroll =
    function(opt_desktopViewport) {
  var desktopViewport = opt_desktopViewport || getViewportForTesting();
  var bumpScroller = desktopViewport.getBumpScrollerForTesting();
  if (!bumpScroller) {
    Promise.resolve(true);
  }
  return this.verifyScroll(undefined, undefined, desktopViewport);
};
