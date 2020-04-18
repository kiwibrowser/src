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

import android.graphics.Matrix;
import android.graphics.RectF;

import com.android.ex.camera2.portability.debug.Log;

/**
 * The device info for all attached cameras.
 */
public interface CameraDeviceInfo {

    static final int NO_DEVICE = -1;

    /**
     * @param cameraId Which device to interrogate.
     * @return The static characteristics of the specified device, or {@code null} on error.
     */
    Characteristics getCharacteristics(int cameraId);

    /**
     * @return The total number of the available camera devices.
     */
    int getNumberOfCameras();

    /**
     * @return The first (lowest) ID of the back cameras or {@code NO_DEVICE}
     *         if not available.
     */
    int getFirstBackCameraId();

    /**
     * @return The first (lowest) ID of the front cameras or {@code NO_DEVICE}
     *         if not available.
     */
    int getFirstFrontCameraId();

    /**
     * Device characteristics for a single camera.
     */
    public abstract class Characteristics {
        private static final Log.Tag TAG = new Log.Tag("CamDvcInfChar");

        /**
         * @return Whether the camera faces the back of the device.
         */
        public abstract boolean isFacingBack();

        /**
         * @return Whether the camera faces the device's screen.
         */
        public abstract boolean isFacingFront();

        /**
         * @return The camera sensor orientation, or the counterclockwise angle
         *          from its natural position that the device must be held at
         *          for the sensor to be right side up (in degrees, always a
         *          multiple of 90, and between 0 and 270, inclusive).
         */
        public abstract int getSensorOrientation();

        /**
         * @param currentDisplayOrientation
         *          The current display orientation, measured counterclockwise
         *          from to the device's natural orientation (in degrees, always
         *          a multiple of 90, and between 0 and 270, inclusive).
         * @return
         *          The relative preview image orientation, or the clockwise
         *          rotation angle that must be applied to display preview
         *          frames in the matching orientation, accounting for implicit
         *          mirroring, if applicable (in degrees, always a multiple of
         *          90, and between 0 and 270, inclusive).
         */
        public int getPreviewOrientation(int currentDisplayOrientation) {
            // Drivers tend to mirror the image during front camera preview.
            return getRelativeImageOrientation(currentDisplayOrientation, true);
        }

        /**
         * @param currentDisplayOrientation
         *          The current display orientation, measured counterclockwise
         *          from to the device's natural orientation (in degrees, always
         *          a multiple of 90, and between 0 and 270, inclusive).
         * @return
         *          The relative capture image orientation, or the clockwise
         *          rotation angle that must be applied to display these frames
         *          in the matching orientation (in degrees, always a multiple
         *          of 90, and between 0 and 270, inclusive).
         */
        public int getJpegOrientation(int currentDisplayOrientation) {
            // Don't mirror during capture!
            return getRelativeImageOrientation(currentDisplayOrientation, false);
        }

        /**
         * @param currentDisplayOrientaiton
         *          {@link #getPreviewOrientation}, {@link #getJpegOrientation}
         * @param compensateForMirroring
         *          Whether to account for mirroring in the case of front-facing
         *          cameras, which is necessary iff the OS/driver is
         *          automatically reflecting the image.
         * @return
         *          {@link #getPreviewOrientation}, {@link #getJpegOrientation}
         *
         * @see android.hardware.Camera.setDisplayOrientation
         */
        protected int getRelativeImageOrientation(int currentDisplayOrientation,
                                                  boolean compensateForMirroring) {
            if (!orientationIsValid(currentDisplayOrientation)) {
                return 0;
            }

            int result = 0;
            if (isFacingFront()) {
                result = (getSensorOrientation() + currentDisplayOrientation) % 360;
                if (compensateForMirroring) {
                    result = (360 - result) % 360;
                }
            } else if (isFacingBack()) {
                result = (getSensorOrientation() - currentDisplayOrientation + 360) % 360;
            } else {
                Log.e(TAG, "Camera is facing unhandled direction");
            }
            return result;
        }

        /**
         * @param currentDisplayOrientation
         *          The current display orientation, measured counterclockwise
         *          from to the device's natural orientation (in degrees, always
         *          a multiple of 90, and between 0 and 270, inclusive).
         * @param surfaceDimensions
         *          The dimensions of the {@link android.view.Surface} on which
         *          the preview image is being rendered. It usually only makes
         *          sense for the upper-left corner to be at the origin.
         * @return
         *          The transform matrix that should be applied to the
         *          {@link android.view.Surface} in order for the image to
         *          display properly in the device's current orientation.
         */
        public Matrix getPreviewTransform(int currentDisplayOrientation, RectF surfaceDimensions) {
            return getPreviewTransform(currentDisplayOrientation, surfaceDimensions,
                    new RectF(surfaceDimensions));
        }

        /**
         * @param currentDisplayOrientation
         *          The current display orientation, measured counterclockwise
         *          from to the device's natural orientation (in degrees, always
         *          a multiple of 90, and between 0 and 270, inclusive).
         * @param surfaceDimensions
         *          The dimensions of the {@link android.view.Surface} on which
         *          the preview image is being rendered. It usually only makes
         *          sense for the upper-left corner to be at the origin.
         * @param desiredBounds
         *          The boundaries within the {@link android.view.Surface} where
         *          the final image should appear. These can be used to
         *          translate and scale the output, but note that the image will
         *          be stretched to fit, possibly changing its aspect ratio.
         * @return
         *          The transform matrix that should be applied to the
         *          {@link android.view.Surface} in order for the image to
         *          display properly in the device's current orientation.
         */
        public Matrix getPreviewTransform(int currentDisplayOrientation, RectF surfaceDimensions,
                                          RectF desiredBounds) {
            if (!orientationIsValid(currentDisplayOrientation) ||
                    surfaceDimensions.equals(desiredBounds)) {
                return new Matrix();
            }

            Matrix transform = new Matrix();
            transform.setRectToRect(surfaceDimensions, desiredBounds, Matrix.ScaleToFit.FILL);
            return transform;
        }

        /**
         * @return Whether the shutter sound can be disabled.
         */
        public abstract boolean canDisableShutterSound();

        protected static boolean orientationIsValid(int angle) {
            if (angle % 90 != 0) {
                Log.e(TAG, "Provided display orientation is not divisible by 90");
                return false;
            }
            if (angle < 0 || angle > 270) {
                Log.e(TAG, "Provided display orientation is outside expected range");
                return false;
            }
            return true;
        }
    }
}
