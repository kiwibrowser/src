// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.view.LayoutInflater;
import android.view.ViewGroup;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.suggestions.SuggestionsConfig;
import org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView;

/**
 * ViewHolder for an item of type {@link ItemViewType#PROGRESS}.
 * Adds a {@link ProgressIndicatorView} to the recycler view.
 */
public class ProgressViewHolder extends NewTabPageViewHolder {
    private final ProgressIndicatorView mProgressIndicator;
    private ProgressItem mListItem;

    public ProgressViewHolder(final SuggestionsRecyclerView recyclerView) {
        super(LayoutInflater.from(recyclerView.getContext())
                        .inflate(R.layout.new_tab_page_progress_indicator, recyclerView, false));
        mProgressIndicator = (ProgressIndicatorView) itemView.findViewById(R.id.snippets_progress);

        if (SuggestionsConfig.scrollToLoad()) {
            ViewGroup.MarginLayoutParams params =
                    (ViewGroup.MarginLayoutParams) mProgressIndicator.getLayoutParams();
            params.topMargin = 0;
            mProgressIndicator.setLayoutParams(params);
        }
    }

    public void onBindViewHolder(ProgressItem item) {
        mListItem = item;
        updateDisplay();
    }

    public void updateDisplay() {
        if (mListItem.isVisible()) {
            mProgressIndicator.showDelayed();
        } else {
            mProgressIndicator.hide();
        }
    }
}
