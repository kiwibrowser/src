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

import android.util.Log;
import android.util.LruCache;

import com.android.bitmap.ReusableBitmap.NullReusableBitmap;
import com.android.bitmap.util.Trace;

/**
 * This subclass provides custom pool behavior. The pool can be set to block on {@link #poll()} if
 * nothing can be returned. This is useful if you know you will incur high costs upon receiving
 * nothing from the pool, and you do not want to incur those costs at the critical moment when the
 * UI is animating.
 *
 * This subclass provides custom cache behavior. Null values can be cached. Later,
 * when the same key is used to retrieve the value, a {@link NullReusableBitmap} singleton will
 * be returned.
 */
public class UnrefedBitmapCache extends UnrefedPooledCache<RequestKey, ReusableBitmap>
        implements BitmapCache {
    private boolean mBlocking = false;
    private final Object mLock = new Object();

    private LruCache<RequestKey, NullReusableBitmap> mNullRequests;

    private final static boolean DEBUG = DecodeTask.DEBUG;
    private final static String TAG = UnrefedBitmapCache.class.getSimpleName();

    public UnrefedBitmapCache(final int targetSizeBytes, final float nonPooledFraction,
            final int nullCapacity) {
        super(targetSizeBytes, nonPooledFraction);

        if (nullCapacity > 0) {
            mNullRequests = new LruCache<RequestKey, NullReusableBitmap>(nullCapacity);
        }
    }

    /**
     * Declare that {@link #poll()} should now block until it can return something.
     */
    @Override
    public void setBlocking(final boolean blocking) {
        synchronized (mLock) {
            if (DEBUG) {
                Log.d(TAG, String.format("AltBitmapCache: block %b", blocking));
            }
            mBlocking = blocking;
            if (!mBlocking) {
                // no longer blocking. Notify every thread.
                mLock.notifyAll();
            }
        }
    }

    @Override
    protected int sizeOf(final ReusableBitmap value) {
        return value.getByteCount();
    }

    /**
     * If {@link #setBlocking(boolean)} has been called with true, this method will block until a
     * resource is available.
     * @return an available resource, or null if none are available. Null will never be returned
     * until blocking is set to false.
     */
    @Override
    public ReusableBitmap poll() {
        ReusableBitmap bitmap;
        synchronized (mLock) {
            while ((bitmap = super.poll()) == null && mBlocking) {
                if (DEBUG) {
                    Log.d(TAG, String.format(
                            "AltBitmapCache: %s waiting", Thread.currentThread().getName()));
                }
                Trace.beginSection("sleep");
                try {
                    // block
                    mLock.wait();
                    if (DEBUG) {
                        Log.d(TAG, String.format("AltBitmapCache: %s notified",
                                Thread.currentThread().getName()));
                    }
                } catch (InterruptedException ignored) {
                }
                Trace.endSection();
            }
        }
        return bitmap;
    }

    @Override
    public void offer(final ReusableBitmap value) {
        synchronized (mLock) {
            super.offer(value);
            if (DEBUG) {
                Log.d(TAG, "AltBitmapCache: offer +1");
            }
            // new resource gained. Notify one thread.
            mLock.notify();
        }
    }

    @Override
    public ReusableBitmap get(final RequestKey key, final boolean incrementRefCount) {
        if (mNullRequests != null && mNullRequests.get(key) != null) {
            return NullReusableBitmap.getInstance();
        }
        return super.get(key, incrementRefCount);
    }

    /**
     * Note: The cache only supports same-sized bitmaps.
     */
    @Override
    public ReusableBitmap put(final RequestKey key, final ReusableBitmap value) {
        if (mNullRequests != null && (value == null || value == NullReusableBitmap.getInstance())) {
            mNullRequests.put(key, NullReusableBitmap.getInstance());
            return null;
        }

        return super.put(key, value);
    }
}
