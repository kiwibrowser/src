// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.support.annotation.Nullable;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;

import java.util.ArrayList;
import java.util.List;

/**
 * This class allows Java code to get and clear the list of recently closed tabs.
 */
public class RecentlyClosedBridge implements RecentlyClosedTabManager {
    private long mNativeBridge;

    @Nullable
    private Runnable mTabsUpdatedRunnable;

    @CalledByNative
    private static void pushTab(
            List<RecentlyClosedTab> tabs, int id, String title, String url) {
        RecentlyClosedTab tab = new RecentlyClosedTab(id, title, url);
        tabs.add(tab);
    }

    /**
     * Initializes this class with the given profile.
     * @param profile The Profile whose recently closed tabs will be queried.
     */
    public RecentlyClosedBridge(Profile profile) {
        mNativeBridge = nativeInit(profile);
    }

    @Override
    public void destroy() {
        assert mNativeBridge != 0;
        nativeDestroy(mNativeBridge);
        mNativeBridge = 0;
        mTabsUpdatedRunnable = null;
    }

    @Override
    public void setTabsUpdatedRunnable(@Nullable Runnable runnable) {
        mTabsUpdatedRunnable = runnable;
    }

    @Override
    public List<RecentlyClosedTab> getRecentlyClosedTabs(int maxTabCount) {
        List<RecentlyClosedTab> tabs = new ArrayList<RecentlyClosedTab>();
        boolean received = nativeGetRecentlyClosedTabs(mNativeBridge, tabs, maxTabCount);
        return received ? tabs : null;
    }

    @Override
    public boolean openRecentlyClosedTab(
            Tab tab, RecentlyClosedTab recentTab, int windowOpenDisposition) {
        return nativeOpenRecentlyClosedTab(mNativeBridge, tab, recentTab.id, windowOpenDisposition);
    }

    @Override
    public void openRecentlyClosedTab() {
        nativeOpenMostRecentlyClosedTab(mNativeBridge);
    }

    @Override
    public void clearRecentlyClosedTabs() {
        nativeClearRecentlyClosedTabs(mNativeBridge);
    }

    /**
     * This method will be called every time the list of recently closed tabs is updated.
     */
    @CalledByNative
    private void onUpdated() {
        if (mTabsUpdatedRunnable != null) mTabsUpdatedRunnable.run();
    }

    private native long nativeInit(Profile profile);
    private native void nativeDestroy(long nativeRecentlyClosedTabsBridge);
    private native boolean nativeGetRecentlyClosedTabs(
            long nativeRecentlyClosedTabsBridge, List<RecentlyClosedTab> tabs, int maxTabCount);
    private native boolean nativeOpenRecentlyClosedTab(long nativeRecentlyClosedTabsBridge,
            Tab tab, int recentTabId, int windowOpenDisposition);
    private native boolean nativeOpenMostRecentlyClosedTab(long nativeRecentlyClosedTabsBridge);
    private native void nativeClearRecentlyClosedTabs(long nativeRecentlyClosedTabsBridge);
}
