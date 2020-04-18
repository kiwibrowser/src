// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   fulfill: function(PiexLoaderResponse):undefined,
 *   reject: function(string):undefined}
 * }}
 */
var PiexRequestCallbacks;

/**
 * Color space.
 * @enum {string}
 */
var ColorSpace = {
  SRGB: 'sRgb',
  ADOBE_RGB: 'adobeRgb'
};

/**
 * @param {{id:number, thumbnail:!ArrayBuffer, orientation:number,
 *          colorSpace: ColorSpace}}
 *     data Data directly returned from NaCl module.
 * @constructor
 * @struct
 */
function PiexLoaderResponse(data) {
  /**
   * @public {number}
   * @const
   */
  this.id = data.id;

  /**
   * @public {!ArrayBuffer}
   * @const
   */
  this.thumbnail = data.thumbnail;

  /**
   * @public {!ImageOrientation}
   * @const
   */
  this.orientation =
      ImageOrientation.fromExifOrientation(data.orientation);

  /**
   * @public {ColorSpace}
   * @const
   */
  this.colorSpace = data.colorSpace;
}

/**
 * Creates a PiexLoader for loading RAW files using a Piex NaCl module.
 *
 * All of the arguments are optional and used for tests only. If not passed,
 * then default implementations and values will be used.
 *
 * @param {function()=} opt_createModule Creates a NaCl module.
 * @param {function(!Element)=} opt_destroyModule Destroys a NaCl module.
 * @param {number=} opt_idleTimeout Idle timeout to destroy NaCl module.
 * @constructor
 * @struct
 */
function PiexLoader(opt_createModule, opt_destroyModule, opt_idleTimeout) {
  /**
   * @private {function():!Element}
   */
  this.createModule_ = opt_createModule || this.defaultCreateModule_.bind(this);

  /**
   * @private {function():!Element}
   */
  this.destroyModule_ =
      opt_destroyModule || this.defaultDestroyModule_.bind(this);

  this.idleTimeoutMs_ = opt_idleTimeout !== undefined ?
      opt_idleTimeout :
      PiexLoader.DEFAULT_IDLE_TIMEOUT_MS;

  /**
   * @private {Element}
   */
  this.naclModule_ = null;

  /**
   * @private {Element}
   */
  this.containerElement_ = null;

  /**
   * @private {number}
   */
  this.unloadTimer_ = 0;

  /**
   * @private {Promise<boolean>}
   */
  this.naclPromise_ = null;

  /**
   * @private {?function(boolean)}
   */
  this.naclPromiseFulfill_ = null;

  /**
   * @private {?function(string=)}
   */
  this.naclPromiseReject_ = null;

  /**
   * @private {!Object<number, ?PiexRequestCallbacks>}
   * @const
   */
  this.requests_ = {};

  /**
   * @private {number}
   */
  this.requestIdCount_ = 0;

  // Bound function so the listeners can be unregistered.
  this.onNaclLoadBound_ = this.onNaclLoad_.bind(this);
  this.onNaclMessageBound_ = this.onNaclMessage_.bind(this);
  this.onNaclErrorBound_ = this.onNaclError_.bind(this);
  this.onNaclCrashBound_ = this.onNaclCrash_.bind(this);
}

/**
 * Idling time before the NaCl module is unloaded. This lets the image loader
 * extension close when inactive.
 *
 * @const {number}
 */
PiexLoader.DEFAULT_IDLE_TIMEOUT_MS = 3000;  // 3 seconds.

/**
 * Creates a NaCl module element.
 *
 * Do not call directly. Use this.loadModule_ instead to support
 * tests.
 *
 * @return {!Element}
 * @private
 */
PiexLoader.prototype.defaultCreateModule_ = function() {
  var embed = document.createElement('embed');
  embed.setAttribute('type', 'application/x-pnacl');
  // The extension nmf is not allowed to load. We uses .nmf.js instead.
  embed.setAttribute('src', '/piex/piex.nmf.txt');
  embed.width = 0;
  embed.height = 0;
  return embed;
};

PiexLoader.prototype.defaultDestroyModule_ = function(module) {
  // The module is destroyed by removing it from DOM in loadNaclModule_().
};

/**
 * @return {!Promise<boolean>}
 * @private
 */
PiexLoader.prototype.loadNaclModule_ = function() {
  if (this.naclPromise_) {
    return this.naclPromise_;
  }

  this.naclPromise_ = new Promise(function(fulfill) {
    chrome.fileManagerPrivate.isPiexLoaderEnabled(fulfill);
  }).then(function(enabled) {
    if (!enabled)
      return false;
    return new Promise(function(fulfill, reject) {
      this.naclPromiseFulfill_ = fulfill;
      this.naclPromiseReject_ = reject;
      this.naclModule_ = this.createModule_();

      // The <EMBED> element is wrapped inside a <DIV>, which has both a 'load'
      // and a 'message' event listener attached.  This wrapping method is used
      // instead of attaching the event listeners directly to the <EMBED>
      // element to ensure that the listeners are active before the NaCl module
      // 'load' event fires.
      var listenerContainer = document.createElement('div');
      listenerContainer.appendChild(this.naclModule_);
      listenerContainer.addEventListener('load', this.onNaclLoadBound_, true);
      listenerContainer.addEventListener(
          'message', this.onNaclMessageBound_, true);
      listenerContainer.addEventListener('error', this.onNaclErrorBound_, true);
      listenerContainer.addEventListener('crash', this.onNaclCrashBound_, true);
      listenerContainer.style.height = '0px';
      this.containerElement_ = listenerContainer;
      document.body.appendChild(listenerContainer);

      // Force a relayout. Workaround for load event not being called on <embed>
      // for a NaCl module. crbug.com/699930
      /** @suppress {suspiciousCode} */ this.naclModule_.offsetTop;
    }.bind(this));
  }.bind(this)).catch(function (error) {
    console.error(error);
    return false;
  });

  return this.naclPromise_;
};

