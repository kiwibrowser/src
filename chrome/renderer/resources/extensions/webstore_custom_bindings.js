// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the webstore API.

var webstoreNatives = requireNative('webstore');

var onInstallStageChanged;
var onDownloadProgress;

function Installer() {
  this._pendingInstall = null;
}

Installer.prototype.install = function(url, onSuccess, onFailure) {
  if (this._pendingInstall)
    throw new Error('A Chrome Web Store installation is already pending.');

  // With native bindings, these calls go through argument validation, which
  // sets optional/missing arguments to null. The native webstore bindings
  // expect either present or undefined, so transform null to undefined.
  if (url === null)
    url = undefined;
  if (onSuccess === null)
    onSuccess = undefined;
  if (onFailure === null)
    onFailure = undefined;

  if (url !== undefined && typeof url !== 'string') {
    throw new Error(
        'The Chrome Web Store item link URL parameter must be a string.');
  }
  if (onSuccess !== undefined && typeof onSuccess !== 'function') {
    throw new Error('The success callback parameter must be a function.');
  }
  if (onFailure !== undefined && typeof onFailure !== 'function') {
    throw new Error('The failure callback parameter must be a function.');
  }

  // Since we call Install() with a bool for if we have listeners, listeners
  // must be set prior to the inline installation starting (this is also
  // noted in the Event documentation in
  // chrome/common/extensions/api/webstore.json).
  var installId = webstoreNatives.Install(
      onInstallStageChanged.hasListeners(),
      onDownloadProgress.hasListeners(),
      url,
      onSuccess,
      onFailure);
  if (installId !== undefined) {
    this._pendingInstall = {
      installId: installId,
      onSuccess: onSuccess,
      onFailure: onFailure
    };
  }
};

Installer.prototype.onInstallResponse =
    function(installId, success, error, resultCode) {
  var pendingInstall = this._pendingInstall;
  if (!pendingInstall || pendingInstall.installId != installId) {
    // TODO(kalman): should this be an error?
    return;
  }

  try {
    if (success && pendingInstall.onSuccess)
      pendingInstall.onSuccess();
    else if (!success && pendingInstall.onFailure)
      pendingInstall.onFailure(error, resultCode);
  } catch (e) {
    console.error('Exception in chrome.webstore.install response handler: ' +
                  e.stack);
  } finally {
    this._pendingInstall = null;
  }
};

Installer.prototype.onInstallStageChanged = function(installStage) {
  onInstallStageChanged.dispatch(installStage);
};

Installer.prototype.onDownloadProgress = function(progress) {
  onDownloadProgress.dispatch(progress);
};

var installer = new Installer();


if (apiBridge) {
  apiBridge.registerCustomHook(function(api) {
    api.apiFunctions.setHandleRequest('install',
                                      function(url, onSuccess, onFailure) {
      installer.install(url, onSuccess, onFailure);
    });

    onInstallStageChanged = api.compiledApi.onInstallStageChanged;
    onDownloadProgress = api.compiledApi.onDownloadProgress;
  });
} else {
  var Event = require('event_bindings').Event;
  onInstallStageChanged =
      new Event(null, [{name: 'stage', type: 'string'}], {unmanaged: true});
  onDownloadProgress =
      new Event(null, [{name: 'progress', type: 'number'}], {unmanaged: true});

  var chromeWebstore = {
    install: function (url, onSuccess, onFailure) {
      installer.install(url, onSuccess, onFailure);
    },
    onInstallStageChanged: onInstallStageChanged,
    onDownloadProgress: onDownloadProgress,
  };
  exports.$set('binding', chromeWebstore);
}

// Called by webstore_bindings.cc.
exports.$set('onInstallResponse',
             $Function.bind(Installer.prototype.onInstallResponse, installer));
exports.$set('onInstallStageChanged',
             $Function.bind(Installer.prototype.onInstallStageChanged,
                            installer));
exports.$set('onDownloadProgress',
             $Function.bind(Installer.prototype.onDownloadProgress, installer));
