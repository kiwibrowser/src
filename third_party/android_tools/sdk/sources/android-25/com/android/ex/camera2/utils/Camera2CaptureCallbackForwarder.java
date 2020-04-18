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

package com.android.ex.camera2.utils;

import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCaptureSession.CaptureCallback;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.os.Handler;

/**
 * Proxy that forwards all updates to another {@link CaptureCallback}, invoking
 * its callbacks on a separate {@link Handler}.
 */
public class Camera2CaptureCallbackForwarder extends CaptureCallback {
    private CaptureCallback mListener;
    private Handler mHandler;

    public Camera2CaptureCallbackForwarder(CaptureCallback listener, Handler handler) {
        mListener = listener;
        mHandler = handler;
    }

    @Override
    public void onCaptureCompleted(final CameraCaptureSession session, final CaptureRequest request,
                                   final TotalCaptureResult result) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mListener.onCaptureCompleted(session, request, result);
            }});
    }

    @Override
    public void onCaptureFailed(final CameraCaptureSession session, final CaptureRequest request,
                                final CaptureFailure failure) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mListener.onCaptureFailed(session, request, failure);
            }});
    }

    @Override
    public void onCaptureProgressed(final CameraCaptureSession session,
                                    final CaptureRequest request,
                                    final CaptureResult partialResult) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mListener.onCaptureProgressed(session, request, partialResult);
            }});
    }

    @Override
    public void onCaptureSequenceAborted(final CameraCaptureSession session, final int sequenceId) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mListener.onCaptureSequenceAborted(session, sequenceId);
            }});
    }

    @Override
    public void onCaptureSequenceCompleted(final CameraCaptureSession session, final int sequenceId,
                                           final long frameNumber) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mListener.onCaptureSequenceCompleted(session, sequenceId, frameNumber);
            }});
    }

    @Override
    public void onCaptureStarted(final CameraCaptureSession session, final CaptureRequest request,
                                 final long timestamp, final long frameNumber) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mListener.onCaptureStarted(session, request, timestamp, frameNumber);
            }});
    }
}
