/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.setupwizardlib.view;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.RectF;
import android.os.Build;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;

/**
 * This class provides sticky header functionality in a recycler view, to use with
 * SetupWizardIllustration. To use this, add a header tagged with "sticky". The header will continue
 * to be drawn when the sticky element hits the top of the view.
 *
 * <p>There are a few things to note:
 * <ol>
 *   <li>The view does not work well with padding. b/16190933
 *   <li>If fitsSystemWindows is true, then this will offset the sticking position by the height of
 *   the system decorations at the top of the screen.
 * </ol>
 */
public class StickyHeaderRecyclerView extends HeaderRecyclerView {

    private View mSticky;
    private int mStatusBarInset = 0;
    private RectF mStickyRect = new RectF();

    public StickyHeaderRecyclerView(Context context) {
        super(context);
    }

    public StickyHeaderRecyclerView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public StickyHeaderRecyclerView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if (mSticky == null) {
            updateStickyView();
        }
        if (mSticky != null) {
            final View headerView = getHeader();
            if (headerView != null && headerView.getHeight() == 0) {
                headerView.layout(0, -headerView.getMeasuredHeight(),
                        headerView.getMeasuredWidth(), 0);
            }
        }
    }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec) {
        super.onMeasure(widthSpec, heightSpec);
        if (mSticky != null) {
            measureChild(getHeader(), widthSpec, heightSpec);
        }
    }

    public void updateStickyView() {
        final View header = getHeader();
        if (header != null) {
            mSticky = header.findViewWithTag("sticky");
        }
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);
        if (mSticky != null) {
            final View headerView = getHeader();
            final int saveCount = canvas.save();
            // The view to draw when sticking to the top
            final View drawTarget = headerView != null ? headerView : mSticky;
            // The offset to draw the view at when sticky
            final int drawOffset = headerView != null ? mSticky.getTop() : 0;
            // Position of the draw target, relative to the outside of the scrollView
            final int drawTop = drawTarget.getTop();
            if (drawTop + drawOffset < mStatusBarInset || !drawTarget.isShown()) {
                // RecyclerView does not translate the canvas, so we can simply draw at the top
                mStickyRect.set(0, -drawOffset + mStatusBarInset, drawTarget.getWidth(),
                        drawTarget.getHeight() - drawOffset + mStatusBarInset);
                canvas.translate(0, mStickyRect.top);
                canvas.clipRect(0, 0, drawTarget.getWidth(), drawTarget.getHeight());
                drawTarget.draw(canvas);
            } else {
                mStickyRect.setEmpty();
            }
            canvas.restoreToCount(saveCount);
        }
    }

    @Override
    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public WindowInsets onApplyWindowInsets(WindowInsets insets) {
        if (getFitsSystemWindows()) {
            mStatusBarInset = insets.getSystemWindowInsetTop();
            insets.replaceSystemWindowInsets(
                    insets.getSystemWindowInsetLeft(),
                    0, /* top */
                    insets.getSystemWindowInsetRight(),
                    insets.getSystemWindowInsetBottom()
            );
        }
        return insets;
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (mStickyRect.contains(ev.getX(), ev.getY())) {
            ev.offsetLocation(-mStickyRect.left, -mStickyRect.top);
            return getHeader().dispatchTouchEvent(ev);
        } else {
            return super.dispatchTouchEvent(ev);
        }
    }
}
