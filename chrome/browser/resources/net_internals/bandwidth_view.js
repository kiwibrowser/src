// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** This view displays summary statistics on bandwidth usage. */
var BandwidthView = (function() {
  'use strict';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function BandwidthView() {
    assertFirstConstructorCall(BandwidthView);

    // Call superclass's constructor.
    superClass.call(this, BandwidthView.MAIN_BOX_ID);

    g_browser.addSessionNetworkStatsObserver(this, true);
    g_browser.addHistoricNetworkStatsObserver(this, true);

    // Register to receive data reduction proxy info.
    g_browser.addDataReductionProxyInfoObserver(this, true);

    // Register to receive bad proxy info.
    g_browser.addBadProxiesObserver(this, true);

    this.sessionNetworkStats_ = null;
    this.historicNetworkStats_ = null;
  }

  BandwidthView.TAB_ID = 'tab-handle-bandwidth';
  BandwidthView.TAB_NAME = 'Bandwidth';
  BandwidthView.TAB_HASH = '#bandwidth';

  // IDs for special HTML elements in bandwidth_view.html
  BandwidthView.MAIN_BOX_ID = 'bandwidth-view-tab-content';
  BandwidthView.ENABLED_ID = 'data-reduction-proxy-enabled';
  BandwidthView.PROXY_CONFIG_ID = 'data-reduction-proxy-config';
  BandwidthView.PROBE_STATUS_ID = 'data-reduction-proxy-probe-status';
  BandwidthView.BYPASS_STATE_CONTAINER_ID =
      'data-reduction-proxy-bypass-state-container';
  BandwidthView.BYPASS_STATE_ID = 'data-reduction-proxy-bypass-state-details';
  BandwidthView.EVENTS_TBODY_ID = 'data-reduction-proxy-view-events-tbody';
  BandwidthView.EVENTS_UL = 'data-reduction-proxy-view-events-list';
  BandwidthView.STATS_BOX_ID = 'bandwidth-stats-table';

  cr.addSingletonGetter(BandwidthView);

  BandwidthView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    data_reduction_proxy_config_: null,
    last_bypass_: null,
    bad_proxy_config_: null,

    onLoadLogFinish: function(data) {
      return this.onBadProxiesChanged(data.badProxies) &&
          this.onDataReductionProxyInfoChanged(data.dataReductionProxyInfo) &&
          (this.onSessionNetworkStatsChanged(data.sessionNetworkStats) ||
           this.onHistoricNetworkStatsChanged(data.historicNetworkStats));
    },

    /**
     * Retains information on bandwidth usage this session.
     */
    onSessionNetworkStatsChanged: function(sessionNetworkStats) {
      this.sessionNetworkStats_ = sessionNetworkStats;
      return this.updateBandwidthUsageTable_();
    },

    /**
     * Displays information on bandwidth usage this session and over the
     * browser's lifetime.
     */
    onHistoricNetworkStatsChanged: function(historicNetworkStats) {
      this.historicNetworkStats_ = historicNetworkStats;
      return this.updateBandwidthUsageTable_();
    },

    /**
     * Updates the UI based on receiving changes in information about the
     * data reduction proxy summary.
     */
    onDataReductionProxyInfoChanged: function(info) {
      $(BandwidthView.EVENTS_TBODY_ID).innerHTML = '';

      if (!info)
        return false;

      if (info.enabled) {
        $(BandwidthView.ENABLED_ID).innerText = 'Enabled';
        $(BandwidthView.PROBE_STATUS_ID).innerText =
            info.probe != null ? info.probe : 'N/A';
        this.last_bypass_ = info.last_bypass;
        this.data_reduction_proxy_config_ = info.proxy_config.params;
      } else {
        $(BandwidthView.ENABLED_ID).innerText = 'Disabled';
        $(BandwidthView.PROBE_STATUS_ID).innerText = 'N/A';
        this.data_reduction_proxy_config_ = null;
      }

      this.updateDataReductionProxyConfig_();

      for (var eventIndex = info.events.length - 1; eventIndex >= 0;
           --eventIndex) {
        var event = info.events[eventIndex];
        var headerRow = addNode($(BandwidthView.EVENTS_TBODY_ID), 'tr');
        var detailsRow = addNode($(BandwidthView.EVENTS_TBODY_ID), 'tr');

        var timeCell = addNode(headerRow, 'td');
        var actionCell = addNode(headerRow, 'td');
        var detailsCell = addNode(detailsRow, 'td');
        detailsCell.colSpan = 2;
        detailsCell.className = 'data-reduction-proxy-view-events-details';
        var eventTime = timeutil.convertTimeTicksToDate(event.time);
        timeutil.addNodeWithDate(timeCell, eventTime);
        this.buildEventRow_(event, actionCell, detailsCell);
      }

      return true;
    },

    /**
     * Updates the UI based on receiving changes in information about bad
     * proxy servers.
     */
    onBadProxiesChanged: function(badProxies) {
      if (!badProxies)
        return false;

      var newBadProxies = [];
      if (badProxies.length == 0) {
        this.last_bypass_ = null;
      } else {
        for (var i = 0; i < badProxies.length; ++i) {
          var entry = badProxies[i];
          newBadProxies[entry.proxy_uri] = entry.bad_until;
        }
      }
      this.bad_proxy_config_ = newBadProxies;
      this.updateDataReductionProxyConfig_();

      return true;
    },

    /**
     * Update the bandwidth usage table.  Returns false on failure.
     */
    updateBandwidthUsageTable_: function() {
      var sessionNetworkStats = this.sessionNetworkStats_;
      var historicNetworkStats = this.historicNetworkStats_;
      if (!sessionNetworkStats || !historicNetworkStats)
        return false;

      var sessionOriginal = sessionNetworkStats.session_original_content_length;
      var sessionReceived = sessionNetworkStats.session_received_content_length;
      var historicOriginal =
          historicNetworkStats.historic_original_content_length;
      var historicReceived =
          historicNetworkStats.historic_received_content_length;

      var rows = [];
      rows.push({
        title: 'Original (KB)',
        sessionValue: bytesToRoundedKilobytes_(sessionOriginal),
        historicValue: bytesToRoundedKilobytes_(historicOriginal)
      });
      rows.push({
        title: 'Received (KB)',
        sessionValue: bytesToRoundedKilobytes_(sessionReceived),
        historicValue: bytesToRoundedKilobytes_(historicReceived)
      });
      rows.push({
        title: 'Savings (KB)',
        sessionValue:
            bytesToRoundedKilobytes_(sessionOriginal - sessionReceived),
        historicValue:
            bytesToRoundedKilobytes_(historicOriginal - historicReceived)
      });
      rows.push({
        title: 'Savings (%)',
        sessionValue: getPercentSavings_(sessionOriginal, sessionReceived),
        historicValue: getPercentSavings_(historicOriginal, historicReceived)
      });

      var input = new JsEvalContext({rows: rows});
      jstProcess(input, $(BandwidthView.STATS_BOX_ID));
      return true;
    },

    /**
     * Renders a Data Reduction Proxy event into the event tbody
     */
    buildEventRow_: function(event, actionCell, detailsCell) {
      if (event.type == EventType.DATA_REDUCTION_PROXY_ENABLED &&
          event.params.enabled == 0) {
        addTextNode(actionCell, 'DISABLED');
      } else {
        var actionText =
            EventTypeNames[event.type].replace('DATA_REDUCTION_PROXY_', '');
        if (event.phase == EventPhase.PHASE_BEGIN ||
            event.phase == EventPhase.PHASE_END) {
          actionText = actionText + ' (' +
              getKeyWithValue(EventPhase, event.phase).replace('PHASE_', '') +
              ')';
        }

        addTextNode(actionCell, actionText);
        this.createEventTable_(event.params, detailsCell);
      }
    },

    /**
     * Updates the data reduction proxy summary block.
     */
    updateDataReductionProxyConfig_: function() {
      $(BandwidthView.PROXY_CONFIG_ID).innerHTML = '';
      $(BandwidthView.BYPASS_STATE_ID).innerHTML = '';
      setNodeDisplay($(BandwidthView.BYPASS_STATE_CONTAINER_ID), false);

      if (this.data_reduction_proxy_config_) {
        var hasBypassedProxy = false;
        var now = timeutil.getCurrentTimeTicks();

        if (this.last_bypass_ &&
            this.hasTimePassedLogTime_(+this.last_bypass_.params.expiration)) {
          // Best effort on iterating the config to search for a bad proxy.
          // A server could exist in a string member of
          // data_reduction_proxy_config_ or within an array of servers in an
          // array member of data_reduction_proxy_config_. As such, search
          // through all string members and string arrays.
          for (var key in this.data_reduction_proxy_config_) {
            var value = this.data_reduction_proxy_config_[key];
            if (typeof value == 'string') {
              if (this.isMarkedAsBad_(value)) {
                hasBypassedProxy = true;
                break;
              }
            } else if (value instanceof Array) {
              for (var index = 1; index < value.length; index++) {
                if (this.isMarkedAsBad_(value[index])) {
                  hasBypassedProxy = true;
                }
              }

              if (hasBypassedProxy) {
                break;
              }
            }
          }
        }

        if (hasBypassedProxy) {
          this.createEventTable_(
              this.last_bypass_.params, $(BandwidthView.BYPASS_STATE_ID));
        }

        this.createEventTable_(
            this.data_reduction_proxy_config_,
            $(BandwidthView.PROXY_CONFIG_ID));
        setNodeDisplay(
            $(BandwidthView.BYPASS_STATE_CONTAINER_ID), hasBypassedProxy);
      }
    },

    /**
     * Checks to see if a proxy server is in marked as bad.
     */
    isMarkedAsBad_: function(proxy) {
      for (var entry in this.bad_proxy_config_) {
        if (entry == proxy &&
            this.hasTimePassedLogTime_(this.bad_proxy_config_[entry])) {
          return true;
        }
      }

      return false;
    },

    /**
     * Checks to see if a given time in ticks has passed the time of the
     * the log. For real time viewing, this is "now", but for loaded logs, it
     * is the time at which the logs were taken.
     */
    hasTimePassedLogTime_: function(timeTicks) {
      var logTime;
      if (MainView.isViewingLoadedLog() && ClientInfo.numericDate) {
        logTime = ClientInfo.numericDate;
      } else {
        logTime = timeutil.getCurrentTime();
      }

      return timeutil.convertTimeTicksToTime(timeTicks) > logTime;
    },

    /**
     * Creates a table of the object obj. Certain keys are special cased for
     * ease of readability.
     */
    createEventTable_: function(obj, parentNode) {
      if (Object.keys(obj).length > 0) {
        var tableNode = addNode(parentNode, 'table');
        tableNode.className = 'borderless-table';
        for (var key in obj) {
          var value = obj[key];
          if (value != null && value.toString() != '') {
            if (key == 'net_error') {
              if (value == 0) {
                value = 'OK';
              } else {
                value = netErrorToString(value);
              }
            } else if (key == 'bypass_type') {
              value = getKeyWithValue(DataReductionProxyBypassEventType, value);
            } else if (key == 'bypass_action_type') {
              value =
                  getKeyWithValue(DataReductionProxyBypassActionType, value);
            } else if (key == 'expiration') {
              value = timeutil.convertTimeTicksToDate(value);
            }
            var tableRow = addNode(tableNode, 'tr');
            addNodeWithText(tableRow, 'td', key);
            addNodeWithText(tableRow, 'td', value);
          }
        }
      }
    }
  };

  /**
   * Converts bytes to kilobytes rounded to one decimal place.
   */
  function bytesToRoundedKilobytes_(val) {
    return (val / 1024).toFixed(1);
  }

  /**
   * Returns bandwidth savings as a percent rounded to one decimal place.
   */
  function getPercentSavings_(original, received) {
    if (original > 0) {
      return ((original - received) * 100 / original).toFixed(1);
    }
    return '0.0';
  }

  return BandwidthView;
})();
