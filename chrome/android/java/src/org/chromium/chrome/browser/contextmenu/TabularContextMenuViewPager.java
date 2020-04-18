// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextmenu;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.support.v4.view.ViewPager;
import android.support.v4.view.animation.LinearOutSlowInInterpolator;
import android.util.AttributeSet;
import android.view.View;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;

/**
 * When there is more than one view for the context menu to display, it wraps the display in a view
 * pager.
 */
public class TabularContextMenuViewPager extends ViewPager {
    private static final int ANIMATION_DURATION_MS = 250;

    private final int mContextMenuMinimumPaddingPx =
            getResources().getDimensionPixelSize(R.dimen.context_menu_min_padding);
    private final Drawable mBackgroundDrawable = ApiCompatibilityUtils.getDrawable(
            getResources(), R.drawable.white_with_rounded_corners);

    private ValueAnimator mAnimator;
    private int mOldHeight;
    private int mCanvasWidth;
    private int mClipHeight;

    private int mDifferenceInHeight;
    private int mPreviousChildIndex = 1;

    public TabularContextMenuViewPager(Context context, AttributeSet attrs) {
        super(context, attrs);
        mBackgroundDrawable.mutate();
    }

    /**
     * Used to show the full ViewPager dialog. Without this the dialog would have no height or
     * width.
     */
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int tabHeight = 0;
        int menuHeight = 0;

        // The width of the context menu is defined so that it leaves space between itself and the
        // screen's edges. It is also bounded to a max size to prevent the menu from stretching
        // across a large display (e.g. a tablet screen).
        int appWindowWidthPx = getResources().getDisplayMetrics().widthPixels;
        int contextMenuWidth = Math.min(appWindowWidthPx - 2 * mContextMenuMinimumPaddingPx,
                getResources().getDimensionPixelSize(R.dimen.context_menu_max_width));

        widthMeasureSpec = MeasureSpec.makeMeasureSpec(contextMenuWidth, MeasureSpec.EXACTLY);

        // getCurrentItem() returns the index of the current page in the pager's pages.
        // It does not take into account the tab layout like getChildCount(), so we add 1.
        int currentChildIndex = getCurrentItem() + 1;

        // The height of the context menu is calculated as the sum of:
        // 1. The tab bar's height, which is only visible when the context menu requires it
        //    (i.e. an ImageLink is clicked)
        // 2. The height of the View being displayed for the current tab.
        for (int i = 0; i < getChildCount(); i++) {
            View child = getChildAt(i);
            child.measure(
                    widthMeasureSpec, MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
            int measuredHeight = child.getMeasuredHeight();

            // The ViewPager also considers the tab layout one of its children, and needs to be
            // treated separately from getting the largest height.
            if (child.getId() == R.id.tab_layout && child.getVisibility() != GONE) {
                tabHeight = measuredHeight;
            } else if (i == currentChildIndex) {
                menuHeight = child.getMeasuredHeight();
                break;
            }
        }
        int fullHeight = menuHeight + tabHeight;
        int appWindowHeightPx = getResources().getDisplayMetrics().heightPixels;
        fullHeight = Math.min(fullHeight, appWindowHeightPx - 2 * mContextMenuMinimumPaddingPx);
        mDifferenceInHeight = fullHeight - mOldHeight;

        if (currentChildIndex == mPreviousChildIndex) {
            // Handles the snapping of the view when its height changes
            // (i.e. an image finished loading or the link became fully visible).
            // The pager will immediately snap to the new height.
            mClipHeight = fullHeight;
            if (menuHeight != 0) mOldHeight = fullHeight;
            heightMeasureSpec = MeasureSpec.makeMeasureSpec(fullHeight, MeasureSpec.EXACTLY);
        } else {
            // Handles the case where the view pager has completely scrolled to a different
            // child. It will measure to the larger height so the clipping is visible.
            initAnimator();
            heightMeasureSpec = MeasureSpec.makeMeasureSpec(
                    Math.max(mOldHeight, fullHeight), MeasureSpec.EXACTLY);
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        mPreviousChildIndex = currentChildIndex;
        // The animation only runs when switching to a tab with a different height.
        if (mAnimator != null) mAnimator.start();
    }

    private void initAnimator() {
        if (mAnimator != null) return;
        mAnimator = ValueAnimator.ofFloat(0f, 1f);
        mAnimator.setDuration(ANIMATION_DURATION_MS);
        mAnimator.setInterpolator(new LinearOutSlowInInterpolator());
        mAnimator.addUpdateListener(new AnimatorUpdateListener() {

            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                float animatedValue = (float) animation.getAnimatedValue();
                if (mDifferenceInHeight < 0) {
                    setTranslationY(animatedValue * -mDifferenceInHeight / 2);
                } else {
                    setTranslationY((1 - animatedValue) * mDifferenceInHeight / 2);
                }
                mClipHeight = mOldHeight + (int) (mDifferenceInHeight * animatedValue);
                invalidate();
            }
        });
        mAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                mOldHeight = mClipHeight;
                setTranslationY(0);
                if (mDifferenceInHeight < 0) requestLayout();
            }
        });
    }

    @Override
    public void draw(Canvas canvas) {
        mCanvasWidth = canvas.getWidth();
        int backgroundOffsetX = getScrollX();
        mBackgroundDrawable.setBounds(
                backgroundOffsetX, 0, canvas.getWidth() + backgroundOffsetX, mClipHeight);
        mBackgroundDrawable.draw(canvas);

        canvas.clipRect(backgroundOffsetX, 0, mCanvasWidth + backgroundOffsetX, mClipHeight);
        super.draw(canvas);
    }
}
