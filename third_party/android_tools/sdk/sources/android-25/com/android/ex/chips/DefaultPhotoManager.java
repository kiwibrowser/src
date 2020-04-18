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

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.provider.ContactsContract;
import android.support.v4.util.LruCache;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Default implementation of {@link com.android.ex.chips.PhotoManager} that
 * queries for photo bytes by using the {@link com.android.ex.chips.RecipientEntry}'s
 * photoThumbnailUri.
 */
public class DefaultPhotoManager implements PhotoManager {
    private static final String TAG = "DefaultPhotoManager";

    private static final boolean DEBUG = false;

    /**
     * For reading photos for directory contacts, this is the chunk size for
     * copying from the {@link InputStream} to the output stream.
     */
    private static final int BUFFER_SIZE = 1024*16;

    private static class PhotoQuery {
        public static final String[] PROJECTION = {
            ContactsContract.CommonDataKinds.Photo.PHOTO
        };

        public static final int PHOTO = 0;
    }

    private final ContentResolver mContentResolver;
    private final LruCache<Uri, byte[]> mPhotoCacheMap;

    public DefaultPhotoManager(ContentResolver contentResolver) {
        mContentResolver = contentResolver;
        mPhotoCacheMap = new LruCache<Uri, byte[]>(PHOTO_CACHE_SIZE);
    }

    @Override
    public void populatePhotoBytesAsync(RecipientEntry entry, PhotoManagerCallback callback) {
        final Uri photoThumbnailUri = entry.getPhotoThumbnailUri();
        if (photoThumbnailUri != null) {
            final byte[] photoBytes = mPhotoCacheMap.get(photoThumbnailUri);
            if (photoBytes != null) {
                entry.setPhotoBytes(photoBytes);
                if (callback != null) {
                    callback.onPhotoBytesPopulated();
                }
            } else {
                if (DEBUG) {
                    Log.d(TAG, "No photo cache for " + entry.getDisplayName()
                            + ". Fetch one asynchronously");
                }
                fetchPhotoAsync(entry, photoThumbnailUri, callback);
            }
        } else if (callback != null) {
            callback.onPhotoBytesAsyncLoadFailed();
        }
    }

    private void fetchPhotoAsync(final RecipientEntry entry, final Uri photoThumbnailUri,
            final PhotoManagerCallback callback) {
        final AsyncTask<Void, Void, byte[]> photoLoadTask = new AsyncTask<Void, Void, byte[]>() {
            @Override
            protected byte[] doInBackground(Void... params) {
                // First try running a query. Images for local contacts are
                // loaded by sending a query to the ContactsProvider.
                final Cursor photoCursor = mContentResolver.query(
                        photoThumbnailUri, PhotoQuery.PROJECTION, null, null, null);
                if (photoCursor != null) {
                    try {
                        if (photoCursor.moveToFirst()) {
                            return photoCursor.getBlob(PhotoQuery.PHOTO);
                        }
                    } finally {
                        photoCursor.close();
                    }
                } else {
                    // If the query fails, try streaming the URI directly.
                    // For remote directory images, this URI resolves to the
                    // directory provider and the images are loaded by sending
                    // an openFile call to the provider.
                    try {
                        InputStream is = mContentResolver.openInputStream(
                                photoThumbnailUri);
                        if (is != null) {
                            byte[] buffer = new byte[BUFFER_SIZE];
                            ByteArrayOutputStream baos = new ByteArrayOutputStream();
                            try {
                                int size;
                                while ((size = is.read(buffer)) != -1) {
                                    baos.write(buffer, 0, size);
                                }
                            } finally {
                                is.close();
                            }
                            return baos.toByteArray();
                        }
                    } catch (IOException ex) {
                        // ignore
                    }
                }
                return null;
            }

            @Override
            protected void onPostExecute(final byte[] photoBytes) {
                entry.setPhotoBytes(photoBytes);
                if (photoBytes != null) {
                    mPhotoCacheMap.put(photoThumbnailUri, photoBytes);
                    if (callback != null) {
                        callback.onPhotoBytesAsynchronouslyPopulated();
                    }
                } else if (callback != null) {
                    callback.onPhotoBytesAsyncLoadFailed();
                }
            }
        };
        photoLoadTask.executeOnExecutor(AsyncTask.SERIAL_EXECUTOR);
    }
}
