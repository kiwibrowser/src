// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.history;

import android.app.Activity;
import android.view.View;

import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BasicNativePage;
import org.chromium.chrome.browser.ChromeActivity;
import org.chromium.chrome.browser.NativePageHost;
import org.chromium.chrome.browser.UrlConstants;
import org.chromium.chrome.browser.snackbar.SnackbarManager.SnackbarManageable;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.util.FeatureUtilities;

/**
 * Native page for managing browsing history.
 */
public class HistoryPage extends BasicNativePage {
    private HistoryManager mHistoryManager;
    private String mTitle;
    private int mThemeColor;

    /**
     * Create a new instance of the history page.
     * @param activity The {@link Activity} used to get context and instantiate the
     *                 {@link HistoryManager}.
     * @param host A NativePageHost to load URLs.
     */
    public HistoryPage(ChromeActivity activity, NativePageHost host) {
        super(activity, host);

        mThemeColor = !host.isIncognito()
                ? super.getThemeColor()
                : ColorUtils.getDefaultThemeColor(activity.getResources(),
                          FeatureUtilities.isChromeModernDesignEnabled(), true);
    }

    @Override
    protected void initialize(ChromeActivity activity, final NativePageHost host) {
        mHistoryManager = new HistoryManager(activity, false,
                ((SnackbarManageable) activity).getSnackbarManager(), host.isIncognito());
        mTitle = activity.getString(R.string.menu_history);
    }

    @Override
    public View getView() {
        return mHistoryManager.getView();
    }

    @Override
    public String getTitle() {
        return mTitle;
    }

    @Override
    public String getHost() {
        return UrlConstants.HISTORY_HOST;
    }

    @Override
    public void destroy() {
        mHistoryManager.onDestroyed();
        mHistoryManager = null;
        super.destroy();
    }

    @Override
    public int getThemeColor() {
        return mThemeColor;
    }

    @VisibleForTesting
    public HistoryManager getHistoryManagerForTesting() {
        return mHistoryManager;
    }
}
