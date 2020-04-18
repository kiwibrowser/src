// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Enum for WebDriver status codes.
 * @enum {number}
 */
var StatusCode = {
  STALE_ELEMENT_REFERENCE: 10,
  UNKNOWN_ERROR: 13,
};

/**
 * Enum for node types.
 * @enum {number}
 */
var NodeType = {
  ELEMENT: 1,
  DOCUMENT: 9,
};

/**
 * Dictionary key to use for holding an element ID.
 * @const
 * @type {string}
 */
var ELEMENT_KEY = 'ELEMENT';

/**
 * True if using W3C Element references.
 * @const
 * @type {boolean}
 */
var w3cEnabled = false;

/**
 * True if shadow dom is enabled.
 * @const
 * @type {boolean}
 */
var SHADOW_DOM_ENABLED = typeof ShadowRoot === 'function';

/**
 * Generates a unique ID to identify an element.
 * @void
 * @return {string} Randomly generated ID.
 */
function generateUUID() {
  var array = new Uint8Array(16);
  window.crypto.getRandomValues(array);
  array[6] = 0x40 | (array[6] & 0x0f);
  array[8] = 0x80 | (array[8] & 0x3f);

  var UUID = "";
  for (var i = 0; i < 16; i++) {
    var temp = array[i].toString(16);
    if (temp.length < 2)
      temp = "0" + temp;
    UUID += temp;
    if (i == 3 || i == 5 || i == 7 || i == 9)
      UUID += "-";
  }
  return UUID;
};

/**
 * A cache which maps IDs <-> cached objects for the purpose of identifying
 * a script object remotely. Uses UUIDs for identification.
 * @constructor
 */
function CacheWithUUID() {
  this.cache_ = {};
}

CacheWithUUID.prototype = {
  /**
   * Stores a given item in the cache and returns a unique UUID.
   *
   * @param {!Object} item The item to store in the cache.
   * @return {number} The UUID for the cached item.
   */
  storeItem: function(item) {
    for (var i in this.cache_) {
      if (item == this.cache_[i])
        return i;
    }
    var id = generateUUID();
    this.cache_[id] = item;
    return id;
  },

  /**
   * Retrieves the cached object for the given ID.
   *
   * @param {number} id The ID for the cached item to retrieve.
   * @return {!Object} The retrieved item.
   */
  retrieveItem: function(id) {
    var item = this.cache_[id];
    if (item)
      return item;
    var error = new Error('not in cache');
    error.code = StatusCode.STALE_ELEMENT_REFERENCE;
    error.message = 'element is not attached to the page document';
    throw error;
  },

  /**
   * Clears stale items from the cache.
   */
  clearStale: function() {
    for (var id in this.cache_) {
      var node = this.cache_[id];
      if (!this.isNodeReachable_(node))
        delete this.cache_[id];
    }
  },

  /**
    * @private
    * @param {!Node} node The node to check.
    * @return {boolean} If the nodes is reachable.
    */
  isNodeReachable_: function(node) {
    var nodeRoot = getNodeRootThroughAnyShadows(node);
    return (nodeRoot == document);
  }


};

/**
 * A cache which maps IDs <-> cached objects for the purpose of identifying
 * a script object remotely.
 * @constructor
 */
function Cache() {
  this.cache_ = {};
  this.nextId_ = 1;
  this.idPrefix_ = Math.random().toString();
}

Cache.prototype = {

  /**
   * Stores a given item in the cache and returns a unique ID.
   *
   * @param {!Object} item The item to store in the cache.
   * @return {number} The ID for the cached item.
   */
  storeItem: function(item) {
    for (var i in this.cache_) {
      if (item == this.cache_[i])
        return i;
    }
    var id = this.idPrefix_  + '-' + this.nextId_;
    this.cache_[id] = item;
    this.nextId_++;
    return id;
  },

  /**
   * Retrieves the cached object for the given ID.
   *
   * @param {number} id The ID for the cached item to retrieve.
   * @return {!Object} The retrieved item.
   */
  retrieveItem: function(id) {
    var item = this.cache_[id];
    if (item)
      return item;
    var error = new Error('not in cache');
    error.code = StatusCode.STALE_ELEMENT_REFERENCE;
    error.message = 'element is not attached to the page document';
    throw error;
  },

  /**
   * Clears stale items from the cache.
   */
  clearStale: function() {
    for (var id in this.cache_) {
      var node = this.cache_[id];
      if (!this.isNodeReachable_(node))
        delete this.cache_[id];
    }
  },

  /**
    * @private
    * @param {!Node} node The node to check.
    * @return {boolean} If the nodes is reachable.
    */
  isNodeReachable_: function(node) {
    var nodeRoot = getNodeRootThroughAnyShadows(node);
    return (nodeRoot == document);
  }
};

/**
 * Returns the root element of the node.  Found by traversing parentNodes until
 * a node with no parent is found.  This node is considered the root.
 * @param {?Node} node The node to find the root element for.
 * @return {?Node} The root node.
 */
function getNodeRoot(node) {
  while (node && node.parentNode) {
    node = node.parentNode;
  }
  return node;
}

/**
 * Returns the root element of the node, jumping up through shadow roots if
 * any are found.
 */
