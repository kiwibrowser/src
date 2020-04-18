// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view displays options for importing data from a log file.
 */
var ImportView = (function() {
  'use strict';

  // This is defined in index.html, but for all intents and purposes is part
  // of this view.
  var LOAD_LOG_FILE_DROP_TARGET_ID = 'import-view-drop-target';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function ImportView() {
    assertFirstConstructorCall(ImportView);

    // Call superclass's constructor.
    superClass.call(this, ImportView.MAIN_BOX_ID);

    this.loadedDiv_ = $(ImportView.LOADED_DIV_ID);

    this.loadFileElement_ = $(ImportView.LOAD_LOG_FILE_ID);
    this.loadFileElement_.onchange = this.logFileChanged.bind(this);
    this.loadStatusText_ = $(ImportView.LOAD_STATUS_TEXT_ID);

    var dropTarget = $(LOAD_LOG_FILE_DROP_TARGET_ID);
    dropTarget.ondragenter = this.onDrag.bind(this);
    dropTarget.ondragover = this.onDrag.bind(this);
    dropTarget.ondrop = this.onDrop.bind(this);
  }

  ImportView.TAB_ID = 'tab-handle-import';
  ImportView.TAB_NAME = 'Import';
  ImportView.TAB_HASH = '#import';

  // IDs for special HTML elements in import_view.html.
  ImportView.MAIN_BOX_ID = 'import-view-tab-content';
  ImportView.LOADED_DIV_ID = 'import-view-loaded-div';
  ImportView.LOAD_LOG_FILE_ID = 'import-view-load-log-file';
  ImportView.LOAD_STATUS_TEXT_ID = 'import-view-load-status-text';
  // Used in tests.
  ImportView.LOADED_INFO_USER_COMMENTS_ID = 'import-view-user-comments';

  cr.addSingletonGetter(ImportView);

  ImportView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    /**
     * Called when a log file is loaded, after clearing the old log entries and
     * loading the new ones.  Returns true to indicate the view should
     * still be visible.
     */
    onLoadLogFinish: function(polledData, unused, logDump) {
      var input = new JsEvalContext(logDump);
      jstProcess(input, $(ImportView.LOADED_DIV_ID));

      setNodeDisplay(this.loadedDiv_, true);
      return true;
    },

    /**
     * Called when something is dragged over the drop target.
     *
     * Returns false to cancel default browser behavior when a single file is
     * being dragged.  When this happens, we may not receive a list of files for
     * security reasons, which is why we allow the |files| array to be empty.
     */
    onDrag: function(event) {
      // NOTE: Use Array.prototype.indexOf here is necessary while WebKit
      // decides which type of data structure dataTransfer.types will be
      // (currently between DOMStringList and Array). These have different APIs
      // so assuming one type or the other was breaking things. See
      // http://crbug.com/115433. TODO(dbeam): Remove when standardized more.
      var indexOf = Array.prototype.indexOf;
      return indexOf.call(event.dataTransfer.types, 'Files') == -1 ||
          event.dataTransfer.files.length > 1;
    },

    /**
     * Called when something is dropped onto the drop target.  If it's a single
     * file, tries to load it as a log file.
     */
    onDrop: function(event) {
      var indexOf = Array.prototype.indexOf;
      if (indexOf.call(event.dataTransfer.types, 'Files') == -1 ||
          event.dataTransfer.files.length != 1) {
        return;
      }
      event.preventDefault();

      // Loading a log file may hide the currently active tab.  Switch to the
      // import tab to prevent this.
      document.location.hash = 'import';

      this.loadLogFile(event.dataTransfer.files[0]);
    },

    /**
     * Called when a log file is selected.
     *
     * Gets the log file from the input element and tries to read from it.
     */
    logFileChanged: function() {
      this.loadLogFile(this.loadFileElement_.files[0]);
    },

    /**
     * Attempts to read from the File |logFile|.
     */
    loadLogFile: function(logFile) {
      if (logFile) {
        this.setLoadFileStatus('Loading log...', true);
        var fileReader = new FileReader();

        fileReader.onload = this.onLoadLogFile.bind(this, logFile);
        fileReader.onerror = this.onLoadLogFileError.bind(this);

        fileReader.readAsText(logFile);
      }
    },

    /**
     * Displays an error message when unable to read the selected log file.
     * Also clears the file input control, so the same file can be reloaded.
     */
    onLoadLogFileError: function(event) {
      this.loadFileElement_.value = null;
      this.setLoadFileStatus(
          'Error ' + getKeyWithValue(FileError, event.target.error.code) +
              '.  Unable to read file.',
          false);
    },

    onLoadLogFile: function(logFile, event) {
      var result = log_util.loadLogFile(event.target.result, logFile.name);
      this.setLoadFileStatus(result, false);
    },

    /**
     * Sets the load from file status text, displayed below the load file
     * button, to |text|.  Also enables or disables the load buttons based on
     * the value of |isLoading|, which must be true if the load process is still
     * ongoing, and false when the operation has stopped, regardless of success
     * of failure.  Also, when loading is done, replaces the load button so the
     * same file can be loaded again.
     */
    setLoadFileStatus: function(text, isLoading) {
      this.enableLoadFileElement_(!isLoading);
      this.loadStatusText_.textContent = text;

      if (!isLoading) {
        // Clear the button, so the same file can be reloaded.  Recreating the
        // element seems to be the only way to do this.
        var loadFileElementId = this.loadFileElement_.id;
        var loadFileElementOnChange = this.loadFileElement_.onchange;
        this.loadFileElement_.outerHTML = this.loadFileElement_.outerHTML;
        this.loadFileElement_ = $(loadFileElementId);
        this.loadFileElement_.onchange = loadFileElementOnChange;
      }

      // Style the log output differently depending on what just happened.
      var pos = text.indexOf('Log loaded.');
      if (isLoading) {
        this.loadStatusText_.className = 'import-view-pending-log';
      } else if (pos == 0) {
        this.loadStatusText_.className = 'import-view-success-log';
      } else if (pos != -1) {
        this.loadStatusText_.className = 'import-view-warning-log';
      } else {
        this.loadStatusText_.className = 'import-view-error-log';
      }
    },

    enableLoadFileElement_: function(enabled) {
      this.loadFileElement_.disabled = !enabled;
    },
  };

  return ImportView;
})();
