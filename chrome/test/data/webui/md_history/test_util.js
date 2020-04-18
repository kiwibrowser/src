// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Replace the current primary element of the test with a new element. Useful
 * as an alternative to PolymerTest.clearBody() which preserves styling.
 * @param {Element} element
 */
function replaceBody(element) {
  const body = document.body;
  const app = body.querySelector('history-app');

  const currentBody = app || body.querySelector('.test-body');
  body.removeChild(currentBody);

  // Clear any query in the URL.
  window.history.replaceState({}, '', '/');

  element.classList.add('test-body');
  body.appendChild(element);
}

/**
 * Replace the document body with a new instance of <history-app>.
 * @return {HistoryAppElement} The app which was created.
 */
function replaceApp() {
  const app = document.createElement('history-app');
  app.id = 'history-app';
  // Disable querying for tests by default.
  app.queryState_.queryingDisabled = true;
  replaceBody(app);
  Polymer.dom.flush();
  return app;
}

/**
 * Create a fake history result with the given timestamp.
 * @param {number|string} timestamp Timestamp of the entry, as a number in ms or
 * a string which can be parsed by Date.parse().
 * @param {string} urlStr The URL to set on this entry.
 * @return {!HistoryEntry} An object representing a history entry.
 */
function createHistoryEntry(timestamp, urlStr) {
  if (typeof timestamp === 'string')
    timestamp += ' UTC';

  const d = new Date(timestamp);
  const url = new URL(urlStr);
  const domain = url.host;
  return {
    allTimestamps: [timestamp],
    // Formatting the relative day is too hard, will instead display
    // YYYY-MM-DD.
    dateRelativeDay: d.toISOString().split('T')[0],
    dateTimeOfDay: d.getUTCHours() + ':' + d.getUTCMinutes(),
    domain: domain,
    starred: false,
    time: d.getTime(),
    title: urlStr,
    url: urlStr
  };
}

/**
 * Create a fake history search result with the given timestamp. Replaces fields
 * from createHistoryEntry to look like a search result.
 * @param {number|string} timestamp Timestamp of the entry, as a number in ms or
 * a string which can be parsed by Date.parse().
 * @param {string} urlStr The URL to set on this entry.
 * @return {!HistoryEntry} An object representing a history entry.
 */
function createSearchEntry(timestamp, urlStr) {
  const entry = createHistoryEntry(timestamp, urlStr);
  entry.dateShort = entry.dateRelativeDay;
  entry.dateTimeOfDay = '';
  entry.dateRelativeDay = '';

  return entry;
}

/**
 * Create a simple HistoryInfo.
 * @param {?string} searchTerm The search term that the info has. Will be empty
 *     string if not specified.
 * @return {!HistoryInfo}
 */
function createHistoryInfo(searchTerm) {
  return {finished: true, term: searchTerm || ''};
}

/**
 * @param {Element} element
 * @param {string} selector
 * @return {Element}
 */
function polymerSelectAll(element, selector) {
  return Polymer.dom(element.root).querySelectorAll(selector);
}

/**
 * Returns a promise which is resolved when |eventName| is fired on |element|
 * and |predicate| is true.
 * @param {HTMLElement} element
 * @param {string} eventName
 * @param {function(Event): boolean} predicate
 * @return {Promise}
 */
function waitForEvent(element, eventName, predicate) {
  if (!predicate)
    predicate = function() {
      return true;
    };

  return new Promise(function(resolve) {
    const listener = function(e) {
      if (!predicate(e))
        return;

      resolve();
      element.removeEventListener(eventName, listener);
    };

    element.addEventListener(eventName, listener);
  });
}

/**
 * Sends a shift click event to |element|.
 * @param {HTMLElement} element
 */
function shiftClick(element) {
  const xy = MockInteractions.middleOfNode(element);
  const props = {
    bubbles: true,
    cancelable: true,
    clientX: xy.x,
    clientY: xy.y,
    buttons: 1,
    shiftKey: true,
  };

  element.dispatchEvent(new MouseEvent('mousedown', props));
  element.dispatchEvent(new MouseEvent('mouseup', props));
  element.dispatchEvent(new MouseEvent('click', props));
}

function disableLinkClicks() {
  document.addEventListener('click', function(e) {
    if (e.defaultPrevented)
      return;

    const eventPath = e.path;
    let anchor = null;
    if (eventPath) {
      for (let i = 0; i < eventPath.length; i++) {
        const element = eventPath[i];
        if (element.tagName === 'A' && element.href) {
          anchor = element;
          break;
        }
      }
    }

    if (!anchor)
      return;

    e.preventDefault();
  });
}

function createSession(name, windows) {
  return {
    collapsed: false,
    deviceType: '',
    name: name,
    modifiedTime: '2 seconds ago',
    tag: name,
    timestamp: 0,
    windows: windows
  };
}

function createWindow(tabUrls) {
  const tabs = tabUrls.map(function(tabUrl) {
    return {sessionId: 456, timestamp: 0, title: tabUrl, url: tabUrl};
  });

  return {tabs: tabs, sessionId: '123', userVisibleTimestamp: 'A while ago'};
}
