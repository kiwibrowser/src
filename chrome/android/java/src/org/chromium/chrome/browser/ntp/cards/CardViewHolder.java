// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp.cards;

import android.content.res.Resources;
import android.graphics.Rect;
import android.support.annotation.CallSuper;
import android.support.annotation.DrawableRes;
import android.support.v4.view.animation.FastOutSlowInInterpolator;
import android.support.v7.widget.RecyclerView;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnAttachStateChangeListener;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.animation.Interpolator;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.ContextMenuManager;
import org.chromium.chrome.browser.ntp.ContextMenuManager.ContextMenuItemId;
import org.chromium.chrome.browser.suggestions.SuggestionsConfig;
import org.chromium.chrome.browser.suggestions.SuggestionsRecyclerView;
import org.chromium.chrome.browser.util.MathUtils;
import org.chromium.chrome.browser.util.ViewUtils;
import org.chromium.chrome.browser.widget.displaystyle.HorizontalDisplayStyle;
import org.chromium.chrome.browser.widget.displaystyle.MarginResizer;
import org.chromium.chrome.browser.widget.displaystyle.UiConfig;

/**
 * Holder for a generic card.
 *
 * Specific behaviors added to the cards:
 *
 * - Cards can peek above the fold if there is enough space.
 *
 * - When peeking, tapping on cards will make them request a scroll up (see
 *   {@link SuggestionsRecyclerView#interceptCardTapped}). Tap events in non-peeking state will be
 *   routed through {@link #onCardTapped()} for subclasses to override.
 *
 * - Cards will get some lateral margins when the viewport is sufficiently wide.
 *   (see {@link HorizontalDisplayStyle#WIDE})
 *
 * Note: If a subclass overrides {@link #onBindViewHolder()}, it should call the
 * parent implementation to reset the private state when a card is recycled.
 */
