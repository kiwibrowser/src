// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @suppress {duplicate} */
var remoting = remoting || {};

(function(){

'use strict';

/**
 * @constructor
 */
remoting.OptionsExporter = function() {
  base.Ipc.getInstance().register('getSettings',
                                  remoting.OptionsExporter.migrateSettings_,
                                  true);
};

remoting.OptionsExporter.migrateSettings_ = function() {
  var result = new base.Deferred();
  chrome.storage.local.get(OPTIONS_KEY_NAME, function(options) {
    // If there are no host options stored, reformat the message response so
    // that the sender doesn't interpret it as an error.
    if (Object.keys(options).length == 0) {
      options[OPTIONS_KEY_NAME] = '{}';
    }
    var nativeMessagingHost = new remoting.HostDaemonFacade();
    nativeMessagingHost.getDaemonState().then(
        function(state) {
          var reverse = new Map([
            [remoting.HostController.State.NOT_IMPLEMENTED, 'NOT_IMPLEMENTED'],
            [remoting.HostController.State.NOT_INSTALLED, 'NOT_INSTALLED'],
            [remoting.HostController.State.STOPPED, 'STOPPED'],
            [remoting.HostController.State.STARTING, 'STARTING'],
            [remoting.HostController.State.STARTED, 'STARTED'],
            [remoting.HostController.State.STOPPING, 'STOPPING'],
          ]);
          options[LOCAL_HOST_STATE_KEY_NAME] = reverse.get(state) || 'UNKNOWN';
          result.resolve(options);
        },
        function(error) {
          options[LOCAL_HOST_STATE_KEY_NAME] = 'UNKNOWN';
          result.resolve(options);
        });
  })
  return result.promise();
};

var OPTIONS_KEY_NAME = 'remoting-host-options';
var LOCAL_HOST_STATE_KEY_NAME = 'local-host-state';

}());
