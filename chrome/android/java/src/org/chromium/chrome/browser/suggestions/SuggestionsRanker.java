// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import org.chromium.chrome.browser.ntp.cards.ActionItem;
import org.chromium.chrome.browser.ntp.cards.SuggestionsSection;
import org.chromium.chrome.browser.ntp.snippets.CategoryInt;
import org.chromium.chrome.browser.ntp.snippets.SnippetArticle;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Attributes ranks to suggestions and related elements.
 *
 * Ranks here are 0-based scores attributed based on the position or loading order of the
 * elements.
 */
public class SuggestionsRanker {
    private final Map<Integer, Integer> mSuggestionsAddedPerSection = new LinkedHashMap<>();
    private int mTotalAddedSuggestions;

    /**
     * Attributes a per section rank to the provided action item.
     * @see ActionItem#getPerSectionRank()
     */
    public void rankActionItem(ActionItem actionItem, SuggestionsSection section) {
        if (actionItem.getPerSectionRank() != -1) return; // Item was already ranked.
        actionItem.setPerSectionRank(section.getSuggestionsCount());
    }

    /**
     * Attributes global and per section rank to the provided suggestion.
     * @see SnippetArticle#getPerSectionRank()
     * @see SnippetArticle#getGlobalRank()
     */
    public void rankSuggestion(SnippetArticle suggestion) {
        if (suggestion.getPerSectionRank() != -1) return; // Suggestion was already ranked.

        int globalRank = mTotalAddedSuggestions++;
        int perSectionRank = mSuggestionsAddedPerSection.get(suggestion.mCategory);
        mSuggestionsAddedPerSection.put(suggestion.mCategory, perSectionRank + 1);

        suggestion.setRank(perSectionRank, globalRank);
    }

    public void registerCategory(@CategoryInt int category) {
        // Check we are not simply resetting an already registered category.
        if (mSuggestionsAddedPerSection.containsKey(category)) return;
        mSuggestionsAddedPerSection.put(category, 0);
    }

    public int getCategoryRank(@CategoryInt int category) {
        int rank = 0;
        for (Integer key : mSuggestionsAddedPerSection.keySet()) {
            if (key == category) return rank;
            ++rank;
        }
        return -1;
    }
}
