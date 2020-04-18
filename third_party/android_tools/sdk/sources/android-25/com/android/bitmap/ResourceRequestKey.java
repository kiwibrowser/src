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

package com.android.bitmap;

import android.content.res.Resources;

import java.io.IOException;
import java.io.InputStream;

/**
 * Simple RequestKey for decoding from a resource id.
 */
public class ResourceRequestKey implements RequestKey {

    private Resources mResources;
    private int mResId;

    /**
     * Create a new request key with the given resource id. A resId of 0 will
     * return a null request key.
     */
    public static ResourceRequestKey from(Resources res, int resId) {
        if (resId != 0) {
            return new ResourceRequestKey(res, resId);
        }
        return null;
    }

    private ResourceRequestKey(Resources res, int resId) {
        mResources = res;
        mResId = resId;
    }

    @Override
    public Cancelable createFileDescriptorFactoryAsync(RequestKey requestKey, Callback callback) {
        return null;
    }

    @Override
    public InputStream createInputStream() throws IOException {
        return mResources.openRawResource(mResId);
    }

    @Override
    public boolean hasOrientationExif() throws IOException {
        return false;
    }

    // START AUTO-GENERATED CODE

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        ResourceRequestKey that = (ResourceRequestKey) o;

        if (mResId != that.mResId) {
            return false;
        }

        return true;
    }

    @Override
    public int hashCode() {
        return mResId;
    }

    // END AUTO-GENERATED CODE

    @Override
    public String toString() {
        return String.format("ResourceRequestKey: %d", mResId);
    }
}
