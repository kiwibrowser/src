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

import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;

/**
 * Junction that allows notifying multiple {@link CaptureCallback}s whenever
 * the {@link CameraCaptureSession} posts a capture-related update.
 */
public class Camera2CaptureCallbackSplitter extends CaptureCallback {
    private final List<CaptureCallback> mRecipients = new LinkedList<>();

    /**
     * @param recipients The listeners to notify. Any {@code null} passed here
     *                   will be completely ignored.
     */
    public Camera2CaptureCallbackSplitter(CaptureCallback... recipients) {
        for (CaptureCallback listener : recipients) {
            if (listener != null) {
                mRecipients.add(listener);
            }
        }
    }

    @Override
    public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request,
                                   TotalCaptureResult result) {
        for (CaptureCallback target : mRecipients) {
            target.onCaptureCompleted(session, request, result);
        }
    }

    @Override
    public void onCaptureFailed(CameraCaptureSession session, CaptureRequest request,
                                CaptureFailure failure) {
        for (CaptureCallback target : mRecipients) {
            target.onCaptureFailed(session, request, failure);
        }
    }

    @Override
    public void onCaptureProgressed(CameraCaptureSession session, CaptureRequest request,
                                    CaptureResult partialResult) {
        for (CaptureCallback target : mRecipients) {
            target.onCaptureProgressed(session, request, partialResult);
        }
    }

    @Override
    public void onCaptureSequenceAborted(CameraCaptureSession session, int sequenceId) {
        for (CaptureCallback target : mRecipients) {
            target.onCaptureSequenceAborted(session, sequenceId);
        }
    }

    @Override
    public void onCaptureSequenceCompleted(CameraCaptureSession session, int sequenceId,
                                           long frameNumber) {
        for (CaptureCallback target : mRecipients) {
            target.onCaptureSequenceCompleted(session, sequenceId, frameNumber);
        }
    }

    @Override
    public void onCaptureStarted(CameraCaptureSession session, CaptureRequest request,
                                 long timestamp, long frameNumber) {
        for (CaptureCallback target : mRecipients) {
            target.onCaptureStarted(session, request, timestamp, frameNumber);
        }
    }
}
