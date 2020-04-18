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

package com.android.setupwizardlib;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build;

import com.android.setupwizardlib.annotations.VisibleForTesting;

import java.lang.ref.SoftReference;

/**
 * This class draws the GLIF pattern used as the status bar background for phones and background for
 * tablets in GLIF layout.
 */
public class GlifPatternDrawable extends Drawable {
    /*
     * This class essentially implements a simple SVG in Java code, with some special handling of
     * scaling when given different bounds.
     */

    /* static section */

    @SuppressLint("InlinedApi")
    private static final int[] ATTRS_PRIMARY_COLOR = new int[]{ android.R.attr.colorPrimary };

    private static final float VIEWBOX_HEIGHT = 768f;
    private static final float VIEWBOX_WIDTH = 1366f;
    // X coordinate of scale focus, as a fraction of of the width. (Range is 0 - 1)
    private static final float SCALE_FOCUS_X = .146f;
    // Y coordinate of scale focus, as a fraction of of the height. (Range is 0 - 1)
    private static final float SCALE_FOCUS_Y = .228f;

    // Alpha component of the color to be drawn, on top of the grayscale pattern. (Range is 0 - 1)
    private static final float COLOR_ALPHA = .8f;
    // Int version of COLOR_ALPHA. (Range is 0 - 255)
    private static final int COLOR_ALPHA_INT = (int) (COLOR_ALPHA * 255);

    // Cap the bitmap size, such that it won't hurt the performance too much
    // and it won't crash due to a very large scale.
    // The drawable will look blurry above this size.
    // This is a multiplier applied on top of the viewbox size.
    // Resulting max cache size = (1.5 x 1366, 1.5 x 768) = (2049, 1152)
    private static final float MAX_CACHED_BITMAP_SCALE = 1.5f;

    private static final int NUM_PATHS = 7;

    private static SoftReference<Bitmap> sBitmapCache;
    private static Path[] sPatternPaths;
    private static int[] sPatternLightness;

