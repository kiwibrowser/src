// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.nfc;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.nfc.FormatException;
import android.nfc.NdefMessage;
import android.nfc.NfcAdapter;
import android.nfc.NfcAdapter.ReaderCallback;
import android.nfc.NfcManager;
import android.nfc.Tag;
import android.nfc.TagLostException;
import android.os.Build;
import android.os.Handler;
import android.os.Process;
import android.util.SparseArray;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.device.mojom.Nfc;
import org.chromium.device.mojom.NfcClient;
import org.chromium.device.mojom.NfcError;
import org.chromium.device.mojom.NfcErrorType;
import org.chromium.device.mojom.NfcMessage;
import org.chromium.device.mojom.NfcPushOptions;
import org.chromium.device.mojom.NfcPushTarget;
import org.chromium.device.mojom.NfcWatchMode;
import org.chromium.device.mojom.NfcWatchOptions;
import org.chromium.mojo.bindings.Callbacks;
import org.chromium.mojo.system.MojoException;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

/** Android implementation of the NFC mojo service defined in device/nfc/nfc.mojom.
 */
public class NfcImpl implements Nfc {
    private static final String TAG = "NfcImpl";
    private static final String ANY_PATH = "/*";

    private final int mHostId;

    private final NfcDelegate mDelegate;

    /**
     * Used to get instance of NFC adapter, @see android.nfc.NfcManager
     */
    private final NfcManager mNfcManager;

    /**
     * NFC adapter. @see android.nfc.NfcAdapter
     */
    private final NfcAdapter mNfcAdapter;

    /**
     * Activity that is in foreground and is used to enable / disable NFC reader mode operations.
     * Can be updated when activity associated with web page is changed. @see #setActivity
     */
    private Activity mActivity;

    /**
     * Flag that indicates whether NFC permission is granted.
     */
    private final boolean mHasPermission;

    /**
     * Implementation of android.nfc.NfcAdapter.ReaderCallback. @see ReaderCallbackHandler
     */
    private ReaderCallbackHandler mReaderCallbackHandler;

    /**
     * Object that contains data that was passed to method
     * #push(NfcMessage message, NfcPushOptions options, PushResponse callback)
     * @see PendingPushOperation
     */
    private PendingPushOperation mPendingPushOperation;

    /**
     * Utility that provides I/O operations for a Tag. Created on demand when Tag is found.
     * @see NfcTagHandler
     */
    private NfcTagHandler mTagHandler;

    /**
     * Client interface used to deliver NFCMessages for registered watch operations.
     * @see #watch
     */
    private NfcClient mClient;

    /**
     * Watcher id that is incremented for each #watch call.
     */
    private int mWatcherId;

    /**
     * Map of watchId <-> NfcWatchOptions. All NfcWatchOptions are matched against tag that is in
     * proximity, when match algorithm (@see #matchesWatchOptions) returns true, watcher with
     * corresponding ID would be notified using NfcClient interface.
     * @see NfcClient#onWatch(int[] id, NfcMessage message)
     */
    private final SparseArray<NfcWatchOptions> mWatchers = new SparseArray<>();

    /**
     * Handler that runs delayed push timeout task.
     */
    private final Handler mPushTimeoutHandler = new Handler();

    /**
     * Runnable responsible for cancelling push operation after specified timeout.
     */
    private Runnable mPushTimeoutRunnable;

    public NfcImpl(int hostId, NfcDelegate delegate) {
        mHostId = hostId;
        mDelegate = delegate;
        int permission = ContextUtils.getApplicationContext().checkPermission(
                Manifest.permission.NFC, Process.myPid(), Process.myUid());
        mHasPermission = permission == PackageManager.PERMISSION_GRANTED;
        Callback<Activity> onActivityUpdatedCallback = new Callback<Activity>() {
            @Override
            public void onResult(Activity activity) {
                setActivity(activity);
            }
        };

        mDelegate.trackActivityForHost(mHostId, onActivityUpdatedCallback);

        if (!mHasPermission || Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
            Log.w(TAG, "NFC operations are not permitted.");
            mNfcAdapter = null;
            mNfcManager = null;
        } else {
            mNfcManager = (NfcManager) ContextUtils.getApplicationContext().getSystemService(
                    Context.NFC_SERVICE);
            if (mNfcManager == null) {
                Log.w(TAG, "NFC is not supported.");
                mNfcAdapter = null;
            } else {
                mNfcAdapter = mNfcManager.getDefaultAdapter();
            }
        }
    }

