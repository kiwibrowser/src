// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['net_internals_test.js']);

// Anonymous namespace
(function() {

/**
 * Checks the display on the DNS tab against the information it should be
 * displaying.
 * @param {object} hostResolverInfo Results from a host resolver info query.
 */
function checkDisplay(hostResolverInfo) {
  expectEquals(
      hostResolverInfo.cache.capacity,
      parseInt($(DnsView.CAPACITY_SPAN_ID).innerText));

  var entries = hostResolverInfo.cache.entries;

  // Don't check exact displayed values, to avoid any races, but make sure
  // values are non-negative and have the correct sum.
  var active = parseInt($(DnsView.ACTIVE_SPAN_ID).innerText);
  var expired = parseInt($(DnsView.EXPIRED_SPAN_ID).innerText);
  expectLE(0, active);
  expectLE(0, expired);
  expectEquals(entries.length, active + expired);

  var tableId = DnsView.CACHE_TBODY_ID;
  NetInternalsTest.checkTbodyRows(tableId, entries.length);

  // Rather than check the exact string in every position, just make sure every
  // entry is not empty, and does not have 'undefined' anywhere, which should
  // find a fair number of potential output errors, without duplicating the
  // entire corresponding function of DnsView.
  for (var row = 0; row < entries.length; ++row) {
    for (column = 0; column < 4; ++column) {
      var text = NetInternalsTest.getTbodyText(tableId, row, column);
      expectNotEquals(text, '');
      expectFalse(/undefined/i.test(text));
    }
  }
}

/**
 * Finds an entry with the specified host name in the |hostResolverInfo| cache,
 * and returns its index.
 * @param {object} hostResolverInfo Results to search.
 * @param {object} hostname The host name to find.
 * @return {int} Index of the specified host.  -1 if not found.
 */
function findEntry(hostResolverInfo, hostname) {
  var entries = hostResolverInfo.cache.entries;
  for (var i = 0; i < entries.length; ++i) {
    if (entries[i].hostname == hostname)
      return i;
  }
  return -1;
}

/**
 * A Task that adds a hostname to the cache and waits for it to appear in the
 * data we receive from the cache.
 * @param {string} hostname Name of host address we're waiting for.
 * @param {string} ipAddress IP address we expect it to have.  Null if we expect
 *     a net error other than OK.
 * @param {int} netError The expected network error code.
 * @param {bool} expired True if we expect the entry to be expired.  The added
 *     entry will have an expiration time far enough away from the current time
 *     that there will be no chance of any races.
 * @extends {NetInternalsTest.Task}
 */
function AddCacheEntryTask(hostname, ipAddress, netError, expired) {
  this.hostname_ = hostname;
  this.ipAddress_ = ipAddress;
  this.netError_ = netError;
  this.expired_ = expired;
  NetInternalsTest.Task.call(this);
}

AddCacheEntryTask.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  /**
   * Adds an entry to the cache and starts waiting to received the results from
   * the browser process.
   */
  start: function() {
    var addCacheEntryParams = [
      this.hostname_, this.ipAddress_, this.netError_, this.expired_ ? -2 : 2
    ];
    chrome.send('addCacheEntry', addCacheEntryParams);
    g_browser.addHostResolverInfoObserver(this, false);
  },

  /**
   * Callback from the BrowserBridge.  Checks if |hostResolverInfo| has the
   * DNS entry specified on creation.  If so, validates it and completes the
   * task.  If not, continues running.
   * @param {object} hostResolverInfo Results of a host resolver info query.
   */
  onHostResolverInfoChanged: function(hostResolverInfo) {
    if (!this.isDone()) {
      checkDisplay(hostResolverInfo);

      var index = findEntry(hostResolverInfo, this.hostname_);
      if (index >= 0) {
        var entry = hostResolverInfo.cache.entries[index];
        if (this.netError_) {
          this.checkError_(entry);
        } else {
          this.checkSuccess_(entry);
        }
        var expirationDate = timeutil.convertTimeTicksToDate(entry.expiration);
        expectEquals(this.expired_, expirationDate < new Date());

        // Expect at least one active or expired entry, depending on |expired_|.
        // To avoid any chance of a race, exact values are not tested.
        var activeMin = this.expired_ ? 0 : 1;
        var expiredMin = this.expired_ ? 1 : 0;
        expectLE(activeMin, parseInt($(DnsView.ACTIVE_SPAN_ID).innerText));
        expectLE(expiredMin, parseInt($(DnsView.EXPIRED_SPAN_ID).innerText));

        // Text for the expiration time of the entry should contain 'Expired'
        // only if |expired_| is true.  Only checked for entries we add
        // ourselves to avoid any expiration time race.
        var expirationText =
            NetInternalsTest.getTbodyText(DnsView.CACHE_TBODY_ID, index, 4);
        expectEquals(this.expired_, /expired/i.test(expirationText));

        this.onTaskDone();
      }
    }
  },

  checkError_: function(entry) {
    expectEquals(this.netError_, entry.error);
  },

  checkSuccess_: function(entry) {
    expectEquals(undefined, entry.error);
    expectEquals(1, entry.addresses.length);
    expectEquals(0, entry.addresses[0].search(this.ipAddress_));
  }
};

/**
 * A Task that simulates a network change and checks that cache entries are
 * expired.
 * @extends {NetInternalsTest.Task}
 */
function NetworkChangeTask() {
  NetInternalsTest.Task.call(this);
}

NetworkChangeTask.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  start: function() {
    chrome.send('changeNetwork');
    g_browser.addHostResolverInfoObserver(this, false);
  },

  /**
   * Callback from the BrowserBridge.  Checks if the entry has been expired.
   * If so, the task completes.
   * @param {object} hostResolverInfo Results of a host resolver info query.
   */
  onHostResolverInfoChanged: function(hostResolverInfo) {
    if (!this.isDone()) {
      checkDisplay(hostResolverInfo);

      var entries = hostResolverInfo.cache.entries;
      var tableId = DnsView.CACHE_TBODY_ID;
      var foundExpired = false;

      // Look for an entry that's expired due to a network change.
      for (var row = 0; row < entries.length; ++row) {
        var text = NetInternalsTest.getTbodyText(tableId, row, 5);
        if (/expired/i.test(text)) {
          foundExpired = true;
        }
      }

      if (foundExpired) {
        // Expect at least one expired entry and at least one network change.
        // To avoid any chance of a race, exact values are not tested.
        expectLE(0, parseInt($(DnsView.ACTIVE_SPAN_ID).innerText));
        expectLE(1, parseInt($(DnsView.EXPIRED_SPAN_ID).innerText));
        expectLE(1, parseInt($(DnsView.NETWORK_SPAN_ID).innerText));
        this.onTaskDone();
      }
    }
  }
};

