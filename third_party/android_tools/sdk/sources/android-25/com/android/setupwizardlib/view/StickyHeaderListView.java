/*
 * Copyright (C) 2015 The Android Open Source Project
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
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.RectF;
import android.os.Build;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.accessibility.AccessibilityEvent;
import android.widget.ListView;

import com.android.setupwizardlib.R;

/**
 * This class provides sticky header functionality in a list view, to use with
 * SetupWizardIllustration. To use this, add a header tagged with "sticky", or a header tagged with
 * "stickyContainer" and one of its child tagged as "sticky". The sticky container will be drawn
 * when the sticky element hits the top of the view.
 *
 * <p>There are a few things to note:
 * <ol>
 *   <li>The two supported scenarios are StickyHeaderListView -> Header (stickyContainer) -> sticky,
 *   and StickyHeaderListView -> Header (sticky). The arrow (->) represents parent/child
 *   relationship and must be immediate child.
 *   <li>The view does not work well with padding. b/16190933
 *   <li>If fitsSystemWindows is true, then this will offset the sticking position by the height of
 *   the system decorations at the top of the screen.
 * </ol>
 *
 * @see StickyHeaderScrollView
 */
public class StickyHeaderListView extends ListView {

    private View mSticky;
    private View mStickyContainer;
    private int mStatusBarInset = 0;
    private RectF mStickyRect = new RectF();

    public StickyHeaderListView(Context context) {
        super(context);
        init(null, android.R.attr.listViewStyle);
    }

    public StickyHeaderListView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, android.R.attr.listViewStyle);
    }

    public StickyHeaderListView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(attrs, defStyleAttr);
    }

    private void init(AttributeSet attrs, int defStyleAttr) {
        final TypedArray a = getContext().obtainStyledAttributes(attrs,
                R.styleable.SuwStickyHeaderListView, defStyleAttr, 0);
        int headerResId = a.getResourceId(R.styleable.SuwStickyHeaderListView_suwHeader, 0);
        if (headerResId != 0) {
            LayoutInflater inflater = LayoutInflater.from(getContext());
            View header = inflater.inflate(headerResId, this, false);
            addHeaderView(header, null, false);
        }
        a.recycle();
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if (mSticky == null) {
            updateStickyView();
        }
    }

    public void updateStickyView() {
        mSticky = findViewWithTag("sticky");
        mStickyContainer = findViewWithTag("stickyContainer");
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (mStickyRect.contains(ev.getX(), ev.getY())) {
            ev.offsetLocation(-mStickyRect.left, -mStickyRect.top);
            return mStickyContainer.dispatchTouchEvent(ev);
        } else {
            return super.dispatchTouchEvent(ev);
        }
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);
        if (mSticky != null) {
            final int saveCount = canvas.save();
            // The view to draw when sticking to the top
            final View drawTarget = mStickyContainer != null ? mStickyContainer : mSticky;
            // The offset to draw the view at when sticky
            final int drawOffset = mStickyContainer != null ? mSticky.getTop() : 0;
            // Position of the draw target, relative to the outside of the scrollView
            final int drawTop = drawTarget.getTop();
            if (drawTop + drawOffset < mStatusBarInset || !drawTarget.isShown()) {
                // ListView does not translate the canvas, so we can simply draw at the top
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
    public void onInitializeAccessibilityEvent(AccessibilityEvent event) {
        super.onInitializeAccessibilityEvent(event);

        // Decoration-only headers should not count as an item for accessibility, adjust the
        // accessibility event to account for that.
        final int numberOfHeaders = mSticky != null ? 1 : 0;
        event.setItemCount(event.getItemCount() - numberOfHeaders);
        event.setFromIndex(Math.max(event.getFromIndex() - numberOfHeaders, 0));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            event.setToIndex(Math.max(event.getToIndex() - numberOfHeaders, 0));
        }
    }
}
