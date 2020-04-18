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

import com.android.bitmap.util.Trace;

import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * An alternative implementation of a pool+cache. This implementation only counts
 * unreferenced objects in its size calculation. Internally, it never evicts from
 * its cache, and instead {@link #poll()} is allowed to return unreferenced cache
 * entries.
 * <p>
 * You would only use this kind of cache if your objects are interchangeable and
 * have significant allocation cost, and if your memory footprint is somewhat
 * flexible.
 * <p>
 * Because this class only counts unreferenced objects toward targetSize,
 * it will have a total memory footprint of:
 * <code>(targetSize) + (# of threads concurrently writing to cache) +
 * (total size of still-referenced entries)</code>
 *
 */
public class UnrefedPooledCache<K, V extends Poolable> implements PooledCache<K, V> {

    private final LinkedHashMap<K, V> mCache;
    private final LinkedBlockingQueue<V> mPool;
    private final int mTargetSize;
    private final LruCache<K, V> mNonPooledCache;

    private static final boolean DEBUG = DecodeTask.DEBUG;
    private static final String TAG = UnrefedPooledCache.class.getSimpleName();

    /**
     * @param targetSize not exactly a max size in practice
     * @param nonPooledFraction the fractional portion in the range [0.0,1.0] of targetSize to
     * dedicate to non-poolable entries
     */
    public UnrefedPooledCache(int targetSize, float nonPooledFraction) {
        mCache = new LinkedHashMap<K, V>(0, 0.75f, true);
        mPool = new LinkedBlockingQueue<V>();
        final int nonPooledSize = Math.round(targetSize * nonPooledFraction);
        if (nonPooledSize > 0) {
            mNonPooledCache = new NonPooledCache(nonPooledSize);
        } else {
            mNonPooledCache = null;
        }
        mTargetSize = targetSize - nonPooledSize;
    }

    @Override
    public V get(K key, boolean incrementRefCount) {
        Trace.beginSection("cache get");
        synchronized (mCache) {
            V result = mCache.get(key);
            if (result == null && mNonPooledCache != null) {
                result = mNonPooledCache.get(key);
            }
            if (incrementRefCount && result != null) {
                result.acquireReference();
            }
            Trace.endSection();
            return result;
        }
    }

    @Override
    public V put(K key, V value) {
        Trace.beginSection("cache put");
        // Null values not supported.
        if (value == null) {
            Trace.endSection();
            return null;
        }
        synchronized (mCache) {
            final V prev;
            if (value.isEligibleForPooling()) {
                prev = mCache.put(key, value);
            } else if (mNonPooledCache != null) {
                prev = mNonPooledCache.put(key, value);
            } else {
                prev = null;
            }
            Trace.endSection();
            return prev;
        }
    }

    @Override
    public void offer(V value) {
        Trace.beginSection("pool offer");
        if (value.getRefCount() != 0 || !value.isEligibleForPooling()) {
            Trace.endSection();
            throw new IllegalArgumentException("unexpected offer of an invalid object: " + value);
        }
        mPool.offer(value);
        Trace.endSection();
    }

    @Override
    public V poll() {
        Trace.beginSection("pool poll");
        final V pooled = mPool.poll();
        if (pooled != null) {
            Trace.endSection();
            return pooled;
        }

        synchronized (mCache) {
            int unrefSize = 0;
            Map.Entry<K, V> eldestUnref = null;
            for (Map.Entry<K, V> entry : mCache.entrySet()) {
                final V value = entry.getValue();
                if (value.getRefCount() > 0 || !value.isEligibleForPooling()) {
                    continue;
                }
                if (eldestUnref == null) {
                    eldestUnref = entry;
                }
                unrefSize += sizeOf(value);
                if (unrefSize > mTargetSize) {
                    break;
                }
            }
            // only return a scavenged cache entry if the cache has enough
            // eligible (unreferenced) items
            if (unrefSize <= mTargetSize) {
                if (DEBUG) {
                    Log.e(TAG, "POOL SCAVENGE FAILED, cache not fully warm yet. szDelta="
                            + (mTargetSize-unrefSize));
                }
                Trace.endSection();
                return null;
            } else {
                mCache.remove(eldestUnref.getKey());
                if (DEBUG) {
                    Log.e(TAG, "POOL SCAVENGE SUCCESS, oldKey=" + eldestUnref.getKey());
                }
                Trace.endSection();
                return eldestUnref.getValue();
            }
        }
    }

    protected int sizeOf(V value) {
        return 1;
    }

    @Override
    public String toDebugString() {
        if (DEBUG) {
            final StringBuilder sb = new StringBuilder("[");
            sb.append(super.toString());
            int size = 0;
            synchronized (mCache) {
                sb.append(" poolCount=");
                sb.append(mPool.size());
                sb.append(" cacheSize=");
                sb.append(mCache.size());
                if (mNonPooledCache != null) {
                    sb.append(" nonPooledCacheSize=");
                    sb.append(mNonPooledCache.size());
                }
                sb.append("\n---------------------");
                for (V val : mPool) {
                    size += sizeOf(val);
                    sb.append("\n\tpool item: ");
                    sb.append(val);
                }
                sb.append("\n---------------------");
                for (Map.Entry<K, V> item : mCache.entrySet()) {
                    final V val = item.getValue();
                    sb.append("\n\tcache key=");
                    sb.append(item.getKey());
                    sb.append(" val=");
                    sb.append(val);
                    size += sizeOf(val);
                }
                sb.append("\n---------------------");
                if (mNonPooledCache != null) {
                    for (Map.Entry<K, V> item : mNonPooledCache.snapshot().entrySet()) {
                        final V val = item.getValue();
                        sb.append("\n\tnon-pooled cache key=");
                        sb.append(item.getKey());
                        sb.append(" val=");
                        sb.append(val);
                        size += sizeOf(val);
                    }
                    sb.append("\n---------------------");
                }
                sb.append("\nTOTAL SIZE=" + size);
            }
            sb.append("]");
            return sb.toString();
        } else {
            return null;
        }
    }

    private class NonPooledCache extends LruCache<K, V> {

        public NonPooledCache(int maxSize) {
            super(maxSize);
        }

        @Override
        protected int sizeOf(K key, V value) {
            return UnrefedPooledCache.this.sizeOf(value);
        }

    }

    @Override
    public void clear() {
        mCache.clear();
        mPool.clear();
    }
}
