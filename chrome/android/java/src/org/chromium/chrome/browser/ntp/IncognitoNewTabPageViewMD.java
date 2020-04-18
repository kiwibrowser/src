// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import static org.chromium.chrome.browser.util.ViewUtils.dpToPx;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.support.annotation.IdRes;
import android.support.annotation.StringRes;
import android.text.Layout;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.style.BulletSpan;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.ui.text.NoUnderlineClickableSpan;
import org.chromium.ui.text.SpanApplier;

/**
 * The Material Design New Tab Page for use in the Incognito profile. This is an extension
 * of the IncognitoNewTabPageView class with improved text content and a more responsive design.
 */
public class IncognitoNewTabPageViewMD extends IncognitoNewTabPageView {
    private final Context mContext;
    private final DisplayMetrics mMetrics;

    private int mWidthDp;
    private int mHeightDp;

    private LinearLayout mContainer;
    private TextView mHeader;
    private TextView mSubtitle;
    private LinearLayout mBulletpointsContainer;
    private TextView mLearnMore;
    private TextView[] mParagraphs;

    private static final int BULLETPOINTS_HORIZONTAL_SPACING_DP = 40;
    private static final int CONTENT_WIDTH_DP = 600;
    private static final int WIDE_LAYOUT_THRESHOLD_DP = 720;
    private static final int CHROME_HOME_LEARN_MORE_BOTTOM_PADDING_DP = 8;

    private static class IncognitoBulletSpan extends BulletSpan {
        public IncognitoBulletSpan() {
            super(0 /* gapWidth */);
        }

        @Override
        public void drawLeadingMargin(Canvas c, Paint p, int x, int dir, int top, int baseline,
                int bottom, CharSequence text, int start, int end, boolean first, Layout l) {
            // Do not draw the standard bullet point. We will include the Unicode bullet point
            // symbol in the text instead.
        }
    }

    /** Default constructor needed to inflate via XML. */
    public IncognitoNewTabPageViewMD(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mMetrics = mContext.getResources().getDisplayMetrics();
    }

    private int pxToDp(int px) {
        return (int) Math.ceil(px / mMetrics.density);
    }

    private int spToPx(int sp) {
        return (int) Math.ceil(sp * mMetrics.scaledDensity);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        populateBulletpoints(R.id.new_tab_incognito_features, R.string.new_tab_otr_not_saved);
        populateBulletpoints(R.id.new_tab_incognito_warning, R.string.new_tab_otr_visible);

        mContainer = (LinearLayout) findViewById(R.id.new_tab_incognito_container);
        mHeader = (TextView) findViewById(R.id.new_tab_incognito_title);
        mSubtitle = (TextView) findViewById(R.id.new_tab_incognito_subtitle);
        mLearnMore = (TextView) findViewById(R.id.learn_more);
        mParagraphs =
                new TextView[] {mSubtitle, (TextView) findViewById(R.id.new_tab_incognito_features),
                        (TextView) findViewById(R.id.new_tab_incognito_warning), mLearnMore};
        mBulletpointsContainer =
                (LinearLayout) findViewById(R.id.new_tab_incognito_bulletpoints_container);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        if (changed) {
            mWidthDp = pxToDp(getMeasuredWidth());
            mHeightDp = pxToDp(getMeasuredHeight());

            adjustTypography();
            adjustIcon();
            adjustLayout();
            adjustLearnMore();
        }
    }

    /**
     * @param element Resource ID of the element to be populated with the bulletpoints.
     * @param content String ID to serve as the text of |element|. Must contain an <em></em> span,
     *         which will be emphasized, and three <li> items, which will be converted to
     *         bulletpoints.
     * Populates |element| with |content|.
     */
    private void populateBulletpoints(@IdRes int element, @StringRes int content) {
        TextView view = (TextView) findViewById(element);
        String text = mContext.getResources().getString(content);

        // TODO(msramek): Unfortunately, our strings are missing the closing "</li>" tag, which
        // is not a problem when they're used in the Desktop WebUI (omitting the tag is valid in
        // HTML5), but it is a problem for SpanApplier. Update the strings and remove this regex.
        // Note that modifying the strings is a non-trivial operation as they went through a special
        // translation process.
        text = text.replaceAll("<li>([^<]+)\n", "<li>$1</li>\n");

        // Format the bulletpoints:
        //   - Disambiguate the <li></li> spans for SpanApplier.
        //   - Add the bulletpoint symbols (Unicode BULLET U+2022)
        //   - Remove leading whitespace (caused by formatting in the .grdp file)
        //   - Remove the trailing newline after the last bulletpoint.
        text = text.replaceFirst(" +<li>([^<]*)</li>", "<li1>     \u2022     $1</li1>");
        text = text.replaceFirst(" +<li>([^<]*)</li>", "<li2>     \u2022     $1</li2>");
        text = text.replaceFirst(" +<li>([^<]*)</li>\n", "<li3>     \u2022     $1</li3>");

        // Remove the <ul></ul> tags which serve no purpose here, including the whitespace around
        // them.
        text = text.replaceAll(" +</?ul>\\n?", "");

        view.setText(SpanApplier.applySpans(text,
                new SpanApplier.SpanInfo("<em>", "</em>",
                        new ForegroundColorSpan(ApiCompatibilityUtils.getColor(
                                mContext.getResources(), R.color.incognito_emphasis))),
                new SpanApplier.SpanInfo("<li1>", "</li1>", new IncognitoBulletSpan()),
                new SpanApplier.SpanInfo("<li2>", "</li2>", new IncognitoBulletSpan()),
                new SpanApplier.SpanInfo("<li3>", "</li3>", new IncognitoBulletSpan())));
    }

