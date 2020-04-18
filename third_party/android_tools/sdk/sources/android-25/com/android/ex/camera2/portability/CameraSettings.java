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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * A class which stores the camera settings.
 */
public abstract class CameraSettings {
    private static final Log.Tag TAG = new Log.Tag("CamSet");

    // Attempts to provide a value outside this range will be ignored.
    private static final int MIN_JPEG_COMPRESSION_QUALITY = 1;
    private static final int MAX_JPEG_COMPRESSION_QUALITY = 100;

    protected final Map<String, String> mGeneralSetting = new TreeMap<>();
    protected final List<Camera.Area> mMeteringAreas = new ArrayList<>();
    protected final List<Camera.Area> mFocusAreas = new ArrayList<>();
    protected boolean mSizesLocked;
    protected int mPreviewFpsRangeMin;
    protected int mPreviewFpsRangeMax;
    protected int mPreviewFrameRate;
    protected Size mCurrentPreviewSize;
    private int mCurrentPreviewFormat;
    protected Size mCurrentPhotoSize;
    protected byte mJpegCompressQuality;
    protected int mCurrentPhotoFormat;
    protected float mCurrentZoomRatio;
    protected int mExposureCompensationIndex;
    protected CameraCapabilities.FlashMode mCurrentFlashMode;
    protected CameraCapabilities.FocusMode mCurrentFocusMode;
    protected CameraCapabilities.SceneMode mCurrentSceneMode;
    protected CameraCapabilities.WhiteBalance mWhiteBalance;
    protected boolean mVideoStabilizationEnabled;
    protected boolean mAutoExposureLocked;
    protected boolean mAutoWhiteBalanceLocked;
    protected boolean mRecordingHintEnabled;
    protected GpsData mGpsData;
    protected Size mExifThumbnailSize;

    /**
     * An immutable class storing GPS related information.
     * <p>It's a hack since we always use GPS time stamp but does not use other
     * fields sometimes. Setting processing method to null means the other
     * fields should not be used.</p>
     */
    public static class GpsData {
        public final double latitude;
        public final double longitude;
        public final double altitude;
        public final long timeStamp;
        public final String processingMethod;

        /**
         * Construct what may or may not actually represent a location,
         * depending on the value of {@code processingMethod}.
         *
         * <p>Setting {@code processingMethod} to {@code null} means that
         * {@code latitude}, {@code longitude}, and {@code altitude} will be
         * completely ignored.</p>
         */
        public GpsData(double latitude, double longitude, double altitude, long timeStamp,
                String processingMethod) {
            if (processingMethod == null &&
                    (latitude != 0.0 || longitude != 0.0 || altitude != 0.0)) {
                Log.w(TAG, "GpsData's nonzero data will be ignored due to null processingMethod");
            }
            this.latitude = latitude;
            this.longitude = longitude;
            this.altitude = altitude;
            this.timeStamp = timeStamp;
            this.processingMethod = processingMethod;
        }

        /** Copy constructor. */
        public GpsData(GpsData src) {
            this.latitude = src.latitude;
            this.longitude = src.longitude;
            this.altitude = src.altitude;
            this.timeStamp = src.timeStamp;
            this.processingMethod = src.processingMethod;
        }
    }

    protected CameraSettings() {
    }

    /**
     * Copy constructor.
     *
     * @param src The source settings.
     * @return The copy of the source.
     */
    protected CameraSettings(CameraSettings src) {
        mGeneralSetting.putAll(src.mGeneralSetting);
        mMeteringAreas.addAll(src.mMeteringAreas);
        mFocusAreas.addAll(src.mFocusAreas);
        mSizesLocked = src.mSizesLocked;
        mPreviewFpsRangeMin = src.mPreviewFpsRangeMin;
        mPreviewFpsRangeMax = src.mPreviewFpsRangeMax;
        mPreviewFrameRate = src.mPreviewFrameRate;
        mCurrentPreviewSize =
                (src.mCurrentPreviewSize == null ? null : new Size(src.mCurrentPreviewSize));
        mCurrentPreviewFormat = src.mCurrentPreviewFormat;
        mCurrentPhotoSize =
                (src.mCurrentPhotoSize == null ? null : new Size(src.mCurrentPhotoSize));
        mJpegCompressQuality = src.mJpegCompressQuality;
        mCurrentPhotoFormat = src.mCurrentPhotoFormat;
        mCurrentZoomRatio = src.mCurrentZoomRatio;
        mExposureCompensationIndex = src.mExposureCompensationIndex;
        mCurrentFlashMode = src.mCurrentFlashMode;
        mCurrentFocusMode = src.mCurrentFocusMode;
        mCurrentSceneMode = src.mCurrentSceneMode;
        mWhiteBalance = src.mWhiteBalance;
        mVideoStabilizationEnabled = src.mVideoStabilizationEnabled;
        mAutoExposureLocked = src.mAutoExposureLocked;
        mAutoWhiteBalanceLocked = src.mAutoWhiteBalanceLocked;
        mRecordingHintEnabled = src.mRecordingHintEnabled;
        mGpsData = src.mGpsData;
        mExifThumbnailSize = src.mExifThumbnailSize;
    }

