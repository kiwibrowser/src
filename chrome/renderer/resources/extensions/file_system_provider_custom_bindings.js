// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the fileSystemProvider API.

var binding =
    apiBridge || require('binding').Binding.create('fileSystemProvider');
var fileSystemProviderInternal =
    getInternalApi ?
        getInternalApi('fileSystemProviderInternal') :
        require('binding').Binding.create('fileSystemProviderInternal')
            .generate();
var registerArgumentMassager = bindingUtil ?
    $Function.bind(bindingUtil.registerEventArgumentMassager, bindingUtil) :
    require('event_bindings').registerArgumentMassager;

/**
 * Maximum size of the thumbnail in bytes.
 * @type {number}
 * @const
 */
var METADATA_THUMBNAIL_SIZE_LIMIT = 32 * 1024 * 1024;

/**
 * Regular expression to validate if the thumbnail URI is a valid data URI,
 * taking into account allowed formats.
 * @type {RegExp}
 * @const
 */
var METADATA_THUMBNAIL_FORMAT = new RegExp(
    '^data:image/(png|jpeg|webp);', 'i');

/**
 * Annotates a date with its serialized value.
 * @param {Date} date Input date.
 * @return {Date} Date with an extra <code>value</code> attribute.
 */
function annotateDate(date) {
  // Copy in case the input date is frozen.
  var result = new Date(date.getTime());
  result.value = result.toString();
  return result;
}

/**
 * Verifies if the passed image URI is valid.
 * @param {*} uri Image URI.
 * @return {boolean} True if valid, valse otherwise.
 */
function verifyImageURI(uri) {
  // The URI is specified by a user, so the type may be incorrect.
  if (typeof uri != 'string' && !(uri instanceof String))
    return false;

  return METADATA_THUMBNAIL_FORMAT.test(uri);
}

/**
 * Verifies if the passed metadata is valid.
 * @param {!GetMetadataOptions|!ReadDirectoryOptions} options
 * @return {boolean} True if valid, false if invalid.
 */
function verifyMetadata(options, metadata) {
  // Ideally we'd like to consider the following as errors, but for backward
  // compatibility they are warnings only.
  if (!options.isDirectory && metadata.isDirectory !== undefined)
    console.warn('IsDirectory specified, but not requested.');
  if (!options.name && metadata.name !== undefined)
    console.warn('Name specified, but not requested.');
  if (!options.size && metadata.size !== undefined)
    console.warn('Size specified, but not requested.');
  if (!options.modificationTime && metadata.modificationTime !== undefined)
    console.warn('Last modification time specified, but not requested.');
  if (!options.mimeType && metadata.mimeType !== undefined) {
    console.warn('MIME type specified, but not requested.');
  } else {
    if (metadata.mimeType === '') {
      warning = 'MIME type must not be an empty string.' +
          'If unknown, then do not set it.';
    }
  }

  if (options.isDirectory && metadata.isDirectory === undefined) {
    console.error('IsDirectory is required for this request.');
    return false;
  }

  if (options.name && metadata.name === undefined) {
    console.error('Name is required for this request.');
    return false;
  }

  if (options.size && metadata.size === undefined) {
    console.error('Size is required for this request.');
    return false;
  }

  if (options.modificationTime && metadata.modificationTime === undefined) {
    console.error('Last modification time is required for this request.');
    return false;
  }

  // It is invalid to return a thumbnail when it's not requested. The
  // restriction is added in order to avoid fetching the thumbnail while
  // it's not needed.
  if (!options.thumbnail && metadata.thumbnail !== undefined) {
    console.error('Thumbnail data provided, but not requested.');
    return false;
  }

  // Check the format and size. Note, that in the C++ layer, there is
  // another sanity check to avoid passing any evil URL.
  if (metadata.thumbnail !== undefined && !verifyImageURI(metadata.thumbnail)) {
    console.error('Thumbnail format invalid.');
    return false;
  }

  if (metadata.thumbnail !== undefined &&
      metadata.thumbnail.length > METADATA_THUMBNAIL_SIZE_LIMIT) {
    console.error('Thumbnail data too large.');
    return false;
  }

  return true;
}

/**
 * Verifies if the passed error code is valid when used to indicate
 * a failure.
 * @param {!string} error
 * @return {boolean} True if valid, false if invalid.
 */
function verifyErrorForFailure(error) {
  if (error === 'OK') {
    console.error('Error code cannot be OK in case of failures.');
    return false;
  }
  return true;
}

/**
 * Annotates an entry metadata by serializing its modifiedTime value.
 * @param {EntryMetadata} metadata Input metadata.
 * @return {EntryMetadata} metadata Annotated metadata, which can be passed
 *     back to the C++ layer.
 */
function annotateMetadata(metadata) {
  var result = {};
  if (metadata.isDirectory !== undefined)
    result.isDirectory = metadata.isDirectory;
  if (metadata.name !== undefined)
    result.name = metadata.name;
  if (metadata.size !== undefined)
    result.size = metadata.size;
  if (metadata.modificationTime !== undefined)
    result.modificationTime = annotateDate(metadata.modificationTime);
  if (metadata.mimeType !== undefined)
    result.mimeType = metadata.mimeType;
  if (metadata.thumbnail !== undefined)
    result.thumbnail = metadata.thumbnail;
  return result;
}

/**
 * Massages arguments of an event raised by the File System Provider API.
 * @param {Array<*>} args Input arguments.
 * @param {function(Array<*>)} dispatch Closure to be called with massaged
 *     arguments.
 */
