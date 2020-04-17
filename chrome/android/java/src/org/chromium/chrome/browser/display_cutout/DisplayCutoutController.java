// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.display_cutout;

import android.graphics.Rect;
import android.support.annotation.Nullable;
import android.support.annotation.VisibleForTesting;
import android.view.Window;
import android.view.WindowManager.LayoutParams;

import org.chromium.blink.mojom.ViewportFit;
import org.chromium.chrome.browser.InsetObserverView;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.content_public.browser.WebContentsObserver;

/**
 * Controls the display cutout state for the tab.
 */
public class DisplayCutoutController implements InsetObserverView.WindowInsetObserver {
    /** These are the property names of the different cutout mode states. */
    private static final String VIEWPORT_FIT_AUTO = "LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT";
    private static final String VIEWPORT_FIT_CONTAIN = "LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER";
    private static final String VIEWPORT_FIT_COVER = "LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES";

    /** The tab that this controller belongs to. */
    private Tab mTab;

    /** The current viewport fit value. */
    private @WebContentsObserver.ViewportFitType int mViewportFit = ViewportFit.AUTO;

    /**
     * The current {@link InsetObserverView} that we are attached to. This can be null if we
     * have not attached to an activity.
     */
    private @Nullable InsetObserverView mInsetObserverView;

    /** Listens to various Tab events. */
    private final TabObserver mTabObserver = new EmptyTabObserver() {
        @Override
        public void onShown(Tab tab) {
            assert tab == mTab;

            // Force a layout update if we are now being shown.
            maybeUpdateLayout();
        }

        @Override
        public void onInteractabilityChanged(boolean interactable) {
            // Force a layout update if the tab is now in the foreground.
            maybeUpdateLayout();
        }

        @Override
        public void onDestroyed(Tab tab) {
            assert tab == mTab;
            destroy();
        }

        @Override
        public void onActivityAttachmentChanged(Tab tab, boolean isAttached) {
            assert tab == mTab;

            if (isAttached) {
                maybeAddInsetObserver();
            } else {
                maybeRemoveInsetObserver();
            }
        }
    };

    /**
     * Constructs a new DisplayCutoutController for a specific tab.
     * @param tab The tab that this controller belongs to.
     */
    public DisplayCutoutController(Tab tab) {
        mTab = tab;

        tab.addObserver(mTabObserver);
        maybeAddInsetObserver();
    }

    /**
     * Add an observer to {@link InsetObserverView} if we have not already added
     * one.
     */
    private void maybeAddInsetObserver() {
        if (mInsetObserverView != null || mTab.getActivity() == null) return;

        mInsetObserverView = mTab.getActivity().getInsetObserverView();
      
        if (mInsetObserverView == null) return;
        mInsetObserverView.addObserver(this);
    }

    /**
     * Remove the observer added to {@link InsetObserverView} if we have added
     * one.
     */
    private void maybeRemoveInsetObserver() {
        if (mInsetObserverView == null) return;

        mInsetObserverView.removeObserver(this);
        mInsetObserverView = null;
    }

    /**
     * Cleans up all internal state.
     */
    private void destroy() {
        mTab.removeObserver(mTabObserver);
        maybeRemoveInsetObserver();
    }

    /**
     * Set the viewport fit value for the tab.
     * @param value The new viewport fit value.
     */
    public void setViewportFit(@WebContentsObserver.ViewportFitType int value) {
        if (value == mViewportFit) return;

        mViewportFit = value;
        maybeUpdateLayout();
    }

    /** Implements {@link WindowInsetsObserver}. */
    @Override
    public void onSafeAreaChanged(Rect area) {
        // TODO(beccahughes): Send this value to Blink.
    }

    @Override
    public void onInsetChanged(int left, int top, int right, int bottom) {}

    /**
     * Converts a {@link ViewportFit} value into the Android P+ equivalent.
     * @returns String containing the {@link LayoutParams} field name of the
     *     equivalent value.
     */
    @VisibleForTesting
    protected String getDisplayCutoutMode() {
        // If we are not interactable then force the default mode.
        if (!mTab.isUserInteractable()) return VIEWPORT_FIT_AUTO;

        switch (mViewportFit) {
            case ViewportFit.CONTAIN:
                return VIEWPORT_FIT_CONTAIN;
            case ViewportFit.COVER:
                return VIEWPORT_FIT_COVER;
            case ViewportFit.AUTO:
            default:
                return VIEWPORT_FIT_AUTO;
        }
    }

    /** Updates the layout based on internal state. */
    @VisibleForTesting
    protected void maybeUpdateLayout() {
        try {
            Window window = mTab.getActivity().getWindow();
            LayoutParams attributes = window.getAttributes();

            LayoutParams.class.getDeclaredField("layoutInDisplayCutoutMode")
                    .setInt(attributes,
                            LayoutParams.class.getDeclaredField(getDisplayCutoutMode())
                                    .getInt(null));
            window.setAttributes(attributes);
        } catch (Exception ex) {
            // API is not available.
            return;
        }
    }
}