    /**
     * Sets Activity that is used to enable / disable NFC reader mode. When Activity is set,
     * reader mode is disabled for old Activity and enabled for the new Activity.
     */
    protected void setActivity(Activity activity) {
        disableReaderMode();
        mActivity = activity;
        enableReaderModeIfNeeded();
    }

    /**
     * Sets NfcClient. NfcClient interface is used to notify mojo NFC service client when NFC
     * device is in proximity and has NfcMessage that matches NfcWatchOptions criteria.
     * @see Nfc#watch(NfcWatchOptions options, WatchResponse callback)
     *
     * @param client @see NfcClient
     */
    @Override
    public void setClient(NfcClient client) {
        mClient = client;
    }

    /**
     * Pushes NfcMessage to Tag or Peer, whenever NFC device is in proximity. At the moment, only
     * passive NFC devices are supported (NfcPushTarget.TAG).
     *
     * @param message that should be pushed to NFC device.
     * @param options that contain information about timeout and target device type.
     * @param callback that is used to notify when push operation is completed.
     */
    @Override
    public void push(NfcMessage message, NfcPushOptions options, PushResponse callback) {
        if (!checkIfReady(callback)) return;

        if (!NfcMessageValidator.isValid(message)) {
            callback.call(createError(NfcErrorType.INVALID_MESSAGE));
            return;
        }

        // Check NfcPushOptions that are not supported by Android platform.
        if (options.target == NfcPushTarget.PEER || options.timeout < 0
                || (options.timeout > Long.MAX_VALUE && !Double.isInfinite(options.timeout))) {
            callback.call(createError(NfcErrorType.NOT_SUPPORTED));
            return;
        }

        // If previous pending push operation is not completed, cancel it.
        if (mPendingPushOperation != null) {
            mPendingPushOperation.complete(createError(NfcErrorType.OPERATION_CANCELLED));
            cancelPushTimeoutTask();
        }

        mPendingPushOperation = new PendingPushOperation(message, options, callback);

        // Schedule push timeout task for new #mPendingPushOperation.
        schedulePushTimeoutTask(options);
        enableReaderModeIfNeeded();
        processPendingPushOperation();
    }

    /**
     * Cancels pending push operation.
     * At the moment, only passive NFC devices are supported (NfcPushTarget.TAG).
     *
     * @param target @see NfcPushTarget
     * @param callback that is used to notify caller when cancelPush() is completed.
     */
    @Override
    public void cancelPush(int target, CancelPushResponse callback) {
        if (!checkIfReady(callback)) return;

        if (target == NfcPushTarget.PEER) {
            callback.call(createError(NfcErrorType.NOT_SUPPORTED));
            return;
        }

        if (mPendingPushOperation == null) {
            callback.call(createError(NfcErrorType.NOT_FOUND));
        } else {
            completePendingPushOperation(createError(NfcErrorType.OPERATION_CANCELLED));
            callback.call(null);
        }
    }

    /**
     * Watch method allows to set filtering criteria for NfcMessages that are found when NFC device
     * is within proximity. On success, watch ID is returned to caller through WatchResponse
     * callback. When NfcMessage that matches NfcWatchOptions is found, it is passed to NfcClient
     * interface together with corresponding watch ID.
     * @see NfcClient#onWatch(int[] id, NfcMessage message)
     *
     * @param options used to filter NfcMessages, @see NfcWatchOptions.
     * @param callback that is used to notify caller when watch() is completed and return watch ID.
     */
    @Override
    public void watch(NfcWatchOptions options, WatchResponse callback) {
        if (!checkIfReady(callback)) return;
        int watcherId = ++mWatcherId;
        mWatchers.put(watcherId, options);
        callback.call(watcherId, null);
        enableReaderModeIfNeeded();
        processPendingWatchOperations();
    }

    /**
     * Cancels NFC watch operation.
     *
     * @param id of watch operation.
     * @param callback that is used to notify caller when cancelWatch() is completed.
     */
    @Override
    public void cancelWatch(int id, CancelWatchResponse callback) {
        if (!checkIfReady(callback)) return;

        if (mWatchers.indexOfKey(id) < 0) {
            callback.call(createError(NfcErrorType.NOT_FOUND));
        } else {
            mWatchers.remove(id);
            callback.call(null);
            disableReaderModeIfNeeded();
        }
    }

