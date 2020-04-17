// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.searchwidget;

import android.content.res.ColorStateList;

import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.omnibox.UrlBarData;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.components.security_state.ConnectionSecurityLevel;
import org.chromium.base.ContextUtils;
import android.graphics.Color;

class SearchBoxDataProvider implements ToolbarDataProvider {
    private Tab mTab;

    /**
     * Called when native library is loaded and a tab has been initialized.
     * @param tab The tab to use.
     */
    public void onNativeLibraryReady(Tab tab) {
        assert LibraryLoader.isInitialized();
        mTab = tab;
    }

    @Override
    public boolean isUsingBrandColor() {
        return false;
    }

    @Override
    public boolean isIncognito() {
        if (mTab == null) return false;
        return mTab.isIncognito();
    }

    @Override
    public Profile getProfile() {
        if (mTab == null) return null;
        return mTab.getProfile();
    }

    @Override
    public UrlBarData getUrlBarData() {
        return UrlBarData.EMPTY;
    }

    @Override
    public String getTitle() {
        return "";
    }

    @Override
    public Tab getTab() {
        return mTab;
    }

    @Override
    public boolean hasTab() {
        return mTab != null;
    }

    @Override
    public int getPrimaryColor() {
        if (ContextUtils.getAppSharedPreferences() != null && (ContextUtils.getAppSharedPreferences().getBoolean("user_night_mode_enabled", false) || ContextUtils.getAppSharedPreferences().getString("active_theme", "").equals("Diamond Black")))
          return Color.BLACK;
        return 0;
    }

    @Override
    public NewTabPage getNewTabPageForCurrentTab() {
        return null;
    }

    @Override
    public String getCurrentUrl() {
        return SearchWidgetProvider.getDefaultSearchEngineUrl();
    }

    @Override
    public boolean shouldShowGoogleG(String urlBarText) {
        return false;
    }

    @Override
    public boolean isOfflinePage() {
        return false;
    }

    @Override
    public boolean shouldShowVerboseStatus() {
        return false;
    }

    @Override
    public int getSecurityLevel() {
        return ConnectionSecurityLevel.NONE;
    }

    @Override
    public int getSecurityIconResource(boolean isTablet) {
        return 0;
    }

    @Override
    public ColorStateList getSecurityIconColorStateList() {
        return null;
    }

    @Override
    public boolean shouldDisplaySearchTerms() {
        return false;
    }
}