    /** Adjusts the font settings. */
    private void adjustTypography() {
        if (mWidthDp <= 240 || mHeightDp <= 320) {
            // Small text on small screens.
            mHeader.setTextSize(20 /* sp */);
            mHeader.setLineSpacing(spToPx(4) /* add */, 1 /* mult */); // 20sp + 4sp = 24sp

            for (TextView paragraph : mParagraphs) paragraph.setTextSize(12 /* sp */);
        } else {
            // Large text on large screens.
            mHeader.setTextSize(24 /* sp */);
            mHeader.setLineSpacing(spToPx(8) /* add */, 1 /* mult */); // 24sp + 8sp = 32sp

            for (TextView paragraph : mParagraphs) paragraph.setTextSize(14 /* sp */);
        }

        // Paragraph line spacing is constant +6sp, defined in R.layout.new_tab_page_incognito_md.
    }

    /** Adjusts the paddings, margins, and the orientation of bulletpoints. */
    private void adjustLayout() {
        int paddingHorizontalDp;
        int paddingVerticalDp;

        boolean bulletpointsArrangedHorizontally;

        boolean usingChromeHome = FeatureUtilities.isChromeHomeEnabled();
        if (mWidthDp <= WIDE_LAYOUT_THRESHOLD_DP || usingChromeHome) {
            // Small padding.
            // Set the padding to a default for Chrome Home, since we want less padding in this
            // case.
            if (usingChromeHome) {
                paddingHorizontalDp = 16;
                paddingVerticalDp = 0;
            } else {
                paddingHorizontalDp = mWidthDp <= 240 ? 24 : 32;
                paddingVerticalDp = (mHeightDp <= 600) ? 32 : 72;
            }

            // Align left.
            mContainer.setGravity(Gravity.START);

            // Decide the bulletpoints orientation.
            // TODO (thildebr): This is never set to anything but false, check if we can remove
            // related code.
            bulletpointsArrangedHorizontally = false;

            // The subtitle is sized automatically, but not wider than CONTENT_WIDTH_DP.
            mSubtitle.setLayoutParams(
                    new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
                            LinearLayout.LayoutParams.WRAP_CONTENT));
            mSubtitle.setMaxWidth(dpToPx(mContext, CONTENT_WIDTH_DP));

            // The bulletpoints container takes the same width as subtitle. Since the width can
            // not be directly measured at this stage, we must calculate it manually.
            mBulletpointsContainer.setLayoutParams(new LinearLayout.LayoutParams(
                    dpToPx(mContext,
                            Math.min(CONTENT_WIDTH_DP, mWidthDp - 2 * paddingHorizontalDp)),
                    LinearLayout.LayoutParams.WRAP_CONTENT));
        } else {
            // Large padding.
            paddingHorizontalDp = 0; // Should not be necessary on a screen this large.
            paddingVerticalDp = mHeightDp <= 320 ? 16 : 72;

            // Align to the center.
            mContainer.setGravity(Gravity.CENTER_HORIZONTAL);

            // Decide the bulletpoints orientation.
            int totalBulletpointsWidthDp = pxToDp(mBulletpointsContainer.getChildAt(0).getWidth())
                    + pxToDp(mBulletpointsContainer.getChildAt(1).getWidth())
                    + BULLETPOINTS_HORIZONTAL_SPACING_DP;
            bulletpointsArrangedHorizontally = totalBulletpointsWidthDp <= CONTENT_WIDTH_DP;

            // The subtitle width is equal to the two sets of bulletpoints if they are arranged
            // horizontally. If not, use the default CONTENT_WIDTH_DP.
            int contentWidthPx = bulletpointsArrangedHorizontally
                    ? dpToPx(mContext, totalBulletpointsWidthDp)
                    : dpToPx(mContext, CONTENT_WIDTH_DP);
            mSubtitle.setLayoutParams(new LinearLayout.LayoutParams(
                    contentWidthPx, LinearLayout.LayoutParams.WRAP_CONTENT));
            mBulletpointsContainer.setLayoutParams(new LinearLayout.LayoutParams(
                    contentWidthPx, LinearLayout.LayoutParams.WRAP_CONTENT));
        }