    /**
     * Cancels all NFC watch operations.
     *
     * @param callback that is used to notify caller when cancelAllWatches() is completed.
     */
    @Override
    public void cancelAllWatches(CancelAllWatchesResponse callback) {
        if (!checkIfReady(callback)) return;

        if (mWatchers.size() == 0) {
            callback.call(createError(NfcErrorType.NOT_FOUND));
        } else {
            mWatchers.clear();
            callback.call(null);
            disableReaderModeIfNeeded();
        }
    }

    /**
     * Suspends all pending operations. Should be called when web page visibility is lost.
     */
    @Override
    public void suspendNfcOperations() {
        disableReaderMode();
    }

    /**
     * Resumes all pending watch / push operations. Should be called when web page becomes visible.
     */
    @Override
    public void resumeNfcOperations() {
        enableReaderModeIfNeeded();
    }

    @Override
    public void close() {
        mDelegate.stopTrackingActivityForHost(mHostId);
        disableReaderMode();
    }

    @Override
    public void onConnectionError(MojoException e) {
        close();
    }

    /**
     * Holds information about pending push operation.
     */
    private static class PendingPushOperation {
        public final NfcMessage nfcMessage;
        public final NfcPushOptions nfcPushOptions;
        private final PushResponse mPushResponseCallback;

        public PendingPushOperation(
                NfcMessage message, NfcPushOptions options, PushResponse callback) {
            nfcMessage = message;
            nfcPushOptions = options;
            mPushResponseCallback = callback;
        }

        /**
         * Completes pending push operation.
         *
         * @param error should be null when operation is completed successfully, otherwise,
         * error object with corresponding NfcErrorType should be provided.
         */
        public void complete(NfcError error) {
            if (mPushResponseCallback != null) mPushResponseCallback.call(error);
        }
    }

    /**
     * Helper method that creates NfcError object from NfcErrorType.
     */
    private NfcError createError(int errorType) {
        NfcError error = new NfcError();
        error.errorType = errorType;
        return error;
    }

    /**
     * Checks if NFC funcionality can be used by the mojo service. If permission to use NFC is
     * granted and hardware is enabled, returns null.
     */
    private NfcError checkIfReady() {
        if (!mHasPermission || mActivity == null) {
            return createError(NfcErrorType.SECURITY);
        } else if (mNfcManager == null || mNfcAdapter == null) {
            return createError(NfcErrorType.NOT_SUPPORTED);
        } else if (!mNfcAdapter.isEnabled()) {
            return createError(NfcErrorType.DEVICE_DISABLED);
        }
        return null;
    }

    /**
     * Uses checkIfReady() method and if NFC cannot be used, calls mojo callback with NfcError.
     *
     * @param WatchResponse Callback that is provided to watch() method.
     * @return boolean true if NFC functionality can be used, false otherwise.
     */
    private boolean checkIfReady(WatchResponse callback) {
        NfcError error = checkIfReady();
        if (error == null) return true;

        callback.call(0, error);
        return false;
    }

    /**
     * Uses checkIfReady() method and if NFC cannot be used, calls mojo callback with NfcError.
     *
     * @param callback Generic callback that is provided to push(), cancelPush(),
     * cancelWatch() and cancelAllWatches() methods.
     * @return boolean true if NFC functionality can be used, false otherwise.
     */
    private boolean checkIfReady(Callbacks.Callback1<NfcError> callback) {
        NfcError error = checkIfReady();
        if (error == null) return true;

        callback.call(error);
        return false;
    }

    /**
     * Implementation of android.nfc.NfcAdapter.ReaderCallback. Callback is called when NFC tag is
     * discovered, Tag object is delegated to mojo service implementation method
     * NfcImpl.onTagDiscovered().
     */
    @TargetApi(Build.VERSION_CODES.KITKAT)
    private static class ReaderCallbackHandler implements ReaderCallback {
        private final NfcImpl mNfcImpl;

        public ReaderCallbackHandler(NfcImpl impl) {
            mNfcImpl = impl;
        }

        @Override
        public void onTagDiscovered(Tag tag) {
            mNfcImpl.onTagDiscovered(tag);
        }
    }

