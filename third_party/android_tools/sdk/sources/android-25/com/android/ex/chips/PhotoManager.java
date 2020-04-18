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

package com.android.ex.chips;

/**
 * Used by the {@link com.android.ex.chips.BaseRecipientAdapter} to handle fetching
 * photos from external sources and caching them for faster lookup later.
 */
public interface PhotoManager {

    /** The number of photos cached in this Adapter. */
    public static final int PHOTO_CACHE_SIZE = 20;

    /**
     * Sets the {@link com.android.ex.chips.RecipientEntry}'s photo bytes. If the photo bytes
     * are cached, this action happens immediately. Otherwise, the work to fetch the photo
     * bytes is performed asynchronously before setting the value on the UI thread.<p/>
     *
     * If the photo bytes were fetched asynchronously,
     * {@link PhotoManagerCallback#onPhotoBytesAsynchronouslyPopulated()} is called. This
     * method is not called if the photo bytes have been cached previously (because no
     * asynchronous work was performed). In that case,
     * {@link PhotoManagerCallback#onPhotoBytesPopulated()} is called.
     */
    void populatePhotoBytesAsync(RecipientEntry entry, PhotoManagerCallback callback);

    interface PhotoManagerCallback {
        void onPhotoBytesPopulated();
        void onPhotoBytesAsynchronouslyPopulated();
        void onPhotoBytesAsyncLoadFailed();
    }
}