    /**
     * @return A copy of this object, as an instance of the implementing class.
     */
    public abstract CameraSettings copy();

    /** General setting **/
    @Deprecated
    public void setSetting(String key, String value) {
        mGeneralSetting.put(key, value);
    }

    /**
     * Changes whether classes outside this class are allowed to set the preview
     * and photo capture sizes.
     *
     * @param locked Whether to prevent changes to these fields.
     *
     * @see #setPhotoSize
     * @see #setPreviewSize
     */
    /*package*/ void setSizesLocked(boolean locked) {
        mSizesLocked = locked;
    }

    /**  Preview **/

    /**
     * Sets the preview FPS range. This call will invalidate prior calls to
     * {@link #setPreviewFrameRate(int)}.
     *
     * @param min The min FPS.
     * @param max The max FPS.
     */
    public void setPreviewFpsRange(int min, int max) {
        if (min > max) {
            int temp = max;
            max = min;
            min = temp;
        }
        mPreviewFpsRangeMax = max;
        mPreviewFpsRangeMin = min;
        mPreviewFrameRate = -1;
    }

    /**
     * @return The min of the preview FPS range.
     */
    public int getPreviewFpsRangeMin() {
        return mPreviewFpsRangeMin;
    }

    /**
     * @return The max of the preview FPS range.
     */
    public int getPreviewFpsRangeMax() {
        return mPreviewFpsRangeMax;
    }

    /**
     * Sets the preview FPS. This call will invalidate prior calls to
     * {@link #setPreviewFpsRange(int, int)}.
     *
     * @param frameRate The target frame rate.
     */
    public void setPreviewFrameRate(int frameRate) {
        if (frameRate > 0) {
            mPreviewFrameRate = frameRate;
            mPreviewFpsRangeMax = frameRate;
            mPreviewFpsRangeMin = frameRate;
        }
    }

    public int getPreviewFrameRate() {
        return mPreviewFrameRate;
    }

    /**
     * @return The current preview size.
     */
    public Size getCurrentPreviewSize() {
        return new Size(mCurrentPreviewSize);
    }

    /**
     * @param previewSize The size to use for preview.
     * @return Whether the operation was allowed (i.e. the sizes are unlocked).
     */
    public boolean setPreviewSize(Size previewSize) {
        if (mSizesLocked) {
            Log.w(TAG, "Attempt to change preview size while locked");
            return false;
        }

        mCurrentPreviewSize = new Size(previewSize);
        return true;
    }

    /**
     * Sets the preview format.
     *
     * @param format
     * @see {@link android.graphics.ImageFormat}.
     */
    public void setPreviewFormat(int format) {
        mCurrentPreviewFormat = format;
    }

    /**
     * @return The preview format.
     * @see {@link android.graphics.ImageFormat}.
     */
    public int getCurrentPreviewFormat() {
        return mCurrentPreviewFormat;
    }

    /** Picture **/

    /**
     * @return The current photo size.
     */
    public Size getCurrentPhotoSize() {
        return new Size(mCurrentPhotoSize);
    }

    /**
     * @param photoSize The size to use for preview.
     * @return Whether the operation was allowed (i.e. the sizes are unlocked).
     */
    public boolean setPhotoSize(Size photoSize) {
        if (mSizesLocked) {
            Log.w(TAG, "Attempt to change photo size while locked");
            return false;
        }

        mCurrentPhotoSize = new Size(photoSize);
        return true;
    }

    /**
     * Sets the format for the photo.
     *
     * @param format The format for the photos taken.
     * @see {@link android.graphics.ImageFormat}.
     */
    public void setPhotoFormat(int format) {
        mCurrentPhotoFormat = format;
    }

    /**
     * @return The format for the photos taken.
     * @see {@link android.graphics.ImageFormat}.
     */
    public int getCurrentPhotoFormat() {
        return mCurrentPhotoFormat;
    }

    /**
     * Sets the JPEG compression quality.
     *
     * @param quality The quality for JPEG.
     */
    public void setPhotoJpegCompressionQuality(int quality) {
        if (quality < MIN_JPEG_COMPRESSION_QUALITY || quality > MAX_JPEG_COMPRESSION_QUALITY) {
            Log.w(TAG, "Ignoring JPEG quality that falls outside the expected range");
            return;
        }
        // This is safe because the positive numbers go up to 127.
        mJpegCompressQuality = (byte) quality;
    }

    public int getPhotoJpegCompressionQuality() {
        return mJpegCompressQuality;
    }

    /** Zoom **/

    /**
     * @return The current zoom ratio. The min is 1.0f.
     */
    public float getCurrentZoomRatio() {
        return mCurrentZoomRatio;
    }

