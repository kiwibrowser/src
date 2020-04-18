// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  /**
   * @implements {extensions.ErrorPageDelegate}
   * @implements {extensions.ItemDelegate}
   * @implements {extensions.KeyboardShortcutDelegate}
   * @implements {extensions.LoadErrorDelegate}
   * @implements {extensions.PackDialogDelegate}
   * @implements {extensions.ToolbarDelegate}
   */
  class Service {
    constructor() {
      /** @private {boolean} */
      this.isDeleting_ = false;

      /** @private {!Set<string>} */
      this.eventsToIgnoreOnce_ = new Set();
    }

    getProfileConfiguration() {
      return new Promise(function(resolve, reject) {
        chrome.developerPrivate.getProfileConfiguration(resolve);
      });
    }

    getItemStateChangedTarget() {
      return chrome.developerPrivate.onItemStateChanged;
    }

    /**
     * @param {string} extensionId
     * @param {!chrome.developerPrivate.EventType} eventType
     * @return {boolean}
     */
    shouldIgnoreUpdate(extensionId, eventType) {
      return this.eventsToIgnoreOnce_.delete(`${extensionId}_${eventType}`);
    }

    /**
     * @param {string} extensionId
     * @param {!chrome.developerPrivate.EventType} eventType
     */
    ignoreNextEvent(extensionId, eventType) {
      this.eventsToIgnoreOnce_.add(`${extensionId}_${eventType}`);
    }

    getProfileStateChangedTarget() {
      return chrome.developerPrivate.onProfileStateChanged;
    }

    getExtensionsInfo() {
      return new Promise(function(resolve, reject) {
        chrome.developerPrivate.getExtensionsInfo(
            {includeDisabled: true, includeTerminated: true}, resolve);
      });
    }

    /** @override */
    getExtensionSize(id) {
      return new Promise(function(resolve, reject) {
        chrome.developerPrivate.getExtensionSize(id, resolve);
      });
    }

    /**
     * Opens a file browser dialog for the user to select a file (or directory).
     * @param {chrome.developerPrivate.SelectType} selectType
     * @param {chrome.developerPrivate.FileType} fileType
     * @return {Promise<string>} The promise to be resolved with the selected
     *     path.
     */
    chooseFilePath_(selectType, fileType) {
      return new Promise(function(resolve, reject) {
        chrome.developerPrivate.choosePath(
            selectType, fileType, function(path) {
              if (chrome.runtime.lastError &&
                  chrome.runtime.lastError != 'File selection was canceled.') {
                reject(chrome.runtime.lastError);
              } else {
                resolve(path || '');
              }
            });
      });
    }

    /** @override */
    updateExtensionCommandKeybinding(extensionId, commandName, keybinding) {
      chrome.developerPrivate.updateExtensionCommand({
        extensionId: extensionId,
        commandName: commandName,
        keybinding: keybinding,
      });
    }

    /** @override */
    updateExtensionCommandScope(extensionId, commandName, scope) {
      // The COMMAND_REMOVED event needs to be ignored since it is sent before
      // the command is added back with the updated scope but can be handled
      // after the COMMAND_ADDED event.
      this.ignoreNextEvent(
          extensionId, chrome.developerPrivate.EventType.COMMAND_REMOVED);
      chrome.developerPrivate.updateExtensionCommand({
        extensionId: extensionId,
        commandName: commandName,
        scope: scope,
      });
    }


    /** @override */
    setShortcutHandlingSuspended(isCapturing) {
      chrome.developerPrivate.setShortcutHandlingSuspended(isCapturing);
    }

    /**
     * Attempts to load an unpacked extension, optionally as another attempt at
     * a previously-specified load.
     * @param {string=} opt_retryGuid
     * @return {!Promise} A signal that loading finished, rejected if any error
     *     occurred.
     * @private
     */
    loadUnpackedHelper_(opt_retryGuid) {
      return new Promise(function(resolve, reject) {
        chrome.developerPrivate.loadUnpacked(
            {failQuietly: true, populateError: true, retryGuid: opt_retryGuid},
            (loadError) => {
              if (chrome.runtime.lastError &&
                  chrome.runtime.lastError.message !=
                      'File selection was canceled.') {
                throw new Error(chrome.runtime.lastError.message);
              }
              if (loadError)
                return reject(loadError);

              resolve();
            });
      });
    }

    /** @override */
    deleteItem(id) {
      if (this.isDeleting_)
        return;
      this.isDeleting_ = true;
      chrome.management.uninstall(id, {showConfirmDialog: true}, () => {
        // The "last error" was almost certainly the user canceling the dialog.
        // Do nothing. We only check it so we don't get noisy logs.
        /** @suppress {suspiciousCode} */
        chrome.runtime.lastError;
        this.isDeleting_ = false;
      });
    }

    /** @override */
    setItemEnabled(id, isEnabled) {
      chrome.management.setEnabled(id, isEnabled);
    }

    /** @override */
    setItemAllowedIncognito(id, isAllowedIncognito) {
      chrome.developerPrivate.updateExtensionConfiguration({
        extensionId: id,
        incognitoAccess: isAllowedIncognito,
      });
    }

    /** @override */
    setItemAllowedOnFileUrls(id, isAllowedOnFileUrls) {
      chrome.developerPrivate.updateExtensionConfiguration({
        extensionId: id,
        fileAccess: isAllowedOnFileUrls,
      });
    }

    /** @override */
    setItemAllowedOnAllSites(id, isAllowedOnAllSites) {
      chrome.developerPrivate.updateExtensionConfiguration({
        extensionId: id,
        runOnAllUrls: isAllowedOnAllSites,
      });
    }

    /** @override */
    setItemCollectsErrors(id, collectsErrors) {
      chrome.developerPrivate.updateExtensionConfiguration({
        extensionId: id,
        errorCollection: collectsErrors,
      });
    }

    /** @override */
    inspectItemView(id, view) {
      chrome.developerPrivate.openDevTools({
        extensionId: id,
        renderProcessId: view.renderProcessId,
        renderViewId: view.renderViewId,
        incognito: view.incognito,
      });
    }

    /**
     * @param {string} url
     * @override
     */
    openUrl(url) {
      window.open(url);
    }

    /** @override */
    reloadItem(id) {
      return new Promise(function(resolve, reject) {
        chrome.developerPrivate.reload(
            id, {failQuietly: true, populateErrorForUnpacked: true},
            (loadError) => {
              if (loadError) {
                reject(loadError);
                return;
              }

              resolve();
            });
      });
    }

    /** @override */
    repairItem(id) {
      chrome.developerPrivate.repairExtension(id);
    }

    /** @override */
    showItemOptionsPage(extension) {
      assert(extension && extension.optionsPage);
      if (extension.optionsPage.openInTab) {
        chrome.developerPrivate.showOptions(extension.id);
      } else {
        extensions.navigation.navigateTo({
          page: Page.DETAILS,
          subpage: Dialog.OPTIONS,
          extensionId: extension.id,
        });
      }
    }

    /** @override */
    setProfileInDevMode(inDevMode) {
      chrome.developerPrivate.updateProfileConfiguration(
          {inDeveloperMode: inDevMode});
    }

    /** @override */
    loadUnpacked() {
      return this.loadUnpackedHelper_();
    }

    /** @override */
    retryLoadUnpacked(retryGuid) {
      return this.loadUnpackedHelper_(retryGuid);
    }

    /** @override */
    choosePackRootDirectory() {
      return this.chooseFilePath_(
          chrome.developerPrivate.SelectType.FOLDER,
          chrome.developerPrivate.FileType.LOAD);
    }

    /** @override */
    choosePrivateKeyPath() {
      return this.chooseFilePath_(
          chrome.developerPrivate.SelectType.FILE,
          chrome.developerPrivate.FileType.PEM);
    }

    /** @override */
    packExtension(rootPath, keyPath, flag, callback) {
      chrome.developerPrivate.packDirectory(rootPath, keyPath, flag, callback);
    }

    /** @override */
    updateAllExtensions() {
      return new Promise((resolve) => {
        chrome.developerPrivate.autoUpdate(resolve);
        chrome.metricsPrivate.recordUserAction('Options_UpdateExtensions');
      });
    }

    /** @override */
    deleteErrors(extensionId, errorIds, type) {
      chrome.developerPrivate.deleteExtensionErrors({
        extensionId: extensionId,
        errorIds: errorIds,
        type: type,
      });
    }

    /** @override */
    requestFileSource(args) {
      return new Promise(function(resolve, reject) {
        chrome.developerPrivate.requestFileSource(args, function(code) {
          resolve(code);
        });
      });
    }

    /** @override */
    showInFolder(id) {
      chrome.developerPrivate.showPath(id);
    }
  }

  cr.addSingletonGetter(Service);

  return {Service: Service};
});
