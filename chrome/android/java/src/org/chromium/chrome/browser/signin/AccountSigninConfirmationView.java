// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.content.Context;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.LinearLayout;
import android.widget.ScrollView;

import org.chromium.chrome.R;

/**
* This view allows the user to confirm signed in account, sync, and service personalization.
*/
public class AccountSigninConfirmationView extends ScrollView {
    /**
     * Scrolled to bottom observer.
     */
    public interface Observer {
        /**
         * On scrolled to bottom. This won't be called more than once per one
         * {@link AccountSigninConfirmationView#setObserver(Observer)} call.
         */
        void onScrolledToBottom();
    }

    private Observer mObserver;
    private ViewTreeObserver.OnGlobalLayoutListener mOnGlobalLayoutListener;
    private ViewTreeObserver.OnScrollChangedListener mOnScrollChangedListener;

    public AccountSigninConfirmationView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onDetachedFromWindow() {
        removeObservers();
        super.onDetachedFromWindow();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // This assumes that view's layout_width and layout_height are set to match_parent.
        assert MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.EXACTLY;
        assert MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.EXACTLY;

        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);

        View head = findViewById(R.id.signin_confirmation_head);
        LinearLayout.LayoutParams headLayoutParams =
                (LinearLayout.LayoutParams) head.getLayoutParams();
        View accountImage = findViewById(R.id.signin_account_image);
        LinearLayout.LayoutParams accountImageLayoutParams =
                (LinearLayout.LayoutParams) accountImage.getLayoutParams();
        if (height > width) {
            // Sets aspect ratio of the head to 16:9.
            headLayoutParams.height = width * 9 / 16;
            accountImageLayoutParams.topMargin = 0;
        } else {
            headLayoutParams.height = LayoutParams.WRAP_CONTENT;

            // Adds top margin.
            accountImageLayoutParams.topMargin =
                    getResources().getDimensionPixelOffset(R.dimen.signin_screen_top_padding);
        }
        head.setLayoutParams(headLayoutParams);
        accountImage.setLayoutParams(accountImageLayoutParams);

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    protected float getTopFadingEdgeStrength() {
        // Disable fading out effect at the top of this ScrollView.
        return 0;
    }

    private void checkScrolledToBottom() {
        int distance = (getChildAt(getChildCount() - 1).getBottom() - (getHeight() + getScrollY()));
        if (distance > findViewById(R.id.signin_settings_control).getPaddingBottom()) return;

        mObserver.onScrolledToBottom();
        removeObservers();
    }

    /**
     * Sets observer. See {@link Observer}. Regardless of the passed value, notifications for
     * the previous observer will be canceled.
     *
     * @param observer Instance that will receive notifications, or null to clear the observer.
     */
    public void setObserver(@Nullable Observer observer) {
        removeObservers();
        if (observer == null) return;

        mObserver = observer;
        mOnGlobalLayoutListener = this::checkScrolledToBottom;
        getViewTreeObserver().addOnGlobalLayoutListener(mOnGlobalLayoutListener);
        mOnScrollChangedListener = this::checkScrolledToBottom;
        getViewTreeObserver().addOnScrollChangedListener(mOnScrollChangedListener);
    }

    private void removeObservers() {
        if (mObserver == null) return;
        mObserver = null;
        getViewTreeObserver().removeOnGlobalLayoutListener(mOnGlobalLayoutListener);
        mOnGlobalLayoutListener = null;
        getViewTreeObserver().removeOnScrollChangedListener(mOnScrollChangedListener);
        mOnScrollChangedListener = null;
    }
}
