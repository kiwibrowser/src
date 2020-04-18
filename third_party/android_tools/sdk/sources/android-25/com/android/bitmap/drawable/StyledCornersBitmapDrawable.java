/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.bitmap.drawable;

import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import android.view.View;

import com.android.bitmap.BitmapCache;

/**
 * A custom ExtendedBitmapDrawable that styles the corners in configurable ways.
 *
 * All four corners can be configured as {@link #CORNER_STYLE_SHARP},
 * {@link #CORNER_STYLE_ROUND}, or {@link #CORNER_STYLE_FLAP}.
 * This is accomplished applying a non-rectangular clip applied to the canvas.
 *
 * A border is draw that conforms to the styled corners.
 *
 * {@link #CORNER_STYLE_FLAP} corners have a colored flap drawn within the bounds.
 */
public class StyledCornersBitmapDrawable extends ExtendedBitmapDrawable {
    private static final String TAG = StyledCornersBitmapDrawable.class.getSimpleName();

    public static final int CORNER_STYLE_SHARP = 0;
    public static final int CORNER_STYLE_ROUND = 1;
    public static final int CORNER_STYLE_FLAP = 2;

    private static final int START_RIGHT = 0;
    private static final int START_BOTTOM = 90;
    private static final int START_LEFT = 180;
    private static final int START_TOP = 270;
    private static final int QUARTER_CIRCLE = 90;
    private static final RectF sRectF = new RectF();

    private final Paint mFlapPaint = new Paint();
    private final Paint mBorderPaint = new Paint();
    private final Paint mCompatibilityModeBackgroundPaint = new Paint();
    private final Path mClipPath = new Path();
    private final Path mCompatibilityModePath = new Path();
    private final float mCornerRoundRadius;
    private final float mCornerFlapSide;

    private int mTopLeftCornerStyle = CORNER_STYLE_SHARP;
    private int mTopRightCornerStyle = CORNER_STYLE_SHARP;
    private int mBottomRightCornerStyle = CORNER_STYLE_SHARP;
    private int mBottomLeftCornerStyle = CORNER_STYLE_SHARP;

    private int mTopStartCornerStyle = CORNER_STYLE_SHARP;
    private int mTopEndCornerStyle = CORNER_STYLE_SHARP;
    private int mBottomEndCornerStyle = CORNER_STYLE_SHARP;
    private int mBottomStartCornerStyle = CORNER_STYLE_SHARP;

    private int mScrimColor;
    private float mBorderWidth;
    private boolean mIsCompatibilityMode;
    private boolean mEatInvalidates;

    /**
     * Create a new StyledCornersBitmapDrawable.
     */
    public StyledCornersBitmapDrawable(Resources res, BitmapCache cache,
            boolean limitDensity, ExtendedOptions opts, float cornerRoundRadius,
            float cornerFlapSide) {
        super(res, cache, limitDensity, opts);

        mCornerRoundRadius = cornerRoundRadius;
        mCornerFlapSide = cornerFlapSide;

        mFlapPaint.setColor(Color.TRANSPARENT);
        mFlapPaint.setStyle(Style.FILL);
        mFlapPaint.setAntiAlias(true);

        mBorderPaint.setColor(Color.TRANSPARENT);
        mBorderPaint.setStyle(Style.STROKE);
        mBorderPaint.setStrokeWidth(mBorderWidth);
        mBorderPaint.setAntiAlias(true);

        mCompatibilityModeBackgroundPaint.setColor(Color.TRANSPARENT);
        mCompatibilityModeBackgroundPaint.setStyle(Style.FILL);
        mCompatibilityModeBackgroundPaint.setAntiAlias(true);

        mScrimColor = Color.TRANSPARENT;
    }

    /**
     * Set the border stroke width of this drawable.
     */
    public void setBorderWidth(final float borderWidth) {
        final boolean changed = mBorderPaint.getStrokeWidth() != borderWidth;
        mBorderPaint.setStrokeWidth(borderWidth);
        mBorderWidth = borderWidth;

        if (changed) {
            invalidateSelf();
        }
    }

    /**
     * Set the border stroke color of this drawable. Set to {@link Color#TRANSPARENT} to disable.
     */
    public void setBorderColor(final int color) {
        final boolean changed = mBorderPaint.getColor() != color;
        mBorderPaint.setColor(color);

        if (changed) {
            invalidateSelf();
        }
    }

    /** Set the corner styles for all four corners specified in RTL friendly ways */
    public void setCornerStylesRelative(int topStart, int topEnd, int bottomEnd, int bottomStart) {
        mTopStartCornerStyle = topStart;
        mTopEndCornerStyle = topEnd;
        mBottomEndCornerStyle = bottomEnd;
        mBottomStartCornerStyle = bottomStart;
        resolveCornerStyles();
    }

