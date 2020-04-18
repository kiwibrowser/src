/* Copyright 2017 The Chromium Authors. All rights reserved.
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file. */

cr.define('safe_browsing', function() {
  'use strict';
  /**
   * Asks the C++ SafeBrowsingUIHandler to get the lists of Safe Browsing
   * ongoing experiments and preferences.
   * The SafeBrowsingUIHandler should reply to addExperiment() and
   * addPreferences() (below).
   */
  function initialize() {
    cr.sendWithPromise('getExperiments', []).then((experiments) =>
        addExperiments(experiments));
    cr.sendWithPromise('getPrefs', []).then((prefs) => addPrefs(prefs));
    cr.sendWithPromise('getDatabaseManagerInfo', []).then(
      function(databaseState) {
        var fullHashCacheState = databaseState.splice(-1,1);
        addDatabaseManagerInfo(databaseState);
        addFullHashCacheInfo(fullHashCacheState);
    });
    cr.sendWithPromise('getSentThreatDetails', []).then((threatDetails) =>
        addThreatDetailsInfo(threatDetails));
    cr.addWebUIListener('threat-details-update', function(result) {
      addThreatDetailsInfo(result);
    });
  }

  function addExperiments(result) {
    var resLength = result.length;
    var experimentsListFormatted = "";

    for (var i = 0; i < resLength; i += 2) {
      experimentsListFormatted += "<div><b>" + result[i + 1] +
          "</b>: " + result[i] + "</div>";
      }
      $('experiments-list').innerHTML = experimentsListFormatted;
  }

  function addPrefs(result) {
      var resLength = result.length;
      var preferencesListFormatted = "";

      for (var i = 0; i < resLength; i += 2) {
        preferencesListFormatted += "<div><b>" + result[i + 1] + "</b>: " +
            result[i] + "</div>";
      }
      $('preferences-list').innerHTML = preferencesListFormatted;
  }

  function addDatabaseManagerInfo(result) {
      var resLength = result.length;
      var preferencesListFormatted = "";

      for (var i = 0; i < resLength; i += 2) {
        preferencesListFormatted += "<div><b>" + result[i] + "</b>: " +
            result[i+1] + "</div>";
      }
      $('database-info-list').innerHTML = preferencesListFormatted;
  }

  function addFullHashCacheInfo(result) {
      $('full-hash-cache-info').innerHTML = result;
  }

  function addThreatDetailsInfo(result) {
      var logDiv = $('threat-details-list');
      if (!logDiv)
        return;
      var textDiv = document.createElement('div');
      textDiv.innerText = result;
      logDiv.appendChild(textDiv);
  }

  return {
    addThreatDetailsInfo: addThreatDetailsInfo,
    initialize: initialize,
  };
});

document.addEventListener('DOMContentLoaded', safe_browsing.initialize);
