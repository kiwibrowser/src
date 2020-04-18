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

package com.android.bitmap;

import android.graphics.Bitmap;

/**
 * A simple bitmap wrapper. Currently supports reference counting and logical width/height
 * (which may differ from a bitmap's reported width/height due to bitmap reuse).
 */
public class ReusableBitmap implements Poolable {

    public final Bitmap bmp;
    private int mWidth;
    private int mHeight;
    private int mOrientation;

    private int mRefCount = 0;
    private final boolean mReusable;

    public ReusableBitmap(final Bitmap bitmap) {
        this(bitmap, true /* reusable */);
    }

    public ReusableBitmap(final Bitmap bitmap, final boolean reusable) {
        bmp = bitmap;
        mReusable = reusable;
    }

    @Override
    public boolean isEligibleForPooling() {
        return mReusable;
    }

    public void setLogicalWidth(int w) {
        mWidth = w;
    }

    public void setLogicalHeight(int h) {
        mHeight = h;
    }

    public int getLogicalWidth() {
        return mWidth;
    }

    public int getLogicalHeight() {
        return mHeight;
    }

    public int getOrientation() {
        return mOrientation;
    }

    public void setOrientation(final int orientation) {
        mOrientation = orientation;
    }

    public int getByteCount() {
        return bmp.getByteCount();
    }

    @Override
    public void acquireReference() {
        mRefCount++;
    }

    @Override
    public void releaseReference() {
        if (mRefCount == 0) {
            throw new IllegalStateException();
        }
        mRefCount--;
    }

    @Override
    public int getRefCount() {
        return mRefCount;
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder("[");
        sb.append(super.toString());
        sb.append(" refCount=");
        sb.append(mRefCount);
        sb.append(" mReusable=");
        sb.append(mReusable);
        sb.append(" bmp=");
        sb.append(bmp);
        sb.append(" logicalW/H=");
        sb.append(mWidth);
        sb.append("/");
        sb.append(mHeight);
        if (bmp != null) {
            sb.append(" sz=");
            sb.append(bmp.getByteCount() >> 10);
            sb.append("KB");
        }
        sb.append("]");
        return sb.toString();
    }

    /**
     * Singleton class to represent a null Bitmap. We don't want to just use a regular
     * ReusableBitmap with a null bmp field because that will render that ReusableBitmap useless
     * and unable to be used by another decode process.
     */
    public final static class NullReusableBitmap extends ReusableBitmap {
        private static NullReusableBitmap sInstance;

        /**
         * Get a singleton.
         */
        public static NullReusableBitmap getInstance() {
            if (sInstance == null) {
                sInstance = new NullReusableBitmap();
            }
            return sInstance;
        }

        private NullReusableBitmap() {
            super(null /* bmp */, false /* reusable */);
        }

        @Override
        public int getByteCount() {
            return 0;
        }

        @Override
        public void releaseReference() { }

        @Override
        public void acquireReference() { }
    }
}
