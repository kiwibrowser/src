// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('chrome.sync.types', function() {
  var typeCountersMap = {};

  /**
   * Redraws the counters table taking advantage of the most recent
   * available information.
   *
   * Makes use of typeCountersMap, which is defined in the containing scope.
   */
  var refreshTypeCountersDisplay = function() {
    var typeCountersArray = [];

    // Transform our map into an array to make jstemplate happy.
    Object.keys(typeCountersMap).sort().forEach(function(t) {
      typeCountersArray.push({
        type: t,
        counters: typeCountersMap[t],
      });
    });

    jstProcess(
        new JsEvalContext({ rows: typeCountersArray }),
        $('type-counters-table'));
  };

  /**
   * Helps to initialize the table by picking up where initTypeCounters() left
   * off.  That function registers this listener and requests that this event
   * be emitted.
   *
   * @param {!Object} e An event containing the list of known sync types.
   */
  var onReceivedListOfTypes = function(e) {
    var types = e.details.types;
    types.map(function(type) {
      if (!typeCountersMap.hasOwnProperty(type)) {
        typeCountersMap[type] = {};
      }
    });
    chrome.sync.events.removeEventListener(
        'onReceivedListOfTypes',
        onReceivedListOfTypes);
    refreshTypeCountersDisplay();
  };

  /**
   * Callback for receipt of updated per-type counters.
   *
   * @param {!Object} e An event containing an updated counter.
   */
  var onCountersUpdated = function(e) {
    var details = e.details;

    var modelType = details.modelType;
    var counters = details.counters;

    if (typeCountersMap.hasOwnProperty(modelType))
      for (k in counters) {
        typeCountersMap[modelType][k] = counters[k];
      }
    refreshTypeCountersDisplay();
  };

  /**
   * Initializes state and callbacks for the per-type counters and status UI.
   */
  var initTypeCounters = function() {
    chrome.sync.events.addEventListener(
        'onCountersUpdated',
        onCountersUpdated);
    chrome.sync.events.addEventListener(
        'onReceivedListOfTypes',
        onReceivedListOfTypes);

    chrome.sync.requestListOfTypes();
    chrome.sync.registerForPerTypeCounters();
  };

  var onLoad = function() {
    initTypeCounters();
  };

  return {
    onLoad: onLoad
  };
});

document.addEventListener('DOMContentLoaded', chrome.sync.types.onLoad, false);
