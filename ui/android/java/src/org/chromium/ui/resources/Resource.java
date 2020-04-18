// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui.resources;

import android.graphics.Bitmap;
import android.graphics.Rect;

import org.chromium.ui.resources.statics.NinePatchData;

/**
 * A basic resource interface that all assets must use to be exposed to the CC layer as
 * UIResourceIds.
 */
public interface Resource {
    /**
     * This may be called more than once so if possible avoid doing redundant work.
     * @return A {@link Bitmap} representing the resource.
     */
    Bitmap getBitmap();

    /**
     * @return The size of the bitmap.
     */
    Rect getBitmapSize();

    /**
     * Returns the nine patch data if the resource is backed by a nine patch bitmap. In all other
     * cases, this will be null.
     * @return The nine patch data for the bitmap or null.
     */
    NinePatchData getNinePatchData();

    /**
     * Creates the native representation of this Resource. Note that the ownership is passed to the
     * caller.
     * @return The pointer to the native Resource.
     */
    long createNativeResource();
}