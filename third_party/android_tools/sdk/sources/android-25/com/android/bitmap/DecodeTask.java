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
import android.graphics.BitmapFactory;
import android.graphics.BitmapRegionDecoder;
import android.graphics.Rect;
import android.os.AsyncTask;
import android.os.ParcelFileDescriptor;
import android.os.ParcelFileDescriptor.AutoCloseInputStream;
import android.util.Log;

import com.android.bitmap.RequestKey.FileDescriptorFactory;
import com.android.bitmap.util.BitmapUtils;
import com.android.bitmap.util.Exif;
import com.android.bitmap.util.RectUtils;
import com.android.bitmap.util.Trace;

import java.io.IOException;
import java.io.InputStream;

/**
 * Decodes an image from either a file descriptor or input stream on a worker thread. After the
 * decode is complete, even if the task is cancelled, the result is placed in the given cache.
 * A {@link DecodeCallback} client may be notified on decode begin and completion.
 * <p>
 * This class uses {@link BitmapRegionDecoder} when possible to minimize unnecessary decoding
 * and allow bitmap reuse on Jellybean 4.1 and later.
 * <p>
 *  GIFs are supported, but their decode does not reuse bitmaps at all. The resulting
 *  {@link ReusableBitmap} will be marked as not reusable
 *  ({@link ReusableBitmap#isEligibleForPooling()} will return false).
 */
public class DecodeTask extends AsyncTask<Void, Void, ReusableBitmap> {

    private final RequestKey mKey;
    private final DecodeOptions mDecodeOpts;
    private final FileDescriptorFactory mFactory;
    private final DecodeCallback mDecodeCallback;
    private final BitmapCache mCache;
    private final BitmapFactory.Options mOpts = new BitmapFactory.Options();

    private ReusableBitmap mInBitmap = null;

    private static final boolean CROP_DURING_DECODE = true;

    private static final String TAG = DecodeTask.class.getSimpleName();
    public static final boolean DEBUG = false;

    /**
     * Callback interface for clients to be notified of decode state changes and completion.
     */
    public interface DecodeCallback {
        /**
         * Notifies that the async task's work is about to begin. Up until this point, the task
         * may have been preempted by the scheduler or queued up by a bottlenecked executor.
         * <p>
         * N.B. this method runs on the UI thread.
         */
        void onDecodeBegin(RequestKey key);
        /**
         * The task is now complete and the ReusableBitmap is available for use. Clients should
         * double check that the request matches what the client is expecting.
         */
        void onDecodeComplete(RequestKey key, ReusableBitmap result);
        /**
         * The task has been canceled, and {@link #onDecodeComplete(RequestKey, ReusableBitmap)}
         * will not be called.
         */
        void onDecodeCancel(RequestKey key);
    }

    /**
   * Create new DecodeTask.
   *
   * @param requestKey The request to decode, also the key to use for the cache.
   * @param decodeOpts The decode options.
   * @param factory    The factory to obtain file descriptors to decode from. If this factory is
     *                 null, then we will decode from requestKey.createInputStream().
   * @param callback   The callback to notify of decode state changes.
   * @param cache      The cache and pool.
   */
    public DecodeTask(RequestKey requestKey, DecodeOptions decodeOpts,
            FileDescriptorFactory factory, DecodeCallback callback, BitmapCache cache) {
        mKey = requestKey;
        mDecodeOpts = decodeOpts;
        mFactory = factory;
        mDecodeCallback = callback;
        mCache = cache;
    }

    @Override
    protected ReusableBitmap doInBackground(Void... params) {
        // enqueue the 'onDecodeBegin' signal on the main thread
        publishProgress();

        return decode();
    }

