// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.omnibox;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ListView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.WindowDelegate;
import org.chromium.chrome.browser.omnibox.OmniboxResultsAdapter.OmniboxResultItem;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.chrome.browser.widget.bottomsheet.BottomSheet;

/**
 * A widget for showing a list of omnibox suggestions.
 */
@VisibleForTesting
public class OmniboxSuggestionsList extends ListView {
    private static final int OMNIBOX_RESULTS_BG_COLOR = 0xFFF5F5F6;
    private static final int OMNIBOX_RESULTS_CHROME_MODERN_BG_COLOR = 0xFFFFFFFF;
    private static final int OMNIBOX_INCOGNITO_RESULTS_BG_COLOR = 0xFF323232;
    private static final int OMNIBOX_INCOGNITO_RESULTS_CHROME_MODERN_BG_COLOR = 0xFF3C4043;

    private final OmniboxSuggestionListEmbedder mEmbedder;
    private final int mSuggestionHeight;
    private final int mSuggestionAnswerHeight;
    private final int mSuggestionDefinitionHeight;
    private final View mAnchorView;

    private final int[] mTempPosition = new int[2];
    private final Rect mTempRect = new Rect();

    private final int mBackgroundVerticalPadding;

    private float mMaxRequiredWidth;
    private float mMaxMatchContentsWidth;

    /**
     * Provides the capabilities required to embed the omnibox suggestion list into the UI.
     */
    public interface OmniboxSuggestionListEmbedder {
        /** Return the anchor view the suggestion list should be drawn below. */
        View getAnchorView();

        /** Return the bottom sheet for the containing UI to be used in sizing. */
        @Nullable
        BottomSheet getBottomSheet();

        /** Return the delegate used to interact with the Window. */
        WindowDelegate getWindowDelegate();

        /** Return whether modern design should be used when styling the popup. */
        boolean useModernDesign();

        /** Return whether the suggestions are being rendered in the tablet UI. */
        boolean isTablet();

        /** Return whether the current state is viewing incognito. */
        boolean isIncognito();
    }

    /**
     * Constructs a new list designed for containing omnibox suggestions.
     * @param context Context used for contained views.
     * @param embedder The embedder for the omnibox list providing access to external views and
     *                 services.
     */
    public OmniboxSuggestionsList(Context context, OmniboxSuggestionListEmbedder embedder) {
        super(context, null, android.R.attr.dropDownListViewStyle);
        mEmbedder = embedder;
        setDivider(null);
        setFocusable(true);
        setFocusableInTouchMode(true);

        mSuggestionHeight =
                context.getResources().getDimensionPixelOffset(R.dimen.omnibox_suggestion_height);
        mSuggestionAnswerHeight = context.getResources().getDimensionPixelOffset(
                R.dimen.omnibox_suggestion_answer_height);
        mSuggestionDefinitionHeight = context.getResources().getDimensionPixelOffset(
                R.dimen.omnibox_suggestion_definition_height);

        int paddingTop = mEmbedder.useModernDesign()
                ? 0
                : context.getResources().getDimensionPixelOffset(
                          R.dimen.omnibox_suggestion_list_padding_top);
        int paddingBottom = context.getResources().getDimensionPixelOffset(
                R.dimen.omnibox_suggestion_list_padding_bottom);
        ApiCompatibilityUtils.setPaddingRelative(this, 0, paddingTop, 0, paddingBottom);

        refreshPopupBackground();
        getBackground().getPadding(mTempRect);

        mBackgroundVerticalPadding =
                mTempRect.top + mTempRect.bottom + getPaddingTop() + getPaddingBottom();

        mAnchorView = mEmbedder.getAnchorView();
    }

    /**
     * Show (and properly size) the suggestions list.
     */
    void show() {
        updateLayoutParams();
        if (getVisibility() != VISIBLE) {
            setVisibility(VISIBLE);
            if (getSelectedItemPosition() != 0) setSelection(0);
        }
    }

    /**
     * Update the suggestion popup background to reflect the current state.
     */
    void refreshPopupBackground() {
        setBackground(getSuggestionPopupBackground());
    }

    /**
     * @return The background for the omnibox suggestions popup.
     */
    private Drawable getSuggestionPopupBackground() {
        int omniboxResultsColorForNonIncognito = OMNIBOX_RESULTS_BG_COLOR;
        int omniboxResultsColorForIncognito = OMNIBOX_INCOGNITO_RESULTS_BG_COLOR;
        if (mEmbedder.useModernDesign()) {
            omniboxResultsColorForNonIncognito = OMNIBOX_RESULTS_CHROME_MODERN_BG_COLOR;
            if (!mEmbedder.isTablet()) {
                omniboxResultsColorForIncognito = OMNIBOX_INCOGNITO_RESULTS_CHROME_MODERN_BG_COLOR;
            }
        }

        int color = mEmbedder.isIncognito() ? omniboxResultsColorForIncognito
                                            : omniboxResultsColorForNonIncognito;
        if (!isHardwareAccelerated()) {
            // When HW acceleration is disabled, changing mSuggestionList' items somehow erases
            // mOmniboxResultsContainer' background from the area not covered by mSuggestionList.
            // To make sure mOmniboxResultsContainer is always redrawn, we make list background
            // color slightly transparent. This makes mSuggestionList.isOpaque() to return false,
            // and forces redraw of the parent view (mOmniboxResultsContainer).
            if (Color.alpha(color) == 255) {
                color = Color.argb(254, Color.red(color), Color.green(color), Color.blue(color));
            }
        }
        return new ColorDrawable(color);
    }

    /**
     * Invalidates all of the suggestion views in the list.  Only applicable when this
     * is visible.
     */
    void invalidateSuggestionViews() {
        if (!isShown()) return;
        for (int i = 0; i < getChildCount(); i++) {
            if (getChildAt(i) instanceof SuggestionView) {
                getChildAt(i).postInvalidateOnAnimation();
            }
        }
    }