function getNodeRootThroughAnyShadows(node) {
  var root = getNodeRoot(node);
  while (SHADOW_DOM_ENABLED && root instanceof ShadowRoot) {
    root = getNodeRoot(root.host);
  }
  return root;
}

/**
 * Returns the global object cache for the page.
 * @param {Document=} opt_doc The document whose cache to retrieve. Defaults to
 *     the current document.
 * @return {!Cache} The page's object cache.
 */
function getPageCache(opt_doc, opt_w3c) {
  var doc = opt_doc || document;
  var w3c = opt_w3c || false;
  var key = '$cdc_asdjflasutopfhvcZLmcfl_';
  if (w3c) {
    if (!(key in doc))
      doc[key] = new CacheWithUUID();
    return doc[key];
  } else {
    if (!(key in doc))
      doc[key] = new Cache();
    return doc[key];
  }
}

/**
 * Wraps the given value to be transmitted remotely by converting
 * appropriate objects to cached object IDs.
 *
 * @param {*} value The value to wrap.
 * @return {*} The wrapped value.
 */
function wrap(value) {
  // As of crrev.com/1316933002, typeof() for some elements will return
  // 'function', not 'object'. So we need to check for both non-null objects, as
  // well Elements that also happen to be callable functions (e.g. <embed> and
  // <object> elements). Note that we can not use |value instanceof Object| here
  // since this does not work with frames/iframes, for example
  // frames[0].document.body instanceof Object == false even though
  // typeof(frames[0].document.body) == 'object'.
  if ((typeof(value) == 'object' && value != null) ||
      (typeof(value) == 'function' && value.nodeName &&
       value.nodeType == NodeType.ELEMENT)) {
    var nodeType = value['nodeType'];
    if (nodeType == NodeType.ELEMENT || nodeType == NodeType.DOCUMENT
        || (SHADOW_DOM_ENABLED && value instanceof ShadowRoot)) {
      var wrapped = {};
      var root = getNodeRootThroughAnyShadows(value);
      wrapped[ELEMENT_KEY] = getPageCache(root, w3cEnabled).storeItem(value);
      return wrapped;
    }

    var obj;
    if (typeof(value.length) == 'number') {
      obj = [];
      for (var i = 0; i < value.length; i++)
        obj[i] = wrap(value[i]);
    } else {
      obj = {};
      for (var prop in value)
        obj[prop] = wrap(value[prop]);
    }
    return obj;
  }
  return value;
}

/**
 * Unwraps the given value by converting from object IDs to the cached
 * objects.
 *
 * @param {*} value The value to unwrap.
 * @param {Cache} cache The cache to retrieve wrapped elements from.
 * @return {*} The unwrapped value.
 */
function unwrap(value, cache) {
  if (typeof(value) == 'object' && value != null) {
    if (ELEMENT_KEY in value)
      return cache.retrieveItem(value[ELEMENT_KEY]);

    var obj;
    if (typeof(value.length) == 'number') {
      obj = [];
      for (var i = 0; i < value.length; i++)
        obj[i] = unwrap(value[i], cache);
    } else {
      obj = {};
      for (var prop in value)
        obj[prop] = unwrap(value[prop], cache);
    }
    return obj;
  }
  return value;
}

/**
 * Calls a given function and returns its value.
 *
 * The inputs to and outputs of the function will be unwrapped and wrapped
 * respectively, unless otherwise specified. This wrapping involves converting
 * between cached object reference IDs and actual JS objects. The cache will
 * automatically be pruned each call to remove stale references.
 *
 * @param  {Array<string>} shadowHostIds The host ids of the nested shadow
 *     DOMs the function should be executed in the context of.
 * @param {function(...[*]) : *} func The function to invoke.
 * @param {!Array<*>} args The array of arguments to supply to the function,
 *     which will be unwrapped before invoking the function.
 * @param {boolean} w3c Whether to return a W3C compliant element reference.
 * @param {boolean=} opt_unwrappedReturn Whether the function's return value
 *     should be left unwrapped.
 * @return {*} An object containing a status and value property, where status
 *     is a WebDriver status code and value is the wrapped value. If an
 *     unwrapped return was specified, this will be the function's pure return
 *     value.
 */
function callFunction(shadowHostIds, func, args, w3c, opt_unwrappedReturn) {
  if (w3c) {
    w3cEnabled = true;
    ELEMENT_KEY = 'element-6066-11e4-a52e-4f735466cecf';

  }
  var cache = getPageCache(null, w3cEnabled);
  cache.clearStale();
  if (shadowHostIds && SHADOW_DOM_ENABLED) {
    for (var i = 0; i < shadowHostIds.length; i++) {
      var host = cache.retrieveItem(shadowHostIds[i]);
      // TODO(zachconrad): Use the olderShadowRoot API when available to check
      // all of the shadow roots.
      cache = getPageCache(host.webkitShadowRoot, w3cEnabled);
      cache.clearStale();
    }
  }

  if (opt_unwrappedReturn)
    return func.apply(null, unwrap(args, cache));

  var status = 0;
  try {
    var returnValue = wrap(func.apply(null, unwrap(args, cache)));
  } catch (error) {
    status = error.code || StatusCode.UNKNOWN_ERROR;
    var returnValue = error.message;
  }
  return {
      status: status,
      value: returnValue
  }
}