    public ReusableBitmap decode() {
        if (isCancelled()) {
            return null;
        }

        ReusableBitmap result = null;
        ParcelFileDescriptor fd = null;
        InputStream in = null;

        try {
            if (mFactory != null) {
                Trace.beginSection("create fd");
                fd = mFactory.createFileDescriptor();
                Trace.endSection();
            } else {
                in = reset(in);
                if (in == null) {
                    return null;
                }
                if (isCancelled()) {
                    return null;
                }
            }

            final boolean isJellyBeanOrAbove = android.os.Build.VERSION.SDK_INT
                    >= android.os.Build.VERSION_CODES.JELLY_BEAN;
            // This blocks during fling when the pool is empty. We block early to avoid jank.
            if (isJellyBeanOrAbove) {
                Trace.beginSection("poll for reusable bitmap");
                mInBitmap = mCache.poll();
                Trace.endSection();
            }

            if (isCancelled()) {
                return null;
            }

            Trace.beginSection("get bytesize");
            final long byteSize;
            if (fd != null) {
                byteSize = fd.getStatSize();
            } else {
                byteSize = -1;
            }
            Trace.endSection();

            Trace.beginSection("get orientation");
            final int orientation;
            if (mKey.hasOrientationExif()) {
                if (fd != null) {
                    // Creating an input stream from the file descriptor makes it useless
                    // afterwards.
                    Trace.beginSection("create orientation fd and stream");
                    final ParcelFileDescriptor orientationFd = mFactory.createFileDescriptor();
                    in = new AutoCloseInputStream(orientationFd);
                    Trace.endSection();
                }
                orientation = Exif.getOrientation(in, byteSize);
                if (fd != null) {
                    try {
                        // Close the temporary file descriptor.
                        in.close();
                    } catch (IOException ignored) {
                    }
                }
            } else {
                orientation = 0;
            }
            final boolean isNotRotatedOr180 = orientation == 0 || orientation == 180;
            Trace.endSection();

            if (orientation != 0) {
                // disable inBitmap-- bitmap reuse doesn't work with different decode regions due
                // to orientation
                if (mInBitmap != null) {
                    mCache.offer(mInBitmap);
                    mInBitmap = null;
                    mOpts.inBitmap = null;
                }
            }

            if (isCancelled()) {
                return null;
            }

            if (fd == null) {
                in = reset(in);
                if (in == null) {
                    return null;
                }
                if (isCancelled()) {
                    return null;
                }
            }

            Trace.beginSection("decodeBounds");
            mOpts.inJustDecodeBounds = true;
            if (fd != null) {
                BitmapFactory.decodeFileDescriptor(fd.getFileDescriptor(), null, mOpts);
            } else {
                BitmapFactory.decodeStream(in, null, mOpts);
            }
            Trace.endSection();

            if (isCancelled()) {
                return null;
            }

            // We want to calculate the sample size "as if" the orientation has been corrected.
            final int srcW, srcH; // Orientation corrected.
            if (isNotRotatedOr180) {
                srcW = mOpts.outWidth;
                srcH = mOpts.outHeight;
            } else {
                srcW = mOpts.outHeight;
                srcH = mOpts.outWidth;
            }

            // BEGIN MANUAL-INLINE calculateSampleSize()

            final float sz = Math
                    .min((float) srcW / mDecodeOpts.destW, (float) srcH / mDecodeOpts.destH);

            final int sampleSize;
            switch (mDecodeOpts.sampleSizeStrategy) {
                case DecodeOptions.STRATEGY_TRUNCATE:
                    sampleSize = (int) sz;
                    break;
                case DecodeOptions.STRATEGY_ROUND_UP:
                    sampleSize = (int) Math.ceil(sz);
                    break;
                case DecodeOptions.STRATEGY_ROUND_NEAREST:
                default:
                    sampleSize = (int) Math.pow(2, (int) (0.5 + (Math.log(sz) / Math.log(2))));
                    break;
            }
            mOpts.inSampleSize = Math.max(1, sampleSize);

            // END MANUAL-INLINE calculateSampleSize()

            mOpts.inJustDecodeBounds = false;
            mOpts.inMutable = true;
            if (isJellyBeanOrAbove && orientation == 0) {
                if (mInBitmap == null) {
                    if (DEBUG) {
                        Log.e(TAG, "decode thread wants a bitmap. cache dump:\n"
                                + mCache.toDebugString());
                    }
                    Trace.beginSection("create reusable bitmap");
                    mInBitmap = new ReusableBitmap(
                            Bitmap.createBitmap(mDecodeOpts.destW, mDecodeOpts.destH,
                                    Bitmap.Config.ARGB_8888));
                    Trace.endSection();

                    if (isCancelled()) {
                        return null;
                    }

                    if (DEBUG) {
                        Log.e(TAG, "*** allocated new bitmap in decode thread: "
                                + mInBitmap + " key=" + mKey);
                    }
                } else {
                    if (DEBUG) {
                        Log.e(TAG, "*** reusing existing bitmap in decode thread: "
                                + mInBitmap + " key=" + mKey);
                    }

                }
                mOpts.inBitmap = mInBitmap.bmp;
            }

            if (isCancelled()) {
                return null;
            }

            if (fd == null) {
                in = reset(in);
                if (in == null) {
                    return null;
                }
                if (isCancelled()) {
                    return null;
                }
            }


            Bitmap decodeResult = null;
            final Rect srcRect = new Rect(); // Not orientation corrected. True coordinates.
            if (CROP_DURING_DECODE) {
                try {
                    Trace.beginSection("decodeCropped" + mOpts.inSampleSize);

                    // BEGIN MANUAL INLINE decodeCropped()

                    final BitmapRegionDecoder brd;
                    if (fd != null) {
                        brd = BitmapRegionDecoder
                                .newInstance(fd.getFileDescriptor(), true /* shareable */);
                    } else {
                        brd = BitmapRegionDecoder.newInstance(in, true /* shareable */);
                    }

                    final Bitmap bitmap;
                    if (isCancelled()) {
                        bitmap = null;
                    } else {
                        // We want to call calculateCroppedSrcRect() on the source rectangle "as
                        // if" the orientation has been corrected.
                        // Center the decode on the top 1/3.
                        BitmapUtils.calculateCroppedSrcRect(srcW, srcH, mDecodeOpts.destW,
                                mDecodeOpts.destH, mDecodeOpts.destH, mOpts.inSampleSize,
                                mDecodeOpts.horizontalCenter, mDecodeOpts.verticalCenter,
                                true /* absoluteFraction */,
                                1f, srcRect);
                        if (DEBUG) {
                            System.out.println("rect for this decode is: " + srcRect
                                    + " srcW/H=" + srcW + "/" + srcH
                                    + " dstW/H=" + mDecodeOpts.destW + "/" + mDecodeOpts.destH);
                        }

                        // calculateCroppedSrcRect() gave us the source rectangle "as if" the
                        // orientation has been corrected. We need to decode the uncorrected
                        // source rectangle. Calculate true coordinates.
                        RectUtils.rotateRectForOrientation(orientation, new Rect(0, 0, srcW, srcH),
                                srcRect);

                        bitmap = brd.decodeRegion(srcRect, mOpts);
                    }
                    brd.recycle();

                    // END MANUAL INLINE decodeCropped()

                    decodeResult = bitmap;
                } catch (IOException e) {
                    // fall through to below and try again with the non-cropping decoder
                    if (fd == null) {
                        in = reset(in);
                        if (in == null) {
                            return null;
                        }
                        if (isCancelled()) {
                            return null;
                        }
                    }

                    e.printStackTrace();
                } finally {
                    Trace.endSection();
                }

                if (isCancelled()) {
                    return null;
                }
            }

            //noinspection PointlessBooleanExpression
            if (!CROP_DURING_DECODE || (decodeResult == null && !isCancelled())) {
                try {
                    Trace.beginSection("decode" + mOpts.inSampleSize);
                    // disable inBitmap-- bitmap reuse doesn't work well below K
                    if (mInBitmap != null) {
                        mCache.offer(mInBitmap);
                        mInBitmap = null;
                        mOpts.inBitmap = null;
                    }
                    decodeResult = decode(fd, in);
                } catch (IllegalArgumentException e) {
                    Log.e(TAG, "decode failed: reason='" + e.getMessage() + "' ss="
                            + mOpts.inSampleSize);

                    if (mOpts.inSampleSize > 1) {
                        // try again with ss=1
                        mOpts.inSampleSize = 1;
                        decodeResult = decode(fd, in);
                    }
                } finally {
                    Trace.endSection();
                }

                if (isCancelled()) {
                    return null;
                }
            }

            if (decodeResult == null) {
                return null;
            }

            if (mInBitmap != null) {
                result = mInBitmap;
                // srcRect is non-empty when using the cropping BitmapRegionDecoder codepath
                if (!srcRect.isEmpty()) {
                    result.setLogicalWidth((srcRect.right - srcRect.left) / mOpts.inSampleSize);
                    result.setLogicalHeight(
                            (srcRect.bottom - srcRect.top) / mOpts.inSampleSize);
                } else {
                    result.setLogicalWidth(mOpts.outWidth);
                    result.setLogicalHeight(mOpts.outHeight);
                }
            } else {
                // no mInBitmap means no pooling
                result = new ReusableBitmap(decodeResult, false /* reusable */);
                if (isNotRotatedOr180) {
                    result.setLogicalWidth(decodeResult.getWidth());
                    result.setLogicalHeight(decodeResult.getHeight());
                } else {
                    result.setLogicalWidth(decodeResult.getHeight());
                    result.setLogicalHeight(decodeResult.getWidth());
                }
            }
            result.setOrientation(orientation);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (fd != null) {
                try {
                    fd.close();
                } catch (IOException ignored) {
                }
            }
            if (in != null) {
                try {
                    in.close();
                } catch (IOException ignored) {
                }
            }

            // Cancellations can't be guaranteed to be correct, so skip the cache
            if (!isCancelled()) {
                // Put result in cache, regardless of null. The cache will handle null results.
                mCache.put(mKey, result);
            }
            if (result != null) {
                result.acquireReference();
                if (DEBUG) {
                    Log.d(TAG, "placed result in cache: key=" + mKey + " bmp="
                        + result + " cancelled=" + isCancelled());
                }
            } else if (mInBitmap != null) {
                if (DEBUG) {
                    Log.d(TAG, "placing failed/cancelled bitmap in pool: key="
                        + mKey + " bmp=" + mInBitmap);
                }
                mCache.offer(mInBitmap);
            }
        }
        return result;
    }

