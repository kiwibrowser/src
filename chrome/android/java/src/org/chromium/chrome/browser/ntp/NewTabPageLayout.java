// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.RippleDrawable;
import android.os.Build;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.suggestions.SiteSection;
import org.chromium.chrome.browser.suggestions.TileGridLayout;

/**
 * Layout for the new tab page. This positions the page elements in the correct vertical positions.
 * There are no separate phone and tablet UIs; this layout adapts based on the available space.
 */
public class NewTabPageLayout extends LinearLayout {
    private final int mTileGridLayoutBleed;
    private final int mSearchboxShadowWidth;

    private View mMiddleSpacer; // Spacer between toolbar and Most Likely.

    private LogoView mSearchProviderLogoView;
    private View mSearchBoxView;
    private ViewGroup mSiteSectionView;

    /**
     * Constructor for inflating from XML.
     */
    public NewTabPageLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        Resources res = getResources();
        mTileGridLayoutBleed = res.getDimensionPixelSize(R.dimen.tile_grid_layout_bleed);
        mSearchboxShadowWidth = res.getDimensionPixelOffset(R.dimen.ntp_search_box_shadow_width);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mMiddleSpacer = findViewById(R.id.ntp_middle_spacer);
        mSearchProviderLogoView = findViewById(R.id.search_provider_logo);
        mSearchBoxView = findViewById(R.id.search_box);
        insertSiteSectionView();
    }

    public void insertSiteSectionView() {
        mSiteSectionView = SiteSection.inflateSiteSection(this);
        ViewGroup.LayoutParams layoutParams = mSiteSectionView.getLayoutParams();
        layoutParams.width = ViewGroup.LayoutParams.WRAP_CONTENT;
        mSiteSectionView.setLayoutParams(layoutParams);

        int insertionPoint = indexOfChild(mMiddleSpacer) + 1;
        addView(mSiteSectionView, insertionPoint);
    }

    /**
     * @return the embedded {@link TileGridLayout}.
     */
    public ViewGroup getSiteSectionView() {
        return mSiteSectionView;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        unifyElementWidths();
    }

    /**
     * Makes the Search Box and Logo as wide as Most Visited.
     */
    private void unifyElementWidths() {
        if (mSiteSectionView.getVisibility() != GONE) {
            final int width = mSiteSectionView.getMeasuredWidth() - mTileGridLayoutBleed;
            measureExactly(mSearchBoxView,
                    width + mSearchboxShadowWidth, mSearchBoxView.getMeasuredHeight());
            measureExactly(mSearchProviderLogoView,
                    width, mSearchProviderLogoView.getMeasuredHeight());
        }
    }

    /**
     * Convenience method to call measure() on the given View with MeasureSpecs converted from the
     * given dimensions (in pixels) with MeasureSpec.EXACTLY.
     */
    private static void measureExactly(View view, int widthPx, int heightPx) {
        view.measure(MeasureSpec.makeMeasureSpec(widthPx, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(heightPx, MeasureSpec.EXACTLY));
    }

    /**
     * Provides the additional capabilities needed for the SearchBox container layout.
     */
    public static class SearchBoxContainerView extends LinearLayout {
        /** Constructor for inflating from XML. */
        public SearchBoxContainerView(Context context, AttributeSet attrs) {
            super(context, attrs);
        }

        @Override
        public boolean onInterceptTouchEvent(MotionEvent ev) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
                    && ev.getActionMasked() == MotionEvent.ACTION_DOWN) {
                if (getBackground() instanceof RippleDrawable) {
                    ((RippleDrawable) getBackground()).setHotspot(ev.getX(), ev.getY());
                }
            }
            return super.onInterceptTouchEvent(ev);
        }
    }
}
