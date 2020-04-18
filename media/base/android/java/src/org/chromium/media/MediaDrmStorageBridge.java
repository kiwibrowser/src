// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import android.annotation.TargetApi;
import android.os.Build;

import org.chromium.base.Callback;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;

/**
 * Origin isolated media drm scope id storage. Isolated origin is guranteed by native
 * implementation. Thus no origin information is stored here.
 */
@JNINamespace("media")
@MainDex
@TargetApi(Build.VERSION_CODES.M)
class MediaDrmStorageBridge {
    private static final long INVALID_NATIVE_MEDIA_DRM_STORAGE_BRIDGE = -1;

    private long mNativeMediaDrmStorageBridge;

    /**
     * Information that need to be persistent on the device. Exposed to JNI.
     */
    @MainDex
    static class PersistentInfo {
        // EME session ID, which is generated randomly.
        private final byte[] mEmeId;

        // Key set ID used to identify persistent license in MediaDrm.
        private final byte[] mKeySetId;

        // Mime type for the license.
        private final String mMimeType;

        @CalledByNative("PersistentInfo")
        private static PersistentInfo create(byte[] emeId, byte[] keySetId, String mime) {
            return new PersistentInfo(emeId, keySetId, mime);
        }

        PersistentInfo(byte[] emeId, byte[] keySetId, String mime) {
            mEmeId = emeId;
            mKeySetId = keySetId;
            mMimeType = mime;
        }

        @CalledByNative("PersistentInfo")
        byte[] emeId() {
            return mEmeId;
        }

        @CalledByNative("PersistentInfo")
        byte[] keySetId() {
            return mKeySetId;
        }

        @CalledByNative("PersistentInfo")
        String mimeType() {
            return mMimeType;
        }
    }

    MediaDrmStorageBridge(long nativeMediaDrmStorageBridge) {
        mNativeMediaDrmStorageBridge = nativeMediaDrmStorageBridge;
        assert isNativeMediaDrmStorageValid();
    }

    /**
     * Called when device provisioning is finished.
     */
    void onProvisioned(Callback<Boolean> cb) {
        if (isNativeMediaDrmStorageValid()) {
            nativeOnProvisioned(mNativeMediaDrmStorageBridge, cb);
        } else {
            cb.onResult(true);
        }
    }

    /**
     * Load |emeId|'s storage into memory.
     */
    void loadInfo(byte[] emeId, Callback<PersistentInfo> cb) {
        if (isNativeMediaDrmStorageValid()) {
            nativeOnLoadInfo(mNativeMediaDrmStorageBridge, emeId, cb);
        } else {
            cb.onResult(null);
        }
    }

    /**
     * Save persistent information. Override the existing value.
     */
    void saveInfo(PersistentInfo info, Callback<Boolean> cb) {
        if (isNativeMediaDrmStorageValid()) {
            nativeOnSaveInfo(mNativeMediaDrmStorageBridge, info, cb);
        } else {
            cb.onResult(false);
        }
    }

    /**
     * Remove persistent information related |emeId|.
     */
    void clearInfo(byte[] emeId, Callback<Boolean> cb) {
        if (isNativeMediaDrmStorageValid()) {
            nativeOnClearInfo(mNativeMediaDrmStorageBridge, emeId, cb);
        } else {
            cb.onResult(true);
        }
    }

    private boolean isNativeMediaDrmStorageValid() {
        return mNativeMediaDrmStorageBridge != INVALID_NATIVE_MEDIA_DRM_STORAGE_BRIDGE;
    }

    private native void nativeOnProvisioned(long nativeMediaDrmStorageBridge, Callback<Boolean> cb);
    private native void nativeOnLoadInfo(
            long nativeMediaDrmStorageBridge, byte[] sessionId, Callback<PersistentInfo> cb);
    private native void nativeOnSaveInfo(
            long nativeMediaDrmStorageBridge, PersistentInfo info, Callback<Boolean> cb);
    private native void nativeOnClearInfo(
            long nativeMediaDrmStorageBridge, byte[] sessionId, Callback<Boolean> cb);
}
