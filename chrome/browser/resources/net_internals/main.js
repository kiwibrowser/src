// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Dictionary of constants (Initialized soon after loading by data from browser,
 * updated on load log).  The *Types dictionaries map strings to numeric IDs,
 * while the *TypeNames are the other way around.
 */
var EventType = null;
var EventTypeNames = null;
var EventPhase = null;
var EventSourceType = null;
var EventSourceTypeNames = null;
var ClientInfo = null;
var NetError = null;
var QuicError = null;
var QuicRstStreamError = null;
var LoadFlag = null;
var CertStatusFlag = null;
var LoadState = null;
var AddressFamily = null;
var DataReductionProxyBypassEventType = null;

/**
 * Dictionary of all constants, used for saving log files.
 */
var Constants = null;

/**
 * Object to communicate between the renderer and the browser.
 * @type {!BrowserBridge}
 */
var g_browser = null;

/**
 * This class is the root view object of the page.  It owns all the other
 * views, and manages switching between them.  It is also responsible for
 * initializing the views and the BrowserBridge.
 */
var MainView = (function() {
  'use strict';

  // We inherit from WindowView
  var superClass = WindowView;

  /**
   * Main entry point. Called once the page has loaded.
   *  @constructor
   */
  function MainView() {
    assertFirstConstructorCall(MainView);

    if (hasTouchScreen())
      document.body.classList.add('touch');

    // This must be initialized before the tabs, so they can register as
    // observers.
    g_browser = BrowserBridge.getInstance();

    // This must be the first constants observer, so other constants observers
    // can safely use the globals, rather than depending on walking through
    // the constants themselves.
    g_browser.addConstantsObserver(new ConstantsObserver());

    // Create the tab switcher.
    this.initTabs_();

    // Cut out a small vertical strip at the top of the window, to display
    // a high level status (i.e. if we are capturing events, or displaying a
    // log file). Below it we will position the main tabs and their content
    // area.
    this.topBarView_ = TopBarView.getInstance(this);
    var verticalSplitView =
        new VerticalSplitView(this.topBarView_, this.tabSwitcher_);

    superClass.call(this, verticalSplitView);

    // Trigger initial layout.
    this.resetGeometry();

    window.onhashchange = this.onUrlHashChange_.bind(this);

    // Select the initial view based on the current URL.
    window.onhashchange();

    // Tell the browser that we are ready to start receiving log events.
    this.topBarView_.switchToSubView('capture');
    g_browser.sendReady();
  }

  cr.addSingletonGetter(MainView);

  // Tracks if we're viewing a loaded log file, so views can behave
  // appropriately.  Global so safe to call during construction.
  var isViewingLoadedLog = false;

  MainView.isViewingLoadedLog = function() {
    return isViewingLoadedLog;
  };

  MainView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    // This is exposed both so the log import/export code can enumerate all the
    // tabs, and for testing.
    tabSwitcher: function() {
      return this.tabSwitcher_;
    },

    /**
     * Prevents receiving/sending events to/from the browser, so loaded data
     * will not be mixed with current Chrome state.  Also hides any interactive
     * HTML elements that send messages to the browser.  Cannot be undone
     * without reloading the page.  Must be called before passing loaded data
     * to the individual views.
     *
     * @param {string} opt_fileName The name of the log file that has been
     *     loaded, if we're loading a log file.
     */
    onLoadLog: function(opt_fileName) {
      isViewingLoadedLog = true;

      this.stopCapturing();
      if (opt_fileName != undefined) {
        // If there's a file name, a log file was loaded, so swap out the status
        // bar to indicate we're no longer capturing events.
        this.topBarView_.switchToSubView('loaded').setFileName(opt_fileName);
      } else {
        // Otherwise, the "Stop Capturing" button was presumably pressed.
        // Don't disable hiding cookies, so created log dumps won't have them,
        // unless the user toggles the option.
        this.topBarView_.switchToSubView('halted');
      }
    },

    switchToViewOnlyMode: function() {
      // Since this won't be dumped to a file, we don't want to remove
      // cookies and credentials.
      log_util.createLogDumpAsync('', log_util.loadLogFile, false);
    },

    stopCapturing: function() {
      g_browser.disable();
      var sheet = document.createElement('style');
      sheet.type = 'text/css';
      sheet.appendChild(document.createTextNode(
          '.hide-when-not-capturing { display: none; }'));
      document.head.appendChild(sheet);
    },

    initTabs_: function() {
      this.tabIdToHash_ = {};
      this.hashToTabId_ = {};

      this.tabSwitcher_ = new TabSwitcherView(this.onTabSwitched_.bind(this));

      // Helper function to add a tab given the class for a view singleton.
      var addTab = function(viewClass) {
        var tabId = viewClass.TAB_ID;
        var tabHash = viewClass.TAB_HASH;
        var tabName = viewClass.TAB_NAME;
        var view = viewClass.getInstance();

        if (!tabId || !view || !tabHash || !tabName) {
          throw Error('Invalid view class for tab');
        }

        if (tabHash.charAt(0) != '#') {
          throw Error('Tab hashes must start with a #');
        }

        this.tabSwitcher_.addTab(tabId, view, tabName, tabHash);
        this.tabIdToHash_[tabId] = tabHash;
        this.hashToTabId_[tabHash] = tabId;
      }.bind(this);

      // Populate the main tabs.  Even tabs that don't contain information for
      // the running OS should be created, so they can load log dumps from other
      // OSes.
      addTab(CaptureView);
      addTab(ImportView);
      addTab(ProxyView);
      addTab(EventsView);
      addTab(TimelineView);
      addTab(DnsView);
      addTab(SocketsView);
      addTab(AltSvcView);
      addTab(SpdyView);
      addTab(QuicView);
      addTab(ReportingView);
      addTab(HttpCacheView);
      addTab(ModulesView);
      addTab(DomainSecurityPolicyView);
      addTab(BandwidthView);
      addTab(PrerenderView);
      addTab(CrosView);

      this.tabSwitcher_.showTabLink(CrosView.TAB_ID, cr.isChromeOS);
    },

    /**
     * This function is called by the tab switcher when the current tab has been
     * changed. It will update the current URL to reflect the new active tab,
     * so the back can be used to return to previous view.
     */
    onTabSwitched_: function(oldTabId, newTabId) {
      // Update data needed by newly active tab, as it may be
      // significantly out of date.
      if (g_browser)
        g_browser.checkForUpdatedInfo();

      // Change the URL to match the new tab.

      var newTabHash = this.tabIdToHash_[newTabId];
      var parsed = parseUrlHash_(window.location.hash);
      if (parsed.tabHash != newTabHash) {
        window.location.hash = newTabHash;
      }
    },

    onUrlHashChange_: function() {
      var parsed = parseUrlHash_(window.location.hash);

      if (!parsed)
        return;

      if (parsed.tabHash == '#export') {
        // The #export tab was removed in M60, after having been
        // deprecated since M58. In case anyone *still* has URLs
        // bookmarked to this, inform them and redirect.
        // TODO(eroman): Delete this around M62.
        parsed.tabHash = undefined;

        // Done on a setTimeout so it doesn't block the initial
        // page load (confirm() is synchronous).
        setTimeout(() => {
          var navigateToNetExport = confirm(
              '#export was removed\nDo you want to navigate to ' +
              'chrome://net-export/ instead?');
          if (navigateToNetExport) {
            window.location.href = 'chrome://net-export';
            return;
          }
        });
      }

      if (!parsed.tabHash) {
        // Default to the events tab.
        parsed.tabHash = EventsView.TAB_HASH;
      }

      var tabId = this.hashToTabId_[parsed.tabHash];

      if (tabId) {
        this.tabSwitcher_.switchToTab(tabId);
        if (parsed.parameters) {
          var view = this.tabSwitcher_.getTabView(tabId);
          view.setParameters(parsed.parameters);
        }
      }
    },

  };

  /**
   * Takes the current hash in form of "#tab&param1=value1&param2=value2&..."
   * and parses it into a dictionary.
   *
   * Parameters and values are decoded with decodeURIComponent().
   */
  function parseUrlHash_(hash) {
    var parameters = hash.split('&');

    var tabHash = parameters[0];
    if (tabHash == '' || tabHash == '#') {
      tabHash = undefined;
    }

    // Split each string except the first around the '='.
    var paramDict = null;
    for (var i = 1; i < parameters.length; i++) {
      var paramStrings = parameters[i].split('=');
      if (paramStrings.length != 2)
        continue;
      if (paramDict == null)
        paramDict = {};
      var key = decodeURIComponent(paramStrings[0]);
      var value = decodeURIComponent(paramStrings[1]);
      paramDict[key] = value;
    }

    return {tabHash: tabHash, parameters: paramDict};
  }

  return MainView;
})();

