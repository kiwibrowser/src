// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests(function() {
  'use strict';

  class MockViewport {
    constructor() {
      this.zooms = [];
      this.zoom = 1;
      this.browserOnlyZoomChange = false;
    }

    setZoom(zoom) {
      this.zooms.push(zoom);
      this.zoom = zoom;
    }

    updateZoomFromBrowserChange(oldBrowserZoom) {
      this.browserOnlyZoomChange = true;
    }
  }

  /**
   * A mock implementation of the function used by ZoomManager to set the
   * browser zoom level.
   */
  class MockBrowserZoomSetter {
    constructor() {
      this.zoom = 1;
      this.started = false;
    }

    /**
     * The function implementing setBrowserZoomFunction.
     */
    setBrowserZoom(zoom) {
      chrome.test.assertFalse(this.started);

      this.zoom = zoom;
      this.started = true;
      return new Promise(function(resolve, reject) {
        this.resolve_ = resolve;
      }.bind(this));
    }

    /**
     * Resolves the promise returned by a call to setBrowserZoom.
     */
    complete() {
      this.resolve_();
      this.started = false;
    }
  };

  return [
    function testZoomChange() {
      let viewport = new MockViewport();
      let browserZoomSetter = new MockBrowserZoomSetter();
      let zoomManager = ZoomManager.create(
          BrowserApi.ZoomBehavior.MANAGE, viewport,
          browserZoomSetter.setBrowserZoom.bind(browserZoomSetter), 1);
      viewport.zoom = 2;
      zoomManager.onPdfZoomChange();
      chrome.test.assertEq(2, browserZoomSetter.zoom);
      chrome.test.assertTrue(browserZoomSetter.started);
      chrome.test.succeed();
    },

    function testBrowserZoomChange() {
      let viewport = new MockViewport();
      let zoomManager = ZoomManager.create(
          BrowserApi.ZoomBehavior.MANAGE, viewport, chrome.test.fail, 1);
      zoomManager.onBrowserZoomChange(3);
      chrome.test.assertEq(1, viewport.zooms.length);
      chrome.test.assertEq(3, viewport.zooms[0]);
      chrome.test.assertEq(3, viewport.zoom);
      chrome.test.succeed();
    },

    function testBrowserZoomChangeEmbedded() {
      let viewport = new MockViewport();
      let zoomManager = ZoomManager.create(
          BrowserApi.ZoomBehavior.PROPAGATE_PARENT, viewport,
          function() { return Promise.reject(); }, 1);

      // Zooming in the browser should not overwrite the viewport's zoom,
      // but be applied seperately.
      viewport.zoom = 2;
      zoomManager.onBrowserZoomChange(3);
      chrome.test.assertEq(2, viewport.zoom);
      chrome.test.assertTrue(viewport.browserOnlyZoomChange);

      chrome.test.succeed();
    },

    function testSmallZoomChange() {
      let viewport = new MockViewport();
      let browserZoomSetter = new MockBrowserZoomSetter();
      let zoomManager = ZoomManager.create(
          BrowserApi.ZoomBehavior.MANAGE, viewport,
          browserZoomSetter.setBrowserZoom.bind(browserZoomSetter), 2);
      viewport.zoom = 2.0001;
      zoomManager.onPdfZoomChange();
      chrome.test.assertEq(1, browserZoomSetter.zoom);
      chrome.test.assertFalse(browserZoomSetter.started);
      chrome.test.succeed();
    },

    function testSmallBrowserZoomChange() {
      let viewport = new MockViewport();
      let zoomManager = ZoomManager.create(
          BrowserApi.ZoomBehavior.MANAGE, viewport, chrome.test.fail, 1);
      zoomManager.onBrowserZoomChange(0.999);
      chrome.test.assertEq(0, viewport.zooms.length);
      chrome.test.assertEq(1, viewport.zoom);
      chrome.test.succeed();
    },

    function testMultiplePdfZoomChanges() {
      let viewport = new MockViewport();
      let browserZoomSetter = new MockBrowserZoomSetter();
      let zoomManager = ZoomManager.create(
          BrowserApi.ZoomBehavior.MANAGE, viewport,
          browserZoomSetter.setBrowserZoom.bind(browserZoomSetter), 1);
      viewport.zoom = 2;
      zoomManager.onPdfZoomChange();
      viewport.zoom = 3;
      zoomManager.onPdfZoomChange();
      chrome.test.assertTrue(browserZoomSetter.started);
      chrome.test.assertEq(2, browserZoomSetter.zoom);
      browserZoomSetter.complete();
      Promise.resolve().then(function() {
        chrome.test.assertTrue(browserZoomSetter.started);
        chrome.test.assertEq(3, browserZoomSetter.zoom);
        chrome.test.succeed();
      });
    },

    function testMultipleBrowserZoomChanges() {
      let viewport = new MockViewport();
      let zoomManager = ZoomManager.create(
          BrowserApi.ZoomBehavior.MANAGE, viewport, chrome.test.fail, 1);
      zoomManager.onBrowserZoomChange(2);
      zoomManager.onBrowserZoomChange(3);
      chrome.test.assertEq(2, viewport.zooms.length);
      chrome.test.assertEq(2, viewport.zooms[0]);
      chrome.test.assertEq(3, viewport.zooms[1]);
      chrome.test.assertEq(3, viewport.zoom);
      chrome.test.succeed();
    },
  ];
}());
