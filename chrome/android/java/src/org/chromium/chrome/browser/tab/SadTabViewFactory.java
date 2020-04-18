// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.tab;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.text.Layout;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.method.LinkMovementMethod;
import android.text.style.BulletSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.components.ui_metrics.SadTabEvent;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

/**
 * A factory class for creating the "Sad Tab" view, which is shown in place of a crashed renderer.
 */
public class SadTabViewFactory {
    /**
     * @param context Context of the resulting Sad Tab view.
     * @param suggestionAction {@link Runnable} to be executed when user clicks "try these
     *                        suggestions".
     * @param buttonAction {@link Runnable} to be executed when the button is pressed.
     *                     (e.g., refreshing the page or sending feedback)
     * @param showSendFeedbackView Whether to show the "send feedback" version of the Sad Tab view.
     * @param isIncognito Whether the Sad Tab view is being showin in an incognito tab.
     * @return A "Sad Tab" view instance which is used in place of a crashed renderer.
     */
    public static View createSadTabView(Context context, final Runnable suggestionAction,
            final Runnable buttonAction, final boolean showSendFeedbackView, boolean isIncognito) {
        // Inflate Sad tab and initialize.
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        View sadTabView = inflater.inflate(R.layout.sad_tab, null);

        TextView titleText = (TextView) sadTabView.findViewById(R.id.sad_tab_title);
        int titleTextId =
                showSendFeedbackView ? R.string.sad_tab_reload_title : R.string.sad_tab_title;
        titleText.setText(titleTextId);

        if (showSendFeedbackView) intializeSuggestionsViews(context, sadTabView, isIncognito);

        TextView messageText = (TextView) sadTabView.findViewById(R.id.sad_tab_message);
        messageText.setText(getHelpMessage(context, suggestionAction, showSendFeedbackView));
        messageText.setMovementMethod(LinkMovementMethod.getInstance());

        Button button = (Button) sadTabView.findViewById(R.id.sad_tab_button);
        int buttonTextId = showSendFeedbackView ? R.string.sad_tab_send_feedback_label
                                                : R.string.sad_tab_reload_label;
        button.setText(buttonTextId);
        button.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                SadTabViewFactory.recordEvent(showSendFeedbackView, SadTabEvent.BUTTON_CLICKED);
                buttonAction.run();
            }
        });

        SadTabViewFactory.recordEvent(showSendFeedbackView, SadTabEvent.DISPLAYED);

        return sadTabView;
    }

    /**
     * Construct and return help message to be displayed on R.id.sad_tab_message.
     * @param context Context of the resulting Sad Tab view. This is needed to load the strings.
     * @param suggestionAction Action to be executed when user clicks "try these suggestions"
     *                         or "learn more".
     * @return Help message to be displayed on R.id.sad_tab_message.
     */
    private static CharSequence getHelpMessage(
            Context context, final Runnable suggestionAction, final boolean showSendFeedback) {
        NoUnderlineClickableSpan linkSpan = new NoUnderlineClickableSpan((view) -> {
            SadTabViewFactory.recordEvent(showSendFeedback, SadTabEvent.HELP_LINK_CLICKED);
            suggestionAction.run();
        });

        if (showSendFeedback) {
            SpannableString learnMoreLink =
                    new SpannableString(context.getString(R.string.sad_tab_reload_learn_more));
            learnMoreLink.setSpan(linkSpan, 0, learnMoreLink.length(), 0);
            return learnMoreLink;
        } else {
            String helpMessage = context.getString(R.string.sad_tab_message) + "\n\n"
                    + context.getString(R.string.sad_tab_suggestions);
            return SpanApplier.applySpans(helpMessage, new SpanInfo("<link>", "</link>", linkSpan));
        }
    }

    /**
     * Initializes the TextViews that display tips for handling repeated crashes.
     * @param context Context of the resulting Sad Tab view.
     * @param sadTabView The parent Sad Tab view that contains the TextViews.
     * @param isIncognito Whether the Sad Tab view is being showing in an incognito tab.
     */
    private static void intializeSuggestionsViews(
            Context context, View sadTabView, boolean isIncognito) {
        TextView suggestionsTitle =
                (TextView) sadTabView.findViewById(R.id.sad_tab_suggestions_title);
        suggestionsTitle.setVisibility(View.VISIBLE);
        suggestionsTitle.setText(R.string.sad_tab_reload_try);

        TextView suggestions = (TextView) sadTabView.findViewById(R.id.sad_tab_suggestions);
        suggestions.setVisibility(View.VISIBLE);

        SpannableStringBuilder spannableString = new SpannableStringBuilder();
        if (!isIncognito) {
            spannableString
                    .append(generateBulletedString(context, R.string.sad_tab_reload_incognito))
                    .append("\n");
        }
        spannableString
                .append(generateBulletedString(context, R.string.sad_tab_reload_restart_browser))
                .append("\n")
                .append(generateBulletedString(context, R.string.sad_tab_reload_restart_device))
                .append("\n");
        suggestions.setText(spannableString);
    }

    /**
     * Generates a bulleted {@link SpannableString}.
     * @param context The {@link Context} used to retrieve the String.
     * @param stringResId The resource id of the String to bullet.
     * @return A {@link SpannableString} with a bullet in front of the provided String.
     */
    private static SpannableString generateBulletedString(Context context, int stringResId) {
        SpannableString bullet = new SpannableString(context.getString(stringResId));
        bullet.setSpan(new SadTabBulletSpan(context), 0, bullet.length(), 0);
        return bullet;
    }

    /**
     * Records enumerated histograms for {@link SadTabEvent}.
     * @param sendFeedbackView Whether the event is for the "send feedback" version of the Sad Tab.
     * @param event The {@link SadTabEvent} to record.
     */
    private static void recordEvent(boolean sendFeedbackView, int event) {
        if (sendFeedbackView) {
            RecordHistogram.recordEnumeratedHistogram(
                    "Tabs.SadTab.Feedback.Event", event, SadTabEvent.MAX_SAD_TAB_EVENT);
        } else {
            RecordHistogram.recordEnumeratedHistogram(
                    "Tabs.SadTab.Reload.Event", event, SadTabEvent.MAX_SAD_TAB_EVENT);
        }
    }

    private static class SadTabBulletSpan extends BulletSpan {
        private int mXOffset;

        public SadTabBulletSpan(Context context) {
            super(context.getResources().getDimensionPixelSize(R.dimen.sad_tab_bullet_gap));
            mXOffset = context.getResources().getDimensionPixelSize(
                    R.dimen.sad_tab_bullet_leading_offset);
        }

        @Override
        public void drawLeadingMargin(Canvas c, Paint p, int x, int dir, int top, int baseline,
                int bottom, CharSequence text, int start, int end, boolean first, Layout l) {
            // Android cuts off the bullet points. Adjust the x-position so that the bullets aren't
            // cut off.
            super.drawLeadingMargin(
                    c, p, x + mXOffset, dir, top, baseline, bottom, text, start, end, first, l);
        }
    }
}
