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

import android.os.ParcelFileDescriptor;

import java.io.IOException;
import java.io.InputStream;

/**
 * The decode task uses this class to get input to decode. You must implement at least one of
 * {@link #createFileDescriptorFactoryAsync(RequestKey, Callback)} or {@link #createInputStream()}.
 * {@link DecodeTask} will prioritize
 * {@link #createFileDescriptorFactoryAsync(RequestKey, Callback)} before falling back to
 * {@link #createInputStream()}.
 *
 * <p>
 * Clients of this interface must also implement {@link #equals(Object)} and {@link #hashCode()} as
 * this object will be used as a cache key.
 *
 * <p>
 * The following is a high level view of the interactions between RequestKey and the rest of the
 * system.
 *
 *       BasicBitmapDrawable
 *           UI Thread
 *              ++
 *       bind() ||            Background Thread
 *              |+-------------------->+
 *              || createFDFasync()   ||
 *              ||                    || Download from url
 *              ||                    || Cache on disk
 *              ||                    ||
 *              ||                    vv
 *              |<--------------------+x
 *              ||        FDFcreated()
 *              ||
 *              ||
 *              ||                DecodeTask
 *              ||             AsyncTask Thread
 *              |+-------------------->+
 *              || new().execute()    ||
 *              ||                    || Decode from FDF
 *              ||                    || or createInputStream()
 *              ||                    ||
 *              ||                    vv
 *              |<--------------------+x
 *              ||  onDecodeComplete()
 *              vv
 * invalidate() xx
 */
public interface RequestKey {

    /**
     * Create an {@link FileDescriptorFactory} for a local file stored on the device and pass it to
     * the given callback. This method will be called in favor of {@link #createInputStream()}},
     * which will only be called if null is returned from this method,
     * or {@link Callback#fileDescriptorFactoryCreated(RequestKey, FileDescriptorFactory)} is called
     * with a null FileDescriptorFactory.
     *
     * Clients should implement this method if files are in the local cache folder, or if files must
     * be downloaded and cached.
     *
     * This method must be called from the UI thread.
     *
     * @param key      The key to create a FileDescriptorFactory for. This key will be passed to the
     *                 callback so it can check whether the key has changed.
     * @param callback The callback to notify once the FileDescriptorFactory is created or has failed
     *                 to be created.
     *                 Do not invoke the callback directly from this method. Instead, create a handler
     *                 and post a Runnable.
     *
     * @return If the client will attempt to create a FileDescriptorFactory, return a Cancelable
     * object to cancel the asynchronous task. If the client wants to create an InputStream instead,
     * return null. The callback must be notified if and only if the client returns a Cancelable
     * object and not null.
     */
    public Cancelable createFileDescriptorFactoryAsync(RequestKey key, Callback callback);

    /**
     * Create an {@link InputStream} for the source. This method will be called if
     * {@link #createFileDescriptorFactoryAsync(RequestKey, Callback)} returns null.
     *
     * Clients should implement this method if files exist in the assets/ folder, or for prototypes
     * that open a connection directly on a URL (be warned that this will cause GCs).
     *
     * This method can be called from any thread.
     */
    public InputStream createInputStream() throws IOException;

    /**
     * Return true if the image source may have be oriented in either portrait or landscape, and
     * will need to be automatically re-oriented based on accompanying Exif metadata.
     *
     * This method can be called from any thread.
     */
    public boolean hasOrientationExif() throws IOException;

    /**
     * Callback for creating the {@link FileDescriptorFactory} asynchronously.
     */
    public interface Callback {

        /**
         * Notifies that the {@link FileDescriptorFactory} has been created. This must be called on
         * the UI thread.
         * @param key The key that the FileDescriptorFactory was created for. The callback should
         *            check that the key has not changed.
         * @param factory The FileDescriptorFactory to decode from. Pass null to cancel decode.
         */
        void fileDescriptorFactoryCreated(RequestKey key, FileDescriptorFactory factory);
    }

    public interface FileDescriptorFactory {
        ParcelFileDescriptor createFileDescriptor();
    }

    /**
     * Interface for a background task that is cancelable.
     */
    public interface Cancelable {

        /**
         * Cancel the background task. This must be called on the UI thread.
         */
        void cancel();
    }
}
