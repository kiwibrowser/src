// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser.suggestions;

import org.chromium.chrome.browser.ntp.cards.ActionItem;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.suggestions.SuggestionsEventReporter;
import org.chromium.chrome.browser.suggestions.SuggestionsRanker;

/**
 * Dummy implementation of {@link SuggestionsEventReporter} that doesn't do anything.
 */
public class DummySuggestionsEventReporter implements SuggestionsEventReporter {
    @Override
    public void onSurfaceOpened() {}

    @Override
    public void onPageShown(
            int[] categories, int[] suggestionsPerCategory, boolean[] isCategoryVisible) {}

    @Override
    public void onSuggestionShown(SnippetArticle suggestion) {}

    @Override
    public void onSuggestionOpened(SnippetArticle suggestion, int windowOpenDisposition,
            SuggestionsRanker suggestionsRanker) {}

    @Override
    public void onSuggestionMenuOpened(SnippetArticle suggestion) {}

    @Override
    public void onMoreButtonShown(@CategoryInt ActionItem category) {}

    @Override
    public void onMoreButtonClicked(@CategoryInt ActionItem category) {}
}