/**
 * A Task that clears the cache by simulating a button click.
 * @extends {NetInternalsTest.Task}
 */
function ClearCacheTask() {
  NetInternalsTest.Task.call(this);
}

ClearCacheTask.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  start: function() {
    $(DnsView.CLEAR_CACHE_BUTTON_ID).onclick();
    this.onTaskDone();
  }
};

/**
 * A Task that waits for the specified hostname entry to disappear from the
 * cache.
 * @param {string} hostname Name of host we're waiting to be removed.
 * @extends {NetInternalsTest.Task}
 */
function WaitForEntryDestructionTask(hostname) {
  this.hostname_ = hostname;
  NetInternalsTest.Task.call(this);
}

WaitForEntryDestructionTask.prototype = {
  __proto__: NetInternalsTest.Task.prototype,

  /**
   * Starts waiting to received the results from the browser process.
   */
  start: function() {
    g_browser.addHostResolverInfoObserver(this, false);
  },

  /**
   * Callback from the BrowserBridge.  Checks if the entry has been removed.
   * If so, the task completes.
   * @param {object} hostResolverInfo Results a host resolver info query.
   */
  onHostResolverInfoChanged: function(hostResolverInfo) {
    if (!this.isDone()) {
      checkDisplay(hostResolverInfo);

      var entry = findEntry(hostResolverInfo, this.hostname_);
      if (entry == -1)
        this.onTaskDone();
    }
  }
};

/**
 * Adds a successful lookup to the DNS cache, then clears the cache.
 */
