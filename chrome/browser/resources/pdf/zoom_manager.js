// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Abstract parent of classes that manage updating the browser
 * with zoom changes and/or updating the viewer's zoom when
 * the browser zoom changes.
 */
class ZoomManager {
  /**
   * @param {!Viewport} viewport A Viewport for which to manage zoom.
   * @param {number} initialZoom The initial browser zoom level.
   */
  constructor(viewport, initialZoom) {
    if (this.constructor === ZoomManager) {
      throw new TypeError('Instantiated abstract class: ZoomManager');
    }
    this.viewport_ = viewport;
    this.browserZoom_ = initialZoom;
  }

  /**
   * Creates the appropriate kind of zoom manager given the zoom behavior.
   *
   * @param {BrowserApi.ZoomBehavior} zoomBehavior How to manage zoom.
   * @param {!Viewport} viewport A Viewport for which to manage zoom.
   * @param {Function} setBrowserZoomFunction A function that sets the browser
   *     zoom to the provided value.
   * @param {number} initialZoom The initial browser zoom level.
   */
  static create(zoomBehavior, viewport, setBrowserZoomFunction, initialZoom) {
    switch (zoomBehavior) {
      case BrowserApi.ZoomBehavior.MANAGE:
        return new ActiveZoomManager(
            viewport, setBrowserZoomFunction, initialZoom);
      case BrowserApi.ZoomBehavior.PROPAGATE_PARENT:
        return new EmbeddedZoomManager(viewport, initialZoom);
      default:
        return new InactiveZoomManager(viewport, initialZoom);
    }
  }

  /**
   * Invoked when a browser-initiated zoom-level change occurs.
   *
   * @param {number} newZoom the zoom level to zoom to.
   */
  onBrowserZoomChange(newZoom) {}

  /**
   * Invoked when an extension-initiated zoom-level change occurs.
   */
  onPdfZoomChange() {}

  /**
   * Combines the internal pdf zoom and the browser zoom to
   * produce the total zoom level for the viewer.
   *
   * @param {number} internalZoom the zoom level internal to the viewer.
   * @return {number} the total zoom level.
   */
  applyBrowserZoom(internalZoom) {
    return this.browserZoom_ * internalZoom;
  }

  /**
   * Given a zoom level, return the internal zoom level needed to
   * produce that zoom level.
   *
   * @param {number} totalZoom the total zoom level.
   * @return {number} the zoom level internal to the viewer.
   */
  internalZoomComponent(totalZoom) {
    return totalZoom / this.browserZoom_;
  }

  /**
   * Returns whether two numbers are approximately equal.
   *
   * @param {number} a The first number.
   * @param {number} b The second number.
   */
  floatingPointEquals(a, b) {
    let MIN_ZOOM_DELTA = 0.01;
    // If the zoom level is close enough to the current zoom level, don't
    // change it. This avoids us getting into an infinite loop of zoom changes
    // due to floating point error.
    return Math.abs(a - b) <= MIN_ZOOM_DELTA;
  }
}

/**
 * InactiveZoomManager has no control over the browser's zoom
 * and does not respond to browser zoom changes.
 */
class InactiveZoomManager extends ZoomManager {}

/**
 * ActiveZoomManager controls the browser's zoom.
 */
class ActiveZoomManager extends ZoomManager {
  /**
   * Constructs a ActiveZoomManager.
   *
   * @param {!Viewport} viewport A Viewport for which to manage zoom.
   * @param {Function} setBrowserZoomFunction A function that sets the browser
   *     zoom to the provided value.
   * @param {number} initialZoom The initial browser zoom level.
   */
  constructor(viewport, setBrowserZoomFunction, initialZoom) {
    super(viewport, initialZoom);
    this.setBrowserZoomFunction_ = setBrowserZoomFunction;
    this.changingBrowserZoom_ = null;
  }

  /**
   * Invoked when a browser-initiated zoom-level change occurs.
   *
   * @param {number} newZoom the zoom level to zoom to.
   */
  onBrowserZoomChange(newZoom) {
    // If we are changing the browser zoom level, ignore any browser zoom level
    // change events. Either, the change occurred before our update and will be
    // overwritten, or the change being reported is the change we are making,
    // which we have already handled.
    if (this.changingBrowserZoom_)
      return;

    if (this.floatingPointEquals(this.browserZoom_, newZoom))
      return;

    this.browserZoom_ = newZoom;
    this.viewport_.setZoom(newZoom);
  }

  /**
   * Invoked when an extension-initiated zoom-level change occurs.
   */
  onPdfZoomChange() {
    // If we are already changing the browser zoom level in response to a
    // previous extension-initiated zoom-level change, ignore this zoom change.
    // Once the browser zoom level is changed, we check whether the extension's
    // zoom level matches the most recently sent zoom level.
    if (this.changingBrowserZoom_)
      return;

    let zoom = this.viewport_.zoom;
    if (this.floatingPointEquals(this.browserZoom_, zoom))
      return;

    this.changingBrowserZoom_ = this.setBrowserZoomFunction_(zoom).then(() => {
      this.browserZoom_ = zoom;
      this.changingBrowserZoom_ = null;

      // The extension's zoom level may have changed while the browser zoom
      // change was in progress. We call back into onPdfZoomChange to ensure
      // the browser zoom is up to date.
      this.onPdfZoomChange();
    });
  }

  /**
   * Combines the internal pdf zoom and the browser zoom to
   * produce the total zoom level for the viewer.
   *
   * @param {number} internalZoom the zoom level internal to the viewer.
   * @return {number} the total zoom level.
   */
  applyBrowserZoom(internalZoom) {
    // The internal zoom and browser zoom are changed together, so the
    // browser zoom is already applied.
    return internalZoom;
  }

  /**
   * Given a zoom level, return the internal zoom level needed to
   * produce that zoom level.
   *
   * @param {number} totalZoom the total zoom level.
   * @return {number} the zoom level internal to the viewer.
   */
  internalZoomComponent(totalZoom) {
    // The internal zoom and browser zoom are changed together, so the
    // internal zoom is the total zoom.
    return totalZoom;
  }
}

/**
 * This EmbeddedZoomManager responds to changes in the browser zoom,
 * but does not control the browser zoom.
 */
class EmbeddedZoomManager extends ZoomManager {
  /**
   * Invoked when a browser-initiated zoom-level change occurs.
   *
   * @param {number} newZoom the new browser zoom level.
   */
  onBrowserZoomChange(newZoom) {
    let oldZoom = this.browserZoom_;
    this.browserZoom_ = newZoom;
    this.viewport_.updateZoomFromBrowserChange(oldZoom);
  }
}