        // Apply the bulletpoints orientation.
        if (bulletpointsArrangedHorizontally) {
            mBulletpointsContainer.setOrientation(LinearLayout.HORIZONTAL);
        } else {
            mBulletpointsContainer.setOrientation(LinearLayout.VERTICAL);
        }

        // Set up paddings and margins.
        int paddingTop;
        int paddingBottom;
        if (usingChromeHome) {
            // Preserve the intentional padding given to the new tab view in Chrome Home to
            // accomodate the bottom navigation menu.
            paddingTop = mContainer.getPaddingTop();
            paddingBottom = mContainer.getPaddingBottom();
        } else {
            paddingTop = paddingBottom = dpToPx(mContext, paddingVerticalDp);
        }
        mContainer.setPadding(dpToPx(mContext, paddingHorizontalDp), paddingTop,
                dpToPx(mContext, paddingHorizontalDp), paddingBottom);

        int spacingPx =
                (int) Math.ceil(mParagraphs[0].getTextSize() * (mHeightDp <= 600 ? 1 : 1.5));

        for (TextView paragraph : mParagraphs) {
            // If bulletpoints are arranged horizontally, there should be space between them.
            int rightMarginPx = (bulletpointsArrangedHorizontally
                                        && paragraph == mBulletpointsContainer.getChildAt(0))
                    ? dpToPx(mContext, BULLETPOINTS_HORIZONTAL_SPACING_DP)
                    : 0;

            ((LinearLayout.LayoutParams) paragraph.getLayoutParams())
                    .setMargins(0, spacingPx, rightMarginPx, 0);
            paragraph.setLayoutParams(paragraph.getLayoutParams()); // Apply the new layout.
        }

        ((LinearLayout.LayoutParams) mHeader.getLayoutParams()).setMargins(0, spacingPx, 0, 0);
        mHeader.setLayoutParams(mHeader.getLayoutParams()); // Apply the new layout.
    }

    /** Adjust the Incognito icon. */
    private void adjustIcon() {
        // The icon resource is 120dp x 120dp (i.e. 120px x 120px at MDPI). This method always
        // resizes the icon view to 120dp x 120dp or smaller, therefore image quality is not lost.

        int sizeDp;
        if (mWidthDp <= WIDE_LAYOUT_THRESHOLD_DP) {
            sizeDp = (mWidthDp <= 240 || mHeightDp <= 480) ? 48 : 72;
        } else {
            sizeDp = mHeightDp <= 480 ? 72 : 120;
        }

        ImageView icon = (ImageView) findViewById(R.id.new_tab_incognito_icon);
        icon.getLayoutParams().width = dpToPx(mContext, sizeDp);
        icon.getLayoutParams().height = dpToPx(mContext, sizeDp);
    }

    /** Adjust the "Learn More" link. */
    private void adjustLearnMore() {
        final String subtitleText =
                mContext.getResources().getString(R.string.new_tab_otr_subtitle);
        boolean learnMoreInSubtitle = mWidthDp > WIDE_LAYOUT_THRESHOLD_DP;

        mSubtitle.setClickable(learnMoreInSubtitle);
        mLearnMore.setVisibility(learnMoreInSubtitle ? View.GONE : View.VISIBLE);

        if (FeatureUtilities.isChromeHomeEnabled()) {
            // Additional padding below "Learn More" helps keep distance from the bottom navigation
            // menu making it easier to tap.
            mLearnMore.setPadding(mLearnMore.getPaddingLeft(), mLearnMore.getPaddingTop(),
                    mLearnMore.getPaddingBottom(),
                    dpToPx(mContext, CHROME_HOME_LEARN_MORE_BOTTOM_PADDING_DP));
        }

        if (!learnMoreInSubtitle) {
            // Revert to the original text.
            mSubtitle.setText(subtitleText);
            mSubtitle.setMovementMethod(null);
            return;
        }

        // Concatenate the original text with a clickable "Learn more" link.
        StringBuilder concatenatedText = new StringBuilder();
        concatenatedText.append(subtitleText);
        concatenatedText.append(" ");
        concatenatedText.append(mContext.getResources().getString(R.string.learn_more));
        SpannableString textWithLearnMoreLink = new SpannableString(concatenatedText.toString());

        NoUnderlineClickableSpan span = new NoUnderlineClickableSpan(
                R.color.google_blue_300, (view) -> getManager().loadIncognitoLearnMore());
        textWithLearnMoreLink.setSpan(
                span, subtitleText.length() + 1, textWithLearnMoreLink.length(), 0 /* flags */);
        mSubtitle.setText(textWithLearnMoreLink);
        mSubtitle.setMovementMethod(LinkMovementMethod.getInstance());
    }
}
