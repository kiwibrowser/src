// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import android.text.TextUtils;

import org.chromium.base.test.util.CallbackHelper;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Stubs out the backends used by the native Android browsing history manager.
 */
public class StubbedHistoryProvider implements HistoryProvider {
    public final CallbackHelper markItemForRemovalCallback = new CallbackHelper();
    public final CallbackHelper removeItemsCallback = new CallbackHelper();

    private BrowsingHistoryObserver mObserver;
    private List<HistoryItem> mItems = new ArrayList<>();
    private List<HistoryItem> mRemovedItems = new ArrayList<>();

    /** The exclusive end position for the last query. **/
    private int mLastQueryEndPosition;
    private String mLastQuery;

    @Override
    public void setObserver(BrowsingHistoryObserver observer) {
        mObserver = observer;
    }

    @Override
    public void queryHistory(String query) {
        mLastQueryEndPosition = 0;
        mLastQuery = query;
        queryHistoryContinuation();
    }

    @Override
    public void queryHistoryContinuation() {
        // Simulate basic paging to facilitate testing loading more items.
        // TODO(twellington): support loading more items while searching.
        int queryStartPosition = mLastQueryEndPosition;
        int queryStartPositionPlusFive = mLastQueryEndPosition + 5;
        boolean hasMoreItems =
                queryStartPositionPlusFive < mItems.size() && TextUtils.isEmpty(mLastQuery);
        int queryEndPosition = hasMoreItems ? queryStartPositionPlusFive : mItems.size();

        mLastQueryEndPosition = queryEndPosition;

        List<HistoryItem> items = new ArrayList<>();
        if (TextUtils.isEmpty(mLastQuery)) {
            items = mItems.subList(queryStartPosition, queryEndPosition);
        } else {
            // Simulate basic search.
            mLastQuery = mLastQuery.toLowerCase(Locale.getDefault());
            for (HistoryItem item : mItems) {
                if (item.getUrl().toLowerCase(Locale.getDefault()).contains(mLastQuery)
                        || item.getTitle().toLowerCase(Locale.getDefault()).contains(mLastQuery)) {
                    items.add(item);
                }
            }
        }

        mObserver.onQueryHistoryComplete(items, hasMoreItems);
    }

    @Override
    public void markItemForRemoval(HistoryItem item) {
        mRemovedItems.add(item);
        markItemForRemovalCallback.notifyCalled();
    }

    @Override
    public void removeItems() {
        for (HistoryItem item : mRemovedItems) {
            mItems.remove(item);
        }
        mRemovedItems.clear();
        removeItemsCallback.notifyCalled();
    }

    @Override
    public void destroy() {}

    public void addItem(HistoryItem item) {
        mItems.add(item);
    }

    public void removeItem(HistoryItem item) {
        mItems.remove(item);
    }

    public static HistoryItem createHistoryItem(int which, long timestamp) {
        long[] nativeTimestamps = {timestamp * 1000};
        if (which == 0) {
            return new HistoryItem("http://google.com/", "www.google.com", "Google", timestamp,
                    nativeTimestamps, false);
        } else if (which == 1) {
            return new HistoryItem(
                    "http://foo.com/", "www.foo.com", "Foo", timestamp, nativeTimestamps, false);
        } else if (which == 2) {
            return new HistoryItem(
                    "http://bar.com/", "www.bar.com", "Bar", timestamp, nativeTimestamps, false);
        } else if (which == 3) {
            return new HistoryItem(
                    "http://news.com/", "www.news.com", "News", timestamp, nativeTimestamps, false);
        } else if (which == 4) {
            return new HistoryItem("http://eng.com/", "www.eng.com", "Engineering", timestamp,
                    nativeTimestamps, false);
        } else if (which == 5) {
            return new HistoryItem("http://blocked.com/", "www.blocked.com", "Cannot Visit",
                    timestamp, nativeTimestamps, true);
        } else {
            return null;
        }
    }

}
