// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.androidoverlay;

import android.os.Handler;
import android.view.Surface;

import org.chromium.base.Log;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

/**
 * DialogOverlayCore::Host implementation that transfers messages to the thread on which it was
 * constructed.  Due to threading concerns, waitForClose is not forwarded.
 */
class ThreadHoppingHost implements DialogOverlayCore.Host {
    private static final String TAG = "ThreadHoppingHost";

    // Number of seconds we'll wait for client cleanup before we give up.  We don't want to keep
    // onSurfaceDestroyed waiting forever.
    private static final int CLEANUP_TIMEOUT_SECONDS = 2;

    // Handler for the host we're proxying to.  Typically Browser::UI.
    private Handler mHandler;

    // Host impl that we're proxying to.
    private final DialogOverlayCore.Host mHost;

    // Semaphore to keep track of whether cleanup has started yet.
    private final Semaphore mSemaphore = new Semaphore(0);

    // We will forward to |host| on whatever thread we're constructed on.
    public ThreadHoppingHost(DialogOverlayCore.Host host) {
        mHandler = new Handler();
        mHost = host;
    }

    @Override
    public void onSurfaceReady(final Surface surface) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mHost.onSurfaceReady(surface);
            }
        });
    }

    @Override
    public void onOverlayDestroyed() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mHost.onOverlayDestroyed();
            }
        });
    }

    // We don't forward via to |mHandler|, since it should be asynchronous.  Else, the |mHandler|
    // thread would block.  Instead, we wait here and somebody must call onCleanup() to let us know
    // that cleanup has started, and that we may return.
    @Override
    public void waitForClose() {
        while (true) {
            try {
                // TODO(liberato): in case of InterruptedException, we really should adjust the
                // timeout to reflect remaining time.
                if (!mSemaphore.tryAcquire(CLEANUP_TIMEOUT_SECONDS, TimeUnit.SECONDS))
                    Log.d(TAG, "Wait for semaphore timed out.");
                break;
            } catch (InterruptedException e) {
            }
        }
    }

    @Override
    public void enforceClose() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mHost.enforceClose();
            }
        });
    }

    // Notify us that cleanup has started.  This is called on |mHandler|'s thread.
    public void onClose() {
        mSemaphore.release(1);
    }
}
