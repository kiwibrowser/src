// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import org.chromium.chrome.browser.ntp.cards.ActionItem;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;
import org.chromium.chrome.browser.suggestions.SuggestionsEventReporter;
import org.chromium.chrome.browser.suggestions.SuggestionsRanker;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.ui.mojom.WindowOpenDisposition;

/** Reports events related to contextual suggestions. */
class ContextualSuggestionsEventReporter implements SuggestionsEventReporter {
    private TabModelSelector mTabModelSelector;
    private ContextualSuggestionsSource mSuggestionSource;

    /**
     * Constructs a new {@link ContextualSuggestionsEventReporter}.
     *
     * @param tabModelSelector The {@link TabModelSelector} for the containing activity.
     * @param suggestionsSource The {@link ContextualSuggestionsSource} used to report events.
     */
    ContextualSuggestionsEventReporter(
            TabModelSelector tabModelSelector, ContextualSuggestionsSource suggestionsSource) {
        mTabModelSelector = tabModelSelector;
        mSuggestionSource = suggestionsSource;
    }

    @Override
    public void onSurfaceOpened() {}

    @Override
    public void onPageShown(
            int[] categories, int[] suggestionsPerCategory, boolean[] isCategoryVisible) {}

    @Override
    public void onSuggestionShown(SnippetArticle suggestion) {}

    @Override
    public void onSuggestionOpened(SnippetArticle suggestion, int windowOpenDisposition,
            SuggestionsRanker suggestionsRanker) {
        int eventId = windowOpenDisposition == WindowOpenDisposition.SAVE_TO_DISK
                ? ContextualSuggestionsEvent.SUGGESTION_DOWNLOADED
                : ContextualSuggestionsEvent.SUGGESTION_CLICKED;
        mSuggestionSource.reportEvent(mTabModelSelector.getCurrentTab().getWebContents(), eventId);
    }

    @Override
    public void onSuggestionMenuOpened(SnippetArticle suggestion) {}

    @Override
    public void onMoreButtonShown(ActionItem category) {}

    @Override
    public void onMoreButtonClicked(ActionItem category) {}
}