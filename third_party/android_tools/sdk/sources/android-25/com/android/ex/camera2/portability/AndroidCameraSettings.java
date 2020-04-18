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

import android.hardware.Camera;
import com.android.ex.camera2.portability.debug.Log;

/**
 * The subclass of {@link CameraSettings} for Android Camera 1 API.
 */
public class AndroidCameraSettings extends CameraSettings {
    private static final Log.Tag TAG = new Log.Tag("AndCamSet");

    private static final String TRUE = "true";
    private static final String RECORDING_HINT = "recording-hint";

    public AndroidCameraSettings(CameraCapabilities capabilities, Camera.Parameters params) {
        if (params == null) {
            Log.w(TAG, "Settings ctor requires a non-null Camera.Parameters.");
            return;
        }

        CameraCapabilities.Stringifier stringifier = capabilities.getStringifier();

        setSizesLocked(false);

        // Preview
        Camera.Size paramPreviewSize = params.getPreviewSize();
        setPreviewSize(new Size(paramPreviewSize.width, paramPreviewSize.height));
        setPreviewFrameRate(params.getPreviewFrameRate());
        int[] previewFpsRange = new int[2];
        params.getPreviewFpsRange(previewFpsRange);
        setPreviewFpsRange(previewFpsRange[Camera.Parameters.PREVIEW_FPS_MIN_INDEX],
                previewFpsRange[Camera.Parameters.PREVIEW_FPS_MAX_INDEX]);
        setPreviewFormat(params.getPreviewFormat());

        // Capture: Focus, flash, zoom, exposure, scene mode.
        if (capabilities.supports(CameraCapabilities.Feature.ZOOM)) {
            setZoomRatio(params.getZoomRatios().get(params.getZoom()) / 100f);
        } else {
            setZoomRatio(CameraCapabilities.ZOOM_RATIO_UNZOOMED);
        }
        setExposureCompensationIndex(params.getExposureCompensation());
        setFlashMode(stringifier.flashModeFromString(params.getFlashMode()));
        setFocusMode(stringifier.focusModeFromString(params.getFocusMode()));
        setSceneMode(stringifier.sceneModeFromString(params.getSceneMode()));

        // Video capture.
        if (capabilities.supports(CameraCapabilities.Feature.VIDEO_STABILIZATION)) {
            setVideoStabilization(isVideoStabilizationEnabled());
        }
        setRecordingHintEnabled(TRUE.equals(params.get(RECORDING_HINT)));

        // Output: Photo size, compression quality
        setPhotoJpegCompressionQuality(params.getJpegQuality());
        Camera.Size paramPictureSize = params.getPictureSize();
        setPhotoSize(new Size(paramPictureSize.width, paramPictureSize.height));
        setPhotoFormat(params.getPictureFormat());
    }

    public AndroidCameraSettings(AndroidCameraSettings other) {
        super(other);
    }

    @Override
    public CameraSettings copy() {
        return new AndroidCameraSettings(this);
    }
}
