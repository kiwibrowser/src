// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.graphics.Canvas;
import android.graphics.Picture;

import org.chromium.base.annotations.JNINamespace;

import java.io.OutputStream;

// A simple wrapper around a SkPicture, that allows final rendering to be performed using the
// chromium skia library.
@JNINamespace("android_webview")
class AwPicture extends Picture {

    private long mNativeAwPicture;

    // There is no explicit destroy method on Picture base-class, so cleanup is always
    // handled via the CleanupReference.
    private static final class DestroyRunnable implements Runnable {
        private long mNativeAwPicture;
        private DestroyRunnable(long nativeAwPicture) {
            mNativeAwPicture = nativeAwPicture;
        }
        @Override
        public void run() {
            nativeDestroy(mNativeAwPicture);
        }
    }

    private CleanupReference mCleanupReference;

    /**
     * @param nativeAwPicture is an instance of the AwPicture native class. Ownership is
     *                        taken by this java instance.
     */
    AwPicture(long nativeAwPicture) {
        mNativeAwPicture = nativeAwPicture;
        mCleanupReference = new CleanupReference(this, new DestroyRunnable(nativeAwPicture));
    }

    @Override
    public Canvas beginRecording(int width, int height) {
        unsupportedOperation();
        return null;
    }

    @Override
    public void endRecording() {
        // Intentional no-op. The native picture ended recording prior to java c'tor call.
    }

    @Override
    public int getWidth() {
        return nativeGetWidth(mNativeAwPicture);
    }

    @Override
    public int getHeight() {
        return nativeGetHeight(mNativeAwPicture);
    }

    @Override
    public void draw(Canvas canvas) {
        nativeDraw(mNativeAwPicture, canvas);
    }

    @Override
    @SuppressWarnings("deprecation")
    public void writeToStream(OutputStream stream) {
        unsupportedOperation();
    }

    private void unsupportedOperation() {
        throw new IllegalStateException("Unsupported in AwPicture");
    }

    private static native void nativeDestroy(long nativeAwPicture);
    private native int nativeGetWidth(long nativeAwPicture);
    private native int nativeGetHeight(long nativeAwPicture);
    private native void nativeDraw(long nativeAwPicture, Canvas canvas);
}

