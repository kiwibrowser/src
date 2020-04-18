// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
* @fileoverview
*
* In Chrome Apps, some platform APIs can only be called from the background
* page (e.g. reloading a chrome.app.AppWindow).  Likewise, some chrome API's
* must be initiated by user interaction, which can only be called from the
* foreground.
*
* This class provides helper functions to invoke methods on different pages
* using chrome.runtime.sendMessage.  Messages are passed in the following
* format:
*     {methodName:{string}, params:{Array}}
*
* chrome.runtime.sendMessage allows multiple handlers to be registered on a
* document, but only one handler can send a response.
* This class uniquely identifies a method with the |methodName| and enforces
* that only one handler can be registered per |methodName| in the document.
*
* For example, to call method foo() in the background page from the foreground
* chrome.app.AppWindow, you can do the following.
* In the background page:
*     base.Ipc.getInstance().register('my.service.name', foo);
*
* In the AppWindow document:
*     base.Ipc.invoke('my.service.name', arg1, arg2, ...).then(
*       function(result) {
*         console.log('The result is ' + result);
*     });
*
* This will invoke foo() with the arg1, arg2, ....
* The return value of foo() will be passed back to the caller in the
* form of a promise.
*/

/** @suppress {duplicate} */
var base = base || {};

(function() {

'use strict';

/**
 * @constructor
 * @private
 */
base.Ipc = function() {
  console.assert(instance_ === null, 'Duplicate base.Ipc constructor.');
  /** @private {!Object<{allowExternal: boolean, method: Function>} */
  this.handlers_ = {};
  this.onMessageHandler_ =
      /** @type {function(*, MessageSender, function (*))} */ (
          this.onMessage_.bind(this));
  chrome.runtime.onMessage.addListener(this.onMessageHandler_);
  chrome.runtime.onMessageExternal.addListener(this.onMessageHandler_);
};

/** @private */
base.Ipc.prototype.dispose_ = function() {
  chrome.runtime.onMessage.removeListener(this.onMessageHandler_);
  chrome.runtime.onMessageExternal.removeListener(this.onMessageHandler_);
};

/**
 * The error strings are only used for debugging purposes and are not localized.
 *
 * @enum {string}
 */
base.Ipc.Error = {
  UNSUPPORTED_REQUEST_TYPE: 'Unsupported method name.',
  UNAUTHORIZED_REQUEST_ORIGIN: 'Unauthorized origin.'
};

/**
 * @constructor
 * @param {string} methodName
 * @param {?Array} params
 * @struct
 * @private
 */
base.Ipc.Request_ = function(methodName, params) {
  this.methodName = methodName;
  this.params = params;
};


/**
 * @param {string} methodName
 * @param {Function} handler The handler can be invoked by calling
 *   base.Ipc.invoke(|methodName|, arg1, arg2, ...)
 * @param {boolean=} opt_allowExternal True if the message is permitted from
 *   origins listed in externally_connectable; false or undefined if the
 *   message is only permitted from the same app.
 * @return {boolean} Whether the handler is successfully registered.
 */
  base.Ipc.prototype.register = function(methodName, handler,
                                         opt_allowExternal) {
  if (methodName in this.handlers_) {
    console.error('service ' + methodName + ' is already registered.');
    return false;
  }
    this.handlers_[methodName] = {
      allowExternal: Boolean(opt_allowExternal),
      method: handler
    };
  return true;
};

/**
 * @param {string} methodName
 */
base.Ipc.prototype.unregister = function(methodName) {
  delete this.handlers_[methodName];
};

/**
 * @param {base.Ipc.Request_} message
 * @param {!MessageSender} sender
 * @param {function(*): void} sendResponse
 * @return {boolean} True if an asynchronous response is pending; false if the
 *   response (if any) has already been sent.
 */
base.Ipc.prototype.onMessage_ = function(message, sender, sendResponse) {
  var methodName = message.methodName;
  if (typeof methodName !== 'string') {
    return;
  }

  var handler = this.handlers_[methodName];
  if (!handler) {
    sendResponse({error: base.Ipc.Error.UNSUPPORTED_REQUEST_TYPE});
    return;
  }

  if (!handler.allowExternal && sender.id !== chrome.runtime.id) {
    sendResponse({error: base.Ipc.Error.UNAUTHORIZED_REQUEST_ORIGIN});
    return;
  }

  try {
    var result = handler.method.apply(null, message.params);
    if (result instanceof Promise) {
      result.
          then(function(response) { sendResponse(response); }).
          catch(function(e) { sendResponse({error: e.message}) });
      return true;
    } else {
      sendResponse(result);
    }
  } catch (/** @type {Error} */ e) {
    sendResponse({error: e.message});
  }
  return false;
};

/**
 * Invokes a method on another page within this extension.
 *
 * @param {string} methodName
 * @param {...} var_args
 * @return {Promise} A Promise that would resolve to the return value of the
 *   handler or reject if the handler throws an exception.
 * @suppress {reportUnknownTypes}
 */
base.Ipc.invoke = function(methodName, var_args) {
  var params = Array.prototype.slice.call(arguments, 1);
  var sendMessage = base.Promise.as(
      chrome.runtime.sendMessage,
      [null, new base.Ipc.Request_(methodName, params)]);

  return sendMessage.then(
    /** @param {?{error: Error}} response */
    function(response) {
      if (response && response.error) {
        return Promise.reject(response.error);
      } else {
        return Promise.resolve(response);
      }
  });
};


/** @type {base.Ipc} */
var instance_ = null;

/** @return {base.Ipc} */
base.Ipc.getInstance = function() {
  if (!instance_) {
    instance_ = new base.Ipc();
  }
  return instance_;
};

base.Ipc.deleteInstance = function() {
  if (instance_) {
    instance_.dispose_();
    instance_ = null;
  }
};

})();
