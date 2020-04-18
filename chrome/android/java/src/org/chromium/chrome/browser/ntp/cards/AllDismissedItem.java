// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.support.annotation.LayoutRes;
import android.support.annotation.StringRes;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.metrics.RecordUserAction;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.metrics.ImpressionTracker.Listener;
import org.chromium.chrome.browser.metrics.OneShotImpressionListener;
import org.chromium.chrome.browser.ntp.NewTabPageUma;
import org.chromium.chrome.browser.suggestions.SuggestionsConfig;
import org.chromium.chrome.browser.util.FeatureUtilities;

import java.util.Calendar;

/**
 * Displayed when all suggested content and their sections have been dismissed. Provides a button
 * to restore the dismissed sections and load new suggestions from the server.
 */
public class AllDismissedItem extends OptionalLeaf {
    private final OneShotImpressionListener mOneShotImpressionTracker =
            new OneShotImpressionListener(() -> {
                if (FeatureUtilities.isChromeHomeEnabled()) {
                    RecordUserAction.record("Suggestions.AllDismissed.Shown");
                }
            });

    @Override
    @ItemViewType
    public int getItemViewType() {
        return ItemViewType.ALL_DISMISSED;
    }

    @Override
    public void onBindViewHolder(NewTabPageViewHolder holder) {
        ((ViewHolder) holder)
                .onBindViewHolder(Calendar.getInstance().get(Calendar.HOUR_OF_DAY),
                        mOneShotImpressionTracker);
    }

    @Override
    public void visitOptionalItem(NodeVisitor visitor) {
        visitor.visitAllDismissedItem();
    }

    public void setVisible(boolean visible) {
        setVisibilityInternal(visible);
    }

    /**
     * ViewHolder for an item of type {@link ItemViewType#ALL_DISMISSED}.
     */
    public static class ViewHolder extends NewTabPageViewHolder {
        private final TextView mBodyTextView;

        public ViewHolder(ViewGroup root, final SectionList sections) {
            super(LayoutInflater.from(root.getContext()).inflate(getLayout(), root, false));
            mBodyTextView = itemView.findViewById(R.id.body_text);

            itemView.findViewById(R.id.action_button).setOnClickListener(v -> {
                NewTabPageUma.recordAction(NewTabPageUma.ACTION_CLICKED_ALL_DISMISSED_REFRESH);
                sections.restoreDismissedSections();
            });
        }

        public void onBindViewHolder(int hourOfDay, Listener listener) {
            @StringRes
            final int messageId;
            if (hourOfDay >= 0 && hourOfDay < 12) {
                messageId = R.string.ntp_all_dismissed_body_text_morning;
            } else if (hourOfDay >= 12 && hourOfDay < 17) {
                messageId = R.string.ntp_all_dismissed_body_text_afternoon;
            } else {
                messageId = R.string.ntp_all_dismissed_body_text_evening;
            }
            mBodyTextView.setText(messageId);
            setImpressionListener(listener);
        }

        @LayoutRes
        private static int getLayout() {
            return SuggestionsConfig.useModernLayout()
                    ? R.layout.content_suggestions_all_dismissed_card_modern
                    : R.layout.new_tab_page_all_dismissed;
        }
    }
}
