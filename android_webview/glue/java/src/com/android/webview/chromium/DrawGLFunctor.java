// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.android.webview.chromium;

import android.graphics.Canvas;
import android.os.Build;
import android.view.View;

import com.android.webview.chromium.WebViewDelegateFactory.WebViewDelegate;

import org.chromium.android_webview.AwContents;

/**
 * Simple Java abstraction and wrapper for the native DrawGLFunctor flow.
 * An instance of this class can be constructed, bound to a single view context (i.e. AwContennts)
 * and then drawn and detached from the view tree any number of times (using requestDrawGL and
 * detach respectively).
 */
class DrawGLFunctor implements AwContents.NativeDrawGLFunctor {
    private static final String TAG = DrawGLFunctor.class.getSimpleName();

    // Pointer to native side instance
    private final DestroyRunnable mDestroyRunnable;
    private final WebViewDelegate mWebViewDelegate;

    public DrawGLFunctor(long viewContext, WebViewDelegate webViewDelegate) {
        mDestroyRunnable = new DestroyRunnable(nativeCreateGLFunctor(viewContext));
        mWebViewDelegate = webViewDelegate;
    }

    @Override
    public void detach(View containerView) {
        if (mDestroyRunnable.mNativeDrawGLFunctor == 0) {
            throw new RuntimeException("detach on already destroyed DrawGLFunctor");
        }
        mWebViewDelegate.detachDrawGlFunctor(containerView, mDestroyRunnable.mNativeDrawGLFunctor);
    }

    private static final boolean sSupportFunctorReleasedCallback =
            (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N);

    @Override
    public boolean requestDrawGL(Canvas canvas, Runnable releasedCallback) {
        if (mDestroyRunnable.mNativeDrawGLFunctor == 0) {
            throw new RuntimeException("requestDrawGL on already destroyed DrawGLFunctor");
        }
        assert canvas != null;
        if (sSupportFunctorReleasedCallback) {
            assert releasedCallback != null;
            mWebViewDelegate.callDrawGlFunction(
                    canvas, mDestroyRunnable.mNativeDrawGLFunctor, releasedCallback);
        } else {
            assert releasedCallback == null;
            mWebViewDelegate.callDrawGlFunction(canvas, mDestroyRunnable.mNativeDrawGLFunctor);
        }
        return true;
    }

    @Override
    public boolean requestInvokeGL(View containerView, boolean waitForCompletion) {
        if (mDestroyRunnable.mNativeDrawGLFunctor == 0) {
            throw new RuntimeException("requestInvokeGL on already destroyed DrawGLFunctor");
        }
        if (!sSupportFunctorReleasedCallback
                && !mWebViewDelegate.canInvokeDrawGlFunctor(containerView)) {
            return false;
        }

        mWebViewDelegate.invokeDrawGlFunctor(
                containerView, mDestroyRunnable.mNativeDrawGLFunctor, waitForCompletion);
        return true;
    }

    @Override
    public boolean supportsDrawGLFunctorReleasedCallback() {
        return sSupportFunctorReleasedCallback;
    }

    @Override
    public Runnable getDestroyRunnable() {
        return mDestroyRunnable;
    }

    public static void setChromiumAwDrawGLFunction(long functionPointer) {
        nativeSetChromiumAwDrawGLFunction(functionPointer);
    }

    // Holds the core resources of the class, everything required to correctly cleanup.
    // IMPORTANT: this class must not hold any reference back to the outer DrawGLFunctor
    // instance, as that will defeat GC of that object.
    private static final class DestroyRunnable implements Runnable {
        private long mNativeDrawGLFunctor;
        DestroyRunnable(long nativeDrawGLFunctor) {
            mNativeDrawGLFunctor = nativeDrawGLFunctor;
            assert mNativeDrawGLFunctor != 0;
        }

        // Called when the outer DrawGLFunctor instance has been GC'ed, i.e this is its finalizer.
        @Override
        public void run() {
            assert mNativeDrawGLFunctor != 0;
            nativeDestroyGLFunctor(mNativeDrawGLFunctor);
            mNativeDrawGLFunctor = 0;
        }
    }

    private static native long nativeCreateGLFunctor(long viewContext);
    private static native void nativeDestroyGLFunctor(long functor);
    private static native void nativeSetChromiumAwDrawGLFunction(long functionPointer);
}
