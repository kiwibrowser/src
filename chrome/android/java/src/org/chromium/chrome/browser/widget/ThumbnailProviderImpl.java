// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget;

import android.graphics.Bitmap;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.util.LruCache;
import android.text.TextUtils;
import android.util.Pair;

import org.chromium.base.DiscardableReferencePool;
import org.chromium.base.SysUtils;
import org.chromium.base.ThreadUtils;

import java.lang.ref.WeakReference;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Concrete implementation of {@link ThumbnailProvider}.
 *
 * Thumbnails are cached in memory and shared across all ThumbnailProviderImpls. There are two
 * levels of caches: One static cache for deduplication (or canonicalization) of bitmaps, and one
 * per-object cache for storing recently used bitmaps. The deduplication cache uses weak references
 * to allow bitmaps to be garbage-collected once they are no longer in use. As long as there is at
 * least one strong reference to a bitmap, it is not going to be GC'd and will therefore stay in the
 * cache. This ensures that there is never more than one (reachable) copy of a bitmap in memory.
 * The {@link RecentlyUsedCache} is limited in size and dropped under memory pressure, or when the
 * object is destroyed.
 *
 * A queue of requests is maintained in FIFO order.
 *
 * TODO(dfalcantara): Figure out how to send requests simultaneously to the utility process without
 *                    duplicating work to decode the same image for two different requests.
 */
public class ThumbnailProviderImpl implements ThumbnailProvider, ThumbnailStorageDelegate {
    /** 5 MB of thumbnails should be enough for everyone. */
    private static final int MAX_CACHE_BYTES = 5 * 1024 * 1024;

    /**
     * Least-recently-used cache that falls back to the deduplication cache on misses.
     * This propagates bitmaps that were only in the deduplication cache back into the LRU cache
     * and also moves them to the front to ensure correct eviction order.
     * Cache key is a pair of the filepath and the height/width of the thumbnail. Value is
     * the thumbnail.
     */
    private static class RecentlyUsedCache extends LruCache<Pair<String, Integer>, Bitmap> {
        private RecentlyUsedCache() {
            super(MAX_CACHE_BYTES);
        }

        @Override
        protected Bitmap create(Pair<String, Integer> key) {
            WeakReference<Bitmap> cachedBitmap = sDeduplicationCache.get(key);
            return cachedBitmap == null ? null : cachedBitmap.get();
        }

        @Override
        protected int sizeOf(Pair<String, Integer> key, Bitmap thumbnail) {
            return thumbnail.getByteCount();
        }
    }

    /**
     * Discardable reference to the {@link RecentlyUsedCache} that can be dropped under memory
     * pressure.
     */
    private DiscardableReferencePool.DiscardableReference<RecentlyUsedCache> mBitmapCache;

    /**
     * The reference pool that contains the {@link #mBitmapCache}. Used to recreate a new cache
     * after the old one has been dropped.
     */
    private final DiscardableReferencePool mReferencePool;

    /**
     * Static cache used for deduplicating bitmaps. The key is a pair of file name and thumbnail
     * size (as for the {@link #mBitmapCache}.
     */
    private static Map<Pair<String, Integer>, WeakReference<Bitmap>> sDeduplicationCache =
            new HashMap<>();

    /** Queue of files to retrieve thumbnails for. */
    private final Deque<ThumbnailRequest> mRequestQueue = new ArrayDeque<>();

    /** The native side pointer that is owned and destroyed by the Java class. */
    private long mNativeThumbnailProvider;

    /** Request that is currently having its thumbnail retrieved. */
    private ThumbnailRequest mCurrentRequest;

    private ThumbnailDiskStorage mStorage;

    public ThumbnailProviderImpl(DiscardableReferencePool referencePool) {
        ThreadUtils.assertOnUiThread();
        mReferencePool = referencePool;
        mBitmapCache = referencePool.put(new RecentlyUsedCache());
        mStorage = ThumbnailDiskStorage.create(this);
    }

    @Override
    public void destroy() {
        ThreadUtils.assertOnUiThread();
        mStorage.destroy();
    }

    /**
     * The returned bitmap will have at least one of its dimensions smaller than or equal to the
     * size specified in the request. Requests with no file path or content ID will not be
     * processed.
     *
     * @param request Parameters that describe the thumbnail being retrieved.
     */
    @Override
    public void getThumbnail(ThumbnailRequest request) {
        ThreadUtils.assertOnUiThread();

        if (TextUtils.isEmpty(request.getContentId())) {
            return;
        }

        Bitmap cachedBitmap = getBitmapFromCache(request.getContentId(), request.getIconSize());
        if (cachedBitmap != null) {
            request.onThumbnailRetrieved(request.getContentId(), cachedBitmap);
            return;
        }

        mRequestQueue.offer(request);
        processQueue();
    }

