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
import android.content.pm.ApplicationInfo;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;
import android.util.AttributeSet;
import android.util.LayoutDirection;
import android.view.Gravity;
import android.view.ViewOutlineProvider;
import android.widget.FrameLayout;

import com.android.setupwizardlib.R;

/**
 * Class to draw the illustration of setup wizard. The {@code aspectRatio} attribute determines the
 * aspect ratio of the top padding, which leaves space for the illustration. Draws the illustration
 * drawable to fit the width of the view and fills the rest with the background.
 *
 * <p>If an aspect ratio is set, then the aspect ratio of the source drawable is maintained.
 * Otherwise the the aspect ratio will be ignored, only increasing the width of the illustration.
 */
public class Illustration extends FrameLayout {

    // Size of the baseline grid in pixels
    private float mBaselineGridSize;
    private Drawable mBackground;
    private Drawable mIllustration;
    private final Rect mViewBounds = new Rect();
    private final Rect mIllustrationBounds = new Rect();
    private float mScale = 1.0f;
    private float mAspectRatio = 0.0f;

    public Illustration(Context context) {
        super(context);
        init(null, 0);
    }

    public Illustration(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, 0);
    }

    @TargetApi(VERSION_CODES.HONEYCOMB)
    public Illustration(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(attrs, defStyleAttr);
    }

    // All the constructors delegate to this init method. The 3-argument constructor is not
    // available in FrameLayout before v11, so call super with the exact same arguments.
    private void init(AttributeSet attrs, int defStyleAttr) {
        if (attrs != null) {
            TypedArray a = getContext().obtainStyledAttributes(attrs,
                    R.styleable.SuwIllustration, defStyleAttr, 0);
            mAspectRatio = a.getFloat(R.styleable.SuwIllustration_suwAspectRatio, 0.0f);
            a.recycle();
        }
        // Number of pixels of the 8dp baseline grid as defined in material design specs
        mBaselineGridSize = getResources().getDisplayMetrics().density * 8;
        setWillNotDraw(false);
    }

    /**
     * The background will be drawn to fill up the rest of the view. It will also be scaled by the
     * same amount as the foreground so their textures look the same.
     */
    // Override the deprecated setBackgroundDrawable method to support API < 16. View.setBackground
    // forwards to setBackgroundDrawable in the framework implementation.
    @SuppressWarnings("deprecation")
    @Override
    public void setBackgroundDrawable(Drawable background) {
        if (background == mBackground) {
            return;
        }
        mBackground = background;
        invalidate();
        requestLayout();
    }

    /**
     * Sets the drawable used as the illustration. The drawable is expected to have intrinsic
     * width and height defined and will be scaled to fit the width of the view.
     */
    public void setIllustration(Drawable illustration) {
        if (illustration == mIllustration) {
            return;
        }
        mIllustration = illustration;
        invalidate();
        requestLayout();
    }

    /**
     * Set the aspect ratio reserved for the illustration. This overrides the top padding of the
     * view according to the width of this view and the aspect ratio. Children views will start
     * being laid out below this aspect ratio.
     *
     * @param aspectRatio A float value specifying the aspect ratio (= width / height). 0 to not
     *                    override the top padding.
     */
    public void setAspectRatio(float aspectRatio) {
        mAspectRatio = aspectRatio;
        invalidate();
        requestLayout();
    }

    @Override
    @Deprecated
    public void setForeground(Drawable d) {
        setIllustration(d);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        if (mAspectRatio != 0.0f) {
            int parentWidth = MeasureSpec.getSize(widthMeasureSpec);
            int illustrationHeight = (int) (parentWidth / mAspectRatio);
            illustrationHeight -= illustrationHeight % mBaselineGridSize;
            setPadding(0, illustrationHeight, 0, 0);
        }
        if (VERSION.SDK_INT >= VERSION_CODES.LOLLIPOP) {
            //noinspection AndroidLintInlinedApi
            setOutlineProvider(ViewOutlineProvider.BOUNDS);
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        final int layoutWidth = right - left;
        final int layoutHeight = bottom - top;
        if (mIllustration != null) {
            int intrinsicWidth = mIllustration.getIntrinsicWidth();
            int intrinsicHeight = mIllustration.getIntrinsicHeight();

            mViewBounds.set(0, 0, layoutWidth, layoutHeight);
            if (mAspectRatio != 0f) {
                mScale = layoutWidth / (float) intrinsicWidth;
                intrinsicWidth = layoutWidth;
                intrinsicHeight = (int) (intrinsicHeight * mScale);
            }
            Gravity.apply(Gravity.FILL_HORIZONTAL | Gravity.TOP, intrinsicWidth,
                    intrinsicHeight, mViewBounds, mIllustrationBounds);
            mIllustration.setBounds(mIllustrationBounds);
        }
        if (mBackground != null) {
            // Scale the background bounds by the same scale to compensate for the scale done to the
            // canvas in onDraw.
            mBackground.setBounds(0, 0, (int) Math.ceil(layoutWidth / mScale),
                    (int) Math.ceil((layoutHeight - mIllustrationBounds.height()) / mScale));
        }
        super.onLayout(changed, left, top, right, bottom);
    }

    @Override
    public void onDraw(Canvas canvas) {
        if (mBackground != null) {
            // Draw the background filling parts not covered by the illustration
            canvas.save();
            canvas.translate(0, mIllustrationBounds.height());
            // Scale the background so its size matches the foreground
            canvas.scale(mScale, mScale, 0, 0);
            if (VERSION.SDK_INT > VERSION_CODES.JELLY_BEAN_MR1 &&
                    shouldMirrorDrawable(mBackground, getLayoutDirection())) {
                // Flip the illustration for RTL layouts
                canvas.scale(-1, 1);
                canvas.translate(-mBackground.getBounds().width(), 0);
            }
            mBackground.draw(canvas);
            canvas.restore();
        }
        if (mIllustration != null) {
            canvas.save();
            if (VERSION.SDK_INT > VERSION_CODES.JELLY_BEAN_MR1 &&
                    shouldMirrorDrawable(mIllustration, getLayoutDirection())) {
                // Flip the illustration for RTL layouts
                canvas.scale(-1, 1);
                canvas.translate(-mIllustrationBounds.width(), 0);
            }
            // Draw the illustration
            mIllustration.draw(canvas);
            canvas.restore();
        }
        super.onDraw(canvas);
    }

    private boolean shouldMirrorDrawable(Drawable drawable, int layoutDirection) {
        if (layoutDirection == LayoutDirection.RTL) {
            if (VERSION.SDK_INT >= VERSION_CODES.KITKAT) {
                return drawable.isAutoMirrored();
            } else if (VERSION.SDK_INT >= VERSION_CODES.JELLY_BEAN_MR1) {
                final int flags = getContext().getApplicationInfo().flags;
                //noinspection AndroidLintInlinedApi
                return (flags & ApplicationInfo.FLAG_SUPPORTS_RTL) != 0;
            }
        }
        return false;
    }
}
