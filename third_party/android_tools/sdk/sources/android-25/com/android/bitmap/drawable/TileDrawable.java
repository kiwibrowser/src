/*
 * Copyright (C) 2013 The Android Open Source Project
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

import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;

import com.android.bitmap.drawable.ExtendedBitmapDrawable.ExtendedOptions;

/**
 * A drawable that wraps another drawable and places it in the center of this space. This drawable
 * allows a background color for the "tile", and has a fade-out transition when
 * {@link #setVisible(boolean, boolean)} indicates that it is no longer visible.
 */
public class TileDrawable extends Drawable implements Drawable.Callback {

    private final ExtendedOptions mOpts;
    private final Paint mPaint = new Paint();
    private final Drawable mInner;
    private final int mInnerWidth;
    private final int mInnerHeight;

    protected final ValueAnimator mFadeOutAnimator;

    public TileDrawable(Drawable inner, int innerWidth, int innerHeight, int fadeOutDurationMs,
            ExtendedOptions opts) {
        mOpts = opts;
        mInner = inner != null ? inner.mutate() : null;
        mInnerWidth = innerWidth;
        mInnerHeight = innerHeight;
        if (inner != null) {
            mInner.setCallback(this);
        }

        mFadeOutAnimator = ValueAnimator.ofInt(255, 0)
                .setDuration(fadeOutDurationMs);
        mFadeOutAnimator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                setAlpha((Integer) animation.getAnimatedValue());
            }
        });

        reset();
    }

    public void reset() {
        setAlpha(0);
        setVisible(false);
    }

    public Drawable getInnerDrawable() {
        return mInner;
    }

    @Override
    protected void onBoundsChange(Rect bounds) {
        super.onBoundsChange(bounds);

        if (mInner == null) {
            return;
        }

        if (bounds.isEmpty()) {
            mInner.setBounds(0, 0, 0, 0);
        } else {
            final int l = bounds.left + (bounds.width() / 2) - (mInnerWidth / 2);
            final int t = bounds.top + (bounds.height() / 2) - (mInnerHeight / 2);
            mInner.setBounds(l, t, l + mInnerWidth, t + mInnerHeight);
        }
    }

    @Override
    public void draw(Canvas canvas) {
        if (!isVisible() && mPaint.getAlpha() == 0) {
            return;
        }
        final int alpha = mPaint.getAlpha();
        mPaint.setColor(mOpts.backgroundColor);
        mPaint.setAlpha(alpha);
        canvas.drawRect(getBounds(), mPaint);
        if (mInner != null) mInner.draw(canvas);
    }

    @Override
    public void setAlpha(int alpha) {
        final int old = mPaint.getAlpha();
        mPaint.setAlpha(alpha);
        setInnerAlpha(alpha);
        if (alpha != old) {
            invalidateSelf();
        }
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
        mPaint.setColorFilter(cf);
        if (mInner != null) mInner.setColorFilter(cf);
    }

    @Override
    public int getOpacity() {
        return 0;
    }

    protected int getCurrentAlpha() {
        return mPaint.getAlpha();
    }

    public boolean setVisible(boolean visible) {
        return setVisible(visible, true /* dontcare */);
    }

    @Override
    public boolean setVisible(boolean visible, boolean restart) {
        if (mInner != null) mInner.setVisible(visible, restart);
        final boolean changed = super.setVisible(visible, restart);
        if (changed) {
            if (isVisible()) {
                // pop in (no-op)
                // the transition will still be smooth if the previous state's layer fades out
                mFadeOutAnimator.cancel();
                setAlpha(255);
            } else {
                // fade out
                if (mPaint.getAlpha() == 255) {
                    mFadeOutAnimator.start();
                }
            }
        }
        return changed;
    }

    @Override
    protected boolean onLevelChange(int level) {
        if (mInner != null)
            return mInner.setLevel(level);
        else {
            return super.onLevelChange(level);
        }
    }

    /**
     * Changes the alpha on just the inner wrapped drawable.
     */
    public void setInnerAlpha(int alpha) {
        if (mInner != null) mInner.setAlpha(alpha);
    }

    @Override
    public void invalidateDrawable(Drawable who) {
        invalidateSelf();
    }

    @Override
    public void scheduleDrawable(Drawable who, Runnable what, long when) {
        scheduleSelf(what, when);
    }

    @Override
    public void unscheduleDrawable(Drawable who, Runnable what) {
        unscheduleSelf(what);
    }
}
