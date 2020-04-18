/*
 * Copyright (C) 2011 Google Inc.
 * Licensed to The Android Open Source Project.
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

package com.android.ex.photo.util;

import android.content.ContentResolver;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Build;
import android.util.Base64;
import android.util.Log;

import com.android.ex.photo.PhotoViewController;
import com.android.ex.photo.loaders.PhotoBitmapLoaderInterface.BitmapResult;

import java.io.ByteArrayInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.regex.Pattern;


/**
 * Image utilities
 */
public class ImageUtils {
    // Logging
    private static final String TAG = "ImageUtils";

    /** Minimum class memory class to use full-res photos */
    private final static long MIN_NORMAL_CLASS = 32;
    /** Minimum class memory class to use small photos */
    private final static long MIN_SMALL_CLASS = 24;

    private static final String BASE64_URI_PREFIX = "base64,";
    private static final Pattern BASE64_IMAGE_URI_PATTERN = Pattern.compile("^(?:.*;)?base64,.*");

    public static enum ImageSize {
        EXTRA_SMALL,
        SMALL,
        NORMAL,
    }

    public static final ImageSize sUseImageSize;
    static {
        // On HC and beyond, assume devices are more capable
        if (Build.VERSION.SDK_INT >= 11) {
            sUseImageSize = ImageSize.NORMAL;
        } else {
            if (PhotoViewController.sMemoryClass >= MIN_NORMAL_CLASS) {
                // We have plenty of memory; use full sized photos
                sUseImageSize = ImageSize.NORMAL;
            } else if (PhotoViewController.sMemoryClass >= MIN_SMALL_CLASS) {
                // We have slight less memory; use smaller sized photos
                sUseImageSize = ImageSize.SMALL;
            } else {
                // We have little memory; use very small sized photos
                sUseImageSize = ImageSize.EXTRA_SMALL;
            }
        }
    }

    /**
     * @return true if the MimeType type is image
     */
    public static boolean isImageMimeType(String mimeType) {
        return mimeType != null && mimeType.startsWith("image/");
    }

    /**
     * Create a bitmap from a local URI
     *
     * @param resolver The ContentResolver
     * @param uri      The local URI
     * @param maxSize  The maximum size (either width or height)
     * @return The new bitmap or null
     */
    public static BitmapResult createLocalBitmap(final ContentResolver resolver, final Uri uri,
            final int maxSize) {
        final BitmapResult result = new BitmapResult();
        final InputStreamFactory factory = createInputStreamFactory(resolver, uri);
        try {
            final Point bounds = getImageBounds(factory);
            if (bounds == null) {
                result.status = BitmapResult.STATUS_EXCEPTION;
                return result;
            }

            final BitmapFactory.Options opts = new BitmapFactory.Options();
            opts.inSampleSize = Math.max(bounds.x / maxSize, bounds.y / maxSize);
            result.bitmap = decodeStream(factory, null, opts);
            result.status = BitmapResult.STATUS_SUCCESS;
            return result;

        } catch (FileNotFoundException exception) {
            // Do nothing - the photo will appear to be missing
        } catch (IOException exception) {
            result.status = BitmapResult.STATUS_EXCEPTION;
        } catch (IllegalArgumentException exception) {
            // Do nothing - the photo will appear to be missing
        } catch (SecurityException exception) {
            result.status = BitmapResult.STATUS_EXCEPTION;
        }
        return result;
    }

