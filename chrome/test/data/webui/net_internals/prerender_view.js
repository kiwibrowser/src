// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['net_internals_test.js']);

// Anonymous namespace
(function() {

/**
 * Phases of a PrerenderTask.
 * @enum
 */
var STATE = {
  // The task has been created, but not yet started.
  NONE: -1,

  // We've switched to the prerender tab, but have yet to receive the
  // resulting onPrerenderInfoChanged event with no prerenders active or in
  // the history.
  START_PRERENDERING: 0,
  // We're waiting for the prerender loader to start in a background tab,
  // as well as the prerendered view to be created. Only visit this state if
  // |shouldSucceed| is true.
  NEED_NAVIGATE: 1,
  // The prerendered view has been created, and we've started the navigation
  // to it.  We're waiting for it to move to the history.  We may see the
  // prerender one or more times in the active list, or it may move straight
  // to the history.  We will not receive any event with both history and
  // active prerenders empty while in this state, as we only send
  // notifications when the values change.
  HISTORY_WAIT: 2
};

/**
 * Task that tries to prerender a page.  The URL is received from the previous
 * Task.  A loader page is opened in a background tab, which triggers the
 * prerender.  |shouldSucceed| indicates whether the prerender is expected to
 * succeed or not.  If it's false, we just wait for the page to fail, possibly
 * seeing it as active first.  If it's true, we navigate to the URL in the
 * background tab.
 *
 * Checks that we see all relevant events, and update the corresponding tables.
 * In both cases, we exit the test once we see the prerender in the history.
 * |finalStatus| is the expected status value when the page reaches the
 * history.
 *
 * @param {bool} shouldSucceed Whether or not the prerender should succeed.
 * @param {string} finalStatus The expected value of |final_status|.
 * @extends {NetInternalsTest.Task}
 * @constructor
 */
function PrerenderTask(shouldSucceed, finalStatus) {
  NetInternalsTest.Task.call(this);

  this.startedSuccessfulPrerender_ = false;
  this.url_ = null;
  this.shouldSucceed_ = shouldSucceed;
  this.finalStatus_ = finalStatus;
  this.state_ = STATE.NONE;
}

PrerenderTask.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  /**
   * Switches to prerender tab and starts waiting until we receive prerender
   * info (With no pages prerendering) before starting to prerender.
   * @param {string} url URL to be prerendered.
   */
  start: function(url) {
    assertEquals('string', typeof url);
    this.state_ = STATE.START_PRERENDERING;
    this.url_ = url;
    g_browser.addPrerenderInfoObserver(this, true);
    NetInternalsTest.switchToView('prerender');
  },

  /**
   * PrerenderInfoObserver function.  Tracks state transitions, checks the
   * table sizes, and does some sanity checking on received data.
   * @param {object} prerenderInfo State of prerendering pages.
   */
  onPrerenderInfoChanged: function(prerenderInfo) {
    if (this.isDone())
      return;

    // Verify that prerendering is enabled.
    assertTrue(prerenderInfo.enabled, 'Prerendering not enabled.');

    // Check number of rows in both tables.
    NetInternalsTest.checkTbodyRows(
        PrerenderView.HISTORY_TABLE_ID, prerenderInfo.history.length);
    NetInternalsTest.checkTbodyRows(
        PrerenderView.ACTIVE_TABLE_ID, prerenderInfo.active.length);

    if (this.state_ == STATE.START_PRERENDERING) {
      this.startPrerendering_(prerenderInfo);
    } else if (this.state_ == STATE.NEED_NAVIGATE) {
      // Can't safely swap in a prerender until the main frame has committed.
      // Waiting until the load has completed isn't necessary, but it's simpler.
      if (!prerenderInfo.active[0].is_loaded)
        return;
      this.navigate_(prerenderInfo);
    } else if (this.state_ == STATE.HISTORY_WAIT) {
      this.checkDone_(prerenderInfo);
    } else {
      assertNotReached();
    }
  },

  /**
   * Start by triggering a prerender of |url_| in a background tab.
   * At this point, we expect no active or historical prerender entries.
   * @param {Object} prerenderInfo State of prerendering pages.
   */
  startPrerendering_: function(prerenderInfo) {
    expectEquals(0, prerenderInfo.active.length);
    expectEquals(0, prerenderInfo.history.length);
    if (this.shouldSucceed_) {
      chrome.send('prerenderPage', [this.url_]);

      this.state_ = STATE.NEED_NAVIGATE;
    } else {
      // If the prerender is going to fail, we can add the prerender link to the
      // current document, so we will create one less process.  Unfortunately,
      // if the prerender is going to succeed, we have to create a new process
      // with the prerender link, to avoid the prerender being cancelled due to
      // a session storage namespace mismatch.
      var link = document.createElement('link');
      link.rel = 'prerender';
      link.href = this.url_;
      document.head.appendChild(link);

      this.state_ = STATE.HISTORY_WAIT;
    }
  },

  /**
   * Navigate to the prerendered page in the background tab.
   * @param {Object} prerenderInfo State of prerendering pages.
   */
  navigate_: function(prerenderInfo) {
    expectEquals(0, prerenderInfo.history.length);
    assertEquals(1, prerenderInfo.active.length);
    expectEquals(this.url_, prerenderInfo.active[0].url);
    expectTrue(this.shouldSucceed_);
    chrome.send('navigateToPrerender', [this.url_]);
    this.state_ = STATE.HISTORY_WAIT;
  },

  /**
   * We expect to either see the failure url as an active entry, or see it
   * in the history.  In the latter case, the test completes.
   * @param {Object} prerenderInfo State of prerendering pages.
   */
  checkDone_: function(prerenderInfo) {
    // If we see the url as active, continue running the test.
    if (prerenderInfo.active.length == 1) {
      expectEquals(this.url_, prerenderInfo.active[0].url);
      expectEquals(0, prerenderInfo.history.length);
      return;
    }

    // The prerender of |url_| is now in the history.
    this.checkHistory_(prerenderInfo);
  },

  /**
   * Check if the history is consistent with expectations, and end the test.
   * @param {Object} prerenderInfo State of prerendering pages.
   */
  checkHistory_: function(prerenderInfo) {
    expectEquals(0, prerenderInfo.active.length);
    assertEquals(1, prerenderInfo.history.length);
    expectEquals(this.url_, prerenderInfo.history[0].url);
    expectEquals(this.finalStatus_, prerenderInfo.history[0].final_status);

    this.onTaskDone();
  }
};

/**
 * Prerender a page and navigate to it, once prerendering starts.
 */
TEST_F('NetInternalsTest', 'netInternalsPrerenderViewSucceed', function() {
  var taskQueue = new NetInternalsTest.TaskQueue(true);
  taskQueue.addTask(new NetInternalsTest.GetTestServerURLTask('/title1.html'));
  taskQueue.addTask(new PrerenderTask(true, 'Used'));
  taskQueue.run();
});

/**
 * Prerender a page that is expected to fail.
 */
TEST_F('NetInternalsTest', 'netInternalsPrerenderViewFail', function() {
  var taskQueue = new NetInternalsTest.TaskQueue(true);
  taskQueue.addTask(
      new NetInternalsTest.GetTestServerURLTask('/download-test1.lib'));
  taskQueue.addTask(new PrerenderTask(false, 'Download'));
  taskQueue.run();
});

})();  // Anonymous namespace
