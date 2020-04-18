/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.bitmap.util;

import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.RectF;

public class RectUtils {

    /**
     * Transform the upright full rectangle so that it bounds the original rotated image,
     * given by the orientation. Transform the upright partial rectangle such that it would apply
     * to the same region of the transformed full rectangle.
     *
     * The top-left of the transformed full rectangle will always be placed at (0, 0).
     * @param orientation The exif orientation (0, 90, 180, 270) of the original image. The
     *                    transformed full and partial rectangles will be in this orientation's
     *                    coordinate space.
     * @param fullRect    The upright full rectangle. This rectangle will be modified.
     * @param partialRect The upright partial rectangle. This rectangle will be modified.
     */
    public static void rotateRectForOrientation(final int orientation, final Rect fullRect,
            final Rect partialRect) {
        final Matrix matrix = new Matrix();
        // Exif orientation specifies how the camera is rotated relative to the actual subject.
        // First rotate in the opposite direction.
        matrix.setRotate(-orientation);
        final RectF fullRectF = new RectF(fullRect);
        final RectF partialRectF = new RectF(partialRect);
        matrix.mapRect(fullRectF);
        matrix.mapRect(partialRectF);
        // Then translate so that the upper left corner of the rotated full rect is at (0,0).
        matrix.reset();
        matrix.setTranslate(-fullRectF.left, -fullRectF.top);
        matrix.mapRect(fullRectF);
        matrix.mapRect(partialRectF);
        // Orientation transformation is complete.
        fullRect.set((int) fullRectF.left, (int) fullRectF.top, (int) fullRectF.right,
                (int) fullRectF.bottom);
        partialRect.set((int) partialRectF.left, (int) partialRectF.top, (int) partialRectF.right,
                (int) partialRectF.bottom);
    }

    public static void rotateRect(final int degrees, final int px, final int py, final Rect rect) {
        final RectF rectF = new RectF(rect);
        final Matrix matrix = new Matrix();
        matrix.setRotate(degrees, px, py);
        matrix.mapRect(rectF);
        rect.set((int) rectF.left, (int) rectF.top, (int) rectF.right, (int) rectF.bottom);
    }
}
