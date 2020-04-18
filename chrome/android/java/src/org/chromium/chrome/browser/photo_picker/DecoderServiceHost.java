// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.photo_picker;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.StrictMode;
import android.os.SystemClock;
import android.support.annotation.Nullable;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.util.ConversionUtils;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * A class to communicate with the {@link DecoderService}.
 */
public class DecoderServiceHost extends IDecoderServiceCallback.Stub {
    // A tag for logging error messages.
    private static final String TAG = "ImageDecoderHost";

    // The number of successful decodes, per batch.
    private int mSuccessfulDecodes = 0;

    // The number of runtime failures during decoding, per batch.
    private int mFailedDecodesRuntime = 0;

    // The number of out of memory failures during decoding, per batch.
    private int mFailedDecodesMemory = 0;

    // A callback to use for testing to see if decoder is ready.
    static ServiceReadyCallback sReadyCallbackForTesting;

    IDecoderService mIRemoteService;
    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            mIRemoteService = IDecoderService.Stub.asInterface(service);
            mBound = true;
            for (ServiceReadyCallback callback : mCallbacks) {
                callback.serviceReady();
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            Log.e(TAG, "Service has unexpectedly disconnected");
            mIRemoteService = null;
            mBound = false;
        }
    };

    /**
     * Interface for notifying clients of the service being ready.
     */
    public interface ServiceReadyCallback {
        /**
         * A function to define to receive a notification once the service is up and running.
         */
        void serviceReady();
    }

    /**
     * An interface notifying clients when an image has finished decoding.
     */
    public interface ImageDecodedCallback {
        /**
         * A function to define to receive a notification that an image has been decoded.
         * @param filePath The file path for the newly decoded image.
         * @param bitmap The results of the decoding (or placeholder image, if failed).
         */
        void imageDecodedCallback(String filePath, Bitmap bitmap);
    }

    /**
     * Class for keeping track of the data involved with each request.
     */
    private static class DecoderServiceParams {
        // The path to the file containing the bitmap to decode.
        public String mFilePath;

        // The requested size (width and height) of the bitmap, once decoded.
        public int mSize;

        // The callback to use to communicate the results of the decoding.
        ImageDecodedCallback mCallback;

        // The timestamp for when the request was sent for decoding.
        long mTimestamp;

        public DecoderServiceParams(String filePath, int size, ImageDecodedCallback callback) {
            mFilePath = filePath;
            mSize = size;
            mCallback = callback;
        }
    }

    // Map of file paths to decoder parameters in order of request.
    private LinkedHashMap<String, DecoderServiceParams> mRequests = new LinkedHashMap<>();
    LinkedHashMap<String, DecoderServiceParams> getRequests() {
        return mRequests;
    }

    // The callbacks used to notify the clients when the service is ready.
    List<ServiceReadyCallback> mCallbacks = new ArrayList<ServiceReadyCallback>();

    // Flag indicating whether we are bound to the service.
    private boolean mBound;

    private final Context mContext;

    /**
     * The DecoderServiceHost constructor.
     * @param callback The callback to use when communicating back to the client.
     */
    public DecoderServiceHost(ServiceReadyCallback callback, Context context) {
        mCallbacks.add(callback);
        if (sReadyCallbackForTesting != null) {
            mCallbacks.add(sReadyCallbackForTesting);
        }
        mContext = context;
    }

    /**
     * Initiate binding with the {@link DecoderService}.
     * @param context The context to use.
     */
    public void bind(Context context) {
        Intent intent = new Intent(mContext, DecoderService.class);
        intent.setAction(IDecoderService.class.getName());
        mContext.bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
    }

    /**
     * Unbind from the {@link DecoderService}.
     * @param context The context to use.
     */
    public void unbind(Context context) {
        if (mBound) {
            context.unbindService(mConnection);
            mBound = false;
        }
    }

    /**
     * Accepts a request to decode a single image. Queues up the request and reports back
     * asynchronously on |callback|.
     * @param filePath The path to the file to decode.
     * @param size The requested size (width and height) of the resulting bitmap.
     * @param callback The callback to use to communicate the decoding results.
     */
    public void decodeImage(String filePath, int size, ImageDecodedCallback callback) {
        DecoderServiceParams params = new DecoderServiceParams(filePath, size, callback);
        mRequests.put(filePath, params);
        if (mRequests.size() == 1) dispatchNextDecodeImageRequest();
    }

    /**
     * Dispatches the next image for decoding (from the queue).
     */
    private void dispatchNextDecodeImageRequest() {
        if (mRequests.entrySet().iterator().hasNext()) {
            DecoderServiceParams params = mRequests.entrySet().iterator().next().getValue();
            params.mTimestamp = SystemClock.elapsedRealtime();
            dispatchDecodeImageRequest(params.mFilePath, params.mSize);
        } else {
            int totalRequests = mSuccessfulDecodes + mFailedDecodesRuntime + mFailedDecodesMemory;
            if (totalRequests > 0) {
                int runtimeFailures = 100 * mFailedDecodesRuntime / totalRequests;
                RecordHistogram.recordPercentageHistogram(
                        "Android.PhotoPicker.DecoderHostFailureRuntime", runtimeFailures);

                int memoryFailures = 100 * mFailedDecodesMemory / totalRequests;
                RecordHistogram.recordPercentageHistogram(
                        "Android.PhotoPicker.DecoderHostFailureOutOfMemory", memoryFailures);

                mSuccessfulDecodes = 0;
                mFailedDecodesRuntime = 0;
                mFailedDecodesMemory = 0;
            }
        }
    }

    @Override
    public void onDecodeImageDone(final Bundle payload) {
        // As per the Android documentation, AIDL callbacks can (and will) happen on any thread, so
        // make sure the code runs on the UI thread, since further down the callchain the code will
        // end up creating UI objects.
        ThreadUtils.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    // Read the reply back from the service.
                    String filePath = payload.getString(DecoderService.KEY_FILE_PATH);
                    Boolean success = payload.getBoolean(DecoderService.KEY_SUCCESS);
                    Bitmap bitmap = success
                            ? (Bitmap) payload.getParcelable(DecoderService.KEY_IMAGE_BITMAP)
                            : null;
                    long decodeTime = payload.getLong(DecoderService.KEY_DECODE_TIME);
                    mSuccessfulDecodes++;
                    closeRequest(filePath, bitmap, decodeTime);
                } catch (RuntimeException e) {
                    mFailedDecodesRuntime++;
                } catch (OutOfMemoryError e) {
                    mFailedDecodesMemory++;
                }
            }
        });
    }

    /**
     * Ties up all the loose ends from the decoding request (communicates the results of the
     * decoding process back to the client, and takes care of house-keeping chores regarding
     * the request queue).
     * @param filePath The path to the image that was just decoded.
     * @param bitmap The resulting decoded bitmap, or null if decoding fails.
     * @param decodeTime The length of time it took to decode the bitmap.
     */
    public void closeRequest(String filePath, @Nullable Bitmap bitmap, long decodeTime) {
        DecoderServiceParams params = getRequests().get(filePath);
        if (params != null) {
            long endRpcCall = SystemClock.elapsedRealtime();
            RecordHistogram.recordTimesHistogram("Android.PhotoPicker.RequestProcessTime",
                    endRpcCall - params.mTimestamp, TimeUnit.MILLISECONDS);

            params.mCallback.imageDecodedCallback(filePath, bitmap);

            if (decodeTime != -1 && bitmap != null) {
                RecordHistogram.recordTimesHistogram(
                        "Android.PhotoPicker.ImageDecodeTime", decodeTime, TimeUnit.MILLISECONDS);

                int sizeInKB = bitmap.getByteCount() / ConversionUtils.BYTES_PER_KILOBYTE;
                RecordHistogram.recordCustomCountHistogram(
                        "Android.PhotoPicker.ImageByteCount", sizeInKB, 1, 100000, 50);
            }
            getRequests().remove(filePath);
        }
        dispatchNextDecodeImageRequest();
    }

    /**
     * Communicates with the server to decode a single bitmap.
     * @param filePath The path to the image on disk.
     * @param size The requested width and height of the resulting bitmap.
     */
    private void dispatchDecodeImageRequest(String filePath, int size) {
        // Obtain a file descriptor to send over to the sandboxed process.
        File file = new File(filePath);
        FileInputStream inputFile = null;
        ParcelFileDescriptor pfd = null;
        Bundle bundle = new Bundle();

        // The restricted utility process can't open the file to read the
        // contents, so we need to obtain a file descriptor to pass over.
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            try {
                inputFile = new FileInputStream(file);
                FileDescriptor fd = inputFile.getFD();
                pfd = ParcelFileDescriptor.dup(fd);
                bundle.putParcelable(DecoderService.KEY_FILE_DESCRIPTOR, pfd);
            } catch (IOException e) {
                Log.e(TAG, "Unable to obtain FileDescriptor: " + e);
                closeRequest(filePath, null, -1);
            }
        } finally {
            try {
                if (inputFile != null) inputFile.close();
            } catch (IOException e) {
                Log.e(TAG, "Unable to close inputFile: " + e);
            }
            StrictMode.setThreadPolicy(oldPolicy);
        }

        if (pfd == null) return;

        // Prepare and send the data over.
        bundle.putString(DecoderService.KEY_FILE_PATH, filePath);
        bundle.putInt(DecoderService.KEY_SIZE, size);
        try {
            mIRemoteService.decodeImage(bundle, this);
            pfd.close();
        } catch (RemoteException e) {
            Log.e(TAG, "Communications failed (Remote): " + e);
            closeRequest(filePath, null, -1);
        } catch (IOException e) {
            Log.e(TAG, "Communications failed (IO): " + e);
            closeRequest(filePath, null, -1);
        }
    }

    /**
     * Cancels a request to decode an image (if it hasn't already been dispatched).
     * @param filePath The path to the image to cancel decoding.
     */
    public void cancelDecodeImage(String filePath) {
        mRequests.remove(filePath);
    }

    /** Sets a callback to use when the service is ready. For testing use only. */
    @VisibleForTesting
    public static void setReadyCallback(ServiceReadyCallback callback) {
        sReadyCallbackForTesting = callback;
    }
}
