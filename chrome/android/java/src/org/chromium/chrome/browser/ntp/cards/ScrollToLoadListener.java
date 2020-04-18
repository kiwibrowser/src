// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;

import org.chromium.base.ThreadUtils;

/**
 * The ScrollToLoadListener requests fetching more items when the user approaches the end of their
 * feed, simulating an infinite feed. It is a {@link RecyclerView.OnScrollListener} to be attached
 * to the {@link org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView} and must be called
 * manually to fetch more when the users approaches the end of their feed through dismissing items.
 *
 * To determine whether the user is close enough to the end of the feed to load more, it checks
 * whether the sentinel is visible. The sentinel is a child of the RecyclerView that is
 * {@link #SENTINEL_OFFSET} from the end.
 */
public class ScrollToLoadListener extends RecyclerView.OnScrollListener {
    /** How far back from the end of the RecyclerView the sentinel is. */
    private static final int SENTINEL_OFFSET = 5;

    private final NewTabPageAdapter mAdapter;
    private final LinearLayoutManager mLayoutManager;
    private final SectionList mSections;

    private boolean mSentinelPreviouslyVisible;
    private int mPreviousItemCount;

    public ScrollToLoadListener(
            NewTabPageAdapter adapter, LinearLayoutManager layoutManager, SectionList sections) {
        mAdapter = adapter;
        mLayoutManager = layoutManager;
        mSections = sections;
    }

    @Override
    public void onScrolled(RecyclerView recyclerView, int dx, int dy) {
        // onScrolled is called both when the RecyclerView is scrolled and when its layout changes,
        // for example when the RecyclerView is resized due to the BottomSheet being opened or
        // because the phone orientation changes.

        if (mAdapter.getItemCount() == 0) return;  // Adapter hasn't been populated yet.

        fetchMoreIfNearEnd();
    }

    /**
     * To be called when an item on the SuggestionsRecyclerView is dismissed to allow loading more
     * suggestions if that brings the user closer to the end of their feed.
     */
    public void onItemDismissed() {
        fetchMoreIfNearEnd();
    }

    private void fetchMoreIfNearEnd() {
        boolean sentinelVisible = mLayoutManager.findLastVisibleItemPosition()
                > mAdapter.getItemCount() - SENTINEL_OFFSET;

        // We fetch when the sentinel becomes visible - this means that the user is scrolling down
        // and once the user is at the bottom (because a fetch returned no results) they will have
        // to scroll up a fair amount then scroll back down again before triggering another fetch.
        boolean sentinelBecameVisible = sentinelVisible && !mSentinelPreviouslyVisible;

        // We have an edge case where the user scrolls down, the sentinel becomes visible and we
        // trigger a fetch. The fetch then returns a few items, increasing the size of the
        // suggestions list, but not enough to make the sentinel be off screen. When the user
        // scrolls down we want to fetch again, but since the sentinel has not become visible (it
        // was already visible), we wouldn't trigger. To catch this case, we also trigger a fetch
        // when the sentinel is visible and the number of items has changed.
        boolean sentinelVisibleButTooFewItemsFetched = sentinelVisible
                && mAdapter.getItemCount() > mPreviousItemCount;

        if (sentinelBecameVisible || sentinelVisibleButTooFewItemsFetched) {
            mPreviousItemCount = mAdapter.getItemCount();
            // We need to post this since onScrolled may run during a measure & layout pass.
            ThreadUtils.postOnUiThread(mSections::fetchMore);
        }

        mSentinelPreviouslyVisible = sentinelVisible;
    }
}