function ConstantsObserver() {}

/**
 * Loads all constants from |constants|.  On failure, global dictionaries are
 * not modifed.
 * @param {Object} receivedConstants The map of received constants.
 */
ConstantsObserver.prototype.onReceivedConstants = function(receivedConstants) {
  if (!areValidConstants(receivedConstants))
    return;

  Constants = receivedConstants;

  EventType = Constants.logEventTypes;
  EventTypeNames = makeInverseMap(EventType);
  EventPhase = Constants.logEventPhase;
  EventSourceType = Constants.logSourceType;
  EventSourceTypeNames = makeInverseMap(EventSourceType);
  ClientInfo = Constants.clientInfo;
  LoadFlag = Constants.loadFlag;
  NetError = Constants.netError;
  QuicError = Constants.quicError;
  QuicRstStreamError = Constants.quicRstStreamError;
  AddressFamily = Constants.addressFamily;
  LoadState = Constants.loadState;
  DataReductionProxyBypassEventType =
      Constants.dataReductionProxyBypassEventType;
  DataReductionProxyBypassActionType =
      Constants.dataReductionProxyBypassActionType;
  // certStatusFlag may not be present when loading old log Files
  if (typeof(Constants.certStatusFlag) == 'object')
    CertStatusFlag = Constants.certStatusFlag;
  else
    CertStatusFlag = {};

  timeutil.setTimeTickOffset(Constants.timeTickOffset);
};

