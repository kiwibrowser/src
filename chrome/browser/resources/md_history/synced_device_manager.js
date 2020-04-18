// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{device: string,
 *           lastUpdateTime: string,
 *           opened: boolean,
 *           separatorIndexes: !Array<number>,
 *           timestamp: number,
 *           tabs: !Array<!ForeignSessionTab>,
 *           tag: string}}
 */
let ForeignDeviceInternal;

Polymer({
  is: 'history-synced-device-manager',

  properties: {
    /**
     * @type {?Array<!ForeignSession>}
     */
    sessionList: {
      type: Array,
      observer: 'updateSyncedDevices',
    },

    searchTerm: {
      type: String,
      observer: 'searchTermChanged',
    },

    /**
     * An array of synced devices with synced tab data.
     * @type {!Array<!ForeignDeviceInternal>}
     */
    syncedDevices_: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /** @private */
    signInState: {
      type: Boolean,
      observer: 'signInStateChanged_',
    },

    /** @private */
    guestSession_: {
      type: Boolean,
      value: loadTimeData.getBoolean('isGuestSession'),
    },

    /** @private */
    fetchingSyncedTabs_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    hasSeenForeignData_: Boolean,

    /**
     * The session ID referring to the currently active action menu.
     * @private {?string}
     */
    actionMenuModel_: String,
  },

  listeners: {
    'open-menu': 'onOpenMenu_',
    'update-focus-grid': 'updateFocusGrid_',
  },

  /** @type {?cr.ui.FocusGrid} */
  focusGrid_: null,

  /** @override */
  attached: function() {
    this.focusGrid_ = new cr.ui.FocusGrid();

    // Update the sign in state.
    chrome.send('otherDevicesInitialized');
    md_history.BrowserService.getInstance().recordHistogram(
        SYNCED_TABS_HISTOGRAM_NAME, SyncedTabsHistogram.INITIALIZED,
        SyncedTabsHistogram.LIMIT);
  },

  /** @override */
  detached: function() {
    this.focusGrid_.destroy();
  },

  /** @return {HTMLElement} */
  getContentScrollTarget: function() {
    return this;
  },

  /**
   * @param {!ForeignSession} session
   * @return {!ForeignDeviceInternal}
   * @private
   */
  createInternalDevice_: function(session) {
    let tabs = [];
    const separatorIndexes = [];
    for (let i = 0; i < session.windows.length; i++) {
      const windowId = session.windows[i].sessionId;
      const newTabs = session.windows[i].tabs;
      if (newTabs.length == 0)
        continue;

      newTabs.forEach(function(tab) {
        tab.windowId = windowId;
      });

      let windowAdded = false;
      if (!this.searchTerm) {
        // Add all the tabs if there is no search term.
        tabs = tabs.concat(newTabs);
        windowAdded = true;
      } else {
        const searchText = this.searchTerm.toLowerCase();
        for (let j = 0; j < newTabs.length; j++) {
          const tab = newTabs[j];
          if (tab.title.toLowerCase().indexOf(searchText) != -1) {
            tabs.push(tab);
            windowAdded = true;
          }
        }
      }
      if (windowAdded && i != session.windows.length - 1)
        separatorIndexes.push(tabs.length - 1);
    }
    return {
      device: session.name,
      lastUpdateTime: '– ' + session.modifiedTime,
      opened: true,
      separatorIndexes: separatorIndexes,
      timestamp: session.timestamp,
      tabs: tabs,
      tag: session.tag,
    };
  },

  /** @private */
  onSignInTap_: function() {
    chrome.send('startSignInFlow');
  },

  /** @private */
  onOpenMenu_: function(e) {
    const menu = /** @type {CrActionMenuElement} */ (this.$.menu.get());
    this.actionMenuModel_ = e.detail.tag;
    menu.showAt(e.detail.target);
    md_history.BrowserService.getInstance().recordHistogram(
        SYNCED_TABS_HISTOGRAM_NAME, SyncedTabsHistogram.SHOW_SESSION_MENU,
        SyncedTabsHistogram.LIMIT);
  },

  /** @private */
  onOpenAllTap_: function() {
    const menu = assert(this.$.menu.getIfExists());
    const browserService = md_history.BrowserService.getInstance();
    browserService.recordHistogram(
        SYNCED_TABS_HISTOGRAM_NAME, SyncedTabsHistogram.OPEN_ALL,
        SyncedTabsHistogram.LIMIT);
    browserService.openForeignSessionAllTabs(assert(this.actionMenuModel_));
    this.actionMenuModel_ = null;
    menu.close();
  },

  /** @private */
  updateFocusGrid_: function() {
    if (!this.focusGrid_)
      return;

    this.focusGrid_.destroy();

    this.debounce('updateFocusGrid', function() {
      Polymer.dom(this.root)
          .querySelectorAll('history-synced-device-card')
          .reduce(
              function(prev, cur) {
                return prev.concat(cur.createFocusRows());
              },
              [])
          .forEach((row) => {
            this.focusGrid_.addRow(row);
          });
      this.focusGrid_.ensureRowActive(1);
    });
  },

  /** @private */
  onDeleteSessionTap_: function() {
    const menu = assert(this.$.menu.getIfExists());
    const browserService = md_history.BrowserService.getInstance();
    browserService.recordHistogram(
        SYNCED_TABS_HISTOGRAM_NAME, SyncedTabsHistogram.HIDE_FOR_NOW,
        SyncedTabsHistogram.LIMIT);
    browserService.deleteForeignSession(assert(this.actionMenuModel_));
    this.actionMenuModel_ = null;
    menu.close();
  },

  /** @private */
  clearDisplayedSyncedDevices_: function() {
    this.syncedDevices_ = [];
  },

  /**
   * Decide whether or not should display no synced tabs message.
   * @param {boolean} signInState
   * @param {number} syncedDevicesLength
   * @param {boolean} guestSession
   * @return {boolean}
   */
  showNoSyncedMessage: function(
      signInState, syncedDevicesLength, guestSession) {
    if (guestSession)
      return true;

    return signInState && syncedDevicesLength == 0;
  },

  /**
   * Shows the signin guide when the user is not signed in and not in a guest
   * session.
   * @param {boolean} signInState
   * @param {boolean} guestSession
   * @return {boolean}
   */
  showSignInGuide: function(signInState, guestSession) {
    const show = !signInState && !guestSession;
    if (show) {
      md_history.BrowserService.getInstance().recordAction(
          'Signin_Impression_FromRecentTabs');
    }

    return show;
  },

  /**
   * Decide what message should be displayed when user is logged in and there
   * are no synced tabs.
   * @return {string}
   */
  noSyncedTabsMessage: function() {
    let stringName = this.fetchingSyncedTabs_ ? 'loading' : 'noSyncedResults';
    if (this.searchTerm !== '')
      stringName = 'noSearchResults';
    return loadTimeData.getString(stringName);
  },

  /**
   * Replaces the currently displayed synced tabs with |sessionList|. It is
   * common for only a single session within the list to have changed, We try to
   * avoid doing extra work in this case. The logic could be more intelligent
   * about updating individual tabs rather than replacing whole sessions, but
   * this approach seems to have acceptable performance.
   * @param {?Array<!ForeignSession>} sessionList
   */
  updateSyncedDevices: function(sessionList) {
    this.fetchingSyncedTabs_ = false;

    if (!sessionList)
      return;

    if (sessionList.length > 0 && !this.hasSeenForeignData_) {
      this.hasSeenForeignData_ = true;
      md_history.BrowserService.getInstance().recordHistogram(
          SYNCED_TABS_HISTOGRAM_NAME, SyncedTabsHistogram.HAS_FOREIGN_DATA,
          SyncedTabsHistogram.LIMIT);
    }

    const devices = [];
    sessionList.forEach((session) => {
      const device = this.createInternalDevice_(session);
      if (device.tabs.length != 0)
        devices.push(device);
    });

    this.syncedDevices_ = devices;
  },

  /**
   * Get called when user's sign in state changes, this will affect UI of synced
   * tabs page. Sign in promo gets displayed when user is signed out, and
   * different messages are shown when there are no synced tabs.
   */
  signInStateChanged_: function() {
    this.fire('history-view-changed');

    // User signed out, clear synced device list and show the sign in promo.
    if (!this.signInState) {
      this.clearDisplayedSyncedDevices_();
      return;
    }
    // User signed in, show the loading message when querying for synced
    // devices.
    this.fetchingSyncedTabs_ = true;
  },

  searchTermChanged: function(searchTerm) {
    this.clearDisplayedSyncedDevices_();
    this.updateSyncedDevices(this.sessionList);
  }
});