    /**
     * Sets the zoom ratio.
     * @param ratio The new zoom ratio. Should be in the range between 1.0 to
     *              the value returned from {@link
     *              com.android.camera.cameradevice.CameraCapabilities#getMaxZoomRatio()}.
     * @throws java.lang.UnsupportedOperationException if the ratio is not
     *         supported.
     */
    public void setZoomRatio(float ratio) {
        mCurrentZoomRatio = ratio;
    }

    /** Exposure **/

    public void setExposureCompensationIndex(int index) {
        mExposureCompensationIndex = index;
    }

    /**
     * @return The exposure compensation, with 0 meaning unadjusted.
     */
    public int getExposureCompensationIndex() {
        return mExposureCompensationIndex;
    }

    public void setAutoExposureLock(boolean locked) {
        mAutoExposureLocked = locked;
    }

    public boolean isAutoExposureLocked() {
        return mAutoExposureLocked;
    }

    /**
     * @param areas The areas for autoexposure. The coordinate system has domain
     *              and range [-1000,1000], measured relative to the visible
     *              preview image, with orientation matching that of the sensor.
     *              This means the coordinates must be transformed to account
     *              for the devices rotation---but not the zoom level---before
     *              being passed into this method.
     */
    public void setMeteringAreas(List<Camera.Area> areas) {
        mMeteringAreas.clear();
        if (areas != null) {
            mMeteringAreas.addAll(areas);
        }
    }

    public List<Camera.Area> getMeteringAreas() {
        return new ArrayList<Camera.Area>(mMeteringAreas);
    }

    /** Flash **/

    public CameraCapabilities.FlashMode getCurrentFlashMode() {
        return mCurrentFlashMode;
    }

    public void setFlashMode(CameraCapabilities.FlashMode flashMode) {
        mCurrentFlashMode = flashMode;
    }

    /** Focus **/

    /**
     * Sets the focus mode.
     * @param focusMode The focus mode to use.
     */
    public void setFocusMode(CameraCapabilities.FocusMode focusMode) {
        mCurrentFocusMode = focusMode;
    }

    /**
     * @return The current focus mode.
     */
    public CameraCapabilities.FocusMode getCurrentFocusMode() {
        return mCurrentFocusMode;
    }

    /**
     * @param areas The areas to focus. The coordinate system has domain and
     *              range [-1000,1000], measured relative to the visible preview
     *              image, with orientation matching that of the sensor. This
     *              means the coordinates must be transformed to account for
     *              the devices rotation---but not the zoom level---before being
     *              passed into this method.
     */
    public void setFocusAreas(List<Camera.Area> areas) {
        mFocusAreas.clear();
        if (areas != null) {
            mFocusAreas.addAll(areas);
        }
    }

    public List<Camera.Area> getFocusAreas() {
        return new ArrayList<Camera.Area>(mFocusAreas);
    }

    /** White balance **/

    public void setWhiteBalance(CameraCapabilities.WhiteBalance whiteBalance) {
        mWhiteBalance = whiteBalance;
    }

    public CameraCapabilities.WhiteBalance getWhiteBalance() {
        return mWhiteBalance;
    }

    public void setAutoWhiteBalanceLock(boolean locked) {
        mAutoWhiteBalanceLocked = locked;
    }

    public boolean isAutoWhiteBalanceLocked() {
        return mAutoWhiteBalanceLocked;
    }

    /** Scene mode **/

    /**
     * @return The current scene mode.
     */
    public CameraCapabilities.SceneMode getCurrentSceneMode() {
        return mCurrentSceneMode;
    }

    /**
     * Sets the scene mode for capturing.
     *
     * @param sceneMode The scene mode to use.
     * @throws java.lang.UnsupportedOperationException if it's not supported.
     */
    public void setSceneMode(CameraCapabilities.SceneMode sceneMode) {
        mCurrentSceneMode = sceneMode;
    }

    /** Other Features **/

    public void setVideoStabilization(boolean enabled) {
        mVideoStabilizationEnabled = enabled;
    }

    public boolean isVideoStabilizationEnabled() {
        return mVideoStabilizationEnabled;
    }

    public void setRecordingHintEnabled(boolean hintEnabled) {
        mRecordingHintEnabled = hintEnabled;
    }

    public boolean isRecordingHintEnabled() {
        return mRecordingHintEnabled;
    }

    public void setGpsData(GpsData data) {
        mGpsData = new GpsData(data);
    }

    public GpsData getGpsData() {
        return (mGpsData == null ? null : new GpsData(mGpsData));
    }

    public void clearGpsData() {
        mGpsData = null;
    }

    /**
     * Sets the size of the thumbnail in EXIF header. To suppress thumbnail
     * generation, set a size of (0,0).
     *
     * @param s The size for the thumbnail. If {@code null}, agent will not
     *          set a thumbnail size.
     */
    public void setExifThumbnailSize(Size s) {
        mExifThumbnailSize = s;
    }

    /**
     * Gets the size of the thumbnail in EXIF header.
     *
     * @return desired thumbnail size, or null if no size was set
     */
    public Size getExifThumbnailSize() {
        return (mExifThumbnailSize == null) ? null : new Size(mExifThumbnailSize);
    }
}