public abstract class CardViewHolder
        extends NewTabPageViewHolder implements ContextMenuManager.Delegate {
    private static final Interpolator TRANSITION_INTERPOLATOR = new FastOutSlowInInterpolator();

    /** Value used for max peeking card height and padding. */
    private final int mMaxPeekPadding;

    /**
     * The card shadow is part of the drawable nine-patch and not drawn via setElevation(),
     * so it is included in the height and width of the drawable. This member contains the
     * dimensions of the shadow (from the drawable's padding), so it can be used to offset the
     * position in calculations.
     */
    private final Rect mCardShadow = new Rect();

    private final int mCardGap;

    private final int mDefaultLateralMargin;
    private final int mWideLateralMargin;

    protected final SuggestionsRecyclerView mRecyclerView;

    protected final UiConfig mUiConfig;
    private final MarginResizer mMarginResizer;

    /**
     * To what extent the card is "peeking". 0 means the card is not peeking at all and spans the
     * full width of its parent. 1 means it is fully peeking and will be shown with a margin.
     */
    private float mPeekingPercentage;

    @DrawableRes
    private int mBackground;

    /**
     * @param layoutId resource id of the layout to inflate and to use as card.
     * @param recyclerView ViewGroup that will contain the newly created view.
     * @param uiConfig The NTP UI configuration object used to adjust the card UI.
     * @param contextMenuManager The manager responsible for the context menu.
     */
    public CardViewHolder(int layoutId, final SuggestionsRecyclerView recyclerView,
            UiConfig uiConfig, final ContextMenuManager contextMenuManager) {
        super(inflateView(layoutId, recyclerView));

        Resources resources = recyclerView.getResources();
        ApiCompatibilityUtils.getDrawable(resources, R.drawable.card_single)
                .getPadding(mCardShadow);

        mCardGap = recyclerView.getResources().getDimensionPixelSize(R.dimen.snippets_card_gap);

        mMaxPeekPadding = resources.getDimensionPixelSize(R.dimen.snippets_padding);

        mRecyclerView = recyclerView;

        itemView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (recyclerView.interceptCardTapped(CardViewHolder.this)) return;
                onCardTapped();
            }
        });

        itemView.setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
            @Override
            public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo menuInfo) {
                if (isPeeking()) return;

                contextMenuManager.createContextMenu(menu, itemView, CardViewHolder.this);
            }
        });

        mUiConfig = uiConfig;

        // Configure the resizer to use negative margins on regular display to balance out the
        // lateral shadow of the card 9-patch and avoid a rounded corner effect.
        int cardCornerRadius = resources.getDimensionPixelSize(R.dimen.card_corner_radius);
        assert mCardShadow.left == mCardShadow.right;
        if (SuggestionsConfig.useModernLayout()) {
            mDefaultLateralMargin =
                    resources.getDimensionPixelSize(R.dimen.content_suggestions_card_modern_margin);
        } else {
            mDefaultLateralMargin = -(mCardShadow.left + cardCornerRadius);
        }
        mWideLateralMargin = resources.getDimensionPixelSize(R.dimen.ntp_wide_card_lateral_margins);

        mMarginResizer =
                new MarginResizer(itemView, uiConfig, mDefaultLateralMargin, mWideLateralMargin);
    }

    @Override
    public boolean isItemSupported(@ContextMenuItemId int menuItemId) {
        return menuItemId == ContextMenuManager.ID_REMOVE && isDismissable();
    }

    @Override
    public void removeItem() {
        getRecyclerView().dismissItemWithAnimation(this);
    }

    @Override
    public void openItem(int windowDisposition) {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getUrl() {
        return null;
    }

    @Override
    public boolean isDismissable() {
        if (isPeeking()) return false;

        int position = getAdapterPosition();
        if (position == RecyclerView.NO_POSITION) return false;

        return !mRecyclerView.getNewTabPageAdapter().getItemDismissalGroup(position).isEmpty();
    }

    @Override
    public void onContextMenuCreated() {}

    /**
     * Called when the NTP cards adapter is requested to update the currently visible ViewHolder
     * with data.
     */
    @CallSuper
    public void onBindViewHolder() {
        // Reset the transparency and translation in case a dismissed card is being recycled.
        itemView.setAlpha(1f);
        itemView.setTranslationX(0f);

        itemView.addOnAttachStateChangeListener(new OnAttachStateChangeListener() {
            @Override
            public void onViewAttachedToWindow(View view) {}

            @Override
            public void onViewDetachedFromWindow(View view) {
                // In some cases a view can be removed while a user is interacting with it, without
                // calling ItemTouchHelper.Callback#clearView(), which we rely on for bottomSpacer
                // calculations. So we call this explicitly here instead.
                // See https://crbug.com/664466, b/32900699
                mRecyclerView.onItemDismissFinished(mRecyclerView.findContainingViewHolder(view));
                itemView.removeOnAttachStateChangeListener(this);
            }
        });

        // Make sure we use the right background.
        updateLayoutParams();

        mMarginResizer.attach();

        mRecyclerView.onCardBound(this);
    }

    @Override
    public void recycle() {
        mMarginResizer.detach();
        super.recycle();
    }

    @Override
    public void updateLayoutParams() {
        // Nothing to do for dismissed cards.
        if (getAdapterPosition() == RecyclerView.NO_POSITION) return;

        // Nothing to do for the modern layout.
        if (SuggestionsConfig.useModernLayout()) return;

        NewTabPageAdapter adapter = mRecyclerView.getNewTabPageAdapter();

        // Each card has the full elevation effect (the shadow) in the 9-patch. If the next item is
        // a card a negative bottom margin is set so the next card is overlaid slightly on top of
        // this one and hides the bottom shadow.
        int abovePosition = getAdapterPosition() - 1;
        boolean hasCardAbove = abovePosition >= 0 && isCard(adapter.getItemViewType(abovePosition));
        int belowPosition = getAdapterPosition() + 1;
        boolean hasCardBelow = false;
        if (belowPosition < adapter.getItemCount()) {
            // The PROMO card has an empty margin and will not be right against the preceding card,
            // so we don't consider it a card from the point of view of the preceding one.
            @ItemViewType int belowViewType = adapter.getItemViewType(belowPosition);
            hasCardBelow = isCard(belowViewType) && belowViewType != ItemViewType.PROMO;
        }

        @DrawableRes
        int selectedBackground = selectBackground(hasCardAbove, hasCardBelow);
        if (mBackground == selectedBackground) return;

        mBackground = selectedBackground;
        ViewUtils.setNinePatchBackgroundResource(itemView, selectedBackground);

        // By default the apparent distance between two cards is the sum of the bottom and top
        // height of their shadows. We want |mCardGap| instead, so we set the bottom margin to
        // the difference.
        // noinspection ResourceType
        RecyclerView.LayoutParams layoutParams = getParams();
        layoutParams.bottomMargin =
                hasCardBelow ? (mCardGap - (mCardShadow.top + mCardShadow.bottom)) : 0;
        itemView.setLayoutParams(layoutParams);
    }

    /**
     * Resets the appearance of the card to not peeking.
     */
    public void setNotPeeking() {
        setPeekingPercentage(0);
    }

    /**
     * Change the width, padding and child opacity of the card to give a smooth transition from
     * peeking to fully expanded as the user scrolls.
     * @param availableSpace space (pixels) available between the bottom of the screen and the
     *                       above-the-fold section, where the card can peek.
     */
    public void updatePeek(int availableSpace) {
        // If 1 padding unit (|mMaxPeekPadding|) is visible, the card is fully peeking. This is
        // reduced as the card is scrolled up, until 2 padding units are visible and the card is
        // not peeking anymore at all. Anything not between 0 and 1 is clamped.
        setPeekingPercentage(
                MathUtils.clamp(2f - (float) availableSpace / mMaxPeekPadding, 0f, 1f));
    }

    /**
     * @return Whether the card is peeking.
     */
    public boolean isPeeking() {
        return mPeekingPercentage > 0f;
    }

    /**
     * Override this to react when the card is tapped. This method will not be called if the card is
     * currently peeking.
     */
    protected void onCardTapped() {}

    private void setPeekingPercentage(float peekingPercentage) {
        if (mPeekingPercentage == peekingPercentage) return;

        mPeekingPercentage = peekingPercentage;

        int peekPadding = (int) (mMaxPeekPadding
                * TRANSITION_INTERPOLATOR.getInterpolation(1f - peekingPercentage));

        // Modify the padding so as the margin increases, the padding decreases, keeping the card's
        // contents in the same position. The top and bottom remain the same.
        int lateralPadding;
        if (mUiConfig.getCurrentDisplayStyle().horizontal != HorizontalDisplayStyle.WIDE) {
            lateralPadding = peekPadding;
        } else {
            lateralPadding = mMaxPeekPadding;
        }
        itemView.setPadding(lateralPadding, mMaxPeekPadding, lateralPadding, mMaxPeekPadding);

        // Adjust the margins. The shadow width is offset via the default lateral margin.
        mMarginResizer.setMargins(mDefaultLateralMargin + mMaxPeekPadding - peekPadding,
                mWideLateralMargin);

        // Set the opacity of the card content to be 0 when peeking and 1 when full width.
        int itemViewChildCount = ((ViewGroup) itemView).getChildCount();
        for (int i = 0; i < itemViewChildCount; ++i) {
            View snippetChild = ((ViewGroup) itemView).getChildAt(i);
            snippetChild.setAlpha(peekPadding / (float) mMaxPeekPadding);
        }
    }

    private static View inflateView(int resourceId, ViewGroup parent) {
        return LayoutInflater.from(parent.getContext()).inflate(resourceId, parent, false);
    }

    public static boolean isCard(@ItemViewType int type) {
        switch (type) {
            case ItemViewType.SNIPPET:
            case ItemViewType.STATUS:
            case ItemViewType.ACTION:
            case ItemViewType.PROMO:
                return true;
            case ItemViewType.ABOVE_THE_FOLD:
            case ItemViewType.SITE_SECTION:
            case ItemViewType.HEADER:
            case ItemViewType.PROGRESS:
            case ItemViewType.FOOTER:
            case ItemViewType.ALL_DISMISSED:
                return false;
            default:
                assert false;
        }
        return false;
    }

    @DrawableRes
    protected int selectBackground(boolean hasCardAbove, boolean hasCardBelow) {
        if (hasCardAbove && hasCardBelow) return R.drawable.card_middle;
        if (!hasCardAbove && hasCardBelow) return R.drawable.card_top;
        if (hasCardAbove && !hasCardBelow) return R.drawable.card_bottom;
        return R.drawable.card_single;
    }

    public SuggestionsRecyclerView getRecyclerView() {
        return mRecyclerView;
    }
}
