// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.toolbar;

import android.content.res.ColorStateList;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import org.chromium.chrome.browser.ntp.NewTabPage;
import org.chromium.chrome.browser.omnibox.UrlBarData;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.components.security_state.ConnectionSecurityLevel;

/**
 * Defines the data that is exposed to properly render the Toolbar.
 */
public interface ToolbarDataProvider {
    /**
     * @return The tab that contains the information currently displayed in the toolbar.
     */
    @Nullable
    Tab getTab();

    /**
     * @return Whether ToolbarDataProvider currently has a tab related to it.
     */
    boolean hasTab();

    /**
     * @return The current url for the current tab. Returns empty string when there is no tab.
     */
    @NonNull
    String getCurrentUrl();

    /**
     * @return The NewTabPage shown for the current Tab or null if one is not being shown.
     */
    NewTabPage getNewTabPageForCurrentTab();

    /**
     * @return Whether the toolbar is currently being displayed for incognito.
     */
    boolean isIncognito();

    /**
     * @return The current {@link Profile}.
     */
    Profile getProfile();

    /**
     * @return The contents of the {@link org.chromium.chrome.browser.omnibox.UrlBar}.
     */
    UrlBarData getUrlBarData();

    /**
     * @return The title of the current tab, or the empty string if there is currently no tab.
     */
    String getTitle();

    /**
     * @return The primary color to use for the background drawable.
     */
    int getPrimaryColor();

    /**
     * @return Whether the current primary color is a brand color.
     */
    boolean isUsingBrandColor();

    /**
     * @return Whether the page currently shown is an offline page.
     */
    boolean isOfflinePage();

    /**
     * @param urlBarText The text currently displayed in the url bar.
     * @return Whether the Google 'G' should be shown in the location bar.
     */
    boolean shouldShowGoogleG(String urlBarText);

    /**
     * @return Whether verbose status next to the security icon should be displayed.
     */
    boolean shouldShowVerboseStatus();

    /**
     * @return The current {@link ConnectionSecurityLevel}.
     */
    @ConnectionSecurityLevel
    int getSecurityLevel();

    /**
     * @return The resource ID of the icon that should be displayed or 0 if no icon should be shown.
     */
    @DrawableRes
    int getSecurityIconResource(boolean isTablet);

    /**
     * @return The {@link ColorStateList} to use to tint the security state icon.
     */
    ColorStateList getSecurityIconColorStateList();

    /**
     * @return Whether or not we should display search terms instead of a URL for query in omnibox.
     */
    boolean shouldDisplaySearchTerms();
}
