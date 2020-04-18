// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Contents of lines that act as delimiters for multi-line values.
var DELIM_START = '---------- START ----------';
var DELIM_END = '---------- END ----------';

// Limit file size to 10 MiB to prevent hanging on accidental upload.
var MAX_FILE_SIZE = 10485760;

function getValueDivForButton(button) {
  return $(button.id.substr(0, button.id.length - 4));
}

function getButtonForValueDiv(valueDiv) {
  return $(valueDiv.id + '-btn');
}

function handleDragOver(e) {
  e.dataTransfer.dropEffect = 'copy';
  e.preventDefault();
}

function handleDrop(e) {
  var file = e.dataTransfer.files[0];
  if (file) {
    e.preventDefault();
    importLog(file);
  }
}

function showError(fileName) {
  $('status').textContent = loadTimeData.getStringF('parseError', fileName);
}

/**
 * Toggles whether an item is collapsed or expanded.
 */
function changeCollapsedStatus() {
  var valueDiv = getValueDivForButton(this);
  if (valueDiv.parentNode.className == 'number-collapsed') {
    valueDiv.parentNode.className = 'number-expanded';
    this.textContent = loadTimeData.getString('collapseBtn');
  } else {
    valueDiv.parentNode.className = 'number-collapsed';
    this.textContent = loadTimeData.getString('expandBtn');
  }
}

/**
 * Collapses all log items.
 */
function collapseAll() {
  var valueDivs = document.getElementsByClassName('stat-value');
  for (var i = 0; i < valueDivs.length; i++) {
    var button = getButtonForValueDiv(valueDivs[i]);
    if (button && button.className != 'button-hidden') {
      button.textContent = loadTimeData.getString('expandBtn');
      valueDivs[i].parentNode.className = 'number-collapsed';
    }
  }
}

/**
 * Expands all log items.
 */
function expandAll() {
  var valueDivs = document.getElementsByClassName('stat-value');
  for (var i = 0; i < valueDivs.length; i++) {
    var button = getButtonForValueDiv(valueDivs[i]);
    if (button && button.className != 'button-hidden') {
      button.textContent = loadTimeData.getString('collapseBtn');
      valueDivs[i].parentNode.className = 'number-expanded';
    }
  }
}

/**
 * Collapse only those log items with multi-line values.
 */
function collapseMultiLineStrings() {
  var valueDivs = document.getElementsByClassName('stat-value');
  var nameDivs = document.getElementsByClassName('stat-name');
  for (var i = 0; i < valueDivs.length; i++) {
    var button = getButtonForValueDiv(valueDivs[i]);
    button.onclick = changeCollapsedStatus;
    if (valueDivs[i].scrollHeight > (nameDivs[i].scrollHeight * 2)) {
      button.className = '';
      button.textContent = loadTimeData.getString('expandBtn');
      valueDivs[i].parentNode.className = 'number-collapsed';
    } else {
      button.className = 'button-hidden';
      valueDivs[i].parentNode.className = 'number';
    }
  }
}

/**
 * Read in a log asynchronously, calling parseSystemLog if successful.
 * @param {File} file The file to read.
 */
function importLog(file) {
  if (file && file.size <= MAX_FILE_SIZE) {
    var reader = new FileReader();
    reader.onload = function() {
      if (parseSystemLog(this.result)) {
        // Reset table title and status
        $('tableTitle').textContent =
            loadTimeData.getStringF('logFileTableTitle', file.name);
        $('status').textContent = '';
      } else {
        showError(file.name);
      }
    };
    reader.readAsText(file);
  } else if (file) {
    showError(file.name);
  }
}

/**
 * Convert text-based log into list of name-value pairs.
 * @param {string} text The raw text of a log.
 * @return {boolean} True if the log was parsed successfully.
 */
function parseSystemLog(text) {
  var details = [];
  var lines = text.split('\n');
  for (var i = 0, len = lines.length; i < len; i++) {
    // Skip empty lines.
    if (!lines[i])
      continue;

    var delimiter = lines[i].indexOf('=');
    if (delimiter <= 0) {
      if (i == lines.length - 1)
        break;
      // If '=' is missing here, format is wrong.
      return false;
    }

    var name = lines[i].substring(0, delimiter);
    var value = '';
    // Set value if non-empty
    if (lines[i].length > delimiter + 1)
      value = lines[i].substring(delimiter + 1);

    // Delimiters are based on kMultilineIndicatorString, kMultilineStartString,
    // and kMultilineEndString in components/feedback/feedback_data.cc.
    // If these change, we should check for both the old and new versions.
    if (value == '<multiline>') {
      // Skip start delimiter.
      if (i == len - 1 || lines[++i].indexOf(DELIM_START) == -1)
        return false;

      ++i;
      value = '';
      // Append lines between start and end delimiters.
      while (i < len && lines[i] != DELIM_END)
        value += lines[i++] + '\n';

      // Remove trailing newline.
      if (value)
        value = value.substr(0, value.length - 1);
    }
    details.push({'statName': name, 'statValue': value});
  }

  var templateData = {'details': details};
  jstProcess(new JsEvalContext(templateData), $('t'));

  collapseMultiLineStrings();
  return true;
}

document.addEventListener('DOMContentLoaded', function() {
  jstProcess(loadTimeData.createJsEvalContext(), $('t'));

  $('collapseAll').onclick = collapseAll;
  $('expandAll').onclick = expandAll;

  var tp = $('t');
  tp.addEventListener('dragover', handleDragOver, false);
  tp.addEventListener('drop', handleDrop, false);

  collapseMultiLineStrings();
});
