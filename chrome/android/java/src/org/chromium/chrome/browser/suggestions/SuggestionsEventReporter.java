// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import org.chromium.chrome.browser.ntp.cards.ActionItem;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;

/**
 * Exposes methods to report suggestions related events, for UMA or Fetch scheduling purposes.
 */
public interface SuggestionsEventReporter {
    /**
     * Notifies about new suggestions surfaces being opened: the bottom sheet opening or a NTP being
     * created.
     */
    void onSurfaceOpened();

    /**
     * Tracks per-page-load metrics for content suggestions.
     * @param categories The categories of content suggestions.
     * @param suggestionsPerCategory The number of content suggestions in each category.
     * @param isCategoryVisible A boolean array representing which categories (possibly empty) are
     *                          visible in the UI.
     */
    void onPageShown(int[] categories, int[] suggestionsPerCategory, boolean[] isCategoryVisible);

    /**
     * Tracks impression metrics for a content suggestion.
     * @param suggestion The content suggestion that was shown to the user.
     */
    void onSuggestionShown(SnippetArticle suggestion);

    /**
     * Tracks interaction metrics for a content suggestion.
     * @param suggestion The content suggestion that the user opened.
     * @param windowOpenDisposition How the suggestion was opened (current tab, new tab,
     *                              new window etc).
     * @param suggestionsRanker The ranker used to get extra information about that suggestion.
     */
    void onSuggestionOpened(SnippetArticle suggestion, int windowOpenDisposition,
            SuggestionsRanker suggestionsRanker);

    /**
     * Tracks impression metrics for the long-press menu for a content suggestion.
     * @param suggestion The content suggestion for which the long-press menu was opened.
     */
    void onSuggestionMenuOpened(SnippetArticle suggestion);

    /**
     * Tracks impression metrics for a category's action button ("More").
     * @param category The action button that was shown.
     */
    void onMoreButtonShown(@CategoryInt ActionItem category);

    /**
     * Tracks click metrics for a category's action button ("More").
     * @param category The action button that was clicked.
     */
    void onMoreButtonClicked(@CategoryInt ActionItem category);
}
