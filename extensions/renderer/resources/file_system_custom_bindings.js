// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the fileSystem API.

var binding = apiBridge || require('binding').Binding.create('fileSystem');
var sendRequest = bindingUtil ?
    $Function.bind(bindingUtil.sendRequest, bindingUtil) :
    require('sendRequest').sendRequest;
var getFileBindingsForApi =
    require('fileEntryBindingUtil').getFileBindingsForApi;
var fileBindings = getFileBindingsForApi('fileSystem');
var bindFileEntryCallback = fileBindings.bindFileEntryCallback;
var entryIdManager = fileBindings.entryIdManager;
var fileSystemNatives = requireNative('file_system_natives');
var safeCallbackApply = require('uncaught_exception_handler').safeCallbackApply;

binding.registerCustomHook(function(bindingsAPI) {
  var apiFunctions = bindingsAPI.apiFunctions;
  var fileSystem = bindingsAPI.compiledApi;

  function bindFileEntryFunction(functionName) {
    apiFunctions.setUpdateArgumentsPostValidate(
        functionName, function(fileEntry, callback) {
      var fileSystemName = fileEntry.filesystem.name;
      var relativePath = $String.slice(fileEntry.fullPath, 1);
      return [fileSystemName, relativePath, callback];
    });
  }
  $Array.forEach(['getDisplayPath', 'getWritableEntry', 'isWritableEntry'],
                  bindFileEntryFunction);

  $Array.forEach(['getWritableEntry', 'chooseEntry', 'restoreEntry'],
                  function(functionName) {
    bindFileEntryCallback(functionName, apiFunctions);
  });

  apiFunctions.setHandleRequest('retainEntry', function(fileEntry) {
    var id = entryIdManager.getEntryId(fileEntry);
    if (!id)
      return '';
    var fileSystemName = fileEntry.filesystem.name;
    var relativePath = $String.slice(fileEntry.fullPath, 1);

    sendRequest('fileSystem.retainEntry', [id, fileSystemName, relativePath],
                bindingUtil ? undefined : this.definition.parameters,
                undefined);
    return id;
  });

  apiFunctions.setHandleRequest('isRestorable',
      function(id, callback) {
    var savedEntry = entryIdManager.getEntryById(id);
    if (savedEntry) {
      safeCallbackApply('fileSystem.isRestorable', {}, callback, [true]);
    } else {
      sendRequest('fileSystem.isRestorable', [id, callback],
                  bindingUtil ? undefined : this.definition.parameters,
                  undefined);
    }
  });

  apiFunctions.setUpdateArgumentsPostValidate('restoreEntry',
      function(id, callback) {
    var savedEntry = entryIdManager.getEntryById(id);
    if (savedEntry) {
      // We already have a file entry for this id so pass it to the callback and
      // send a request to the browser to move it to the back of the LRU.
      safeCallbackApply('fileSystem.restoreEntry', {}, callback, [savedEntry]);
      return [id, false, null];
    } else {
      // Ask the browser process for a new file entry for this id, to be passed
      // to |callback|.
      return [id, true, callback];
    }
  });

  apiFunctions.setCustomCallback('requestFileSystem',
      function(name, request, callback, response) {
    var fileSystem;
    if (response && response.file_system_id) {
      fileSystem = fileSystemNatives.GetIsolatedFileSystem(
          response.file_system_id, response.file_system_path);
    }
    safeCallbackApply('fileSystem.requestFileSystem', request, callback,
                      [fileSystem]);
  });

  // TODO(benwells): Remove these deprecated versions of the functions.
  fileSystem.getWritableFileEntry = function() {
    console.log("chrome.fileSystem.getWritableFileEntry is deprecated");
    console.log("Please use chrome.fileSystem.getWritableEntry instead");
    $Function.apply(fileSystem.getWritableEntry, this, arguments);
  };

  fileSystem.isWritableFileEntry = function() {
    console.log("chrome.fileSystem.isWritableFileEntry is deprecated");
    console.log("Please use chrome.fileSystem.isWritableEntry instead");
    $Function.apply(fileSystem.isWritableEntry, this, arguments);
  };

  fileSystem.chooseFile = function() {
    console.log("chrome.fileSystem.chooseFile is deprecated");
    console.log("Please use chrome.fileSystem.chooseEntry instead");
    $Function.apply(fileSystem.chooseEntry, this, arguments);
  };
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
