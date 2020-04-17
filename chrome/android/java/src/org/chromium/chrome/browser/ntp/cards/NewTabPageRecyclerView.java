// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Region;
import android.view.LayoutInflater;
import android.view.View;

import org.chromium.base.ContextUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.NewTabPage.FakeboxDelegate;
import org.chromium.chrome.browser.ntp.NewTabPageLayout;
import org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.ui.base.DeviceFormFactor;

/**
 * Simple wrapper on top of a RecyclerView that will acquire focus when tapped.  Ensures the
 * New Tab page receives focus when clicked.
 */
public class NewTabPageRecyclerView extends SuggestionsRecyclerView {
    private final int mToolbarHeight;
    private final int mSearchBoxTransitionLength;

    /** View used to calculate the position of the cards' snap point. */
    private final NewTabPageLayout mAboveTheFoldView;

    /** Whether the location bar is shown as part of the UI. */
    private boolean mContainsLocationBar;

    /** The fake search box delegate used for determining if the url bar has focus. */
    private FakeboxDelegate mFakeboxDelegate;

    public NewTabPageRecyclerView(Context context) {
        super(context);

        Resources res = getContext().getResources();
        mToolbarHeight = res.getDimensionPixelSize(R.dimen.toolbar_height_no_shadow)
                + res.getDimensionPixelSize(R.dimen.toolbar_progress_bar_height);
        mSearchBoxTransitionLength =
                res.getDimensionPixelSize(R.dimen.ntp_search_box_transition_length);

        mAboveTheFoldView = (NewTabPageLayout) LayoutInflater.from(getContext())
                                    .inflate(R.layout.new_tab_page_layout, this, false);
        if (ContextUtils.getAppSharedPreferences().getBoolean("enable_bottom_toolbar", false)) {
             mAboveTheFoldView.setPadding(0, 0, 0, 0);
        }
    }

    public NewTabPageLayout getAboveTheFoldView() {
        return mAboveTheFoldView;
    }

    public void setContainsLocationBar(boolean containsLocationBar) {
        mContainsLocationBar = containsLocationBar;
    }

    public void setFakeboxDelegate(FakeboxDelegate fakeboxDelegate) {
        mFakeboxDelegate = fakeboxDelegate;
    }

    @Override
    protected boolean getTouchEnabled() {
        if (!super.getTouchEnabled()) return false;

        if (DeviceFormFactor.isTablet()) return true;

        // The RecyclerView should not accept touch events while the URL bar is focused. This
        // prevents the RecyclerView from requesting focus during the URL focus animation, which
        // would cause the focus animation to be canceled. See https://crbug.com/798084.
        return mFakeboxDelegate == null || !mFakeboxDelegate.isUrlBarFocused();
    }

    /**
     * Returns the approximate adapter position that the user has scrolled to. The purpose of this
     * value is that it can be stored and later retrieved to restore a scroll position that is
     * familiar to the user, showing (part of) the same content the user was previously looking at.
     * This position is valid for that purpose regardless of device orientation changes. Note that
     * if the underlying data has changed in the meantime, different content would be shown for this
     * position.
     */
    public int getScrollPosition() {
        return getLinearLayoutManager().findFirstVisibleItemPosition();
    }

    /**
     * Calculates the position to scroll to in order to move out of a region where the RecyclerView
     * should not stay at rest.
     * @param currentScroll the current scroll position.
     * @param regionStart the beginning of the region to scroll out of.
     * @param regionEnd the end of the region to scroll out of.
     * @param flipPoint the threshold used to decide which bound of the region to scroll to.
     * @return the position to scroll to.
     */
    private static int calculateSnapPositionForRegion(
            int currentScroll, int regionStart, int regionEnd, int flipPoint) {
        assert regionStart <= flipPoint;
        assert flipPoint <= regionEnd;

        if (currentScroll < regionStart || currentScroll > regionEnd) return currentScroll;

        if (currentScroll < flipPoint) {
            return regionStart;
        } else {
            return regionEnd;
        }
    }

    /**
     * If the RecyclerView is currently scrolled to between regionStart and regionEnd, smooth scroll
     * out of the region to the nearest edge.
     */
    private static int calculateSnapPositionForRegion(
            int currentScroll, int regionStart, int regionEnd) {
        return calculateSnapPositionForRegion(
                currentScroll, regionStart, regionEnd, (regionStart + regionEnd) / 2);
    }

    /**
     * Snaps the scroll point of the RecyclerView to prevent the user from scrolling to midway
     * through a transition and to allow peeking card behaviour.
     */
    public void snapScroll(View fakeBox, int parentHeight) {
        int initialScroll = computeVerticalScrollOffset();

        int scrollTo = calculateSnapPosition(initialScroll, fakeBox, parentHeight);

        // Calculating the snap position should be idempotent.
        assert scrollTo == calculateSnapPosition(scrollTo, fakeBox, parentHeight);

        smoothScrollBy(0, scrollTo - initialScroll);
    }

    @VisibleForTesting
    int calculateSnapPosition(int scrollPosition, View fakeBox, int parentHeight) {
        if (mContainsLocationBar) {
            // Snap scroll to prevent only part of the toolbar from showing.
            scrollPosition = calculateSnapPositionForRegion(scrollPosition, 0, mToolbarHeight);

            // Snap scroll to prevent resting in the middle of the omnibox transition.
            int fakeBoxUpperBound = fakeBox.getTop() + fakeBox.getPaddingTop();
            scrollPosition = calculateSnapPositionForRegion(scrollPosition,
                    fakeBoxUpperBound - mSearchBoxTransitionLength, fakeBoxUpperBound);
        }

        return scrollPosition;
    }

    @Override
    public boolean gatherTransparentRegion(Region region) {
        ViewUtils.gatherTransparentRegionsForOpaqueView(this, region);
        return true;
    }
}
