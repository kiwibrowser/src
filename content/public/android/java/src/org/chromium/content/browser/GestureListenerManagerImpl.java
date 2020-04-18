// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.HapticFeedbackConstants;
import android.view.View;

import org.chromium.base.ObserverList;
import org.chromium.base.ObserverList.RewindableIterator;
import org.chromium.base.TraceEvent;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.content.browser.input.ImeAdapterImpl;
import org.chromium.content.browser.selection.SelectionPopupControllerImpl;
import org.chromium.content.browser.webcontents.WebContentsImpl;
import org.chromium.content.browser.webcontents.WebContentsUserData;
import org.chromium.content_public.browser.ContentViewCore.InternalAccessDelegate;
import org.chromium.content_public.browser.GestureListenerManager;
import org.chromium.content_public.browser.GestureStateListener;
import org.chromium.content_public.browser.WebContents;
import org.chromium.content_public.browser.WebContents.UserDataFactory;
import org.chromium.ui.base.GestureEventType;
import org.chromium.ui.base.ViewAndroidDelegate;

/**
 * Implementation of the interface {@link GestureListenerManager}. Manages
 * the {@link GestureStateListener} instances, and invokes them upon
 * notification of various events.
 * Instantiated object is held inside {@link WebContentsUserData} that is
 * managed by {@link WebContents}.
 */
@JNINamespace("content")
public class GestureListenerManagerImpl implements GestureListenerManager, WindowEventObserver {
    private static final class UserDataFactoryLazyHolder {
        private static final UserDataFactory<GestureListenerManagerImpl> INSTANCE =
                GestureListenerManagerImpl::new;
    }

    private final WebContentsImpl mWebContents;
    private final ObserverList<GestureStateListener> mListeners;
    private final RewindableIterator<GestureStateListener> mIterator;
    private ViewAndroidDelegate mViewDelegate;
    private InternalAccessDelegate mScrollDelegate;

    // The outstanding fling start events that hasn't got fling end yet. It may be > 1 because
    // onFlingEnd() is called asynchronously.
    private int mPotentiallyActiveFlingCount;

    private long mNativeGestureListenerManager;

    /**
     * Whether a touch scroll sequence is active, used to hide text selection
     * handles. Note that a scroll sequence will *always* bound a pinch
     * sequence, so this will also be true for the duration of a pinch gesture.
     */
    private boolean mIsTouchScrollInProgress;

    /**
     * @param webContents {@link WebContents} object.
     * @return {@link GestureListenerManager} object used for the give WebContents.
     *         Creates one if not present.
     */
    public static GestureListenerManagerImpl fromWebContents(WebContents webContents) {
        return WebContentsUserData.fromWebContents(
                webContents, GestureListenerManagerImpl.class, UserDataFactoryLazyHolder.INSTANCE);
    }

    public GestureListenerManagerImpl(WebContents webContents) {
        mWebContents = (WebContentsImpl) webContents;
        mListeners = new ObserverList<GestureStateListener>();
        mIterator = mListeners.rewindableIterator();
        mViewDelegate = mWebContents.getViewAndroidDelegate();
        WindowEventObserverManager.from(mWebContents).addObserver(this);
        mNativeGestureListenerManager = nativeInit(mWebContents);
    }

    /**
     * Reset the Java object in the native so this class stops receiving events.
     */
    public void reset() {
        if (mNativeGestureListenerManager != 0) nativeReset(mNativeGestureListenerManager);
    }

    private void resetGestureDetection() {
        if (mNativeGestureListenerManager != 0) {
            nativeResetGestureDetection(mNativeGestureListenerManager);
        }
    }

    public void setScrollDelegate(InternalAccessDelegate scrollDelegate) {
        mScrollDelegate = scrollDelegate;
    }

    @Override
    public void addListener(GestureStateListener listener) {
        mListeners.addObserver(listener);
    }

    @Override
    public void removeListener(GestureStateListener listener) {
        mListeners.removeObserver(listener);
    }

    @Override
    public void updateMultiTouchZoomSupport(boolean supportsMultiTouchZoom) {
        if (mNativeGestureListenerManager == 0) return;
        nativeSetMultiTouchZoomSupportEnabled(
                mNativeGestureListenerManager, supportsMultiTouchZoom);
    }

    @Override
    public void updateDoubleTapSupport(boolean supportsDoubleTap) {
        if (mNativeGestureListenerManager == 0) return;
        nativeSetDoubleTapSupportEnabled(mNativeGestureListenerManager, supportsDoubleTap);
    }

    /** Update all the listeners after touch down event occurred. */
    @CalledByNative
    private void updateOnTouchDown() {
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onTouchDown();
    }