/**
 * Returns true if it's given a valid-looking constants object.
 * @param {Object} receivedConstants The received map of constants.
 * @return {boolean} True if the |receivedConstants| object appears valid.
 */
function areValidConstants(receivedConstants) {
  return typeof(receivedConstants) == 'object' &&
      typeof(receivedConstants.logEventTypes) == 'object' &&
      typeof(receivedConstants.clientInfo) == 'object' &&
      typeof(receivedConstants.logEventPhase) == 'object' &&
      typeof(receivedConstants.logSourceType) == 'object' &&
      typeof(receivedConstants.loadFlag) == 'object' &&
      typeof(receivedConstants.netError) == 'object' &&
      typeof(receivedConstants.addressFamily) == 'object' &&
      typeof(receivedConstants.timeTickOffset) == 'string' &&
      typeof(receivedConstants.logFormatVersion) == 'number';
}

/**
 * Returns the name for netError.
 *
 * Example: netErrorToString(-105) should return
 * "ERR_NAME_NOT_RESOLVED".
 * @param {number} netError The net error code.
 * @return {string} The name of the given error.
 */
function netErrorToString(netError) {
  return getKeyWithValue(NetError, netError);
}

/**
 * Returns the name for quicError.
 *
 * Example: quicErrorToString(25) should return
 * "TIMED_OUT".
 * @param {number} quicError The QUIC error code.
 * @return {string} The name of the given error.
 */
function quicErrorToString(quicError) {
  return getKeyWithValue(QuicError, quicError);
}

/**
 * Returns the name for quicRstStreamError.
 *
 * Example: quicRstStreamErrorToString(3) should return
 * "BAD_APPLICATION_PAYLOAD".
 * @param {number} quicRstStreamError The QUIC RST_STREAM error code.
 * @return {string} The name of the given error.
 */
function quicRstStreamErrorToString(quicRstStreamError) {
  return getKeyWithValue(QuicRstStreamError, quicRstStreamError);
}

/**
 * Returns a string representation of |family|.
 * @param {number} family An AddressFamily
 * @return {string} A representation of the given family.
 */
function addressFamilyToString(family) {
  var str = getKeyWithValue(AddressFamily, family);
  // All the address family start with ADDRESS_FAMILY_*.
  // Strip that prefix since it is redundant and only clutters the output.
  return str.replace(/^ADDRESS_FAMILY_/, '');
}