/**
 * @private
 */
PiexLoader.prototype.unloadNaclModule_ = function() {
  this.containerElement_.removeEventListener('load', this.onNaclLoadBound_);
  this.containerElement_.removeEventListener(
      'message', this.onNaclMessageBound_);
  this.containerElement_.removeEventListener('error', this.onNaclErrorBound_);
  this.containerElement_.removeEventListener('crash', this.onNaclCrashBound_);
  this.containerElement_.parentNode.removeChild(this.containerElement_);
  this.containerElement_ = null;

  this.destroyModule_();
  this.naclModule_ = null;
  this.naclPromise_ = null;
  this.naclPromiseFulfill_ = null;
  this.naclPromiseReject_ = null;
};

/**
 * @param {Event} event
 * @private
 */
PiexLoader.prototype.onNaclLoad_ = function(event) {
  console.assert(this.naclPromiseFulfill_);
  this.naclPromiseFulfill_(true);
};

/**
 * @param {Event} event
 * @private
 */
PiexLoader.prototype.onNaclMessage_ = function(event) {
  var id = event.data.id;
  if (!event.data.error) {
    var response = new PiexLoaderResponse(event.data);
    console.assert(this.requests_[id]);
    this.requests_[id].fulfill(response);
  } else {
    console.assert(this.requests_[id]);
    this.requests_[id].reject(event.data.error);
  }
  delete this.requests_[id];
  if (Object.keys(this.requests_).length === 0)
    this.scheduleUnloadOnIdle_();
};

/**
 * @param {Event} event
 * @private
 */
PiexLoader.prototype.onNaclError_ = function(event) {
  console.assert(this.naclPromiseReject_);
  this.naclPromiseReject_(this.naclModule_['lastError']);
};

/**
 * @param {Event} event
 * @private
 */
PiexLoader.prototype.onNaclCrash_ = function(event) {
  console.assert(this.naclPromiseReject_);
  this.naclPromiseReject_('PiexLoader crashed.');
};

/**
 * Schedules unloading the NaCl module after IDLE_TIMEOUT_MS passes.
 * @private
 */
PiexLoader.prototype.scheduleUnloadOnIdle_ = function() {
  if (this.unloadTimer_)
    clearTimeout(this.unloadTimer_);
  this.unloadTimer_ =
      setTimeout(this.onIdleTimeout_.bind(this), this.idleTimeoutMs_);
};

/**
 * @private
 */
PiexLoader.prototype.onIdleTimeout_ = function() {
  this.unloadNaclModule_();
};

/**
 * Simulates time passed required to fire the closure enqueued with setTimeout.
 *
 * Note, that if there is no active timer set with setTimeout earlier, then
 * nothing will happen.
 *
 * This method is used to avoid waiting for DEFAULT_IDLE_TIMEOUT_MS in tests.
 * Also, it allows to avoid flakyness by effectively removing any dependency
 * on execution speed of the test (tests set the timeout to a very large value
 * and only rely on this method to simulate passed time).
 */
PiexLoader.prototype.simulateIdleTimeoutPassedForTests = function() {
  if (this.unloadTimer_) {
    clearTimeout(this.unloadTimer_);
    this.onIdleTimeout_();
  }
};

/**
 * Starts to load RAW image.
 * @param {string} url
 * @return {!Promise<!PiexLoaderResponse>}
 */
PiexLoader.prototype.load = function(url) {
  var requestId = this.requestIdCount_++;

  if (this.unloadTimer_) {
    clearTimeout(this.unloadTimer_);
    this.unloadTimer_ = 0;
  }

  // Prevents unloading the NaCl module during handling the promises below.
  this.requests_[requestId] = null;

  return this.loadNaclModule_().then(function(loaded) {
    if (!loaded)
      return Promise.reject('Piex is not loaded');
    var message = {id: requestId, name: 'loadThumbnail', url: url};
    this.naclModule_.postMessage(message);
    return new Promise(function(fulfill, reject) {
             delete this.requests_[requestId];
             this.requests_[message.id] = {fulfill: fulfill, reject: reject};
           }.bind(this))
        .catch(function(error) {
          delete this.requests_[requestId];
          console.error('PiexLoaderError: ', error);
          return Promise.reject(error);
        });
  }.bind(this));
};
