// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * The status view at the top of the page when in capturing mode.
 */
var CaptureStatusView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  function CaptureStatusView() {
    superClass.call(this, CaptureStatusView.MAIN_BOX_ID);

    this.getDropdown_().onchange = this.onSelectAction_.bind(this);
    this.getDropdown_().selectedIndex = -1;

    this.capturedEventsCountBox_ =
        $(CaptureStatusView.CAPTURED_EVENTS_COUNT_ID);
    this.updateEventCounts_();

    EventsTracker.getInstance().addLogEntryObserver(this);
  }

  // IDs for special HTML elements in status_view.html
  CaptureStatusView.MAIN_BOX_ID = 'capture-status-view';
  CaptureStatusView.ACTIONS_DROPDOWN_ID = 'capture-status-view-actions';
  CaptureStatusView.CAPTURED_EVENTS_COUNT_ID =
      'capture-status-view-captured-events-count';

  CaptureStatusView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    /**
     * Called whenever new log entries have been received.
     */
    onReceivedLogEntries: function(logEntries) {
      this.updateEventCounts_();
    },

    /**
     * Called whenever all log events are deleted.
     */
    onAllLogEntriesDeleted: function() {
      this.updateEventCounts_();
    },

    /**
     * Updates the counters showing how many events have been captured.
     */
    updateEventCounts_: function() {
      this.capturedEventsCountBox_.textContent =
          EventsTracker.getInstance().getNumCapturedEvents();
    },

    getDropdown_: function() {
      return $(CaptureStatusView.ACTIONS_DROPDOWN_ID);
    },

    onSelectAction_: function() {
      var dropdown = this.getDropdown_();
      var action = dropdown.value;
      dropdown.selectedIndex = -1;

      if (action == 'stop') {
        $(CaptureView.STOP_BUTTON_ID).click();
      } else if (action == 'reset') {
        $(CaptureView.RESET_BUTTON_ID).click();
      } else if (action == 'clear-cache') {
        g_browser.sendClearAllCache();
      } else if (action == 'flush-sockets') {
        g_browser.sendFlushSocketPools();
      } else if (action == 'switch-time-format') {
        var tracker = SourceTracker.getInstance();
        tracker.setUseRelativeTimes(!tracker.getUseRelativeTimes());
      } else {
        throw Error('Unrecognized action: ' + action);
      }
    },
  };

  return CaptureStatusView;
})();