    /**
     * Enables reader mode, allowing NFC device to read / write NFC tags.
     * @see android.nfc.NfcAdapter#enableReaderMode
     */
    private void enableReaderModeIfNeeded() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) return;

        if (mReaderCallbackHandler != null || mActivity == null || mNfcAdapter == null) return;

        // Do not enable reader mode, if there are no active push / watch operations.
        if (mPendingPushOperation == null && mWatchers.size() == 0) return;

        mReaderCallbackHandler = new ReaderCallbackHandler(this);
        mNfcAdapter.enableReaderMode(mActivity, mReaderCallbackHandler,
                NfcAdapter.FLAG_READER_NFC_A | NfcAdapter.FLAG_READER_NFC_B
                        | NfcAdapter.FLAG_READER_NFC_F | NfcAdapter.FLAG_READER_NFC_V,
                null);
    }

    /**
     * Disables reader mode.
     * @see android.nfc.NfcAdapter#disableReaderMode
     */
    @TargetApi(Build.VERSION_CODES.KITKAT)
    private void disableReaderMode() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) return;

        // There is no API that could query whether reader mode is enabled for adapter.
        // If mReaderCallbackHandler is null, reader mode is not enabled.
        if (mReaderCallbackHandler == null) return;

        mReaderCallbackHandler = null;
        if (mActivity != null && mNfcAdapter != null && !mActivity.isDestroyed()) {
            mNfcAdapter.disableReaderMode(mActivity);
        }
    }

    /**
     * Checks if there are pending push / watch operations and disables readre mode
     * whenever necessary.
     */
    private void disableReaderModeIfNeeded() {
        if (mPendingPushOperation == null && mWatchers.size() == 0) {
            disableReaderMode();
        }
    }

    /**
     * Handles completion of pending push operation, cancels timeout task and completes push
     * operation. On error, invalidates #mTagHandler.
     */
    private void pendingPushOperationCompleted(NfcError error) {
        completePendingPushOperation(error);
        if (error != null) mTagHandler = null;
    }

    /**
     * Completes pending push operation and disables reader mode if needed.
     */
    private void completePendingPushOperation(NfcError error) {
        if (mPendingPushOperation == null) return;

        cancelPushTimeoutTask();
        mPendingPushOperation.complete(error);
        mPendingPushOperation = null;
        disableReaderModeIfNeeded();
    }

    /**
     * Checks whether there is a #mPendingPushOperation and writes data to NFC tag. In case of
     * exception calls pendingPushOperationCompleted() with appropriate error object.
     */
    private void processPendingPushOperation() {
        if (mTagHandler == null || mPendingPushOperation == null) return;

        if (mTagHandler.isTagOutOfRange()) {
            mTagHandler = null;
            return;
        }

        try {
            mTagHandler.connect();
            mTagHandler.write(NfcTypeConverter.toNdefMessage(mPendingPushOperation.nfcMessage));
            pendingPushOperationCompleted(null);
        } catch (InvalidNfcMessageException e) {
            Log.w(TAG, "Cannot write data to NFC tag. Invalid NfcMessage.");
            pendingPushOperationCompleted(createError(NfcErrorType.INVALID_MESSAGE));
        } catch (TagLostException e) {
            Log.w(TAG, "Cannot write data to NFC tag. Tag is lost.");
            pendingPushOperationCompleted(createError(NfcErrorType.IO_ERROR));
        } catch (FormatException | IllegalStateException | IOException e) {
            Log.w(TAG, "Cannot write data to NFC tag. IO_ERROR.");
            pendingPushOperationCompleted(createError(NfcErrorType.IO_ERROR));
        }
    }

    /**
     * Reads NfcMessage from a tag and forwards message to matching method.
     */
    private void processPendingWatchOperations() {
        if (mTagHandler == null || mClient == null || mWatchers.size() == 0) return;

        // Skip reading if there is a pending push operation and ignoreRead flag is set.
        if (mPendingPushOperation != null && mPendingPushOperation.nfcPushOptions.ignoreRead) {
            return;
        }

        if (mTagHandler.isTagOutOfRange()) {
            mTagHandler = null;
            return;
        }

        NdefMessage message = null;

        try {
            mTagHandler.connect();
            message = mTagHandler.read();
            if (message.getByteArrayLength() > NfcMessage.MAX_SIZE) {
                Log.w(TAG, "Cannot read data from NFC tag. NfcMessage exceeds allowed size.");
                return;
            }
        } catch (TagLostException e) {
            Log.w(TAG, "Cannot read data from NFC tag. Tag is lost.");
        } catch (FormatException | IllegalStateException | IOException e) {
            Log.w(TAG, "Cannot read data from NFC tag. IO_ERROR.");
        }

        if (message != null) notifyMatchingWatchers(message);
    }

    /**
     * Iterates through active watchers and if any of those match NfcWatchOptions criteria,
     * delivers NfcMessage to the client.
     */
    private void notifyMatchingWatchers(NdefMessage message) {
        try {
            NfcMessage nfcMessage = NfcTypeConverter.toNfcMessage(message);
            List<Integer> watchIds = new ArrayList<Integer>();
            for (int i = 0; i < mWatchers.size(); i++) {
                NfcWatchOptions options = mWatchers.valueAt(i);
                if (matchesWatchOptions(nfcMessage, options)) watchIds.add(mWatchers.keyAt(i));
            }

            if (watchIds.size() != 0) {
                int[] ids = new int[watchIds.size()];
                for (int i = 0; i < watchIds.size(); ++i) {
                    ids[i] = watchIds.get(i).intValue();
                }
                mClient.onWatch(ids, nfcMessage);
            }
        } catch (UnsupportedEncodingException e) {
            Log.w(TAG, "Cannot convert NdefMessage to NfcMessage.");
        }
    }

    /**
     * Implements matching algorithm.
     */
    private boolean matchesWatchOptions(NfcMessage message, NfcWatchOptions options) {
        // Valid WebNFC message must have non-empty url.
        if (options.mode == NfcWatchMode.WEBNFC_ONLY
                && (message.url == null || message.url.isEmpty())) {
            return false;
        }

        // Filter by WebNfc watch Id.
        if (!matchesWebNfcId(message.url, options.url)) return false;

        // Matches any record / media type.
        if ((options.mediaType == null || options.mediaType.isEmpty())
                && options.recordFilter == null) {
            return true;
        }

        // Filter by mediaType and recordType
        for (int i = 0; i < message.data.length; i++) {
            boolean matchedMediaType;
            boolean matchedRecordType;

            if (options.mediaType == null || options.mediaType.isEmpty()) {
                // If media type for the watch options is empty, match all media types.
                matchedMediaType = true;
            } else {
                matchedMediaType = options.mediaType.equals(message.data[i].mediaType);
            }

            if (options.recordFilter == null) {
                // If record type filter for the watch options is null, match all record types.
                matchedRecordType = true;
            } else {
                matchedRecordType = options.recordFilter.recordType == message.data[i].recordType;
            }

            if (matchedMediaType && matchedRecordType) return true;
        }

        return false;
    }

    /**
     * WebNfc Id match algorithm.
     * https://w3c.github.io/web-nfc/#url-pattern-match-algorithm
     */
    private boolean matchesWebNfcId(String id, String pattern) {
        if (id != null && !id.isEmpty() && pattern != null && !pattern.isEmpty()) {
            try {
                URL id_url = new URL(id);
                URL pattern_url = new URL(pattern);

                if (!id_url.getProtocol().equals(pattern_url.getProtocol())) return false;
                if (!id_url.getHost().endsWith("." + pattern_url.getHost())
                        && !id_url.getHost().equals(pattern_url.getHost())) {
                    return false;
                }
                if (pattern_url.getPath().equals(ANY_PATH)) return true;
                if (id_url.getPath().startsWith(pattern_url.getPath())) return true;
                return false;

            } catch (MalformedURLException e) {
                return false;
            }
        }

        return true;
    }

    /**
     * Called by ReaderCallbackHandler when NFC tag is in proximity.
     */
    public void onTagDiscovered(Tag tag) {
        processPendingOperations(NfcTagHandler.create(tag));
    }

    /**
     * Processes pending operation when NFC tag is in proximity.
     */
    protected void processPendingOperations(NfcTagHandler tagHandler) {
        mTagHandler = tagHandler;
        processPendingWatchOperations();
        processPendingPushOperation();
        if (mTagHandler != null && mTagHandler.isConnected()) {
            try {
                mTagHandler.close();
            } catch (IOException e) {
                Log.w(TAG, "Cannot close NFC tag connection.");
            }
        }
    }

    /**
     * Schedules task that is executed after timeout and cancels pending push operation.
     */
    private void schedulePushTimeoutTask(NfcPushOptions options) {
        assert mPushTimeoutRunnable == null;
        // Default timeout value.
        if (Double.isInfinite(options.timeout)) return;

        // Create and schedule timeout.
        mPushTimeoutRunnable = new Runnable() {
            @Override
            public void run() {
                completePendingPushOperation(createError(NfcErrorType.TIMER_EXPIRED));
            }
        };

        mPushTimeoutHandler.postDelayed(mPushTimeoutRunnable, (long) options.timeout);
    }

    /**
     * Cancels push timeout task.
     */
    void cancelPushTimeoutTask() {
        if (mPushTimeoutRunnable == null) return;

        mPushTimeoutHandler.removeCallbacks(mPushTimeoutRunnable);
        mPushTimeoutRunnable = null;
    }
}
