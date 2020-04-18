/*
 * Copyright 2018 The Android Open Source Project
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
package android.support.customtabs.browseractions;

import android.content.ContentResolver;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.FileDescriptor;
import java.io.IOException;

/**
 * The {@link AsyncTask} handles:
 *  1. Inflate a fallback view when the requested image is not ready.
 *  2. Load the image from given uri generated from {@link BrowserServiceFileProvider}.
 *  3. Update the UI when image is ready.
 * To use this class:
 *  1. Override handlePreLoadingFallback() to inflate a fallback UI when image is not ready.
 *  2. Override onBitmapFileReady(Bitmap) to update the UI.
 */
public abstract class BrowserServiceImageReadTask extends AsyncTask<Uri, Void, Bitmap> {
    private static final String TAG = "BrowserServiceImageReadTask";
    private final ContentResolver mResolver;

    public BrowserServiceImageReadTask(ContentResolver resolver) {
        super();
        mResolver = resolver;
    }

    @Override
    protected Bitmap doInBackground(Uri... params) {
        try {
            ParcelFileDescriptor descriptor = mResolver.openFileDescriptor(params[0], "r");
            if (descriptor == null) return null;
            FileDescriptor fileDescriptor = descriptor.getFileDescriptor();
            Bitmap bitmap = BitmapFactory.decodeFileDescriptor(fileDescriptor);
            descriptor.close();
            return bitmap;
        } catch (IOException e) {
            Log.e(TAG, "Failed to read bitmap", e);
        }
        return null;
    }

    @Override
    protected final void onPostExecute(Bitmap bitmap) {
        onBitmapFileReady(bitmap);
    }

    /**
     * Called when bitmap image is read from disk and ready to use.
     * @param bitmap The bitmap corresponds to the given uri.
     */
    protected abstract void onBitmapFileReady(Bitmap bitmap);
}