    /** Removes a particular file from the pending queue. */
    @Override
    public void cancelRetrieval(ThumbnailRequest request) {
        ThreadUtils.assertOnUiThread();
        if (mRequestQueue.contains(request)) mRequestQueue.remove(request);
    }

    /**
     * Removes the thumbnails (different sizes) with {@code contentId} from disk.
     * @param contentId The content ID of the thumbnail to remove.
     */
    @Override
    public void removeThumbnailsFromDisk(String contentId) {
        mStorage.removeFromDisk(contentId);
    }

    private void processQueue() {
        ThreadUtils.postOnUiThread(this::processNextRequest);
    }

    private RecentlyUsedCache getBitmapCache() {
        RecentlyUsedCache bitmapCache = mBitmapCache.get();
        if (bitmapCache == null) {
            bitmapCache = new RecentlyUsedCache();
            mBitmapCache = mReferencePool.put(bitmapCache);
        }
        return bitmapCache;
    }

    private Bitmap getBitmapFromCache(String contentId, int bitmapSizePx) {
        Bitmap cachedBitmap = getBitmapCache().get(Pair.create(contentId, bitmapSizePx));
        assert cachedBitmap == null || !cachedBitmap.isRecycled();
        return cachedBitmap;
    }

    private void processNextRequest() {
        ThreadUtils.assertOnUiThread();
        if (mCurrentRequest != null) return;
        if (mRequestQueue.isEmpty()) {
            // If the request queue is empty, schedule compaction for when the main loop is idling.
            Looper.myQueue().addIdleHandler(() -> {
                compactDeduplicationCache();
                return false;
            });
            return;
        }

        mCurrentRequest = mRequestQueue.poll();

        Bitmap cachedBitmap =
                getBitmapFromCache(mCurrentRequest.getContentId(), mCurrentRequest.getIconSize());
        if (cachedBitmap == null) {
            handleCacheMiss(mCurrentRequest);
        } else {
            // Send back the already-processed file.
            onThumbnailRetrieved(mCurrentRequest.getContentId(), cachedBitmap);
        }
    }

    /**
     * In the event of a cache miss from the in-memory cache, the thumbnail request is routed to one
     * of the following :
     * 1. May be the thumbnail request can directly provide the thumbnail.
     * 2. Otherwise, the request is sent to {@link ThumbnailDiskStorage} which is a disk cache. If
     * not found in disk cache, it would request the {@link ThumbnailGenerator} to generate a new
     * thumbnail for the given file path.
     * @param request Parameters that describe the thumbnail being retrieved
     */
    private void handleCacheMiss(ThumbnailProvider.ThumbnailRequest request) {
        boolean providedByThumbnailRequest = request.getThumbnail(
                bitmap -> onThumbnailRetrieved(request.getContentId(), bitmap));

        if (!providedByThumbnailRequest) {
            // Asynchronously process the file to make a thumbnail.
            assert !TextUtils.isEmpty(request.getFilePath());
            mStorage.retrieveThumbnail(request);
        }
    }

    /**
     * Called when thumbnail is ready, retrieved from memory cache or by
     * {@link ThumbnailDiskStorage} or by {@link ThumbnailRequest#getThumbnail()}.
     * @param contentId Content ID for the thumbnail retrieved.
     * @param bitmap The thumbnail retrieved.
     */
    @Override
    public void onThumbnailRetrieved(@NonNull String contentId, @Nullable Bitmap bitmap) {
        if (bitmap != null) {
            // The bitmap returned here is retrieved from the native side. The image decoder there
            // scales down the image (if it is too big) so that one of its sides is smaller than or
            // equal to the required size. We check here that the returned image satisfies this
            // criteria.
            assert Math.min(bitmap.getWidth(), bitmap.getHeight()) <= mCurrentRequest.getIconSize();
            assert TextUtils.equals(mCurrentRequest.getContentId(), contentId);

            // We set the key pair to contain the required size (maximum dimension (pixel) of the
            // smaller side) instead of the minimal dimension of the thumbnail so that future
            // fetches of this thumbnail can recognise the key in the cache.
            Pair<String, Integer> key = Pair.create(contentId, mCurrentRequest.getIconSize());
            if (!SysUtils.isLowEndDevice()) {
                getBitmapCache().put(key, bitmap);
            }
            sDeduplicationCache.put(key, new WeakReference<>(bitmap));
            mCurrentRequest.onThumbnailRetrieved(contentId, bitmap);
        }

        mCurrentRequest = null;
        processQueue();
    }

    /**
     * Compacts the deduplication cache by removing all entries that have been cleared by the
     * garbage collector.
     */
    private void compactDeduplicationCache() {
        // Too many angle brackets for clang-format :-(
        // clang-format off
        for (Iterator<Map.Entry<Pair<String, Integer>, WeakReference<Bitmap>>> it =
                sDeduplicationCache.entrySet().iterator(); it.hasNext();) {
            // clang-format on
            if (it.next().getValue().get() == null) it.remove();
        }
    }
}
