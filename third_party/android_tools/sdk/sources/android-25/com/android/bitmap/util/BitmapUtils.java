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

package com.android.bitmap.util;

import android.graphics.Rect;

public abstract class BitmapUtils {

    /**
     * Calculate a rectangle for the given input and output parameters. The output
     * rectangle to use is written in the given outRect.
     *
     * @param srcW the source width
     * @param srcH the source height
     * @param dstW the destination width
     * @param dstH the destination height
     * @param dstSliceH the height extent (in destination coordinates) to
     *            exclude when cropping. You would typically pass dstH, unless
     *            you are trying to normalize different items to the same
     *            vertical crop range.
     * @param sampleSize a scaling factor that rect calculation will only use if
     *            it's more aggressive than regular scaling
     * @param horizSliceFrac horizontal slice fraction determines the horizontal
     *            center point for the crop rect. Range is from [0.0, 1.0]. To
     *            perform a horizontally centered crop, use 0.5.
     * @param vertSliceFrac vertical slice fraction determines the vertical
     *            center point for the crop rect. Range is from [0.0, 1.0]. To
     *            perform a vertically centered crop, use 0.5. Otherwise, see
     *            absoluteFrac.
     * @param absoluteFrac determines how the vertSliceFrac affects the vertical
     *            center point. If this parameter is true, the vertical center
     *            of the resulting output rectangle will be exactly
     *            [vertSliceFrac * srcH], with care taken to keep the bounds
     *            within the source rectangle. If this parameter is false, the
     *            vertical center will be calculated so that the values of
     *            vertSliceFrac from 0.0 to 1.0 will linearly cover the entirety
     *            of the source rectangle.
     * @param verticalMultiplier an optional multiplier that will alter the
     *            output Rect's aspect ratio to be this much taller in the event
     *            that y is the limiting dimension
     * @param outRect a Rect to write the resulting crop coordinates into
     */
    public static void calculateCroppedSrcRect(final int srcW, final int srcH, final int dstW,
            final int dstH, final int dstSliceH, int sampleSize, final float horizSliceFrac,
            final float vertSliceFrac, final boolean absoluteFrac, final float verticalMultiplier,
            final Rect outRect) {
        if (sampleSize < 1) {
            sampleSize = 1;
        }
        final float regularScale;
        final float wScale = (float) srcW / dstW;
        final float hScale = (float) srcH / dstH;
        if (hScale < wScale) {
            regularScale = hScale / verticalMultiplier;
        } else {
            regularScale = wScale;
        }

        final float scale = Math.min(sampleSize, regularScale);

        final int srcCroppedW = Math.round(dstW * scale);
        final int srcCroppedH = Math.round(dstH * scale);
        final int srcCroppedSliceH = Math.round(dstSliceH * scale);
        final int srcHalfSliceH = Math.min(srcCroppedSliceH, srcH) / 2;

        outRect.left = (int) (horizSliceFrac * (srcW - srcCroppedW));
        outRect.right = outRect.left + srcCroppedW;

        final int centerV;
        if (absoluteFrac) {
            final int minCenterV = srcHalfSliceH;
            final int maxCenterV = srcH - srcHalfSliceH;
            centerV = Math.max(minCenterV, Math.min(maxCenterV, Math.round(srcH * vertSliceFrac)));
        } else {
            centerV = Math
                    .round(Math.abs(srcH - srcCroppedSliceH) * vertSliceFrac + srcHalfSliceH);
        }

        outRect.top = centerV - srcCroppedH / 2;
        outRect.bottom = outRect.top + srcCroppedH;
    }

    /**
     * @param srcW
     * @param srcH
     * @param dstW
     * @param dstH
     * @param outRect
     */
    public static void calculateCroppedSrcRect(int srcW, int srcH, int dstW, int dstH,
            Rect outRect) {
        calculateCroppedSrcRect(srcW, srcH, dstW, dstH, Integer.MAX_VALUE, outRect);
    }

    public static void calculateCroppedSrcRect(int srcW, int srcH, int dstW, int dstH,
            int sampleSize, Rect outRect) {
        if (sampleSize < 1) {
            sampleSize = 1;
        }
        final float regularScale = Math.min(
                (float) srcW / dstW,
                (float) srcH / dstH);

        final float scale = Math.min(sampleSize, regularScale);

        final int srcCroppedW = Math.round(dstW * scale);
        final int srcCroppedH = Math.round(dstH * scale);

        outRect.left = (srcW - srcCroppedW) / 2;
        outRect.right = outRect.left + srcCroppedW;

        outRect.top = (srcH - srcCroppedH) / 2;
        outRect.bottom = outRect.top + srcCroppedH;
    }
}
