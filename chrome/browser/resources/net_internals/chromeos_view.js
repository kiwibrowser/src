// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view displays information on ChromeOS specific features.
 */
var CrosView = (function() {
  'use strict';

  var fileContent;
  var passcode = '';

  /**
   *  Clear file input div
   *
   *  @private
   */
  function clearFileInput_() {
    $(CrosView.IMPORT_DIV_ID).innerHTML = $(CrosView.IMPORT_DIV_ID).innerHTML;
    $(CrosView.IMPORT_ONC_ID)
        .addEventListener('change', handleFileChangeEvent_, false);
  }

  /**
   *  Send file contents and passcode to C++ cros network library.
   *
   *  @private
   */
  function importONCFile_() {
    clearParseStatus_();
    if (fileContent)
      g_browser.importONCFile(fileContent, passcode);
    else
      setParseStatus_('ONC file parse failed: cannot read file');
    clearFileInput_();
  }

  /**
   *  Set the passcode var, and trigger onc import.
   *
   *  @param {string} value The passcode value.
   *  @private
   */
  function setPasscode_(value) {
    passcode = value;
    if (passcode)
      importONCFile_();
  }

  /**
   *  Unhide the passcode prompt input field and give it focus.
   *
   *  @private
   */
  function promptForPasscode_() {
    $(CrosView.PASSCODE_ID).hidden = false;
    $(CrosView.PASSCODE_INPUT_ID).focus();
    $(CrosView.PASSCODE_INPUT_ID).select();
  }

  /**
   *  Set the fileContent var, and trigger onc import if the file appears to
   *  not be encrypted, or prompt for passcode if the file is encrypted.
   *
   *  @private
   *  @param {string} text contents of selected file.
   */
  function setFileContent_(result) {
    fileContent = result;
    // Parse the JSON to get at the top level "Type" property.
    var jsonObject;
    // Ignore any parse errors: they'll get handled in the C++ import code.
    try {
      jsonObject = JSON.parse(fileContent);
    } catch (error) {
    }
    // Check if file is encrypted.
    if (jsonObject && jsonObject.hasOwnProperty('Type') &&
        jsonObject.Type == 'EncryptedConfiguration') {
      promptForPasscode_();
    } else {
      importONCFile_();
    }
  }

  /**
   *  Clear ONC file parse status.  Clears and hides the parse status div.
   *
   *  @private
   */
  function clearParseStatus_(error) {
    var parseStatus = $(CrosView.PARSE_STATUS_ID);
    parseStatus.hidden = true;
    parseStatus.textContent = '';
  }

  /**
   *  Set ONC file parse status.
   *
   *  @private
   */
  function setParseStatus_(error) {
    var parseStatus = $(CrosView.PARSE_STATUS_ID);
    parseStatus.hidden = false;
    parseStatus.textContent = error ? 'ONC file parse failed: ' + error :
                                      'ONC file successfully parsed';
    reset_();
  }

  /**
   *  Set storing debug logs status.
   *
   *  @private
   */
  function setStoreDebugLogsStatus_(status) {
    $(CrosView.STORE_DEBUG_LOGS_STATUS_ID).innerText = status;
  }

  /**
   *  Set status for current debug mode.
   *
   *  @private
   */
  function setNetworkDebugModeStatus_(status) {
    $(CrosView.DEBUG_STATUS_ID).innerText = status;
  }

  /**
   *  An event listener for the file selection field.
   *
   *  @private
   */
  function handleFileChangeEvent_(event) {
    clearParseStatus_();
    var file = event.target.files[0];
    var reader = new FileReader();
    reader.onloadend = function(e) {
      setFileContent_(reader.result);
    };
    reader.readAsText(file);
  }

  /**
   *  Add event listeners for the file selection, passcode input
   *  fields, for the button for debug logs storing and for buttons
   *  for debug mode selection.
   *
   *  @private
   */
  function addEventListeners_() {
    $(CrosView.IMPORT_ONC_ID)
        .addEventListener('change', handleFileChangeEvent_, false);

    $(CrosView.PASSCODE_INPUT_ID).addEventListener('change', function(event) {
      setPasscode_(this.value);
    }, false);

    $(CrosView.STORE_DEBUG_LOGS_ID).addEventListener('click', function(event) {
      $(CrosView.STORE_DEBUG_LOGS_STATUS_ID).innerText = '';
      g_browser.storeDebugLogs();
    }, false);

    $(CrosView.DEBUG_WIFI_ID).addEventListener('click', function(event) {
      setNetworkDebugMode_('wifi');
    }, false);
    $(CrosView.DEBUG_ETHERNET_ID).addEventListener('click', function(event) {
      setNetworkDebugMode_('ethernet');
    }, false);
    $(CrosView.DEBUG_CELLULAR_ID).addEventListener('click', function(event) {
      setNetworkDebugMode_('cellular');
    }, false);
    $(CrosView.DEBUG_WIMAX_ID).addEventListener('click', function(event) {
      setNetworkDebugMode_('wimax');
    }, false);
    $(CrosView.DEBUG_NONE_ID).addEventListener('click', function(event) {
      setNetworkDebugMode_('none');
    }, false);
  }

  /**
   *  Reset fileContent and passcode vars.
   *
   *  @private
   */
  function reset_() {
    fileContent = undefined;
    passcode = '';
    $(CrosView.PASSCODE_ID).hidden = true;
  }

  /**
   *  Enables or disables debug mode for a specified subsystem.
   *
   *  @private
   */
  function setNetworkDebugMode_(subsystem) {
    $(CrosView.DEBUG_STATUS_ID).innerText = '';
    g_browser.setNetworkDebugMode(subsystem);
  }

  /**
   *  @constructor
   *  @extends {DivView}
   */
  function CrosView() {
    assertFirstConstructorCall(CrosView);

    // Call superclass's constructor.
    DivView.call(this, CrosView.MAIN_BOX_ID);

    g_browser.addCrosONCFileParseObserver(this);
    g_browser.addStoreDebugLogsObserver(this);
    g_browser.addSetNetworkDebugModeObserver(this);
    addEventListeners_();
  }

  CrosView.TAB_ID = 'tab-handle-chromeos';
  CrosView.TAB_NAME = 'ChromeOS';
  CrosView.TAB_HASH = '#chromeos';

  CrosView.MAIN_BOX_ID = 'chromeos-view-tab-content';
  CrosView.IMPORT_DIV_ID = 'chromeos-view-import-div';
  CrosView.IMPORT_ONC_ID = 'chromeos-view-import-onc';
  CrosView.PASSCODE_ID = 'chromeos-view-password-div';
  CrosView.PASSCODE_INPUT_ID = 'chromeos-view-onc-password';
  CrosView.PARSE_STATUS_ID = 'chromeos-view-parse-status';
  CrosView.STORE_DEBUG_LOGS_ID = 'chromeos-view-store-debug-logs';
  CrosView.STORE_DEBUG_LOGS_STATUS_ID = 'chromeos-view-store-debug-logs-status';
  CrosView.DEBUG_WIFI_ID = 'chromeos-view-network-debugging-wifi';
  CrosView.DEBUG_ETHERNET_ID = 'chromeos-view-network-debugging-ethernet';
  CrosView.DEBUG_CELLULAR_ID = 'chromeos-view-network-debugging-cellular';
  CrosView.DEBUG_WIMAX_ID = 'chromeos-view-network-debugging-wimax';
  CrosView.DEBUG_NONE_ID = 'chromeos-view-network-debugging-none';
  CrosView.DEBUG_STATUS_ID = 'chromeos-view-network-debugging-status';

  cr.addSingletonGetter(CrosView);

  CrosView.prototype = {
    // Inherit from DivView.
    __proto__: DivView.prototype,

    onONCFileParse: setParseStatus_,
    onStoreDebugLogs: setStoreDebugLogsStatus_,
    onSetNetworkDebugMode: setNetworkDebugModeStatus_,
  };

  return CrosView;
})();
