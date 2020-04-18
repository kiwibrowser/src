// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

/**
 * An interface that is notified of events and state changes related to gesture processing
 * from content layer.
 */
public interface GestureStateListener {
    /**
     * Called when the pinch gesture starts.
     */
    default public void onPinchStarted() {}

    /**
     * Called when the pinch gesture ends.
     */
    default public void onPinchEnded() {}

    /**
     * Called when a fling starts.
     */
    default public void onFlingStartGesture(int scrollOffsetY, int scrollExtentY) {}

    /**
     * Called when a fling has ended.
     */
    default public void onFlingEndGesture(int scrollOffsetY, int scrollExtentY) {}

    /**
     * Called to indicate that a scroll update gesture had been consumed by the page.
     * This callback is called whenever any layer is scrolled (like a frame or div). It is
     * not called when a JS touch handler consumes the event (preventDefault), it is not called
     * for JS-initiated scrolling.
     */
    default public void onScrollUpdateGestureConsumed() {}

    /**
     * Called when a scroll gesture has started.
     */
    default public void onScrollStarted(int scrollOffsetY, int scrollExtentY) {}

    /**
     * Called when a scroll gesture has stopped.
     */
    default public void onScrollEnded(int scrollOffsetY, int scrollExtentY) {}

    /**
     * Called when the min or max scale factor may have been changed.
     */
    default public void onScaleLimitsChanged(float minPageScaleFactor, float maxPageScaleFactor) {}

    /**
     * Called when the scroll offsets or extents may have changed.
     */
    default public void onScrollOffsetOrExtentChanged(int scrollOffsetY, int scrollExtentY) {}

    /**
     * Called at the beginning of any kind of touch event when the user's finger first touches down
     * onto the screen.  The resulting gesture may be a single tap, long-press, or scroll.
     */
    default public void onTouchDown() {}

    /**
     * Called after a single-tap gesture event was dispatched to the renderer,
     * indicating whether or not the gesture was consumed.
     */
    default public void onSingleTap(boolean consumed) {}

    /**
     * Called after a single-tap gesture event was processed by the renderer,
     * but was not handled.
     */
    default public void onShowUnhandledTapUIIfNeeded(int x, int y) {}

    /**
     * Called when the gesture source loses window focus.
     */
    default public void onWindowFocusChanged(boolean hasWindowFocus) {}

    /**
     * Called when a long press gesture event was processed by the rendereer.
     */
    default public void onLongPress() {}

    /**
     * Called when the gesture source is being destroyed.
     */
    default public void onDestroyed() {}
}