    /**
     * Return an input stream that can be read from the beginning using the most efficient way,
     * given an input stream that may or may not support reset(), or given null.
     *
     * The returned input stream may or may not be the same stream.
     */
    private InputStream reset(InputStream in) throws IOException {
        Trace.beginSection("create stream");
        if (in == null) {
            in = mKey.createInputStream();
        } else if (in.markSupported()) {
            in.reset();
        } else {
            try {
                in.close();
            } catch (IOException ignored) {
            }
            in = mKey.createInputStream();
        }
        Trace.endSection();
        return in;
    }

    private Bitmap decode(ParcelFileDescriptor fd, InputStream in) {
        final Bitmap result;
        if (fd != null) {
            result = BitmapFactory.decodeFileDescriptor(fd.getFileDescriptor(), null, mOpts);
        } else {
            result = BitmapFactory.decodeStream(in, null, mOpts);
        }
        return result;
    }

    public void cancel() {
        cancel(true);
        mOpts.requestCancelDecode();
    }

    @Override
    protected void onProgressUpdate(Void... values) {
        mDecodeCallback.onDecodeBegin(mKey);
    }

    @Override
    public void onPostExecute(ReusableBitmap result) {
        mDecodeCallback.onDecodeComplete(mKey, result);
    }

    @Override
    protected void onCancelled(ReusableBitmap result) {
        mDecodeCallback.onDecodeCancel(mKey);
        if (result == null) {
            return;
        }

        result.releaseReference();
        if (mInBitmap == null) {
            // not reusing bitmaps: can recycle immediately
            result.bmp.recycle();
        }
    }

