// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!MessagePort=} opt_messagePort Message port overriding the default
 *     worker port.
 * @extends {MetadataProvider}
 * @constructor
 * @struct
 */
function ContentMetadataProvider(opt_messagePort) {
  MetadataProvider.call(
      this,
      ContentMetadataProvider.PROPERTY_NAMES);

  /**
   * Pass all URLs to the metadata reader until we have a correct filter.
   * @private {RegExp}
   */
  this.urlFilter_ = /.*/;

  /**
   * @private {!MessagePort}
   * @const
   */
  this.dispatcher_ = opt_messagePort ?
      opt_messagePort :
      new SharedWorker(ContentMetadataProvider.WORKER_SCRIPT).port;
  this.dispatcher_.onmessage = this.onMessage_.bind(this);
  this.dispatcher_.postMessage({verb: 'init'});
  this.dispatcher_.start();

  /**
   * Initialization is not complete until the Worker sends back the
   * 'initialized' message.  See below.
   * @private {boolean}
   */
  this.initialized_ = false;

  /**
   * Map from Entry.toURL() to callback.
   * Note that simultaneous requests for same url are handled in MetadataCache.
   * @private {!Object<!string, !Array<function(!MetadataItem)>>}
   * @const
   */
  this.callbacks_ = {};
}

/**
 * @const {!Array<string>}
 */
ContentMetadataProvider.PROPERTY_NAMES = [
  'contentImageTransform',
  'contentThumbnailTransform',
  'contentThumbnailUrl',
  'exifLittleEndian',
  'ifd',
  'imageHeight',
  'imageWidth',
  'mediaAlbum',
  'mediaArtist',
  'mediaDuration',
  'mediaGenre',
  'mediaMimeType',
  'mediaTitle',
  'mediaTrack',
  'mediaYearRecorded',
];

/**
 * Path of a worker script.
 * @public {string}
 */
ContentMetadataProvider.WORKER_SCRIPT =
    'chrome-extension://hhaomjibdihmijegdhdafkllkbggdgoj/' +
    'foreground/js/metadata/metadata_dispatcher.js';

/**
 * Converts content metadata from parsers to the internal format.
 * @param {Object} metadata The content metadata.
 * @return {!MetadataItem} Converted metadata.
 */
ContentMetadataProvider.convertContentMetadata = function(metadata) {
  var item = new MetadataItem();
  item.contentImageTransform = metadata['imageTransform'];
  item.contentThumbnailTransform = metadata['thumbnailTransform'];
  item.contentThumbnailUrl = metadata['thumbnailURL'];
  item.exifLittleEndian = metadata['littleEndian'];
  item.ifd = metadata['ifd'];
  item.imageHeight = metadata['height'];
  item.imageWidth = metadata['width'];
  item.mediaMimeType = metadata['mimeType'];
  return item;
};

ContentMetadataProvider.prototype.__proto__ = MetadataProvider.prototype;

/**
 * @override
 */
ContentMetadataProvider.prototype.get = function(requests) {
  if (!requests.length)
    return Promise.resolve([]);

  var promises = [];
  for (var i = 0; i < requests.length; i++) {
    promises.push(new Promise(function(request, fulfill) {
      this.getImpl_(request.entry, request.names, fulfill);
    }.bind(this, requests[i])));
  }
  return Promise.all(promises);
};

/**
 * Fetches the metadata.
 * @param {!Entry} entry File entry.
 * @param {!Array<string>} names Requested metadata type.
 * @param {function(!MetadataItem)} callback Callback expects metadata value.
 *     This callback is called asynchronously.
 * @private
 */
ContentMetadataProvider.prototype.getImpl_ = function(entry, names, callback) {
  if (entry.isDirectory) {
    setTimeout(callback.bind(null, this.createError_(entry.toURL(),
        'get',
        'we don\'t generate thumbnails for directory')), 0);
    return;
  }
  // TODO(ryoh): mediaGalleries API does not handle
  // image metadata correctly.
  // We parse it in our pure js parser.
  // chrome/browser/media_galleries/fileapi/supported_image_type_validator.cc
  var type = FileType.getType(entry);
  if (type && type.type === 'image') {
    var url = entry.toURL();
    if (this.callbacks_[url]) {
      this.callbacks_[url].push(callback);
    } else {
      this.callbacks_[url] = [callback];
      this.dispatcher_.postMessage({verb: 'request', arguments: [url]});
    }
    return;
  }
  this.getFromMediaGalleries_(entry, names).then(callback);
};

