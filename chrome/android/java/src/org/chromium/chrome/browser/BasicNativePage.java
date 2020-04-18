// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import android.app.Activity;
import android.content.res.Resources;
import android.view.View;
import android.widget.FrameLayout.LayoutParams;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.content_public.common.BrowserControlsState;

/**
 * A basic implementation of a white {@link NativePage} that docks below the toolbar.
 */
public abstract class BasicNativePage extends EmptyTabObserver implements NativePage {
    private final Activity mActivity;
    private final NativePageHost mHost;
    private final int mBackgroundColor;
    private final int mThemeColor;
    private int mTopMargin;
    private int mBottomMargin;

    private String mUrl;

    public BasicNativePage(ChromeActivity activity, NativePageHost host) {
        this(activity, host, false);
    }

    public BasicNativePage(ChromeActivity activity, NativePageHost host, boolean isIncognito) {
        initialize(activity, host);
        mActivity = activity;
        mHost = host;

        mThemeColor = ColorUtils.getDefaultThemeColor(activity.getResources(),
                FeatureUtilities.isChromeModernDesignEnabled(), isIncognito);
        mBackgroundColor = isIncognito
                ? ApiCompatibilityUtils.getColor(activity.getResources(), R.color.ntp_bg_incognito)
                : mThemeColor;

        Resources res = mActivity.getResources();

        mTopMargin = 0;
        mBottomMargin = activity.getFullscreenManager().getBottomControlsHeight();
        mTopMargin = res.getDimensionPixelSize(R.dimen.tab_strip_height)
                + res.getDimensionPixelSize(R.dimen.toolbar_height_no_shadow);

        if (host.getActiveTab() != null) {
            host.getActiveTab().addObserver(this);
        }

        updateMargins(host.getActiveTab() != null
                        ? host.getActiveTab().getBrowserControlsStateConstraints()
                        : BrowserControlsState.SHOWN);
    }

    /**
     * Subclasses shall implement this method to initialize the UI that they hold.
     */
    protected abstract void initialize(ChromeActivity activity, NativePageHost host);

    @Override
    public void onBrowserControlsConstraintsUpdated(
            Tab tab, @BrowserControlsState int constraints) {
        updateMargins(constraints);
    }

    @Override
    public abstract View getView();

    @Override
    public String getUrl() {
        return mUrl;
    }

    @Override
    public int getBackgroundColor() {
        return mBackgroundColor;
    }

    @Override
    public int getThemeColor() {
        return mThemeColor;
    }

    @Override
    public boolean needsToolbarShadow() {
        return true;
    }

    @Override
    public void updateForUrl(String url) {
        mUrl = url;
    }

    @Override
    public void destroy() {
        if (mHost.getActiveTab() == null) return;
        mHost.getActiveTab().removeObserver(this);
    }

    /**
     * Tells the native page framework that the url should be changed.
     */
    public void onStateChange(String url) {
        if (url.equals(mUrl)) return;
        mHost.loadUrl(new LoadUrlParams(url), /* incognito = */ false);
    }

    /**
     * Updates the top and bottom margin depending on wether the browser controls are shown or
     * hidden.
     */
    private void updateMargins(@BrowserControlsState int constraints) {
        int topMargin = mTopMargin;
        int bottomMargin = mBottomMargin;
        if (constraints == BrowserControlsState.HIDDEN) {
            topMargin = 0;
            bottomMargin = 0;
        }
        LayoutParams layoutParams =
                new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
        layoutParams.setMargins(0, topMargin, 0, bottomMargin);
        getView().setLayoutParams(layoutParams);
    }
}
