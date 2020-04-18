// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Loads and resizes an image.
 * @constructor
 */
function ImageLoader() {
  /**
   * Persistent cache object.
   * @type {ImageCache}
   * @private
   */
  this.cache_ = new ImageCache();

  /**
   * Manages pending requests and runs them in order of priorities.
   * @type {Scheduler}
   * @private
   */
  this.scheduler_ = new Scheduler();

  /**
   * Piex loader for RAW images.
   * @private {!PiexLoader}
   */
  this.piexLoader_ = new PiexLoader();

  // Grant permissions to all volumes, initialize the cache and then start the
  // scheduler.
  chrome.fileManagerPrivate.getVolumeMetadataList(function(volumeMetadataList) {
    // Listen for mount events, and grant permissions to volumes being mounted.
    chrome.fileManagerPrivate.onMountCompleted.addListener(
        function(event) {
          if (event.eventType === 'mount' && event.status === 'success') {
            chrome.fileSystem.requestFileSystem(
                {volumeId: event.volumeMetadata.volumeId}, function() {});
          }
        });
    var initPromises = volumeMetadataList.map(function(volumeMetadata) {
      var requestPromise = new Promise(function(callback) {
        chrome.fileSystem.requestFileSystem(
            {volumeId: volumeMetadata.volumeId},
            /** @type {function(FileSystem=)} */(callback));
      });
      return requestPromise;
    });
    initPromises.push(new Promise(function(resolve, reject) {
      this.cache_.initialize(resolve);
    }.bind(this)));

    // After all initialization promises are done, start the scheduler.
    Promise.all(initPromises).then(this.scheduler_.start.bind(this.scheduler_));
  }.bind(this));

  // Listen for incoming requests.
  chrome.runtime.onMessageExternal.addListener(
      function(request, sender, sendResponse) {
        if (ImageLoader.ALLOWED_CLIENTS.indexOf(sender.id) !== -1) {
          // Sending a response may fail if the receiver already went offline.
          // This is not an error, but a normal and quite common situation.
          var failSafeSendResponse = function(response) {
            try {
              sendResponse(response);
            }
            catch (e) {
              // Ignore the error.
            }
          };
          if (typeof request.orientation === 'number') {
            request.orientation =
                ImageOrientation.fromDriveOrientation(request.orientation);
          } else if (request.orientation) {
            request.orientation =
                ImageOrientation.fromRotationAndScale(request.orientation);
          } else {
            request.orientation = new ImageOrientation(1, 0, 0, 1);
          }
          return this.onMessage_(sender.id,
                                 /** @type {LoadImageRequest} */ (request),
                                 failSafeSendResponse);
        }
      }.bind(this));
}

/**
 * List of extensions allowed to perform image requests.
 *
 * @const
 * @type {Array<string>}
 */
ImageLoader.ALLOWED_CLIENTS = [
  'hhaomjibdihmijegdhdafkllkbggdgoj',  // File Manager's extension id.
  'nlkncpkkdoccmpiclbokaimcnedabhhm',  // Gallery's extension id.
  'jcgeabjmjgoblfofpppfkcoakmfobdko',  // Video Player's extension id.
];

/**
 * Handles a request. Depending on type of the request, starts or stops
 * an image task.
 *
 * @param {string} senderId Sender's extension id.
 * @param {!LoadImageRequest} request Request message as a hash array.
 * @param {function(Object)} callback Callback to be called to return response.
 * @return {boolean} True if the message channel should stay alive until the
 *     callback is called.
 * @private
 */
ImageLoader.prototype.onMessage_ = function(senderId, request, callback) {
  var requestId = senderId + ':' + request.taskId;
  if (request.cancel) {
    // Cancel a task.
    this.scheduler_.remove(requestId);
    return false;  // No callback calls.
  } else {
    // Create a request task and add it to the scheduler (queue).
    var requestTask = new ImageRequest(
        requestId, this.cache_, this.piexLoader_, request, callback);
    this.scheduler_.add(requestTask);
    return true;  // Request will call the callback.
  }
};

/**
 * Returns the singleton instance.
 * @return {ImageLoader} ImageLoader object.
 */
ImageLoader.getInstance = function() {
  if (!ImageLoader.instance_)
    ImageLoader.instance_ = new ImageLoader();
  return ImageLoader.instance_;
};
