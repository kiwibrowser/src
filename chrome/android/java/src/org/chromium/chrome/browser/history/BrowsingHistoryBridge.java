// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import org.chromium.base.annotations.CalledByNative;

import java.util.ArrayList;
import java.util.List;

/** The JNI bridge for Android to fetch and manipulate browsing history. */
public class BrowsingHistoryBridge implements HistoryProvider {
    private BrowsingHistoryObserver mObserver;
    private long mNativeHistoryBridge;
    private boolean mRemovingItems;
    private boolean mHasPendingRemoveRequest;

    public BrowsingHistoryBridge(boolean isIncognito) {
        mNativeHistoryBridge = nativeInit(isIncognito);
    }

    @Override
    public void setObserver(BrowsingHistoryObserver observer) {
        mObserver = observer;
    }

    @Override
    public void destroy() {
        if (mNativeHistoryBridge != 0) {
            nativeDestroy(mNativeHistoryBridge);
            mNativeHistoryBridge = 0;
        }
    }

    @Override
    public void queryHistory(String query) {
        nativeQueryHistory(mNativeHistoryBridge, new ArrayList<HistoryItem>(), query);
    }

    @Override
    public void queryHistoryContinuation() {
        nativeQueryHistoryContinuation(mNativeHistoryBridge, new ArrayList<HistoryItem>());
    }

    @Override
    public void markItemForRemoval(HistoryItem item) {
        nativeMarkItemForRemoval(mNativeHistoryBridge, item.getUrl(), item.getNativeTimestamps());
    }

    @Override
    public void removeItems() {
        // Only one remove request may be in-flight at any given time. If items are currently being
        // removed, queue the new request and return early.
        if (mRemovingItems) {
            mHasPendingRemoveRequest = true;
            return;
        }
        mRemovingItems = true;
        mHasPendingRemoveRequest = false;
        nativeRemoveItems(mNativeHistoryBridge);
    }

    @CalledByNative
    public static void createHistoryItemAndAddToList(List<HistoryItem> items, String url,
            String domain, String title, long mostRecentJavaTimestamp, long[] nativeTimestamps,
            boolean blockedVisit) {
        items.add(new HistoryItem(
                url, domain, title, mostRecentJavaTimestamp, nativeTimestamps, blockedVisit));
    }

    @CalledByNative
    public void onQueryHistoryComplete(List<HistoryItem> items, boolean hasMorePotentialMatches) {
        if (mObserver != null) mObserver.onQueryHistoryComplete(items, hasMorePotentialMatches);
    }

    @CalledByNative
    public void onRemoveComplete() {
        mRemovingItems = false;
        if (mHasPendingRemoveRequest) removeItems();
    }

    @CalledByNative
    public void onRemoveFailed() {
        mRemovingItems = false;
        if (mHasPendingRemoveRequest) removeItems();
        // TODO(twellington): handle remove failures.
    }

    @CalledByNative
    public void onHistoryDeleted() {
        if (mObserver != null) mObserver.onHistoryDeleted();
    }

    @CalledByNative
    public void hasOtherFormsOfBrowsingData(boolean hasOtherForms) {
        if (mObserver != null) {
            mObserver.hasOtherFormsOfBrowsingData(hasOtherForms);
        }
    }

    private native long nativeInit(boolean isIncognito);
    private native void nativeDestroy(long nativeBrowsingHistoryBridge);
    private native void nativeQueryHistory(
            long nativeBrowsingHistoryBridge, List<HistoryItem> historyItems, String query);
    private native void nativeQueryHistoryContinuation(
            long nativeBrowsingHistoryBridge, List<HistoryItem> historyItems);
    private native void nativeMarkItemForRemoval(
            long nativeBrowsingHistoryBridge, String url, long[] nativeTimestamps);
    private native void nativeRemoveItems(long nativeBrowsingHistoryBridge);
}
