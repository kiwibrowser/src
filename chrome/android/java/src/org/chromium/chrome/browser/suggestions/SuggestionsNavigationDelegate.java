// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.support.annotation.Nullable;

import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.LoadUrlParams;

/**
 * Interface exposing to the suggestion surface methods to navigate to other parts of the browser.
 */
public interface SuggestionsNavigationDelegate {
    /** @return Whether context menus should allow the option to open a link in incognito. */
    boolean isOpenInIncognitoEnabled();

    /** @return Whether context menus should allow the option to open a link in a new window. */
    boolean isOpenInNewWindowEnabled();

    /** Opens the bookmarks page in the current tab. */
    void navigateToBookmarks();

    /** Opens the Download Manager UI in the current tab. */
    void navigateToDownloadManager();

    /** Opens the recent tabs page in the current tab. */
    void navigateToRecentTabs();

    /** Opens the help page for the content suggestions in the current tab. */
    void navigateToHelpPage();

    /** Opens the suggestion page without recording metrics. */
    void navigateToSuggestionUrl(int windowOpenDisposition, String url);

    /**
     * Opens a content suggestion and records related metrics.
     * @param windowOpenDisposition How to open (current tab, new tab, new window etc).
     * @param article The content suggestion to open.
     */
    void openSnippet(int windowOpenDisposition, SnippetArticle article);

    /**
     * Opens an URL with the desired disposition.
     * @return The tab where the URL is being loaded, if it is accessible. Cases where no tab is
     * returned include opening incognito tabs or opening the URL in a new window.
     */
    @Nullable
    Tab openUrl(int windowOpenDisposition, LoadUrlParams loadUrlParams);
}