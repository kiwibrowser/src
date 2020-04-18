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
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.graphics.Shader.TileMode;
import android.graphics.drawable.BitmapDrawable;

import com.android.bitmap.BitmapCache;

/**
 * Custom BasicBitmapDrawable implementation for circular images.
 *
 * This draws all bitmaps as a circle with an optional border stroke.
 */
public class CircularBitmapDrawable extends ExtendedBitmapDrawable {
    private final Paint mBitmapPaint = new Paint();
    private final Paint mBorderPaint = new Paint();
    private final Rect mRect = new Rect();
    private final Matrix mMatrix = new Matrix();

    private float mBorderWidth;
    private Bitmap mShaderBitmap;

    public CircularBitmapDrawable(Resources res,
            BitmapCache cache, boolean limitDensity) {
        this(res, cache, limitDensity, null);
    }

    public CircularBitmapDrawable(Resources res,
            BitmapCache cache, boolean limitDensity, ExtendedOptions opts) {
        super(res, cache, limitDensity, opts);

        mBitmapPaint.setAntiAlias(true);
        mBitmapPaint.setFilterBitmap(true);
        mBitmapPaint.setDither(true);

        mBorderPaint.setColor(Color.TRANSPARENT);
        mBorderPaint.setStyle(Style.STROKE);
        mBorderPaint.setStrokeWidth(mBorderWidth);
        mBorderPaint.setAntiAlias(true);
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

    @Override
    protected void onDrawBitmap(final Canvas canvas, final Rect src,
            final Rect dst) {
        onDrawCircularBitmap(getBitmap().bmp, canvas, src, dst, 1f);
    }

    @Override
    protected void onDrawPlaceholderOrProgress(final Canvas canvas,
            final TileDrawable drawable) {
        Rect bounds = getBounds();
        if (drawable.getInnerDrawable() instanceof BitmapDrawable) {
            BitmapDrawable placeholder =
                (BitmapDrawable) drawable.getInnerDrawable();
            Bitmap bitmap = placeholder.getBitmap();
            float alpha = placeholder.getPaint().getAlpha() / 255f;
            mRect.set(0, 0, bitmap.getWidth(), bitmap.getHeight());
            onDrawCircularBitmap(bitmap, canvas, mRect, bounds, alpha);
        } else {
          super.onDrawPlaceholderOrProgress(canvas, drawable);
        }

        // Then draw the border.
        canvas.drawCircle(bounds.centerX(), bounds.centerY(),
                bounds.width() / 2f - mBorderWidth / 2, mBorderPaint);
    }

    /**
     * Call this method with a given bitmap to draw it onto the given canvas, masked by a circular
     * BitmapShader.
     */
    protected void onDrawCircularBitmap(final Bitmap bitmap, final Canvas canvas,
            final Rect src, final Rect dst) {
        onDrawCircularBitmap(bitmap, canvas, src, dst, 1f);
    }

    /**
     * Call this method with a given bitmap to draw it onto the given canvas, masked by a circular
     * BitmapShader. The alpha parameter is the value from 0f to 1f to attenuate the alpha by.
     */
    protected void onDrawCircularBitmap(final Bitmap bitmap, final Canvas canvas,
            final Rect src, final Rect dst, final float alpha) {
        // Draw bitmap through shader first.
        BitmapShader shader = (BitmapShader) mBitmapPaint.getShader();
        if (shader == null || mShaderBitmap != bitmap) {
          shader = new BitmapShader(bitmap, TileMode.CLAMP, TileMode.CLAMP);
          mShaderBitmap = bitmap;
        }

        mMatrix.reset();
        // Fit bitmap to bounds.
        float scale = Math.max((float) dst.width() / src.width(),
                (float) dst.height() / src.height());
        mMatrix.postScale(scale, scale);
        // Translate bitmap to dst bounds.
        mMatrix.postTranslate(dst.left, dst.top);
        shader.setLocalMatrix(mMatrix);
        mBitmapPaint.setShader(shader);

        int oldAlpha = mBitmapPaint.getAlpha();
        mBitmapPaint.setAlpha((int) (oldAlpha * alpha));
        canvas.drawCircle(dst.centerX(), dst.centerY(), dst.width() / 2,
                mBitmapPaint);
        mBitmapPaint.setAlpha(oldAlpha);

        // Then draw the border.
        canvas.drawCircle(dst.centerX(), dst.centerY(),
                dst.width() / 2f - mBorderWidth / 2, mBorderPaint);
    }

    @Override
    public void setAlpha(int alpha) {
        super.setAlpha(alpha);

        final int old = mBitmapPaint.getAlpha();
        mBitmapPaint.setAlpha(alpha);
        if (alpha != old) {
            invalidateSelf();
        }
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
        super.setColorFilter(cf);
        mPaint.setColorFilter(cf);
    }
}