    /**
     * Wrapper around {@link BitmapFactory#decodeStream(InputStream, Rect,
     * BitmapFactory.Options)} that returns {@code null} on {@link
     * OutOfMemoryError}.
     *
     * @param factory    Used to create input streams that holds the raw data to be decoded into a
     *                   bitmap.
     * @param outPadding If not null, return the padding rect for the bitmap if
     *                   it exists, otherwise set padding to [-1,-1,-1,-1]. If
     *                   no bitmap is returned (null) then padding is
     *                   unchanged.
     * @param opts       null-ok; Options that control downsampling and whether the
     *                   image should be completely decoded, or just is size returned.
     * @return The decoded bitmap, or null if the image data could not be
     * decoded, or, if opts is non-null, if opts requested only the
     * size be returned (in opts.outWidth and opts.outHeight)
     */
    public static Bitmap decodeStream(final InputStreamFactory factory, final Rect outPadding,
            final BitmapFactory.Options opts) throws FileNotFoundException {
        InputStream is = null;
        try {
            // Determine the orientation for this image
            is = factory.createInputStream();
            final int orientation = Exif.getOrientation(is, -1);
            if (is != null) {
                is.close();
            }

            // Decode the bitmap
            is = factory.createInputStream();
            final Bitmap originalBitmap = BitmapFactory.decodeStream(is, outPadding, opts);

            if (is != null && originalBitmap == null && !opts.inJustDecodeBounds) {
                Log.w(TAG, "ImageUtils#decodeStream(InputStream, Rect, Options): "
                        + "Image bytes cannot be decoded into a Bitmap");
                throw new UnsupportedOperationException(
                        "Image bytes cannot be decoded into a Bitmap.");
            }

            // Rotate the Bitmap based on the orientation
            if (originalBitmap != null && orientation != 0) {
                final Matrix matrix = new Matrix();
                matrix.postRotate(orientation);
                return Bitmap.createBitmap(originalBitmap, 0, 0, originalBitmap.getWidth(),
                        originalBitmap.getHeight(), matrix, true);
            }
            return originalBitmap;
        } catch (OutOfMemoryError oome) {
            Log.e(TAG, "ImageUtils#decodeStream(InputStream, Rect, Options) threw an OOME", oome);
            return null;
        } catch (IOException ioe) {
            Log.e(TAG, "ImageUtils#decodeStream(InputStream, Rect, Options) threw an IOE", ioe);
            return null;
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {
                    // Do nothing
                }
            }
        }
    }

    /**
     * Gets the image bounds
     *
     * @param factory Used to create the InputStream.
     *
     * @return The image bounds
     */
    private static Point getImageBounds(final InputStreamFactory factory)
            throws IOException {
        final BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inJustDecodeBounds = true;
        decodeStream(factory, null, opts);

        return new Point(opts.outWidth, opts.outHeight);
    }

    private static InputStreamFactory createInputStreamFactory(final ContentResolver resolver,
            final Uri uri) {
        final String scheme = uri.getScheme();
        if ("data".equals(scheme)) {
            return new DataInputStreamFactory(resolver, uri);
        }
        return new BaseInputStreamFactory(resolver, uri);
    }

    /**
     * Utility class for when an InputStream needs to be read multiple times. For example, one pass
     * may load EXIF orientation, and the second pass may do the actual Bitmap decode.
     */
    public interface InputStreamFactory {

        /**
         * Create a new InputStream. The caller of this method must be able to read the input
         * stream starting from the beginning.
         * @return
         */
        InputStream createInputStream() throws FileNotFoundException;
    }

    private static class BaseInputStreamFactory implements InputStreamFactory {
        protected final ContentResolver mResolver;
        protected final Uri mUri;

        public BaseInputStreamFactory(final ContentResolver resolver, final Uri uri) {
            mResolver = resolver;
            mUri = uri;
        }

        @Override
        public InputStream createInputStream() throws FileNotFoundException {
            return mResolver.openInputStream(mUri);
        }
    }

    private static class DataInputStreamFactory extends BaseInputStreamFactory {
        private byte[] mData;

        public DataInputStreamFactory(final ContentResolver resolver, final Uri uri) {
            super(resolver, uri);
        }

        @Override
        public InputStream createInputStream() throws FileNotFoundException {
            if (mData == null) {
                mData = parseDataUri(mUri);
                if (mData == null) {
                    return super.createInputStream();
                }
            }
            return new ByteArrayInputStream(mData);
        }

        private byte[] parseDataUri(final Uri uri) {
            final String ssp = uri.getSchemeSpecificPart();
            try {
                if (ssp.startsWith(BASE64_URI_PREFIX)) {
                    final String base64 = ssp.substring(BASE64_URI_PREFIX.length());
                    return Base64.decode(base64, Base64.URL_SAFE);
                } else if (BASE64_IMAGE_URI_PATTERN.matcher(ssp).matches()){
                    final String base64 = ssp.substring(
                            ssp.indexOf(BASE64_URI_PREFIX) + BASE64_URI_PREFIX.length());
                    return Base64.decode(base64, Base64.DEFAULT);
                } else {
                    return null;
                }
            } catch (IllegalArgumentException ex) {
                Log.e(TAG, "Mailformed data URI: " + ex);
                return null;
            }
        }
    }
}