/**
 * Gets a metadata from mediaGalleries API
 *
 * @param {!Entry} entry File entry.
 * @param {!Array<string>} names Requested metadata type.
 * @return {!Promise<!MetadataItem>}  Promise that resolves with the metadata of
 *    the entry.
 * @private
 */
ContentMetadataProvider.prototype.getFromMediaGalleries_ =
    function(entry, names) {
  var self = this;
  return new Promise(function(resolve, reject) {
    entry.file(function(blob) {
      var metadataType = 'mimeTypeOnly';
      if (names.indexOf('mediaArtist') !== -1 ||
          names.indexOf('mediaTitle') !== -1 ||
          names.indexOf('mediaTrack') !== -1 ||
          names.indexOf('mediaYearRecorded') !== -1) {
        metadataType = 'mimeTypeAndTags';
      }
      if (names.indexOf('contentThumbnailUrl') !== -1) {
        metadataType = 'all';
      }
      chrome.mediaGalleries.getMetadata(blob, {metadataType: metadataType},
          function(metadata) {
            if (chrome.runtime.lastError) {
              resolve(self.createError_(entry.toURL(),
                  'resolving metadata',
                  chrome.runtime.lastError.toString()));
            } else {
              self.convertMediaMetadataToMetadataItem_(entry, metadata)
                  .then(resolve, reject);
            }
          });
    }, function(err) {
      resolve(self.createError_(entry.toURL(),
          'loading file entry',
          'failed to open file entry'));
    });
  });
};

/**
 * Dispatches a message from a metadata reader to the appropriate on* method.
 * @param {Object} event The event.
 * @private
 */
ContentMetadataProvider.prototype.onMessage_ = function(event) {
  var data = event.data;
  switch (data.verb) {
    case 'initialized':
      this.onInitialized_(data.arguments[0]);
      break;
    case 'result':
      this.onResult_(
          data.arguments[0],
          data.arguments[1] ?
          ContentMetadataProvider.convertContentMetadata(data.arguments[1]) :
          new MetadataItem());
      break;
    case 'error':
      var error = this.createError_(
          data.arguments[0],
          data.arguments[1],
          data.arguments[2]);
      this.onResult_(
          data.arguments[0],
          error);
      break;
    case 'log':
      this.onLog_(data.arguments[0]);
      break;
    default:
      assertNotReached();
      break;
  }
};

/**
 * Handles the 'initialized' message from the metadata reader Worker.
 * @param {RegExp} regexp Regexp of supported urls.
 * @private
 */
ContentMetadataProvider.prototype.onInitialized_ = function(regexp) {
  this.urlFilter_ = regexp;

  // Tests can monitor for this state with
  // ExtensionTestMessageListener listener("worker-initialized");
  // ASSERT_TRUE(listener.WaitUntilSatisfied());
  // Automated tests need to wait for this, otherwise we crash in
  // browser_test cleanup because the worker process still has
  // URL requests in-flight.
  util.testSendMessage('worker-initialized');
  this.initialized_ = true;
};

/**
 * Handles the 'result' message from the worker.
 * @param {string} url File url.
 * @param {!MetadataItem} metadataItem The metadata item.
 * @private
 */
ContentMetadataProvider.prototype.onResult_ = function(url, metadataItem) {
  var callbacks = this.callbacks_[url];
  delete this.callbacks_[url];
  for (var i = 0; i < callbacks.length; i++) {
    callbacks[i](metadataItem);
  }
};

/**
 * Handles the 'log' message from the worker.
 * @param {Array<*>} arglist Log arguments.
 * @private
 */