    public static GlifPatternDrawable getDefault(Context context) {
        int colorPrimary = 0;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            final TypedArray a = context.obtainStyledAttributes(ATTRS_PRIMARY_COLOR);
            colorPrimary = a.getColor(0, Color.BLACK);
            a.recycle();
        }
        return new GlifPatternDrawable(colorPrimary);
    }

    @VisibleForTesting
    public static void invalidatePattern() {
        sBitmapCache = null;
    }

    /* non-static section */

    private int mColor;
    private Paint mTempPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private ColorFilter mColorFilter;

    public GlifPatternDrawable(int color) {
        setColor(color);
    }

    @Override
    public void draw(Canvas canvas) {
        final Rect bounds = getBounds();
        int drawableWidth = bounds.width();
        int drawableHeight = bounds.height();
        Bitmap bitmap = null;
        if (sBitmapCache != null) {
            bitmap = sBitmapCache.get();
        }
        if (bitmap != null) {
            final int bitmapWidth = bitmap.getWidth();
            final int bitmapHeight = bitmap.getHeight();
            // Invalidate the cache if this drawable is bigger and we can still create a bigger
            // cache.
            if (drawableWidth > bitmapWidth
                    && bitmapWidth < VIEWBOX_WIDTH * MAX_CACHED_BITMAP_SCALE) {
                bitmap = null;
            } else if (drawableHeight > bitmapHeight
                    && bitmapHeight < VIEWBOX_HEIGHT * MAX_CACHED_BITMAP_SCALE) {
                bitmap = null;
            }
        }

        if (bitmap == null) {
            // Reset the paint so it can be used to draw the paths in renderOnCanvas
            mTempPaint.reset();

            bitmap = createBitmapCache(drawableWidth, drawableHeight);
            sBitmapCache = new SoftReference<>(bitmap);

            // Reset the paint to so it can be used to draw the bitmap
            mTempPaint.reset();
        }

        canvas.save();
        canvas.clipRect(bounds);

        scaleCanvasToBounds(canvas, bitmap, bounds);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB
                && canvas.isHardwareAccelerated()) {
            mTempPaint.setColorFilter(mColorFilter);
            canvas.drawBitmap(bitmap, 0, 0, mTempPaint);
        } else {
            // Software renderer doesn't work properly with ColorMatrix filter on ALPHA_8 bitmaps.
            canvas.drawColor(Color.BLACK);
            mTempPaint.setColor(Color.WHITE);
            canvas.drawBitmap(bitmap, 0, 0, mTempPaint);
            canvas.drawColor(mColor);
        }

        canvas.restore();
    }

    @VisibleForTesting
    public Bitmap createBitmapCache(int drawableWidth, int drawableHeight) {
        float scaleX = drawableWidth / VIEWBOX_WIDTH;
        float scaleY = drawableHeight / VIEWBOX_HEIGHT;
        float scale = Math.max(scaleX, scaleY);
        scale = Math.min(MAX_CACHED_BITMAP_SCALE, scale);


        int scaledWidth = (int) (VIEWBOX_WIDTH * scale);
        int scaledHeight = (int) (VIEWBOX_HEIGHT * scale);

        // Use ALPHA_8 mask to save memory, since the pattern is grayscale only anyway.
        Bitmap bitmap = Bitmap.createBitmap(
                scaledWidth,
                scaledHeight,
                Bitmap.Config.ALPHA_8);
        Canvas bitmapCanvas = new Canvas(bitmap);
        renderOnCanvas(bitmapCanvas, scale);
        return bitmap;
    }

    private void renderOnCanvas(Canvas canvas, float scale) {
        canvas.save();
        canvas.scale(scale, scale);

        mTempPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC));

        // Draw the pattern by creating the paths, adjusting the colors and drawing them. The path
        // values are extracted from the SVG of the pattern file.

        if (sPatternPaths == null) {
            sPatternPaths = new Path[NUM_PATHS];
            // Lightness values of the pattern, range 0 - 255
            sPatternLightness = new int[] { 10, 40, 51, 66, 91, 112, 130 };

            Path p = sPatternPaths[0] = new Path();
            p.moveTo(1029.4f, 357.5f);
            p.lineTo(1366f, 759.1f);
            p.lineTo(1366f, 0f);
            p.lineTo(1137.7f, 0f);
            p.close();

            p = sPatternPaths[1] = new Path();
            p.moveTo(1138.1f, 0f);
            p.rLineTo(-144.8f, 768f);
            p.rLineTo(372.7f, 0f);
            p.rLineTo(0f, -524f);
            p.cubicTo(1290.7f, 121.6f, 1219.2f, 41.1f, 1178.7f, 0f);
            p.close();

            p = sPatternPaths[2] = new Path();
            p.moveTo(949.8f, 768f);
            p.rCubicTo(92.6f, -170.6f, 213f, -440.3f, 269.4f, -768f);
            p.lineTo(585f, 0f);
            p.rLineTo(2.1f, 766f);
            p.close();

            p = sPatternPaths[3] = new Path();
            p.moveTo(471.1f, 768f);
            p.rMoveTo(704.5f, 0f);
            p.cubicTo(1123.6f, 563.3f, 1027.4f, 275.2f, 856.2f, 0f);
            p.lineTo(476.4f, 0f);
            p.rLineTo(-5.3f, 768f);
            p.close();

            p = sPatternPaths[4] = new Path();
            p.moveTo(323.1f, 768f);
            p.moveTo(777.5f, 768f);
            p.cubicTo(661.9f, 348.8f, 427.2f, 21.4f, 401.2f, 25.4f);
            p.lineTo(323.1f, 768f);
            p.close();

            p = sPatternPaths[5] = new Path();
            p.moveTo(178.44286f, 766.85714f);
            p.lineTo(308.7f, 768f);
            p.cubicTo(381.7f, 604.6f, 481.6f, 344.3f, 562.2f, 0f);
            p.lineTo(0f, 0f);
            p.close();

            p = sPatternPaths[6] = new Path();
            p.moveTo(146f, 0f);
            p.lineTo(0f, 0f);
            p.lineTo(0f, 768f);
            p.lineTo(394.2f, 768f);
            p.cubicTo(327.7f, 475.3f, 228.5f, 201f, 146f, 0f);
            p.close();
        }

        for (int i = 0; i < NUM_PATHS; i++) {
            // Color is 0xAARRGGBB, so alpha << 24 will create a color with (alpha)% black.
            // Although the color components don't really matter, since the backing bitmap cache is
            // ALPHA_8.
            mTempPaint.setColor(sPatternLightness[i] << 24);
            canvas.drawPath(sPatternPaths[i], mTempPaint);
        }

        canvas.restore();
        mTempPaint.reset();
    }

    @VisibleForTesting
    public void scaleCanvasToBounds(Canvas canvas, Bitmap bitmap, Rect drawableBounds) {
        int bitmapWidth = bitmap.getWidth();
        int bitmapHeight = bitmap.getHeight();
        float scaleX = drawableBounds.width() / (float) bitmapWidth;
        float scaleY = drawableBounds.height() / (float) bitmapHeight;

        // First scale both sides to fit independently.
        canvas.scale(scaleX, scaleY);
        if (scaleY > scaleX) {
            // Adjust x-scale to maintain aspect ratio using the pivot, so that more of the texture
            // and less of the blank space on the left edge is seen.
            canvas.scale(scaleY / scaleX, 1f, SCALE_FOCUS_X * bitmapWidth, 0f);
        } else if (scaleX > scaleY) {
            // Adjust y-scale to maintain aspect ratio using the pivot, so that an intersection of
            // two "circles" can always be seen.
            canvas.scale(1f, scaleX / scaleY, 0f, SCALE_FOCUS_Y * bitmapHeight);
        }
    }

    @Override
    public void setAlpha(int i) {
        // Ignore
    }

    @Override
    public void setColorFilter(ColorFilter colorFilter) {
        // Ignore
    }

    @Override
    public int getOpacity() {
        return 0;
    }

    /**
     * Sets the color used as the base color of this pattern drawable. The alpha component of the
     * color will be ignored.
     */
    public void setColor(int color) {
        final int r = Color.red(color);
        final int g = Color.green(color);
        final int b = Color.blue(color);
        mColor = Color.argb(COLOR_ALPHA_INT, r, g, b);
        mColorFilter = new ColorMatrixColorFilter(new float[] {
                0, 0, 0, 1 - COLOR_ALPHA, r * COLOR_ALPHA,
                0, 0, 0, 1 - COLOR_ALPHA, g * COLOR_ALPHA,
                0, 0, 0, 1 - COLOR_ALPHA, b * COLOR_ALPHA,
                0, 0, 0,               0,             255
        });
        invalidateSelf();
    }

    /**
     * @return The color used as the base color of this pattern drawable. The alpha component of
     * this is always 255.
     */
    public int getColor() {
        return Color.argb(255, Color.red(mColor), Color.green(mColor), Color.blue(mColor));
    }
}
