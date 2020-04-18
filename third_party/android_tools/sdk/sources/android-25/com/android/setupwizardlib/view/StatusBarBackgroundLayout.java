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
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Build.VERSION_CODES;
import android.util.AttributeSet;
import android.view.WindowInsets;
import android.widget.FrameLayout;

import com.android.setupwizardlib.R;

/**
 * A FrameLayout subclass that will responds to onApplyWindowInsets to draw a drawable in the top
 * inset area, making a background effect for the navigation bar. To make use of this layout,
 * specify the system UI visibility {@link android.view.View#SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN} and
 * set specify fitsSystemWindows.
 *
 * <p>This view is a normal FrameLayout if either of those are not set, or if the platform version
 * is lower than Lollipop.
 */
public class StatusBarBackgroundLayout extends FrameLayout {

    private Drawable mStatusBarBackground;
    private Object mLastInsets;  // Use generic Object type for compatibility

    public StatusBarBackgroundLayout(Context context) {
        super(context);
        init(context, null, 0);
    }

    public StatusBarBackgroundLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context, attrs, 0);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public StatusBarBackgroundLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs, defStyleAttr);
    }

    private void init(Context context, AttributeSet attrs, int defStyleAttr) {
        final TypedArray a = context.obtainStyledAttributes(attrs,
                R.styleable.SuwStatusBarBackgroundLayout, defStyleAttr, 0);
        final Drawable statusBarBackground =
                a.getDrawable(R.styleable.SuwStatusBarBackgroundLayout_suwStatusBarBackground);
        setStatusBarBackground(statusBarBackground);
        a.recycle();
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (Build.VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            if (mLastInsets == null) {
                requestApplyInsets();
            }
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (Build.VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            if (mLastInsets != null) {
                final int insetTop = ((WindowInsets) mLastInsets).getSystemWindowInsetTop();
                if (insetTop > 0) {
                    mStatusBarBackground.setBounds(0, 0, getWidth(), insetTop);
                    mStatusBarBackground.draw(canvas);
                }
            }
        }
    }

    public void setStatusBarBackground(Drawable background) {
        mStatusBarBackground = background;
        if (Build.VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            setWillNotDraw(background == null);
            setFitsSystemWindows(background != null);
            invalidate();
        }
    }

    public Drawable getStatusBarBackground() {
        return mStatusBarBackground;
    }

    @Override
    public WindowInsets onApplyWindowInsets(WindowInsets insets) {
        mLastInsets = insets;
        return super.onApplyWindowInsets(insets);
    }
}
