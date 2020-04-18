// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.suggestions;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.os.SystemClock;
import android.support.annotation.IntDef;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.R;
import org.chromium.ui.base.LocalizationUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.concurrent.TimeUnit;

/**
 * When suggestions cards are displayed on a white background, thumbnails with white backgrounds
 * have a gradient overlaid to provide contrast at the edge of the cards.
 */
public class ThumbnailGradient {
    /** If all the RGB values of a pixel are greater than this value, it is counted as 'light'. */
    private static final int LIGHT_PIXEL_THRESHOLD = 0xcc;

    /** The percent of the border pictures that need to be 'light' for a Bitmap to be 'light'. */
    private static final float PIXEL_BORDER_RATIO = 0.4f;

    /** The corner of the image the gradient is darkest. */
    private static final int TOP_LEFT = 0;
    private static final int TOP_RIGHT = 1;

    @IntDef({TOP_LEFT, TOP_RIGHT})
    @Retention(RetentionPolicy.SOURCE)
    private @interface GradientDirection {}

    /**
     * If the {@link Bitmap} should have a gradient applied this method returns a Drawable
     * containing the Bitmap and a gradient. Otherwise it returns a BitmapDrawable containing just
     * the Bitmap.
     */
    public static Drawable createDrawableWithGradientIfNeeded(Bitmap bitmap, Resources resources) {
        int direction = getGradientDirection();

        // We want to keep an eye on how long this takes.
        long time = SystemClock.elapsedRealtime();
        boolean lightImage = hasLightCorner(bitmap, direction);
        RecordHistogram.recordTimesHistogram("Thumbnails.Gradient.ImageDetectionTime",
                SystemClock.elapsedRealtime() - time, TimeUnit.MILLISECONDS);

        RecordHistogram.recordBooleanHistogram(
                "Thumbnails.Gradient.ImageRequiresGradient", lightImage);

        if (lightImage) {
            Drawable gradient = ApiCompatibilityUtils.getDrawable(resources,
                    direction == TOP_LEFT ? R.drawable.thumbnail_gradient_top_left
                                          : R.drawable.thumbnail_gradient_top_right);

            return new LayerDrawable(
                    new Drawable[] {new BitmapDrawable(resources, bitmap), gradient});
        }

        return new BitmapDrawable(resources, bitmap);
    }

    /**
     * Determines whether a Bitmap has a light corner.
     */
    private static boolean hasLightCorner(Bitmap bitmap, @GradientDirection int direction) {
        int lightPixels = 0;

        final int width = bitmap.getWidth();
        final int height = bitmap.getHeight();
        // We test all the pixels along the top and one side. The |-1| is so we don't count the
        // corner twice.
        final int threshold = (int) ((width + height - 1) * PIXEL_BORDER_RATIO);

        for (int x = 0; x < width; x++) {
            if (isPixelLight(bitmap.getPixel(x, 0))) lightPixels++;
        }

        // If we've already exceeded the threshold of light pixels, don't bother counting the rest.
        if (lightPixels > threshold) {
            return true;
        }

        final int x = direction == TOP_LEFT ? 0 : width - 1;
        // Avoid counting the corner pixels twice.
        for (int y = 1; y < height - 1; y++) {
            if (isPixelLight(bitmap.getPixel(x, y))) lightPixels++;
        }

        return lightPixels > threshold;
    }

    /**
     * Whether a pixel counts as light.
     */
    private static boolean isPixelLight(int color) {
        return Color.red(color) > LIGHT_PIXEL_THRESHOLD && Color.blue(color) > LIGHT_PIXEL_THRESHOLD
                && Color.green(color) > LIGHT_PIXEL_THRESHOLD;
    }

    /**
     * The gradient should come from the upper corner of the image that is touching the side of the
     * card.
     */
    @GradientDirection
    private static int getGradientDirection() {
        // The drawable is set up correctly for the modern layout, but needs to be flipped for the
        // large thumbnail layout.
        boolean modern = SuggestionsConfig.useModernLayout();

        // The drawable resource does not get flipped automatically if we are in RTL, so we must
        // flip it ourselves.
        boolean rtl = LocalizationUtils.isLayoutRtl();

        return modern == rtl ? TOP_RIGHT : TOP_LEFT;
    }
}