    /**
     * Updates the maximum widths required to render the suggestions.
     * This is needed for infinite suggestions where we try to vertically align the leading
     * ellipsis.
     */
    void resetMaxTextWidths() {
        mMaxRequiredWidth = 0;
        mMaxMatchContentsWidth = 0;
    }

    /**
     * Updates the max text width values for the suggestions.
     * @param requiredWidth a new required width.
     * @param matchContentsWidth a new match contents width.
     */
    void updateMaxTextWidths(float requiredWidth, float matchContentsWidth) {
        mMaxRequiredWidth = Math.max(mMaxRequiredWidth, requiredWidth);
        mMaxMatchContentsWidth = Math.max(mMaxMatchContentsWidth, matchContentsWidth);
    }

    /**
     * @return max required width for the suggestions.
     */
    float getMaxRequiredWidth() {
        return mMaxRequiredWidth;
    }

    /**
     * @return max match contents width for the suggestions.
     */
    float getMaxMatchContentsWidth() {
        return mMaxMatchContentsWidth;
    }

    /**
     * Update the layout params to ensure the suggestion popup is properly sized.
     */
    void updateLayoutParams() {
        boolean updateLayout = false;
        FrameLayout.LayoutParams layoutParams = (FrameLayout.LayoutParams) getLayoutParams();
        if (layoutParams == null) {
            layoutParams = new FrameLayout.LayoutParams(0, 0);
            setLayoutParams(layoutParams);
        }

        // Compare the relative positions of the anchor view to the list parent view to
        // determine the offset to apply to the suggestions list.  By using layout positioning,
        // this avoids issues where getLocationInWindow can be inaccurate on certain devices.
        View contentView =
                mEmbedder.getAnchorView().getRootView().findViewById(android.R.id.content);
        ViewUtils.getRelativeLayoutPosition(contentView, mAnchorView, mTempPosition);
        int anchorX = mTempPosition[0];
        int anchorY = mTempPosition[1];

        ViewUtils.getRelativeLayoutPosition(contentView, (View) getParent(), mTempPosition);
        int parentY = mTempPosition[1];

        int anchorBottomRelativeToContent = anchorY + mAnchorView.getMeasuredHeight();
        int desiredTopMargin = anchorBottomRelativeToContent - parentY;
        if (layoutParams.topMargin != desiredTopMargin) {
            layoutParams.topMargin = desiredTopMargin;
            updateLayout = true;
        }

        int contentLeft = contentView.getLeft();
        int anchorLeftRelativeToContent = anchorX - contentLeft;
        if (layoutParams.leftMargin != anchorLeftRelativeToContent) {
            layoutParams.leftMargin = anchorLeftRelativeToContent;
            updateLayout = true;
        }

        mEmbedder.getWindowDelegate().getWindowVisibleDisplayFrame(mTempRect);
        int decorHeight = mEmbedder.getWindowDelegate().getDecorViewHeight();
        int additionalHeightForBottomNavMenu = mEmbedder.getBottomSheet() != null
                ? mEmbedder.getBottomSheet().getToolbarShadowHeight()
                : 0;
        int availableViewportHeight =
                Math.min(mTempRect.height(), decorHeight) + additionalHeightForBottomNavMenu;
        int availableListHeight = availableViewportHeight - anchorBottomRelativeToContent;
        // The suggestions should consume all available space in Modern on phone.
        int desiredHeight = mEmbedder.useModernDesign() && !mEmbedder.isTablet()
                ? availableListHeight
                : Math.min(availableListHeight, getIdealHeight());
        if (layoutParams.height != desiredHeight) {
            layoutParams.height = desiredHeight;
            updateLayout = true;
        }

        int desiredWidth = getDesiredWidth();
        if (layoutParams.width != desiredWidth) {
            layoutParams.width = desiredWidth;
            updateLayout = true;
        }

        if (updateLayout) requestLayout();
    }

    private int getIdealHeight() {
        int idealListSize = mBackgroundVerticalPadding;
        for (int i = 0; i < getAdapter().getCount(); i++) {
            Object obj = getAdapter().getItem(i);
            if (!(obj instanceof OmniboxResultItem)) {
                throw new IllegalStateException("Invalid item in omnibox dropdown: " + obj);
            }
            OmniboxResultItem item = (OmniboxResultItem) obj;
            if (!TextUtils.isEmpty(item.getSuggestion().getAnswerContents())) {
                int num = SuggestionView.parseNumAnswerLines(
                        item.getSuggestion().getAnswer().getSecondLine().getTextFields());
                if (num > 1) {
                    idealListSize += mSuggestionDefinitionHeight;
                } else {
                    idealListSize += mSuggestionAnswerHeight;
                }
            } else {
                idealListSize += mSuggestionHeight;
            }
        }
        return idealListSize;
    }

    private int getDesiredWidth() {
        return mAnchorView.getWidth();
    }

    @Override
    protected void layoutChildren() {
        super.layoutChildren();
        // In ICS, the selected view is not marked as selected despite calling setSelection(0),
        // so we bootstrap this after the children have been laid out.
        if (!isInTouchMode() && getSelectedView() != null) {
            getSelectedView().setSelected(true);
        }
    }

    /**
     * Update the layout direction of the suggestions.
     */
    void updateSuggestionsLayoutDirection(int layoutDirection) {
        if (!isShown()) return;

        for (int i = 0; i < getChildCount(); i++) {
            View childView = getChildAt(i);
            if (!(childView instanceof SuggestionView)) continue;
            ApiCompatibilityUtils.setLayoutDirection(childView, layoutDirection);
        }
    }
}