TEST_F('NetInternalsTest', 'netInternalsDnsViewSuccess', function() {
  NetInternalsTest.switchToView('dns');
  var taskQueue = new NetInternalsTest.TaskQueue(true);
  taskQueue.addTask(
      new AddCacheEntryTask('somewhere.com', '1.2.3.4', 0, false));
  taskQueue.addTask(new ClearCacheTask());
  taskQueue.addTask(new WaitForEntryDestructionTask('somewhere.com'));
  taskQueue.run(true);
});

/**
 * Adds a failed lookup to the DNS cache, then clears the cache.
 */
TEST_F('NetInternalsTest', 'netInternalsDnsViewFail', function() {
  NetInternalsTest.switchToView('dns');
  var taskQueue = new NetInternalsTest.TaskQueue(true);
  taskQueue.addTask(new AddCacheEntryTask(
      'nowhere.com', '', NetError.ERR_NAME_NOT_RESOLVED, false));
  taskQueue.addTask(new ClearCacheTask());
  taskQueue.addTask(new WaitForEntryDestructionTask('nowhere.com'));
  taskQueue.run(true);
});

/**
 * Adds an expired successful lookup to the DNS cache, then clears the cache.
 */
TEST_F('NetInternalsTest', 'netInternalsDnsViewExpired', function() {
  NetInternalsTest.switchToView('dns');
  var taskQueue = new NetInternalsTest.TaskQueue(true);
  taskQueue.addTask(new AddCacheEntryTask('somewhere.com', '1.2.3.4', 0, true));
  taskQueue.addTask(new ClearCacheTask());
  taskQueue.addTask(new WaitForEntryDestructionTask('somewhere.com'));
  taskQueue.run(true);
});

/**
 * Adds two entries to the DNS cache, clears the cache, and then repeats.
 */
TEST_F('NetInternalsTest', 'netInternalsDnsViewAddTwoTwice', function() {
  NetInternalsTest.switchToView('dns');
  var taskQueue = new NetInternalsTest.TaskQueue(true);
  for (var i = 0; i < 2; ++i) {
    taskQueue.addTask(
        new AddCacheEntryTask('somewhere.com', '1.2.3.4', 0, false));
    taskQueue.addTask(new AddCacheEntryTask(
        'nowhere.com', '', NetError.ERR_NAME_NOT_RESOLVED, true));
    taskQueue.addTask(new ClearCacheTask());
    taskQueue.addTask(new WaitForEntryDestructionTask('somewhere.com'));
    taskQueue.addTask(new WaitForEntryDestructionTask('nowhere.com'));
  }
  taskQueue.run(true);
});

/**
 * Makes sure that opening and then closing an incognito window clears the
 * DNS cache.  To keep things simple, we add a fake cache entry ourselves,
 * rather than having the incognito browser create one.
 */
TEST_F('NetInternalsTest', 'netInternalsDnsViewIncognitoClears', function() {
  NetInternalsTest.switchToView('dns');
  var taskQueue = new NetInternalsTest.TaskQueue(true);
  taskQueue.addTask(new NetInternalsTest.CreateIncognitoBrowserTask());
  taskQueue.addTask(new AddCacheEntryTask('somewhere.com', '1.2.3.4', 0, true));
  taskQueue.addTask(NetInternalsTest.getCloseIncognitoBrowserTask());
  taskQueue.addTask(new WaitForEntryDestructionTask('somewhere.com'));
  taskQueue.run(true);
});

/**
 * Adds a successful lookup to the DNS cache, then simulates a network change
 * and checks that the entry expires.
 */
TEST_F('NetInternalsTest', 'netInternalsDnsViewNetworkChanged', function() {
  NetInternalsTest.switchToView('dns');
  var taskQueue = new NetInternalsTest.TaskQueue(true);
  taskQueue.addTask(
      new AddCacheEntryTask('somewhere.com', '1.2.3.4', 0, false));
  taskQueue.addTask(new NetworkChangeTask());
  taskQueue.addTask(new ClearCacheTask());
  taskQueue.addTask(new WaitForEntryDestructionTask('somewhere.com'));
  taskQueue.run(true);
});

})();  // Anonymous namespace
