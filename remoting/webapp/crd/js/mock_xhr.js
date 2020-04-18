// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A mock version of remoting.Xhr.  Compared to
 * sinon.useMockXhr, this allows unit tests to be written at a higher
 * level, and it eliminates a fair amount of boilerplate involved in
 * making the sinon mocks work with asynchronous calls
 * (cf. gcd_client_unittest.js vs. gcd_client_with_mock_xhr.js for an
 * example).
 */

(function() {
'use strict';

/**
 * @constructor
 * @param {remoting.Xhr.Params} params
 */
remoting.MockXhr = function(params) {
  origXhr['checkParams_'](params);

  /** @const {remoting.Xhr.Params} */
  this.params = normalizeParams(params);

  /** @private {base.Deferred<!remoting.Xhr.Response>} */
  this.deferred_ = null;

  /** @type {remoting.Xhr.Response} */
  this.response_ = null;

  /** @type {boolean} */
  this.aborted_ = false;
};

/**
 * Converts constuctor parameters to a normalized form that hides
 * details of how the constructor is called.
 * @param {remoting.Xhr.Params} params
 * @return {remoting.Xhr.Params} The argument with all missing fields
 *     filled in with default values.
 */
var normalizeParams = function(params) {
  return {
    method: params.method,
    url: params.url,
    urlParams: typeof params.urlParams == 'object' ?
        base.copyWithoutNullFields(params.urlParams) :
        params.urlParams,
    textContent: params.textContent,
    jsonContent: params.jsonContent,
    formContent: params.formContent === undefined ? undefined :
        base.copyWithoutNullFields(params.formContent),
    headers: base.copyWithoutNullFields(params.headers),
    withCredentials: Boolean(params.withCredentials),
    oauthToken: params.oauthToken,
    useIdentity: Boolean(params.useIdentity),
    acceptJson: Boolean(params.acceptJson)
  };
};

/**
 * Psuedo-override from remoting.Xhr.
 * @return {void}
 */
remoting.MockXhr.prototype.abort = function() {
  this.aborted_ = true;
};

/**
 * Psuedo-override from remoting.Xhr.
 * @return {!Promise<!remoting.Xhr.Response>}
 */
remoting.MockXhr.prototype.start = function() {
  runMatchingHandler(this);
  if (!this.deferred_) {
    this.deferred_ = new base.Deferred();
    this.maybeRespond_();
  }
  return this.deferred_.promise();
};

/**
 * Tells this object to send an empty response to the current or next
 * request.
 * @param {number} status The HTTP status code to respond with.
 */
remoting.MockXhr.prototype.setEmptyResponse = function(status) {
  this.setResponse_(new remoting.Xhr.Response(
      status,
      'mock status text from setEmptyResponse',
      null,
      '',
      false));
};

/**
 * Tells this object to send a text/plain response to the current or
 * next request.
 * @param {number} status The HTTP status code to respond with.
 * @param {string} body The content to respond with.
 */
remoting.MockXhr.prototype.setTextResponse = function(status, body) {
  this.setResponse_(new remoting.Xhr.Response(
      status,
      'mock status text from setTextResponse',
      null,
      body || '',
      false));
};

/**
 * Tells this object to send an application/json response to the
 * current or next request.
 * @param {number} status The HTTP status code to respond with.
 * @param {*} body The content to respond with.
 */
remoting.MockXhr.prototype.setJsonResponse = function(status, body) {
  if (!this.params.acceptJson) {
    throw new Error('client does not want JSON response');
  }
  this.setResponse_(new remoting.Xhr.Response(
      status,
      'mock status text from setJsonResponse',
      null,
      JSON.stringify(body),
      true));
};

/**
 * Sets the response to be used for the current or next request.
 * @param {!remoting.Xhr.Response} response
 * @private
 */
remoting.MockXhr.prototype.setResponse_ = function(response) {
  console.assert(this.response_ == null,
                 'Duplicate setResponse_() invocation.');
  this.response_ = response;
  this.maybeRespond_();
};

/**
 * Sends a response if one is available.
 * @private
 */
remoting.MockXhr.prototype.maybeRespond_ = function() {
  if (this.deferred_ && this.response_ && !this.aborted_) {
    this.deferred_.resolve(this.response_);
  }
};

/**
 * The original value of the remoting.Xhr constructor.  The JSDoc type
 * is that of the remoting.Xhr constructor function.
 * @type {?function(this: remoting.Xhr, remoting.Xhr.Params):void}
 */
var origXhr = null;

/**
 * @type {!Array<remoting.MockXhr.UrlHandler>}
 */
var handlers = [];

/**
 * Registers a handler for a given method and URL.  The |urlPattern|
 * argument may either be a string, which must equal a URL to match
 * it, or a RegExp.
 *
 * Matching handlers are run when a FakeXhr's |start| method is
 * called.  The handler should generally call one of
 * |set{Test,Json,Empty}Response|
 *
 * @param {?string} method The HTTP method to respond to, or null to
 *     respond to any method.
 * @param {?string|!RegExp} urlPattern The URL or pattern to respond
 *     to, or null to match any URL.
 * @param {function(!remoting.MockXhr):void} callback The handler
 *     function to call when a matching XHR is started.
 * @param {boolean=} opt_reuse If true, the response can be used for
 *     multiple requests.
 */
remoting.MockXhr.setResponseFor = function(
    method, urlPattern, callback, opt_reuse) {
  handlers.push({
    method: method,
    urlPattern: urlPattern,
    callback: callback,
    reuse: !!opt_reuse
  });
};

/**
 * Installs a response with no content.  See |setResponseFor| for
 * more details on how the parameters work.
 *
 * @param {?string} method
 * @param {?string|!RegExp} urlPattern
 * @param {number=} opt_status The status code to return.
 * @param {boolean=} opt_reuse
 */
remoting.MockXhr.setEmptyResponseFor = function(
    method, urlPattern, opt_status, opt_reuse) {
  remoting.MockXhr.setResponseFor(
      method, urlPattern, function(/** remoting.MockXhr */ xhr) {
        xhr.setEmptyResponse(opt_status === undefined ? 204 : opt_status);
      }, opt_reuse);
};

/**
 * Installs a 200 response with text content.  See |setResponseFor|
 * for more details on how the parameters work.
 *
 * @param {?string} method
 * @param {?string|!RegExp} urlPattern
 * @param {string} content
 * @param {boolean=} opt_reuse
 */
remoting.MockXhr.setTextResponseFor = function(
    method, urlPattern, content, opt_reuse) {
  remoting.MockXhr.setResponseFor(
      method, urlPattern, function(/** remoting.MockXhr */ xhr) {
        xhr.setTextResponse(200, content);
      }, opt_reuse);
};

/**
 * Installs a 200 response with JSON content.  See |setResponseFor|
 * for more details on how the parameters work.
 *
 * @param {?string} method
 * @param {?string|!RegExp} urlPattern
 * @param {*} content
 * @param {boolean=} opt_reuse
 */
remoting.MockXhr.setJsonResponseFor = function(
    method, urlPattern, content, opt_reuse) {
  remoting.MockXhr.setResponseFor(
      method, urlPattern, function(/** remoting.MockXhr */ xhr) {
        xhr.setJsonResponse(200, content);
      }, opt_reuse);
};

/**
 * Runs the most first handler for a given method and URL.
 * @param {!remoting.MockXhr} xhr
 */
var runMatchingHandler = function(xhr) {
  for (var i = 0; i < handlers.length; i++) {
    var handler = handlers[i];
    if (handler.method == null || handler.method != xhr.params.method) {
      continue;
    }
    if (handler.urlPattern == null) {
      // Let the handler run.
    } else if (typeof handler.urlPattern == 'string') {
      if (xhr.params.url != handler.urlPattern) {
        continue;
      }
    } else {
      var regexp = /** @type {RegExp} */ (handler.urlPattern);
      if (!regexp.test(xhr.params.url)) {
        continue;
      }
    }
    if (!handler.reuse) {
      handlers.splice(i, 1);
    }
    handler.callback(xhr);
    return;
  };
  throw new Error(
      'No handler registered for ' + xhr.params.method +
      ' to '+ xhr.params.url);
};

/**
 * Activates this mock.
 */
remoting.MockXhr.activate = function() {
  console.assert(origXhr == null, 'Xhr mocking already active');
  origXhr = remoting.Xhr;
  remoting.MockXhr.Response = remoting.Xhr.Response;
  remoting['Xhr'] = remoting.MockXhr;
};

/**
 * Restores the original definiton of |remoting.Xhr|.
 */
remoting.MockXhr.restore = function() {
  console.assert(origXhr != null, 'Xhr mocking not active');
  remoting['Xhr'] = origXhr;
  origXhr = null;
  handlers = [];
};

})();

// Can't put put typedefs inside a function :-(
/**
 * @typedef {{
 *   method:?string,
 *   urlPattern:(?string|RegExp),
 *   callback:function(!remoting.MockXhr):void,
 *   reuse:boolean
 * }}
 */
remoting.MockXhr.UrlHandler;
