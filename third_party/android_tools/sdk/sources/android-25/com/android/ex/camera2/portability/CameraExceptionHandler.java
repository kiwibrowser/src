/*
 * Copyright (C) 2012 The Android Open Source Project
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

/**
 * A handler for all camera api runtime exceptions.
 * The default behavior is to throw the runtime exception.
 */
public class CameraExceptionHandler {
    private Handler mHandler;

    private CameraExceptionCallback mCallback =
            new CameraExceptionCallback() {
                @Override
                public void onCameraError(int errorCode) {
                }
                @Override
                public void onCameraException(
                        RuntimeException e, String commandHistory, int action, int state) {
                    throw e;
                }
                @Override
                public void onDispatchThreadException(RuntimeException e) {
                    throw e;
                }
            };

    /**
     * A callback helps to handle RuntimeException thrown by camera framework.
     */
    public static interface CameraExceptionCallback {
        public void onCameraError(int errorCode);
        public void onCameraException(
                RuntimeException e, String commandHistory, int action, int state);
        public void onDispatchThreadException(RuntimeException e);
    }

    /**
     * Construct a new instance of {@link CameraExceptionHandler} with a custom callback which will
     * be executed on a specific {@link Handler}.
     *
     * @param callback The callback which will be invoked.
     * @param handler The handler in which the callback will be invoked in.
     */
    public CameraExceptionHandler(CameraExceptionCallback callback, Handler handler) {
        mHandler = handler;
        mCallback = callback;
    }

    /**
     * Construct a new instance of {@link CameraExceptionHandler} with a default callback which will
     * be executed on a specific {@link Handler}.
     *
     * @param handler The handler in which the default callback will be invoked in.
     */
    public CameraExceptionHandler(Handler handler) {
        mHandler = handler;
    }

    /**
     * Invoke @{link CameraExceptionCallback} when an error is reported by Android camera framework.
     *
     * @param errorCode An integer to represent the error code.
     * @see android.hardware.Camera#setErrorCallback(android.hardware.Camera.ErrorCallback)
     */
    public void onCameraError(final int errorCode) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mCallback.onCameraError(errorCode);
            }
        });
    }

    /**
     * Invoke @{link CameraExceptionCallback} when a runtime exception is thrown by Android camera
     * framework.
     *
     * @param ex The runtime exception object.
     */
    public void onCameraException(
            final RuntimeException ex, final String commandHistory,
            final int action, final int state) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mCallback.onCameraException(ex, commandHistory, action, state);
            }
        });
    }

    /**
     * Invoke @{link CameraExceptionCallback} when a runtime exception is thrown by
     * @{link DispatchThread}.
     *
     * @param ex The runtime exception object.
     */
    public void onDispatchThreadException(final RuntimeException ex) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mCallback.onDispatchThreadException(ex);
            }
        });
    }
}