    /** Checks if there's outstanding fling start events that hasn't got fling end yet. */
    public boolean hasPotentiallyActiveFling() {
        return mPotentiallyActiveFlingCount > 0;
    }

    // WindowEventObserver

    @Override
    public void onWindowFocusChanged(boolean gainFocus) {
        if (!gainFocus) resetGestureDetection();
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onWindowFocusChanged(gainFocus);
        }
    }

    /**
     * Update all the listeners after vertical scroll offset/extent has changed.
     * @param offset New vertical scroll offset.
     * @param extent New vertical scroll extent, or viewport height.
     */
    public void updateOnScrollChanged(int offset, int extent) {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScrollOffsetOrExtentChanged(offset, extent);
        }
    }

    /** Update all the listeners after scrolling end event occurred. */
    public void updateOnScrollEnd() {
        setTouchScrollInProgress(false);
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScrollEnded(verticalScrollOffset(), verticalScrollExtent());
        }
    }

    /**
     * Update all the listeners after min/max scale limit has changed.
     * @param minScale New minimum scale.
     * @param maxScale New maximum scale.
     */
    public void updateOnScaleLimitsChanged(float minScale, float maxScale) {
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScaleLimitsChanged(minScale, maxScale);
        }
    }

    /* Called when ongoing fling gesture needs to be reset. */
    public void resetFlingGesture() {
        if (mPotentiallyActiveFlingCount > 0) {
            onFlingEnd();
            mPotentiallyActiveFlingCount = 0;
        }
    }

    @CalledByNative
    private void onFlingEnd() {
        if (mPotentiallyActiveFlingCount > 0) mPotentiallyActiveFlingCount--;
        // Note that mTouchScrollInProgress should normally be false at this
        // point, but we reset it anyway as another failsafe.
        setTouchScrollInProgress(false);
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onFlingEndGesture(verticalScrollOffset(), verticalScrollExtent());
        }
    }

    @CalledByNative
    private void onFlingStartEventConsumed() {
        mPotentiallyActiveFlingCount++;
        setTouchScrollInProgress(false);
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onFlingStartGesture(verticalScrollOffset(), verticalScrollExtent());
        }
    }

    @CalledByNative
    private void onScrollBeginEventAck() {
        setTouchScrollInProgress(true);
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScrollStarted(verticalScrollOffset(), verticalScrollExtent());
        }
    }

    @CalledByNative
    private void onScrollEndEventAck() {
        updateOnScrollEnd();
    }

    @CalledByNative
    private void onScrollUpdateGestureConsumed() {
        SelectionPopupControllerImpl controller = getSelectionPopupController();
        if (controller != null) controller.destroyPastePopup();
        for (mIterator.rewind(); mIterator.hasNext();) {
            mIterator.next().onScrollUpdateGestureConsumed();
        }
    }

    @CalledByNative
    private void onPinchBeginEventAck() {
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onPinchStarted();
    }

    @CalledByNative
    private void onPinchEndEventAck() {
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onPinchEnded();
    }

    @CalledByNative
    private void onSingleTapEventAck(boolean consumed) {
        SelectionPopupControllerImpl controller = getSelectionPopupController();
        if (controller != null) controller.destroyPastePopup();
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onSingleTap(consumed);
    }

    @CalledByNative
    private void onLongPressAck() {
        mViewDelegate.getContainerView().performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onLongPress();
    }

    @CalledByNative
    private void resetPopupsAndInput(boolean renderProcessGone) {
        PopupController.hidePopupsAndClearSelection(mWebContents);
        resetScrollInProgress();
        if (renderProcessGone) {
            ImeAdapterImpl imeAdapter = ImeAdapterImpl.fromWebContents(mWebContents);
            if (imeAdapter != null) imeAdapter.resetAndHideKeyboard();
        }
    }

    @CalledByNative
    private void onDestroy() {
        for (mIterator.rewind(); mIterator.hasNext();) mIterator.next().onDestroyed();
        mListeners.clear();
        mNativeGestureListenerManager = 0;
    }

    /**
     * Called just prior to a tap or press gesture being forwarded to the renderer.
     */
    @SuppressWarnings("unused")
    @CalledByNative
    private boolean filterTapOrPressEvent(int type, int x, int y) {
        if (type == GestureEventType.LONG_PRESS && offerLongPressToEmbedder()) {
            return true;
        }

        TapDisambiguator tapDisambiguator = TapDisambiguator.fromWebContents(mWebContents);
        if (!tapDisambiguator.isShowing()) tapDisambiguator.setLastTouch(x, y);

        return false;
    }

    @SuppressWarnings("unused")
    @CalledByNative
    private void updateScrollInfo(float scrollOffsetX, float scrollOffsetY, float pageScaleFactor,
            float minPageScaleFactor, float maxPageScaleFactor, float contentWidth,
            float contentHeight, float viewportWidth, float viewportHeight, float topBarShownPix,
            boolean topBarChanged) {
        TraceEvent.begin("GestureListenerManagerImpl:updateScrollInfo");
        RenderCoordinatesImpl rc = mWebContents.getRenderCoordinates();

        // Adjust contentWidth/Height to be always at least as big as
        // the actual viewport (as set by onSizeChanged).
        final float deviceScale = rc.getDeviceScaleFactor();
        View containerView = mViewDelegate.getContainerView();
        contentWidth =
                Math.max(contentWidth, containerView.getWidth() / (deviceScale * pageScaleFactor));
        contentHeight = Math.max(
                contentHeight, containerView.getHeight() / (deviceScale * pageScaleFactor));

        final boolean contentSizeChanged = contentWidth != rc.getContentWidthCss()
                || contentHeight != rc.getContentHeightCss();
        final boolean scaleLimitsChanged = minPageScaleFactor != rc.getMinPageScaleFactor()
                || maxPageScaleFactor != rc.getMaxPageScaleFactor();
        final boolean pageScaleChanged = pageScaleFactor != rc.getPageScaleFactor();
        final boolean scrollChanged = pageScaleChanged || scrollOffsetX != rc.getScrollX()
                || scrollOffsetY != rc.getScrollY();

        if (contentSizeChanged || scrollChanged)
            TapDisambiguator.fromWebContents(mWebContents).hidePopup(true);

        if (scrollChanged) {
            mScrollDelegate.onScrollChanged((int) rc.fromLocalCssToPix(scrollOffsetX),
                    (int) rc.fromLocalCssToPix(scrollOffsetY), (int) rc.getScrollXPix(),
                    (int) rc.getScrollYPix());
        }

        // TODO(jinsukkim): Consider updating the info directly through RenderCoordinates.
        rc.updateFrameInfo(scrollOffsetX, scrollOffsetY, contentWidth, contentHeight, viewportWidth,
                viewportHeight, pageScaleFactor, minPageScaleFactor, maxPageScaleFactor,
                topBarShownPix);

        if (scrollChanged || topBarChanged) {
            updateOnScrollChanged(verticalScrollOffset(), verticalScrollExtent());
        }
        if (scaleLimitsChanged) updateOnScaleLimitsChanged(minPageScaleFactor, maxPageScaleFactor);
        TraceEvent.end("GestureListenerManagerImpl:updateScrollInfo");
    }

    @Override
    public boolean isScrollInProgress() {
        return mIsTouchScrollInProgress || hasPotentiallyActiveFling();
    }

    void setTouchScrollInProgress(boolean touchScrollInProgress) {
        mIsTouchScrollInProgress = touchScrollInProgress;

        // Use the active touch scroll signal for hiding. The animation movement
        // by fling will naturally hide the ActionMode by invalidating its content rect.
        getSelectionPopupController().setScrollInProgress(touchScrollInProgress);
    }

    /**
     * Reset scroll and fling accounting, notifying listeners as appropriate.
     * This is useful as a failsafe when the input stream may have been interruped.
     */
    void resetScrollInProgress() {
        if (!isScrollInProgress()) return;

        final boolean touchScrollInProgress = mIsTouchScrollInProgress;
        setTouchScrollInProgress(false);
        if (touchScrollInProgress) updateOnScrollEnd();
        resetFlingGesture();
    }

    private SelectionPopupControllerImpl getSelectionPopupController() {
        return SelectionPopupControllerImpl.fromWebContents(mWebContents);
    }

    /**
     * Offer a long press gesture to the embedding View, primarily for WebView compatibility.
     *
     * @return true if the embedder handled the event.
     */
    private boolean offerLongPressToEmbedder() {
        return mViewDelegate.getContainerView().performLongClick();
    }

    private int verticalScrollOffset() {
        return mWebContents.getRenderCoordinates().getScrollYPixInt();
    }

    private int verticalScrollExtent() {
        return mWebContents.getRenderCoordinates().getLastFrameViewportHeightPixInt();
    }

    private native long nativeInit(WebContentsImpl webContents);
    private native void nativeReset(long nativeGestureListenerManager);
    private native void nativeResetGestureDetection(long nativeGestureListenerManager);
    private native void nativeSetDoubleTapSupportEnabled(
            long nativeGestureListenerManager, boolean enabled);
    private native void nativeSetMultiTouchZoomSupportEnabled(
            long nativeGestureListenerManager, boolean enabled);
}