function massageArgumentsDefault(args, dispatch) {
  var executionStart = Date.now();
  var options = args[0];
  var onSuccessCallback = function(hasNext) {
    fileSystemProviderInternal.operationRequestedSuccess(
        options.fileSystemId, options.requestId, Date.now() - executionStart);
  };
  var onErrorCallback = function(error) {
    if (!verifyErrorForFailure(error))
      return;
    fileSystemProviderInternal.operationRequestedError(
        options.fileSystemId, options.requestId, error,
        Date.now() - executionStart);
  }
  dispatch([options, onSuccessCallback, onErrorCallback]);
}

registerArgumentMassager(
    'fileSystemProvider.onUnmountRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onGetMetadataRequested',
    function(args, dispatch) {
      var executionStart = Date.now();
      var options = args[0];
      var onSuccessCallback = function(metadata) {
        if (!verifyMetadata(options, metadata)) {
          fileSystemProviderInternal.operationRequestedError(
              options.fileSystemId, options.requestId, 'FAILED',
              Date.now() - executionStart);
          return;
        }

        fileSystemProviderInternal.getMetadataRequestedSuccess(
            options.fileSystemId,
            options.requestId,
            annotateMetadata(metadata),
            Date.now() - executionStart);
      };

      var onErrorCallback = function(error) {
        if (!verifyErrorForFailure(error))
          return;
        fileSystemProviderInternal.operationRequestedError(
            options.fileSystemId, options.requestId, error,
            Date.now() - executionStart);
      }

      dispatch([options, onSuccessCallback, onErrorCallback]);
    });

registerArgumentMassager(
    'fileSystemProvider.onGetActionsRequested',
    function(args, dispatch) {
      var executionStart = Date.now();
      var options = args[0];
      var onSuccessCallback = function(actions) {
        fileSystemProviderInternal.getActionsRequestedSuccess(
            options.fileSystemId,
            options.requestId,
            actions,
            Date.now() - executionStart);
      };

      var onErrorCallback = function(error) {
        if (!verifyErrorForFailure(error))
          return;
        fileSystemProviderInternal.operationRequestedError(
            options.fileSystemId, options.requestId, error,
            Date.now() - executionStart);
      }

      dispatch([options, onSuccessCallback, onErrorCallback]);
    });

registerArgumentMassager(
    'fileSystemProvider.onReadDirectoryRequested',
    function(args, dispatch) {
      var executionStart = Date.now();
      var options = args[0];
      var onSuccessCallback = function(entries, hasNext) {
        var error = false;
        for (var i = 0; i < entries.length; i++) {
          if (!verifyMetadata(options, entries[i])) {
            error = true;
            break;
          }
        }

        if (error) {
          fileSystemProviderInternal.operationRequestedError(
              options.fileSystemId, options.requestId, 'FAILED',
              Date.now() - executionStart);
          return;
        }

        var annotatedEntries = entries.map(annotateMetadata);
        fileSystemProviderInternal.readDirectoryRequestedSuccess(
            options.fileSystemId, options.requestId, annotatedEntries, hasNext,
            Date.now() - executionStart);
      };

      var onErrorCallback = function(error) {
        if (!verifyErrorForFailure(error))
          return;
        fileSystemProviderInternal.operationRequestedError(
            options.fileSystemId, options.requestId, error,
            Date.now() - executionStart);
      }
      dispatch([options, onSuccessCallback, onErrorCallback]);
    });

registerArgumentMassager(
    'fileSystemProvider.onOpenFileRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onCloseFileRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onReadFileRequested',
    function(args, dispatch) {
      var executionStart = Date.now();
      var options = args[0];
      var onSuccessCallback = function(data, hasNext) {
        fileSystemProviderInternal.readFileRequestedSuccess(
            options.fileSystemId, options.requestId, data, hasNext,
            Date.now() - executionStart);
      };
      var onErrorCallback = function(error) {
        if (!verifyErrorForFailure(error))
          return;
        fileSystemProviderInternal.operationRequestedError(
            options.fileSystemId, options.requestId, error,
            Date.now() - executionStart);
      }
      dispatch([options, onSuccessCallback, onErrorCallback]);
    });

registerArgumentMassager(
    'fileSystemProvider.onCreateDirectoryRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onDeleteEntryRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onCreateFileRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onCopyEntryRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onMoveEntryRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onTruncateRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onWriteFileRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onAbortRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onObserveDirectoryRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onUnobserveEntryRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onAddWatcherRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onRemoveWatcherRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onConfigureRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onExecuteActionRequested',
    massageArgumentsDefault);

registerArgumentMassager(
    'fileSystemProvider.onMountRequested',
    function(args, dispatch) {
      var onSuccessCallback = function() {
        // chrome.fileManagerPrivate.addProvidedFileSystem doesn't accept
        // any callbacks, so ignore the callback calls here.
        // The callbacks exist for consistency with other on*Requested events.
      };
      var onErrorCallback = function(error) {
        if (!verifyErrorForFailure(error))
          return;
        // chrome.fileManagerPrivate.addProvidedFileSystem doesn't accept
        // any callbacks, so ignore the callback calls here.
        // The callbacks exist for consistency with other on*Requested events.
      }
      dispatch([onSuccessCallback, onErrorCallback]);
    });

if (!apiBridge)
  exports.$set('binding', binding.generate());