ContentMetadataProvider.prototype.onLog_ = function(arglist) {
  console.log.apply(console, ['ContentMetadataProvider log:'].concat(arglist));
};

/**
 * Dispatches a message from MediaGalleries API to the appropriate on* method.
 * @param {!Entry} entry File entry.
 * @param {!Object} metadata The metadata from MediaGalleries API.
 * @return {!Promise<!MetadataItem>}  Promise that resolves with
 *    converted metadata item.
 * @private
 */
ContentMetadataProvider.prototype.convertMediaMetadataToMetadataItem_ =
    function(entry, metadata) {
  return new Promise(function(resolve, reject) {
    if (!metadata) {
      resolve(this.createError_(entry.toURL(), 'Reading a thumbnail image',
          "Failed to parse metadata"));
      return;
    }
    var item = new MetadataItem();
    var mimeType = metadata['mimeType'];
    item.contentMimeType = mimeType;
    var trans = {scaleX: 1, scaleY: 1, rotate90: 0};
    if (metadata.rotation) {
      switch (metadata.rotation) {
        case 0:
          break;
        case 90:
          trans.rotate90 = 1;
          break;
        case 180:
          trans.scaleX *= -1;
          trans.scaleY *= -1;
          break;
        case 270:
          trans.rotate90 = 1;
          trans.scaleX *= -1;
          trans.scaleY *= -1;
          break;
        default:
          console.error('Unknown rotation angle: ', metadata.rotation);
      }
    }
    if (metadata.rotation) {
      item.contentImageTransform = item.contentThumbnailTransform = trans;
    }
    item.imageHeight = metadata['height'];
    item.imageWidth = metadata['width'];
    item.mediaAlbum = metadata['album'];
    item.mediaArtist = metadata['artist'];
    item.mediaDuration = metadata['duration'];
    item.mediaGenre = metadata['genre'];
    item.mediaTitle = metadata['title'];
    if (metadata['track']) {
      item.mediaTrack = '' + metadata['track'];
    }
    if (metadata.rawTags) {
      metadata.rawTags.forEach(function(entry) {
        if (entry.type === 'mp3') {
          if (entry.tags['date']) {
            item.mediaYearRecorded = entry.tags['date'];
          }
          // It is possible that metadata['track'] is undefined but this is
          // defined.
          if (entry.tags['track']) {
            item.mediaTrack = entry.tags['track'];
          }
        }
      });
    }
    if (metadata.attachedImages && metadata.attachedImages.length > 0) {
      var reader = new FileReader();
      reader.onload = function(e) {
        item.contentThumbnailUrl = e.target.result;
        resolve(item);
      };
      reader.onerror = function(e) {
        resolve(this.createError_(entry.toURL(), 'Reading a thumbnail image',
            reader.error.toString()));
      }.bind(this);
      reader.readAsDataURL(metadata.attachedImages[0]);
    } else {
      resolve(item);
    }
  }.bind(this));
};

/**
 * Handles the 'error' message from the worker.
 * @param {string} url File entry.
 * @param {string} step Step failed.
 * @param {string} errorDescription Error description.
 * @return {!MetadataItem} Error metadata
 * @private
 */
ContentMetadataProvider.prototype.createError_ = function(
    url, step, errorDescription) {
  // For error case, fill all fields with error object.
  var error = new ContentMetadataProvider.Error(url, step, errorDescription);
  var item = new MetadataItem();
  item.contentImageTransformError = error;
  item.contentThumbnailTransformError = error;
  item.contentThumbnailUrlError = error;
  return item;
};

/**
 * Content metadata provider error.
 * @param {string} url File Entry.
 * @param {string} step Step failed.
 * @param {string} errorDescription Error description.
 * @constructor
 * @struct
 * @extends {Error}
 */
ContentMetadataProvider.Error = function(url, step, errorDescription) {
  /**
   * @public {string}
   */
  this.url = url;

  /**
   * @public {string}
   */
  this.step = step;

  /**
   * @public {string}
   */
  this.errorDescription = errorDescription;
};

ContentMetadataProvider.Error.prototype.__proto__ = Error.prototype;