    @Override
    public void onLayoutDirectionChangeLocal(int layoutDirection) {
        resolveCornerStyles();
    }

    /**
     * Get the flap color for all corners that have style {@link #CORNER_STYLE_SHARP}.
     */
    public int getFlapColor() {
        return mFlapPaint.getColor();
    }

    /**
     * Set the flap color for all corners that have style {@link #CORNER_STYLE_SHARP}.
     *
     * Use {@link android.graphics.Color#TRANSPARENT} to disable flap colors.
     */
    public void setFlapColor(int flapColor) {
        boolean changed = mFlapPaint.getColor() != flapColor;
        mFlapPaint.setColor(flapColor);

        if (changed) {
            invalidateSelf();
        }
    }

    /**
     * Get the color of the scrim that is drawn over the contents, but under the flaps and borders.
     */
    public int getScrimColor() {
        return mScrimColor;
    }

    /**
     * Set the color of the scrim that is drawn over the contents, but under the flaps and borders.
     *
     * Use {@link android.graphics.Color#TRANSPARENT} to disable the scrim.
     */
    public void setScrimColor(int color) {
        boolean changed = mScrimColor != color;
        mScrimColor = color;

        if (changed) {
            invalidateSelf();
        }
    }

    /**
     * Sets whether we should work around an issue introduced in Android 4.4.3,
     * where a WebView can corrupt the stencil buffer of the canvas when the canvas is clipped
     * using a non-rectangular Path.
     */
    public void setCompatibilityMode(boolean isCompatibilityMode) {
        boolean changed = mIsCompatibilityMode != isCompatibilityMode;
        mIsCompatibilityMode = isCompatibilityMode;

        if (changed) {
            invalidateSelf();
        }
    }

    /**
     * Sets the color of the container that this drawable is in. The given color will be used in
     * {@link #setCompatibilityMode compatibility mode} to draw fake corners to emulate clipped
     * corners.
     */
    public void setCompatibilityModeBackgroundColor(int color) {
        boolean changed = mCompatibilityModeBackgroundPaint.getColor() != color;
        mCompatibilityModeBackgroundPaint.setColor(color);

        if (changed) {
            invalidateSelf();
        }
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        super.onBoundsChange(bounds);

        recalculatePath();
    }

    /**
     * Override draw(android.graphics.Canvas) instead of
     * {@link #onDraw(android.graphics.Canvas)} to clip all the drawable layers.
     */
    @Override
    public void draw(Canvas canvas) {
        final Rect bounds = getBounds();
        if (bounds.isEmpty()) {
            return;
        }

        pauseInvalidate();

        // Clip to path.
        if (!mIsCompatibilityMode) {
            canvas.save();
            canvas.clipPath(mClipPath);
        }

        // Draw parent within path.
        super.draw(canvas);

        // Draw scrim on top of parent.
        canvas.drawColor(mScrimColor);

        // Draw flaps.
        float left = bounds.left + mBorderWidth / 2;
        float top = bounds.top + mBorderWidth / 2;
        float right = bounds.right - mBorderWidth / 2;
        float bottom = bounds.bottom - mBorderWidth / 2;
        RectF flapCornerRectF = sRectF;
        flapCornerRectF.set(0, 0, mCornerFlapSide + mCornerRoundRadius,
                mCornerFlapSide + mCornerRoundRadius);

        if (mTopLeftCornerStyle == CORNER_STYLE_FLAP) {
            flapCornerRectF.offsetTo(left, top);
            canvas.drawRoundRect(flapCornerRectF, mCornerRoundRadius,
                    mCornerRoundRadius, mFlapPaint);
        }
        if (mTopRightCornerStyle == CORNER_STYLE_FLAP) {
            flapCornerRectF.offsetTo(right - mCornerFlapSide, top);
            canvas.drawRoundRect(flapCornerRectF, mCornerRoundRadius,
                    mCornerRoundRadius, mFlapPaint);
        }
        if (mBottomRightCornerStyle == CORNER_STYLE_FLAP) {
            flapCornerRectF.offsetTo(right - mCornerFlapSide, bottom - mCornerFlapSide);
            canvas.drawRoundRect(flapCornerRectF, mCornerRoundRadius,
                    mCornerRoundRadius, mFlapPaint);
        }
        if (mBottomLeftCornerStyle == CORNER_STYLE_FLAP) {
            flapCornerRectF.offsetTo(left, bottom - mCornerFlapSide);
            canvas.drawRoundRect(flapCornerRectF, mCornerRoundRadius,
                    mCornerRoundRadius, mFlapPaint);
        }

        if (!mIsCompatibilityMode) {
            canvas.restore();
        }

        if (mIsCompatibilityMode) {
            drawFakeCornersForCompatibilityMode(canvas);
        }

        // Draw border around path.
        canvas.drawPath(mClipPath, mBorderPaint);

        resumeInvalidate();
    }

