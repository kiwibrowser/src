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

package com.android.ex.camera2.portability;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.SystemClock;

import com.android.ex.camera2.portability.debug.Log;

import java.util.LinkedList;
import java.util.Queue;

public class DispatchThread extends Thread {
    private static final Log.Tag TAG = new Log.Tag("DispatchThread");
    private static final long MAX_MESSAGE_QUEUE_LENGTH = 256;

    private final Queue<Runnable> mJobQueue;
    private Boolean mIsEnded;
    private Handler mCameraHandler;
    private HandlerThread mCameraHandlerThread;

    public DispatchThread(Handler cameraHandler, HandlerThread cameraHandlerThread) {
        super("Camera Job Dispatch Thread");
        mJobQueue = new LinkedList<Runnable>();
        mIsEnded = new Boolean(false);
        mCameraHandler = cameraHandler;
        mCameraHandlerThread = cameraHandlerThread;
    }

    /**
     * Queues up the job.
     *
     * @param job The job to run.
     */
    public void runJob(Runnable job) {
        if (isEnded()) {
            throw new IllegalStateException(
                    "Trying to run job on interrupted dispatcher thread");
        }
        synchronized (mJobQueue) {
            if (mJobQueue.size() == MAX_MESSAGE_QUEUE_LENGTH) {
                throw new RuntimeException("Camera master thread job queue full");
            }

            mJobQueue.add(job);
            mJobQueue.notifyAll();
        }
    }

    /**
     * Queues up the job and wait for it to be done.
     *
     * @param job The job to run.
     * @param timeoutMs Timeout limit in milliseconds.
     * @param jobMsg The message to log when the job runs timeout.
     * @return Whether the job finishes before timeout.
     */
    public void runJobSync(final Runnable job, Object waitLock, long timeoutMs, String jobMsg) {
        String timeoutMsg = "Timeout waiting " + timeoutMs + "ms for " + jobMsg;
        synchronized (waitLock) {
            long timeoutBound = SystemClock.uptimeMillis() + timeoutMs;
            try {
                runJob(job);
                waitLock.wait(timeoutMs);
                if (SystemClock.uptimeMillis() > timeoutBound) {
                    throw new IllegalStateException(timeoutMsg);
                }
            } catch (InterruptedException ex) {
                if (SystemClock.uptimeMillis() > timeoutBound) {
                    throw new IllegalStateException(timeoutMsg);
                }
            }
        }
    }

    /**
     * Gracefully ends this thread. Will stop after all jobs are processed.
     */
    public void end() {
        synchronized (mIsEnded) {
            mIsEnded = true;
        }
        synchronized(mJobQueue) {
            mJobQueue.notifyAll();
        }
    }

    private boolean isEnded() {
        synchronized (mIsEnded) {
            return mIsEnded;
        }
    }

    @Override
    public void run() {
        while(true) {
            Runnable job = null;
            synchronized (mJobQueue) {
                while (mJobQueue.size() == 0 && !isEnded()) {
                    try {
                        mJobQueue.wait();
                    } catch (InterruptedException ex) {
                        Log.w(TAG, "Dispatcher thread wait() interrupted, exiting");
                        break;
                    }
                }

                job = mJobQueue.poll();
            }

            if (job == null) {
                // mJobQueue.poll() returning null means wait() is
                // interrupted and the queue is empty.
                if (isEnded()) {
                    break;
                }
                continue;
            }

            job.run();

            synchronized (DispatchThread.this) {
                mCameraHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        synchronized (DispatchThread.this) {
                            DispatchThread.this.notifyAll();
                        }
                    }
                });
                try {
                    DispatchThread.this.wait();
                } catch (InterruptedException ex) {
                    // TODO: do something here.
                }
            }
        }
        mCameraHandlerThread.quitSafely();
    }
}
