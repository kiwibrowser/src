// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Javascript for omnibox.html, served from chrome://omnibox/
 * This is used to debug omnibox ranking.  The user enters some text
 * into a box, submits it, and then sees lots of debug information
 * from the autocompleter that shows what omnibox would do with that
 * input.
 *
 * The simple object defined in this javascript file listens for
 * certain events on omnibox.html, sends (when appropriate) the
 * input text to C++ code to start the omnibox autcomplete controller
 * working, and listens from callbacks from the C++ code saying that
 * results are available.  When results (possibly intermediate ones)
 * are available, the Javascript formats them and displays them.
 */

(function() {
/**
 * Register our event handlers.
 */
function initialize() {
  $('omnibox-input-form').addEventListener('submit', startOmniboxQuery, false);
  $('prevent-inline-autocomplete')
      .addEventListener('change', startOmniboxQuery);
  $('prefer-keyword').addEventListener('change', startOmniboxQuery);
  $('page-classification').addEventListener('change', startOmniboxQuery);
  $('show-details').addEventListener('change', refresh);
  $('show-incomplete-results').addEventListener('change', refresh);
  $('show-all-providers').addEventListener('change', refresh);
}

/**
 * @type {OmniboxResultMojo} an array of all autocomplete results we've seen
 *     for this query.  We append to this list once for every call to
 *     handleNewAutocompleteResult.  See omnibox.mojom for details..
 */
var progressiveAutocompleteResults = [];

/**
 * @type {number} the value for cursor position we sent with the most
 *     recent request.  We need to remember this in order to display it
 *     in the output; otherwise it's hard or impossible to determine
 *     from screen captures or print-to-PDFs.
 */
var cursorPositionUsed = -1;

/**
 * Extracts the input text from the text field and sends it to the
 * C++ portion of chrome to handle.  The C++ code will iteratively
 * call handleNewAutocompleteResult as results come in.
 */
function startOmniboxQuery(event) {
  // First, clear the results of past calls (if any).
  progressiveAutocompleteResults = [];
  // Then, call chrome with a five-element list:
  // - first element: the value in the text box
  // - second element: the location of the cursor in the text box
  // - third element: the value of prevent-inline-autocomplete
  // - forth element: the value of prefer-keyword
  // - fifth element: the value of page-classification
  cursorPositionUsed = $('input-text').selectionEnd;
  browserProxy.startOmniboxQuery(
      $('input-text').value, cursorPositionUsed,
      $('prevent-inline-autocomplete').checked, $('prefer-keyword').checked,
      parseInt($('page-classification').value));
  // Cancel the submit action.  i.e., don't submit the form.  (We handle
  // display the results solely with Javascript.)
  event.preventDefault();
}

/**
 * Returns a simple object with information about how to display an
 * autocomplete result data field.
 * @param {string} header the label for the top of the column/table.
 * @param {string} urlLabelForHeader the URL that the header should point
 *     to (if non-empty).
 * @param {string} propertyName the name of the property in the autocomplete
 *     result record that we lookup.
 * @param {boolean} displayAlways whether the property should be displayed
 *     regardless of whether we're in detailed mode.
 * @param {string} tooltip a description of the property that will be
 *     presented as a tooltip when the mouse is hovered over the column title.
 * @constructor
 */
function PresentationInfoRecord(
    header, url, propertyName, displayAlways, tooltip) {
  this.header = header;
  this.urlLabelForHeader = url;
  this.propertyName = propertyName;
  this.displayAlways = displayAlways;
  this.tooltip = tooltip;
}

/**
 * A constant that's used to decide what autocomplete result
 * properties to output in what order.  This is an array of
 * PresentationInfoRecord() objects; for details see that
 * function.
 * @type {Array<Object>}
 * @const
 */
var PROPERTY_OUTPUT_ORDER = [
  new PresentationInfoRecord(
      'Provider', '', 'providerName', true,
      'The AutocompleteProvider suggesting this result.'),
  new PresentationInfoRecord(
      'Type', '', 'type', true, 'The type of the result.'),
  new PresentationInfoRecord(
      'Relevance', '', 'relevance', true,
      'The result score. Higher is more relevant.'),
  new PresentationInfoRecord(
      'Contents', '', 'contents', true,
      'The text that is presented identifying the result.'),
  new PresentationInfoRecord(
      'Can Be Default', '', 'allowedToBeDefaultMatch', false,
      'A green checkmark indicates that the result can be the default ' +
          'match (i.e., can be the match that pressing enter in the omnibox ' +
          'navigates to).'),
  new PresentationInfoRecord(
      'Starred', '', 'starred', false,
      'A green checkmark indicates that the result has been bookmarked.'),
  new PresentationInfoRecord(
      'Has tab match', '', 'hasTabMatch', false,
      'A green checkmark indicates that the result URL matches an open tab.'),
  new PresentationInfoRecord(
      'Description', '', 'description', false, 'The page title of the result.'),
  new PresentationInfoRecord(
      'URL', '', 'destinationUrl', true, 'The URL for the result.'),
  new PresentationInfoRecord(
      'Fill Into Edit', '', 'fillIntoEdit', false,
      'The text shown in the omnibox when the result is selected.'),
  new PresentationInfoRecord(
      'Inline Autocompletion', '', 'inlineAutocompletion', false,
      'The text shown in the omnibox as a blue highlight selection ' +
          'following the cursor, if this match is shown inline.'),
  new PresentationInfoRecord(
      'Del', '', 'deletable', false,
      'A green checkmark indicates that the result can be deleted from ' +
          'the visit history.'),
  new PresentationInfoRecord('Prev', '', 'fromPrevious', false, ''),
  new PresentationInfoRecord(
      'Tran',
      'http://code.google.com/codesearch#OAMlx_jo-ck/src/content/public/' +
          'common/page_transition_types.h&exact_package=chromium&l=24',
      'transition', false, 'How the user got to the result.'),
  new PresentationInfoRecord(
      'Done', '', 'providerDone', false,
      'A green checkmark indicates that the provider is done looking for ' +
          'more results.'),
  new PresentationInfoRecord(
      'Associated Keyword', '', 'associatedKeyword', false,
      'If non-empty, a "press tab to search" hint will be shown and will ' +
          'engage this keyword.'),
  new PresentationInfoRecord(
      'Keyword', '', 'keyword', false,
      'The keyword of the search engine to be used.'),
  new PresentationInfoRecord(
      'Duplicates', '', 'duplicates', false,
      'The number of matches that have been marked as duplicates of this ' +
          'match.'),
  new PresentationInfoRecord(
      'Additional Info', '', 'additionalInfo', false,
      'Provider-specific information about the result.')
];

/**
 * Returns an HTML Element of type table row that contains the
 * headers we'll use for labeling the columns.  If we're in
 * detailedMode, we use all the headers.  If not, we only use ones
 * marked displayAlways.
 */
function createAutocompleteResultTableHeader() {
  var row = document.createElement('tr');
  var inDetailedMode = $('show-details').checked;
  for (var i = 0; i < PROPERTY_OUTPUT_ORDER.length; i++) {
    if (inDetailedMode || PROPERTY_OUTPUT_ORDER[i].displayAlways) {
      var headerCell = document.createElement('th');
      if (PROPERTY_OUTPUT_ORDER[i].urlLabelForHeader != '') {
        // Wrap header text in URL.
        var linkNode = document.createElement('a');
        linkNode.href = PROPERTY_OUTPUT_ORDER[i].urlLabelForHeader;
        linkNode.textContent = PROPERTY_OUTPUT_ORDER[i].header;
        headerCell.appendChild(linkNode);
      } else {
        // Output header text without a URL.
        headerCell.textContent = PROPERTY_OUTPUT_ORDER[i].header;
        headerCell.className = 'table-header';
        headerCell.title = PROPERTY_OUTPUT_ORDER[i].tooltip;
      }
      row.appendChild(headerCell);
    }
  }
  return row;
}

/**
 * @param {AutocompleteMatchMojo} autocompleteSuggestion the particular
 *     autocomplete suggestion we're in the process of displaying.
 * @param {string} propertyName the particular property of the autocomplete
 *     suggestion that should go in this cell.
 * @return {HTMLTableCellElement} that contains the value within this
 *     autocompleteSuggestion associated with propertyName.
 */
function createCellForPropertyAndRemoveProperty(
    autocompleteSuggestion, propertyName) {
  var cell = document.createElement('td');
  if (propertyName in autocompleteSuggestion) {
    if (propertyName == 'additionalInfo') {
      // |additionalInfo| embeds a two-column table of provider-specific data
      // within this cell. |additionalInfo| is an array of
      // AutocompleteAdditionalInfo.
      var additionalInfoTable = document.createElement('table');
      for (var i = 0; i < autocompleteSuggestion[propertyName].length; i++) {
        var additionalInfo = autocompleteSuggestion[propertyName][i];
        var additionalInfoRow = document.createElement('tr');

        // Set the title (name of property) cell text.
        var propertyCell = document.createElement('td');
        propertyCell.textContent = additionalInfo.key + ':';
        propertyCell.className = 'additional-info-property';
        additionalInfoRow.appendChild(propertyCell);

        // Set the value of the property cell text.
        var valueCell = document.createElement('td');
        valueCell.textContent = additionalInfo.value;
        valueCell.className = 'additional-info-value';
        additionalInfoRow.appendChild(valueCell);

        additionalInfoTable.appendChild(additionalInfoRow);
      }
      cell.appendChild(additionalInfoTable);
    } else if (typeof autocompleteSuggestion[propertyName] == 'boolean') {
      // If this is a boolean, display a checkmark or an X instead of
      // the strings true or false.
      if (autocompleteSuggestion[propertyName]) {
        cell.className = 'check-mark';
        cell.textContent = '✔';
      } else {
        cell.className = 'x-mark';
        cell.textContent = '✗';
      }
    } else {
      var text = String(autocompleteSuggestion[propertyName]);
      // If it's a URL wrap it in an href.
      var re = /^(http|https|ftp|chrome|file):\/\//;
      if (re.test(text)) {
        var aCell = document.createElement('a');
        aCell.textContent = text;
        aCell.href = text;
        cell.appendChild(aCell);
      } else {
        // All other data types (integer, strings, etc.) display their
        // normal toString() output.
        cell.textContent = autocompleteSuggestion[propertyName];
      }
    }
  }  // else: if propertyName is undefined, we leave the cell blank
  return cell;
}

/**
 * Appends some human-readable information about the provided
 * autocomplete result to the HTML node with id omnibox-debug-text.
 * The current human-readable form is a few lines about general
 * autocomplete result statistics followed by a table with one line
 * for each autocomplete match.  The input parameter is an OmniboxResultMojo.
 */
function addResultToOutput(result) {
  var output = $('omnibox-debug-text');
  var inDetailedMode = $('show-details').checked;
  var showIncompleteResults = $('show-incomplete-results').checked;
  var showPerProviderResults = $('show-all-providers').checked;

  // Always output cursor position.
  var p = document.createElement('p');
  p.textContent = 'cursor position = ' + cursorPositionUsed;
  output.appendChild(p);

  // Output the result-level features in detailed mode and in
  // show incomplete results mode.  We do the latter because without
  // these result-level features, one can't make sense of each
  // batch of results.
  if (inDetailedMode || showIncompleteResults) {
    var p1 = document.createElement('p');
    p1.textContent =
        'elapsed time = ' + result.timeSinceOmniboxStartedMs + 'ms';
    output.appendChild(p1);
    var p2 = document.createElement('p');
    p2.textContent = 'all providers done = ' + result.done;
    output.appendChild(p2);
    var p3 = document.createElement('p');
    p3.textContent = 'host = ' + result.host;
    if ('isTypedHost' in result) {
      // Only output the isTypedHost information if available.  (It may
      // be missing if the history database lookup failed.)
      p3.textContent =
          p3.textContent + ' has isTypedHost = ' + result.isTypedHost;
    }
    output.appendChild(p3);
  }

  // Combined results go after the lines below.
  var group = document.createElement('a');
  group.className = 'group-separator';
  group.textContent = 'Combined results.';
  output.appendChild(group);

  // Add combined/merged result table.
  var p = document.createElement('p');
  p.appendChild(addResultTableToOutput(result.combinedResults));
  output.appendChild(p);

  // Move forward only if you want to display per provider results.
  if (!showPerProviderResults) {
    return;
  }

  // Individual results go after the lines below.
  var group = document.createElement('a');
  group.className = 'group-separator';
  group.textContent = 'Results for individual providers.';
  output.appendChild(group);

  // Add the per-provider result tables with labels. We do not append the
  // combined/merged result table since we already have the per provider
  // results.
  for (var i = 0; i < result.resultsByProvider.length; i++) {
    var providerResults = result.resultsByProvider[i];
    // If we have no results we do not display anything.
    if (providerResults.results.length == 0) {
      continue;
    }
    var p = document.createElement('p');
    p.appendChild(addResultTableToOutput(providerResults.results));
    output.appendChild(p);
  }
}

/**
 * @param {Object} result an array of AutocompleteMatchMojos.
 * @return {HTMLTableCellElement} that is a user-readable HTML
 *     representation of this object.
 */
function addResultTableToOutput(result) {
  var inDetailedMode = $('show-details').checked;
  // Create a table to hold all the autocomplete items.
  var table = document.createElement('table');
  table.className = 'autocomplete-results-table';
  table.appendChild(createAutocompleteResultTableHeader());
  // Loop over every autocomplete item and add it as a row in the table.
  for (var i = 0; i < result.length; i++) {
    var autocompleteSuggestion = result[i];
    var row = document.createElement('tr');
    // Loop over all the columns/properties and output either them
    // all (if we're in detailed mode) or only the ones marked displayAlways.
    // Keep track of which properties we displayed.
    var displayedProperties = {};
    for (var j = 0; j < PROPERTY_OUTPUT_ORDER.length; j++) {
      if (inDetailedMode || PROPERTY_OUTPUT_ORDER[j].displayAlways) {
        row.appendChild(createCellForPropertyAndRemoveProperty(
            autocompleteSuggestion, PROPERTY_OUTPUT_ORDER[j].propertyName));
        displayedProperties[PROPERTY_OUTPUT_ORDER[j].propertyName] = true;
      }
    }

    // Now, if we're in detailed mode, add all the properties that
    // haven't already been output.  (We know which properties have
    // already been output because we delete the property when we output
    // it.  The only way we have properties left at this point if
    // we're in detailed mode and we're getting back properties
    // not listed in PROPERTY_OUTPUT_ORDER.  Perhaps someone added
    // something to the C++ code but didn't bother to update this
    // Javascript?  In any case, we want to display them.)
    if (inDetailedMode) {
      for (var key in autocompleteSuggestion) {
        if (!displayedProperties[key] &&
            typeof autocompleteSuggestion[key] != 'function') {
          var cell = document.createElement('td');
          cell.textContent = key + '=' + autocompleteSuggestion[key];
          row.appendChild(cell);
        }
      }
    }

    table.appendChild(row);
  }
  return table;
}

/* Repaints the page based on the contents of the array
 * progressiveAutocompleteResults, which represents consecutive
 * autocomplete results.  We only display the last (most recent)
 * entry unless we're asked to display incomplete results.  For an
 * example of the output, play with chrome://omnibox/
 */
function refresh() {
  // Erase whatever is currently being displayed.
  var output = $('omnibox-debug-text');
  output.innerHTML = '';

  if (progressiveAutocompleteResults.length > 0) {  // if we have results
    // Display the results.
    var showIncompleteResults = $('show-incomplete-results').checked;
    var startIndex =
        showIncompleteResults ? 0 : progressiveAutocompleteResults.length - 1;
    for (var i = startIndex; i < progressiveAutocompleteResults.length; i++) {
      addResultToOutput(progressiveAutocompleteResults[i]);
    }
  }
}

// NOTE: Need to keep a global reference to the |pageImpl| such that it is not
// garbage collected, which causes the pipe to close and future calls from C++
// to JS to get dropped.
var pageImpl = null;
var browserProxy = null;

function initializeProxies() {
  browserProxy = new mojom.OmniboxPageHandlerPtr;
  Mojo.bindInterface(
      mojom.OmniboxPageHandler.name, mojo.makeRequest(browserProxy).handle);

  /** @constructor */
  var OmniboxPageImpl = function(request) {
    this.binding_ = new mojo.Binding(mojom.OmniboxPage, this, request);
  };

  OmniboxPageImpl.prototype = {
    /** @override */
    handleNewAutocompleteResult: function(result) {
      progressiveAutocompleteResults.push(result);
      refresh();
    },
  };

  var client = new mojom.OmniboxPagePtr;
  pageImpl = new OmniboxPageImpl(mojo.makeRequest(client));
  browserProxy.setClientPage(client);
}

document.addEventListener('DOMContentLoaded', function() {
  initializeProxies();
  initialize();
});
})();