    @Override
    public void invalidateSelf() {
        if (!mEatInvalidates) {
            super.invalidateSelf();
        } else {
            Log.d(TAG, "Skipping invalidate.");
        }
    }

    protected void drawFakeCornersForCompatibilityMode(final Canvas canvas) {
        final Rect bounds = getBounds();

        float left = bounds.left;
        float top = bounds.top;
        float right = bounds.right;
        float bottom = bounds.bottom;

        // Draw fake round corners.
        RectF fakeCornerRectF = sRectF;
        fakeCornerRectF.set(0, 0, mCornerRoundRadius * 2, mCornerRoundRadius * 2);
        if (mTopLeftCornerStyle == CORNER_STYLE_ROUND) {
            fakeCornerRectF.offsetTo(left, top);
            mCompatibilityModePath.rewind();
            mCompatibilityModePath.moveTo(left, top);
            mCompatibilityModePath.lineTo(left + mCornerRoundRadius, top);
            mCompatibilityModePath.arcTo(fakeCornerRectF, START_TOP, -QUARTER_CIRCLE);
            mCompatibilityModePath.close();
            canvas.drawPath(mCompatibilityModePath, mCompatibilityModeBackgroundPaint);
        }
        if (mTopRightCornerStyle == CORNER_STYLE_ROUND) {
            fakeCornerRectF.offsetTo(right - fakeCornerRectF.width(), top);
            mCompatibilityModePath.rewind();
            mCompatibilityModePath.moveTo(right, top);
            mCompatibilityModePath.lineTo(right, top + mCornerRoundRadius);
            mCompatibilityModePath.arcTo(fakeCornerRectF, START_RIGHT, -QUARTER_CIRCLE);
            mCompatibilityModePath.close();
            canvas.drawPath(mCompatibilityModePath, mCompatibilityModeBackgroundPaint);
        }
        if (mBottomRightCornerStyle == CORNER_STYLE_ROUND) {
            fakeCornerRectF
                    .offsetTo(right - fakeCornerRectF.width(), bottom - fakeCornerRectF.height());
            mCompatibilityModePath.rewind();
            mCompatibilityModePath.moveTo(right, bottom);
            mCompatibilityModePath.lineTo(right - mCornerRoundRadius, bottom);
            mCompatibilityModePath.arcTo(fakeCornerRectF, START_BOTTOM, -QUARTER_CIRCLE);
            mCompatibilityModePath.close();
            canvas.drawPath(mCompatibilityModePath, mCompatibilityModeBackgroundPaint);
        }
        if (mBottomLeftCornerStyle == CORNER_STYLE_ROUND) {
            fakeCornerRectF.offsetTo(left, bottom - fakeCornerRectF.height());
            mCompatibilityModePath.rewind();
            mCompatibilityModePath.moveTo(left, bottom);
            mCompatibilityModePath.lineTo(left, bottom - mCornerRoundRadius);
            mCompatibilityModePath.arcTo(fakeCornerRectF, START_LEFT, -QUARTER_CIRCLE);
            mCompatibilityModePath.close();
            canvas.drawPath(mCompatibilityModePath, mCompatibilityModeBackgroundPaint);
        }

        // Draw fake flap corners.
        if (mTopLeftCornerStyle == CORNER_STYLE_FLAP) {
            mCompatibilityModePath.rewind();
            mCompatibilityModePath.moveTo(left, top);
            mCompatibilityModePath.lineTo(left + mCornerFlapSide, top);
            mCompatibilityModePath.lineTo(left, top + mCornerFlapSide);
            mCompatibilityModePath.close();
            canvas.drawPath(mCompatibilityModePath, mCompatibilityModeBackgroundPaint);
        }
        if (mTopRightCornerStyle == CORNER_STYLE_FLAP) {
            mCompatibilityModePath.rewind();
            mCompatibilityModePath.moveTo(right, top);
            mCompatibilityModePath.lineTo(right, top + mCornerFlapSide);
            mCompatibilityModePath.lineTo(right - mCornerFlapSide, top);
            mCompatibilityModePath.close();
            canvas.drawPath(mCompatibilityModePath, mCompatibilityModeBackgroundPaint);
        }
        if (mBottomRightCornerStyle == CORNER_STYLE_FLAP) {
            mCompatibilityModePath.rewind();
            mCompatibilityModePath.moveTo(right, bottom);
            mCompatibilityModePath.lineTo(right - mCornerFlapSide, bottom);
            mCompatibilityModePath.lineTo(right, bottom - mCornerFlapSide);
            mCompatibilityModePath.close();
            canvas.drawPath(mCompatibilityModePath, mCompatibilityModeBackgroundPaint);
        }
        if (mBottomLeftCornerStyle == CORNER_STYLE_FLAP) {
            mCompatibilityModePath.rewind();
            mCompatibilityModePath.moveTo(left, bottom);
            mCompatibilityModePath.lineTo(left, bottom - mCornerFlapSide);
            mCompatibilityModePath.lineTo(left + mCornerFlapSide, bottom);
            mCompatibilityModePath.close();
            canvas.drawPath(mCompatibilityModePath, mCompatibilityModeBackgroundPaint);
        }
    }