    /**
     * Parameters to pass to the DecodeTask.
     */
    public static class DecodeOptions {

        /**
         * Round sample size to the nearest power of 2. Depending on the source and destination
         * dimensions, we will either truncate, in which case we decode from a bigger region and
         * crop down, or we will round up, in which case we decode from a smaller region and scale
         * up.
         */
        public static final int STRATEGY_ROUND_NEAREST = 0;
        /**
         * Always decode from a bigger region and crop down.
         */
        public static final int STRATEGY_TRUNCATE = 1;

        /**
         * Always decode from a smaller region and scale up.
         */
        public static final int STRATEGY_ROUND_UP = 2;

        /**
         * The destination width to decode to.
         */
        public int destW;
        /**
         * The destination height to decode to.
         */
        public int destH;
        /**
         * If the destination dimensions are smaller than the source image provided by the request
         * key, this will determine where horizontally the destination rect will be cropped from.
         * Value from 0f for left-most crop to 1f for right-most crop.
         */
        public float horizontalCenter;
        /**
         * If the destination dimensions are smaller than the source image provided by the request
         * key, this will determine where vertically the destination rect will be cropped from.
         * Value from 0f for top-most crop to 1f for bottom-most crop.
         */
        public float verticalCenter;
        /**
         * One of the STRATEGY constants.
         */
        public int sampleSizeStrategy;

        public DecodeOptions(final int destW, final int destH) {
            this(destW, destH, 0.5f, 0.5f, STRATEGY_ROUND_NEAREST);
        }

        /**
         * Create new DecodeOptions with horizontally-centered cropping if applicable.
         * @param destW The destination width to decode to.
         * @param destH The destination height to decode to.
         * @param verticalCenter If the destination dimensions are smaller than the source image
         *                       provided by the request key, this will determine where vertically
         *                       the destination rect will be cropped from.
         * @param sampleSizeStrategy One of the STRATEGY constants.
         */
        public DecodeOptions(final int destW, final int destH,
                final float verticalCenter, final int sampleSizeStrategy) {
            this(destW, destH, 0.5f, verticalCenter, sampleSizeStrategy);
        }

        /**
         * Create new DecodeOptions.
         * @param destW The destination width to decode to.
         * @param destH The destination height to decode to.
         * @param horizontalCenter If the destination dimensions are smaller than the source image
         *                         provided by the request key, this will determine where
         *                         horizontally the destination rect will be cropped from.
         * @param verticalCenter If the destination dimensions are smaller than the source image
         *                       provided by the request key, this will determine where vertically
         *                       the destination rect will be cropped from.
         * @param sampleSizeStrategy One of the STRATEGY constants.
         */
        public DecodeOptions(final int destW, final int destH, final float horizontalCenter,
                final float verticalCenter, final int sampleSizeStrategy) {
            this.destW = destW;
            this.destH = destH;
            this.horizontalCenter = horizontalCenter;
            this.verticalCenter = verticalCenter;
            this.sampleSizeStrategy = sampleSizeStrategy;
        }
    }
}
