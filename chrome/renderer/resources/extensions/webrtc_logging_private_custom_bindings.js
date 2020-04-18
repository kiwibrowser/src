// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the webrtcLoggingPrivate API.

var binding = apiBridge ||
              require('binding').Binding.create('webrtcLoggingPrivate');

var getBindDirectoryEntryCallback =
    require('fileEntryBindingUtil').getBindDirectoryEntryCallback;

binding.registerCustomHook(function(binding, id, contextType) {
  var apiFunctions = binding.apiFunctions;
  apiFunctions.setCustomCallback('getLogsDirectory',
                                 getBindDirectoryEntryCallback());
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
