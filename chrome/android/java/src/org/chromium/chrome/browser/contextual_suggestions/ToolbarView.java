// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextual_suggestions;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.widget.ListMenuButton;
import org.chromium.chrome.browser.widget.TintedImageView;

/** The toolbar view, containing an icon, title and close button. */
public class ToolbarView extends FrameLayout {
    private View mCloseButton;
    private ListMenuButton mMenuButton;
    private TextView mTitle;
    private View mShadow;
    private TintedImageView mArrow;
    private View mMainView;

    private int mMaxTranslationPx;

    public ToolbarView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mMainView = findViewById(R.id.main_content);
        mArrow = (TintedImageView) findViewById(R.id.arrow);
        mCloseButton = findViewById(R.id.close_button);
        mMenuButton = findViewById(R.id.more);
        mTitle = (TextView) findViewById(R.id.title);
        mShadow = findViewById(R.id.shadow);

        mMaxTranslationPx = getResources().getDimensionPixelSize(
                R.dimen.contextual_suggestions_toolbar_max_translation);
    }

    void setCloseButtonOnClickListener(OnClickListener listener) {
        mCloseButton.setOnClickListener(listener);
    }

    void setMenuButtonDelegate(ListMenuButton.Delegate delegate) {
        mMenuButton.setDelegate(delegate);
    }

    void setMenuButtonVisibility(boolean visible) {
        mMenuButton.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    void setMenuButtonAlpha(float alpha) {
        mMenuButton.setAlpha(alpha);
    }

    void setTitle(String title) {
        mTitle.setText(title);
    }

    void setShadowVisibility(boolean visible) {
        mShadow.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    void setArrowTintResourceId(int resourceId) {
        mArrow.setTint(ApiCompatibilityUtils.getColorStateList(getResources(), resourceId));
    }

    void setSlimPeekEnabled(boolean slimPeekEnabled) {
        if (!slimPeekEnabled) return;

        int expandedHeight = getResources().getDimensionPixelSize(
                R.dimen.bottom_control_container_slim_expanded_height);
        mMainView.getLayoutParams().height = expandedHeight;
        ((MarginLayoutParams) mShadow.getLayoutParams()).topMargin = expandedHeight;
        mArrow.setVisibility(View.VISIBLE);

        // Set initial translation percent.
        setTranslationPercent(1.f);
    }

    void setTranslationPercent(float percent) {
        mMainView.setTranslationY(percent * mMaxTranslationPx);
        mArrow.setTranslationY((percent - 1.f) * mMaxTranslationPx);

        // Animate the main content alpha from 0.f to 1.f from 66% to 0%.
        float mainAlphaPercent = Math.max(0.f, ((1 - percent) - (1.f / 3.f)) * 1.5f);
        mMainView.setAlpha(mainAlphaPercent);

        // Animate the arrow alpha from 1.f to 0.f from 100% to 33%.
        float arrowAlphaPercent = Math.max(0.f, (percent - (1.f / 3.f)) * 1.5f);
        mArrow.setAlpha(arrowAlphaPercent);
    }
}