    private void pauseInvalidate() {
        mEatInvalidates = true;
    }

    private void resumeInvalidate() {
        mEatInvalidates = false;
    }

    private void recalculatePath() {
        Rect bounds = getBounds();

        if (bounds.isEmpty()) {
            return;
        }

        // Setup.
        float left = bounds.left + mBorderWidth / 2;
        float top = bounds.top + mBorderWidth / 2;
        float right = bounds.right - mBorderWidth / 2;
        float bottom = bounds.bottom - mBorderWidth / 2;
        RectF roundedCornerRectF = sRectF;
        roundedCornerRectF.set(0, 0, 2 * mCornerRoundRadius, 2 * mCornerRoundRadius);
        mClipPath.rewind();

        switch (mTopLeftCornerStyle) {
            case CORNER_STYLE_SHARP:
                mClipPath.moveTo(left, top);
                break;
            case CORNER_STYLE_ROUND:
                roundedCornerRectF.offsetTo(left, top);
                mClipPath.arcTo(roundedCornerRectF, START_LEFT, QUARTER_CIRCLE);
                break;
            case CORNER_STYLE_FLAP:
                mClipPath.moveTo(left, top - mCornerFlapSide);
                mClipPath.lineTo(left + mCornerFlapSide, top);
                break;
        }

        switch (mTopRightCornerStyle) {
            case CORNER_STYLE_SHARP:
                mClipPath.lineTo(right, top);
                break;
            case CORNER_STYLE_ROUND:
                roundedCornerRectF.offsetTo(right - roundedCornerRectF.width(), top);
                mClipPath.arcTo(roundedCornerRectF, START_TOP, QUARTER_CIRCLE);
                break;
            case CORNER_STYLE_FLAP:
                mClipPath.lineTo(right - mCornerFlapSide, top);
                mClipPath.lineTo(right, top + mCornerFlapSide);
                break;
        }

        switch (mBottomRightCornerStyle) {
            case CORNER_STYLE_SHARP:
                mClipPath.lineTo(right, bottom);
                break;
            case CORNER_STYLE_ROUND:
                roundedCornerRectF.offsetTo(right - roundedCornerRectF.width(),
                        bottom - roundedCornerRectF.height());
                mClipPath.arcTo(roundedCornerRectF, START_RIGHT, QUARTER_CIRCLE);
                break;
            case CORNER_STYLE_FLAP:
                mClipPath.lineTo(right, bottom - mCornerFlapSide);
                mClipPath.lineTo(right - mCornerFlapSide, bottom);
                break;
        }

        switch (mBottomLeftCornerStyle) {
            case CORNER_STYLE_SHARP:
                mClipPath.lineTo(left, bottom);
                break;
            case CORNER_STYLE_ROUND:
                roundedCornerRectF.offsetTo(left, bottom - roundedCornerRectF.height());
                mClipPath.arcTo(roundedCornerRectF, START_BOTTOM, QUARTER_CIRCLE);
                break;
            case CORNER_STYLE_FLAP:
                mClipPath.lineTo(left + mCornerFlapSide, bottom);
                mClipPath.lineTo(left, bottom - mCornerFlapSide);
                break;
        }

        // Finish.
        mClipPath.close();
    }

    private void resolveCornerStyles() {
        boolean isLtr = getLayoutDirectionLocal() == View.LAYOUT_DIRECTION_LTR;
        setCornerStyles(
            isLtr ? mTopStartCornerStyle : mTopEndCornerStyle,
            isLtr ? mTopEndCornerStyle : mTopStartCornerStyle,
            isLtr ? mBottomEndCornerStyle : mBottomStartCornerStyle,
            isLtr ? mBottomStartCornerStyle : mBottomEndCornerStyle);
    }

    /** Set the corner styles for all four corners */
    private void setCornerStyles(int topLeft, int topRight, int bottomRight, int bottomLeft) {
        boolean changed = mTopLeftCornerStyle != topLeft
            || mTopRightCornerStyle != topRight
            || mBottomRightCornerStyle != bottomRight
            || mBottomLeftCornerStyle != bottomLeft;

        mTopLeftCornerStyle = topLeft;
        mTopRightCornerStyle = topRight;
        mBottomRightCornerStyle = bottomRight;
        mBottomLeftCornerStyle = bottomLeft;

        if (changed) {
            recalculatePath();
        }
    }
}
