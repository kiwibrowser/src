// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.snippets;

import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeFeatureList;
import org.chromium.chrome.browser.ntp.cards.NewTabPageViewHolder;
import org.chromium.chrome.browser.suggestions.SuggestionsMetrics;
import org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.browser.widget.displaystyle.MarginResizer;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/**
 * View holder for the header of a section of cards.
 */
public class SectionHeaderViewHolder extends NewTabPageViewHolder implements View.OnClickListener {
    private static final double SCROLL_HEADER_HEIGHT_PERCENTAGE = 0.7;

    private final int mMaxSnippetHeaderHeight;
    private final MarginResizer mMarginResizer;

    private final TextView mTitleView;
    private final ImageView mIconView;

    private SectionHeader mHeader;

    public SectionHeaderViewHolder(final SuggestionsRecyclerView recyclerView, UiConfig config) {
        super(LayoutInflater.from(recyclerView.getContext())
                        .inflate(
                                ChromeFeatureList.isEnabled(
                                        ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER)
                                        ? R.layout.new_tab_page_snippets_expandable_header
                                        : R.layout.new_tab_page_snippets_header,
                                recyclerView, false));
        mMaxSnippetHeaderHeight = itemView.getResources().getDimensionPixelSize(
                R.dimen.snippets_article_header_height);

        int wideLateralMargin = recyclerView.getResources().getDimensionPixelSize(
                R.dimen.ntp_wide_card_lateral_margins);
        mMarginResizer = new MarginResizer(itemView, config, 0, wideLateralMargin);

        mTitleView = itemView.findViewById(R.id.header_title);
        mIconView = itemView.findViewById(R.id.header_icon);
    }

    public void onBindViewHolder(SectionHeader header) {
        mHeader = header;
        itemView.setOnClickListener(header.isExpandable() ? this : null);
        mTitleView.setText(header.getHeaderText());
        updateIconDrawable();
        if (ChromeFeatureList.isEnabled(
                    ChromeFeatureList.NTP_ARTICLE_SUGGESTIONS_EXPANDABLE_HEADER)) {
            mIconView.setVisibility(header.isExpandable() ? View.VISIBLE : View.GONE);
        }

        updateDisplay(0, false);
        mMarginResizer.attach();
    }

    @Override
    public void recycle() {
        mMarginResizer.detach();
        mHeader = null;
        super.recycle();
    }

    @Override
    public void onClick(View view) {
        assert mHeader.isExpandable() : "onClick() is called on a non-expandable section header.";
        mHeader.toggleHeader();
        SuggestionsMetrics.recordExpandableHeaderTapped(mHeader.isExpanded());
        SuggestionsMetrics.recordArticlesListVisible();
    }

    /**
     * @return The header height we want to set.
     */
    private int getHeaderHeight(int amountScrolled, boolean canTransition) {
        // If the header cannot transition set the height to the maximum so it always displays.
        if (!canTransition) return mMaxSnippetHeaderHeight;

        // Check if snippet header top is within range to start showing. Set the header height,
        // this is a percentage of how much is scrolled. The balance of the scroll will be used
        // to display the peeking card.
        return MathUtils.clamp((int) (amountScrolled * SCROLL_HEADER_HEIGHT_PERCENTAGE),
                0, mMaxSnippetHeaderHeight);
    }

    /**
     * Update the view for the fade in/out and heading height.
     * @param amountScrolled the number of pixels scrolled, or how far away from the bottom of the
     *                       screen we got.
     * @param canTransition whether we should animate the header sliding in. When {@code false},
     *                      the header will always be fully visible.
     */
    public void updateDisplay(int amountScrolled, boolean canTransition) {
        int headerHeight = getHeaderHeight(amountScrolled, canTransition);

        itemView.setAlpha((float) headerHeight / mMaxSnippetHeaderHeight);
        getParams().height = headerHeight;

        // This request layout is needed to let the rest of the elements know about the modified
        // dimensions of this one. Otherwise scrolling fast can make the peeking card go completely
        // below the fold for example.
        itemView.requestLayout();
    }

    /**
     * Update the image resource for the icon view based on whether the header is expanded.
     */
    private void updateIconDrawable() {
        if (!mHeader.isExpandable()) return;
        mIconView.setImageResource(mHeader.isExpanded() ? R.drawable.ic_expand_less_black_24dp
                                                        : R.drawable.ic_expand_more_black_24dp);
        mIconView.setContentDescription(mIconView.getResources().getString(mHeader.isExpanded()
                        ? R.string.accessibility_collapse_section_header
                        : R.string.accessibility_expand_section_header));
    }

    /**
     * Triggers an update to the icon drawable. Intended to be used as
     * {@link NewTabPageViewHolder.PartialBindCallback}
     */
    static void updateIconDrawable(NewTabPageViewHolder holder) {
        ((SectionHeaderViewHolder) holder).updateIconDrawable();
    }
}
