// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.feed;

import android.view.View;
import android.widget.FrameLayout;

import org.chromium.chrome.browser.BasicNativePage;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.NativePageHost;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.feed.action.FeedActionHandler;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.SuggestionsNavigationDelegateImpl;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;

/**
 * Provides a new tab page that displays an interest feed rendered list of content suggestions.
 */
public class FeedNewTabPage extends BasicNativePage {
    private final FeedActionHandler mActionHandler;

    private View mRootView;
    private String mTitle;

    /**
     * Constructs a new FeedNewTabPage.
     * @param activity The containing {@link ChromeActivity}.
     * @param nativePageHost The host for this native page.
     * @param tabModelSelector The {@link TabModelSelector} for the containing activity.
     */
    public FeedNewTabPage(ChromeActivity activity, NativePageHost nativePageHost,
            TabModelSelector tabModelSelector) {
        super(activity, nativePageHost);

        // Initialize Action Handler
        Profile profile = nativePageHost.getActiveTab().getProfile();
        SuggestionsNavigationDelegateImpl navigationDelegate =
                new SuggestionsNavigationDelegateImpl(
                        activity, profile, nativePageHost, tabModelSelector);
        mActionHandler = new FeedActionHandler(navigationDelegate);
        // TODO(huayinz): Pass the action handler into Stream.
    }

    @Override
    protected void initialize(ChromeActivity activity, NativePageHost host) {
        mRootView = new FrameLayout(activity);
        mRootView.setLayoutParams(new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        // TODO(twellington): Add Stream to root view.

        mTitle = activity.getResources().getString(org.chromium.chrome.R.string.button_new_tab);
    }

    @Override
    public View getView() {
        return mRootView;
    }

    @Override
    public String getTitle() {
        return mTitle;
    }

    @Override
    public String getHost() {
        return UrlConstants.NTP_HOST;
    }
}
