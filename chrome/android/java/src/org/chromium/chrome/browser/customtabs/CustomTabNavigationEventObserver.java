// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import android.support.customtabs.CustomTabsCallback;
import android.support.customtabs.CustomTabsSessionToken;

import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.security_state.ConnectionSecurityLevel;

/**
 * An observer for firing navigation events on {@link CustomTabsCallback}.
 */
public class CustomTabNavigationEventObserver extends EmptyTabObserver {
    // An operation was aborted (due to user action). Should match the value in net_error_list.h.
    private static final int NET_ERROR_ABORTED = -3;

    private final CustomTabsSessionToken mSessionToken;

    public CustomTabNavigationEventObserver(CustomTabsSessionToken sessionToken) {
        mSessionToken = sessionToken;
    }

    @Override
    public void onPageLoadStarted(Tab tab, String url) {
        CustomTabsConnection.getInstance().notifyNavigationEvent(
                mSessionToken, CustomTabsCallback.NAVIGATION_STARTED);
    }

    @Override
    public void onPageLoadFinished(Tab tab) {
        CustomTabsConnection.getInstance().notifyNavigationEvent(
                mSessionToken, CustomTabsCallback.NAVIGATION_FINISHED);
    }

    @Override
    public void onPageLoadFailed(Tab tab, int errorCode) {
        int navigationEvent = errorCode == NET_ERROR_ABORTED ? CustomTabsCallback.NAVIGATION_ABORTED
                                                             : CustomTabsCallback.NAVIGATION_FAILED;
        CustomTabsConnection.getInstance().notifyNavigationEvent(mSessionToken, navigationEvent);
    }

    @Override
    public void onShown(Tab tab) {
        CustomTabsConnection.getInstance().notifyNavigationEvent(
                mSessionToken, CustomTabsCallback.TAB_SHOWN);
    }

    @Override
    public void onHidden(Tab tab) {
        CustomTabsConnection.getInstance().notifyNavigationEvent(
                mSessionToken, CustomTabsCallback.TAB_HIDDEN);
    }

    @Override
    public void onDidAttachInterstitialPage(Tab tab) {
        if (tab.getSecurityLevel() != ConnectionSecurityLevel.DANGEROUS) return;
        CustomTabsConnection.getInstance().notifyNavigationEvent(
                mSessionToken, CustomTabsCallback.NAVIGATION_FAILED);
    }
}
