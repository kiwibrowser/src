// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.gesturenav;

import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.support.annotation.ColorInt;

import org.chromium.chrome.R;

/**
 * Arrow drawable indicating forward/backward direction for Material theme.
 */
class ArrowDrawable extends Drawable {
    private static final int CIRCLE_DIAMETER_DP = 40;
    private static final float CENTER_RADIUS_DP = 8.75f;
    private static final float STROKE_WIDTH_DP = 2.5f;
    private static final int ARROW_WIDTH_DP = 10;

    private final Resources mResources;

    private final RectF mTempBounds = new RectF();
    private final Paint mPaint = new Paint();
    private final Paint mArrowFill = new Paint();
    private final Path mArrow = new Path();

    private final float mArrowWidth;
    private final float mStrokeWidth;
    private final float mStrokeInset;

    private float mArrowScale;
    private float mCenterRadius;
    private float mWidth;
    private float mHeight;
    private int mAlpha;
    private @ColorInt int mBackgroundColor;
    private @ColorInt int mForegroundColor;
    private boolean mIsForward;

    public ArrowDrawable(Resources resources) {
        mResources = resources;
        mForegroundColor = mResources.getColor(R.color.light_active_color);

        float screenDensity = mResources.getDisplayMetrics().density;
        mWidth = CIRCLE_DIAMETER_DP * screenDensity;
        mHeight = CIRCLE_DIAMETER_DP * screenDensity;
        mArrowWidth = ARROW_WIDTH_DP * screenDensity;
        mStrokeWidth = STROKE_WIDTH_DP * screenDensity;
        mCenterRadius = CENTER_RADIUS_DP * screenDensity;
        mStrokeInset = Math.min(mWidth, mHeight) / 2.0f - mCenterRadius;

        mPaint.setStrokeCap(Paint.Cap.SQUARE);
        mPaint.setAntiAlias(true);
        mPaint.setStyle(Style.STROKE);
        mPaint.setStrokeWidth(mStrokeWidth);

        mArrow.setFillType(android.graphics.Path.FillType.EVEN_ODD);
        mArrowFill.setStyle(Paint.Style.FILL);
        mArrowFill.setAntiAlias(true);
    }

    /**
     * Sets the direction the arrow points to.
     * @param forward {@code true} if the arrow points forward.
     */
    public void setDirection(boolean forward) {
        mIsForward = forward;
    }

    /**
     * Set the scale of the arrow.
     * @param scale Scale value of the range 0..1
     */
    public void setArrowScale(float scale) {
        if (scale == mArrowScale) return;
        mArrowScale = scale;
        invalidateSelf();
    }

    /**
     * Update the background color to match the parent view.
     * @param color Background color.
     */
    public void setBackgroundColor(@ColorInt int color) {
        mBackgroundColor = color;
    }

    public void setForegroundColor(@ColorInt int color) {
        mForegroundColor = color;
    }

    @Override
    public int getIntrinsicHeight() {
        return (int) mHeight;
    }

    @Override
    public int getIntrinsicWidth() {
        return (int) mWidth;
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }

    @Override
    public void setColorFilter(ColorFilter filter) {
        mPaint.setColorFilter(filter);
        invalidateSelf();
    }

    @Override
    public void setAlpha(int alpha) {
        if (mAlpha == alpha) return;
        mAlpha = alpha;
        invalidateSelf();
    }

    @Override
    public int getAlpha() {
        return mAlpha;
    }

    @Override
    public void draw(Canvas c) {
        final Rect bounds = getBounds();
        final int saveCount = c.save();
        final RectF lineBounds = mTempBounds;

        lineBounds.set(bounds);
        lineBounds.inset(mStrokeInset, mStrokeInset);
        mPaint.setColor(mForegroundColor);

        c.drawLine(lineBounds.left, (lineBounds.top + lineBounds.bottom) / 2, lineBounds.right,
                (lineBounds.top + lineBounds.bottom) / 2, mPaint);

        float delta = mArrowWidth * mArrowScale / 2f;
        mArrow.reset();
        mArrow.moveTo(delta, -delta);
        mArrow.lineTo(delta, delta);
        mArrow.lineTo(delta * 2, 0);

        // Adjust the position of the triangle so that it is positioned at the end of the line.
        float inset = (int) mStrokeInset / 2f * mArrowScale;
        float x = (float) (mCenterRadius + bounds.exactCenterX());
        float y = (float) bounds.exactCenterY();
        mArrow.offset(x - inset, y);
        mArrow.close();
        mArrowFill.setColor(mForegroundColor);
        if (!mIsForward) c.rotate(180, bounds.exactCenterX(), bounds.exactCenterY());
        c.drawPath(mArrow, mArrowFill);
        c.restoreToCount(saveCount);
    }
}
