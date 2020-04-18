// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Requests the database from the backend.
 */
function requestAutocompleteActionPredictorDb() {
  chrome.send('requestAutocompleteActionPredictorDb');
}

/**
 * Callback from backend with the database contents. Sets up some globals and
 * calls to create the UI.
 * @param {Object} database Information about AutocompleteActionPredictor
 *     including the database as a flattened list, a boolean indicating if the
 *     system is enabled and the current hit weight.
 */
function updateAutocompleteActionPredictorDb(database) {
  console.debug('Updating Table NAP DB');

  var filter = $('filter');
  filter.disabled = false;
  filter.onchange = function() {
    updateAutocompleteActionPredictorDbView(database);
  };

  updateAutocompleteActionPredictorDbView(database);
}

/**
 * Updates the table from the database.
 * @param {Object} database Information about AutocompleteActionPredictor
 *     including the database as a flattened list, a boolean indicating if the
 *     system is enabled and the current hit weight.
 */
function updateAutocompleteActionPredictorDbView(database) {
  var databaseSection = $('databaseTableBody');
  var showEnabled = database.enabled && database.db;

  $('autocompleteActionPredictorEnabledMode').hidden = !showEnabled;
  $('autocompleteActionPredictorDisabledMode').hidden = showEnabled;

  if (!showEnabled)
    return;

  var filter = $('filter');

  // Clear any previous list.
  databaseSection.textContent = '';

  for (var i = 0; i < database.db.length; ++i) {
    var entry = database.db[i];

    if (!filter.checked || entry.confidence > 0) {
      var row = document.createElement('tr');
      row.className =
          (entry.confidence > 0.8 ?
               'action-prerender' :
               (entry.confidence > 0.5 ? 'action-preconnect' : 'action-none'));

      row.appendChild(document.createElement('td')).textContent =
          entry.user_text;
      row.appendChild(document.createElement('td')).textContent = entry.url;
      row.appendChild(document.createElement('td')).textContent =
          entry.hit_count;
      row.appendChild(document.createElement('td')).textContent =
          entry.miss_count;
      row.appendChild(document.createElement('td')).textContent =
          entry.confidence;

      databaseSection.appendChild(row);
    }
  }
  $('countBanner').textContent = 'Entries: ' + databaseSection.children.length;
}

document.addEventListener(
    'DOMContentLoaded', requestAutocompleteActionPredictorDb);
