// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Deferred resource loader for OOBE/Login screens.
 */

cr.define('cr.ui.login.ResourceLoader', function() {
  'use strict';

  // Deferred assets.
  var ASSETS = {};

  /**
   * Register assets for deferred loading.  When the bundle is loaded
   * assets will be added to the current page's DOM: <link> and <script>
   * tags pointing to the CSS and JavaScript will be added to the
   * <head>, and HTML will be appended to a specified element.
   *
   * @param {Object} desc Descriptor for the asset bundle
   * @param {string} desc.id Unique identifier for the asset bundle.
   * @param {Array=} desc.js URLs containing JavaScript sources.
   * @param {Array=} desc.css URLs containing CSS rules.
   * @param {Array<Object>=} desc.html Descriptors for HTML fragments,
   * each of which has a 'url' property and a 'targetID' property that
   * specifies the node under which the HTML should be appended. If 'targetID'
   * is null, then the fetched body will be appended to document.body.
   *
   * Example:
   *   ResourceLoader.registerAssets({
   *     id: 'bundle123',
   *     js: ['//foo.com/src.js', '//bar.com/lib.js'],
   *     css: ['//foo.com/style.css'],
   *     html: [{ url: '//foo.com/tmpls.html' targetID: 'tmpls'}]
   *   });
   *
   * Note: to avoid cross-site requests, all HTML assets must be served
   * from the same host as the rendered page.  For example, if the
   * rendered page is served as chrome://oobe, then all the HTML assets
   * must be served as chrome://oobe/path/to/something.html.
   */
  function registerAssets(desc) {
    var html = desc.html || [];
    var css = desc.css || [];
    var js = desc.js || [];
    ASSETS[desc.id] = {
      html: html, css: css, js: js,
      loaded: false,
      count: html.length + css.length + js.length
    };
  }

  /**
   * Determines whether an asset bundle is defined for a specified id.
   * @param {string} id The possible identifier.
   */
  function hasDeferredAssets(id) {
    return id in ASSETS;
  }

  /**
   * Determines whether an asset bundle has already been loaded.
   * @param {string} id The identifier of the asset bundle.
   */
  function alreadyLoadedAssets(id) {
    return hasDeferredAssets(id) && ASSETS[id].loaded;
  }

  /**
   * Load a stylesheet into the current document.
   * @param {string} id Identifier of the stylesheet's asset bundle.
   * @param {string} url The URL resolving to a stylesheet.
   */
  function loadCSS(id, url) {
    var link = document.createElement('link');
    link.setAttribute('rel', 'stylesheet');
    link.setAttribute('href', url);
    link.onload = resourceLoaded.bind(null, id);
    document.head.appendChild(link);
  }

  /**
   * Load a script into the current document.
   * @param {string} id Identifier of the script's asset bundle.
   * @param {string} url The URL resolving to a script.
   */
  function loadJS(id, url) {
    var script = document.createElement('script');
    script.src = url;
    script.onload = resourceLoaded.bind(null, id);
    document.head.appendChild(script);
  }

  /**
   * Move DOM nodes from one parent element to another.
   * @param {HTMLElement} from Element whose children should be moved.
   * @param {HTMLElement} to Element to which nodes should be appended.
   */
  function moveNodes(from, to) {
    Array.prototype.forEach.call(from.children, function(child) {
      to.appendChild(document.importNode(child, true));
    });
  }

  /**
   * Tests whether an XMLHttpRequest has successfully finished loading.
   * @param {string} url The requested URL.
   * @param {XMLHttpRequest} xhr The XHR object.
   */
  function isSuccessful(url, xhr) {
    var fileURL = /^file:\/\//;
    return xhr.readyState == 4 &&
        (xhr.status == 200 || fileURL.test(url) && xhr.status == 0);
  }

  /*
   * Load a chunk of HTML into the current document.
   * @param {string} id Identifier of the page's asset bundle.
   * @param {Object} html Descriptor of the HTML to fetch.
   * @param {string} html.url The URL resolving to some HTML.
   * @param {string?} html.targetID The element ID to which the retrieved
   * HTML nodes should be appended. If null, then the elements will be appended
   * to document.body instead.
   */
  function loadHTML(id, html) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', html.url);
    xhr.onreadystatechange = function() {
      if (isSuccessful(html.url, xhr)) {
        moveNodes(this.responseXML.head, document.head);
        moveNodes(this.responseXML.body, $(html.targetID) || document.body);

        resourceLoaded(id);
      }
    };
    xhr.responseType = 'document';
    xhr.send();
  }

  /**
   * Record that a resource has been loaded for an asset bundle.  When
   * all the resources have been loaded the callback that was specified
   * in the loadAssets call is invoked.
   * @param {string} id Identifier of the asset bundle.
   */
  function resourceLoaded(id) {
    var assets = ASSETS[id];
    assets.count--;
    if (assets.count == 0)
      finishedLoading(id);
  }

  /**
   * Finishes loading an asset bundle.
   * @param {string} id Identifier of the asset bundle.
   */
  function finishedLoading(id) {
    var assets = ASSETS[id];
    console.log('Finished loading asset bundle ' + id);
    assets.loaded = true;
    window.setTimeout(function() {
      assets.callback();
      chrome.send('screenAssetsLoaded', [id]);
    }, 0);
  }

  /**
   * Load an asset bundle, invoking the callback when finished.
   * @param {string} id Identifier for the asset bundle to load.
   * @param {function()=} callback Function to invoke when done loading.
   */
  function loadAssets(id, callback) {
    var assets = ASSETS[id];
    assets.callback = callback || function() {};
    console.log('Loading asset bundle ' + id);
    if (alreadyLoadedAssets(id))
      console.warn('asset bundle', id, 'already loaded!');
    if (assets.count == 0) {
      finishedLoading(id);
    } else {
      assets.css.forEach(loadCSS.bind(null, id));
      assets.js.forEach(loadJS.bind(null, id));
      assets.html.forEach(loadHTML.bind(null, id));
    }
  }

  /**
   * Load an asset bundle after the document has been loaded and Chrome is idle.
   * @param {string} id Identifier for the asset bundle to load.
   * @param {function()=} callback Function to invoke when done loading.
   * @param {number=} opt_idleTimeoutMs The maximum amount of time to wait for
   * an idle notification.
   */
  function loadAssetsOnIdle(id, callback, opt_idleTimeoutMs) {
    opt_idleTimeoutMs = opt_idleTimeoutMs || 250;

    var loadOnIdle = function() {
      window.requestIdleCallback(function() {
        loadAssets(id, callback);
      }, { timeout: opt_idleTimeoutMs });
    };

    if (document.readyState == 'loading') {
      window.addEventListener('DOMContentLoaded', loadOnIdle);
    } else {
      // DOMContentLoaded has already been called if document.readyState is
      // 'interactive' or 'complete', so invoke the callback immediately.
      loadOnIdle();
    }
  }

  /**
   * Wait until the element with the given |id| has finished its layout,
   * specifically, after it has an offsetHeight > 0.
   * @param {string|function()} selector Identifier of the element to wait
   * or a callback function to obtain element to wait for.
   * @param {function()} callback Function to invoke when done loading.
   */
  function waitUntilLayoutComplete(selector, callback) {
    if (typeof selector == 'string') {
      var id = selector;
      selector = function() { return $(id) };
    }

    var doWait = function() {
      var element = selector();

      if (!element || !element.offsetHeight) {
        requestAnimationFrame(doWait);
        return;
      }

      callback(element);
    };

    requestAnimationFrame(doWait);
  }

  return {
    alreadyLoadedAssets: alreadyLoadedAssets,
    hasDeferredAssets: hasDeferredAssets,
    loadAssets: loadAssets,
    loadAssetsOnIdle: loadAssetsOnIdle,
    waitUntilLayoutComplete: waitUntilLayoutComplete,
    registerAssets: registerAssets
  };
});
