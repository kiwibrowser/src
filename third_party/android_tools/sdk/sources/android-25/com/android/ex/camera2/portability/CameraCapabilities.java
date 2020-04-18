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

import com.android.ex.camera2.portability.debug.Log;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.TreeSet;

/**
 * This class holds all the static information of a camera's capabilities.
 * <p>
 * The design of this class is thread-safe and can be passed around regardless
 * of which thread using it.
 * </p>
 */
public class CameraCapabilities {

    private static Log.Tag TAG = new Log.Tag("CamCapabs");

    /** Zoom ratio used for seeing sensor's full field of view. */
    protected static final float ZOOM_RATIO_UNZOOMED = 1.0f;

    /* All internal states are declared final and should be thread-safe. */

    protected final ArrayList<int[]> mSupportedPreviewFpsRange = new ArrayList<int[]>();
    protected final ArrayList<Size> mSupportedPreviewSizes = new ArrayList<Size>();
    protected final TreeSet<Integer> mSupportedPreviewFormats = new TreeSet<Integer>();
    protected final ArrayList<Size> mSupportedVideoSizes = new ArrayList<Size>();
    protected final ArrayList<Size> mSupportedPhotoSizes = new ArrayList<Size>();
    protected final TreeSet<Integer> mSupportedPhotoFormats = new TreeSet<Integer>();
    protected final EnumSet<SceneMode> mSupportedSceneModes = EnumSet.noneOf(SceneMode.class);
    protected final EnumSet<FlashMode> mSupportedFlashModes = EnumSet.noneOf(FlashMode.class);
    protected final EnumSet<FocusMode> mSupportedFocusModes = EnumSet.noneOf(FocusMode.class);
    protected final EnumSet<WhiteBalance> mSupportedWhiteBalances =
            EnumSet.noneOf(WhiteBalance.class);
    protected final EnumSet<Feature> mSupportedFeatures = EnumSet.noneOf(Feature.class);
    protected Size mPreferredPreviewSizeForVideo;
    protected int mMinExposureCompensation;
    protected int mMaxExposureCompensation;
    protected float mExposureCompensationStep;
    protected int mMaxNumOfFacesSupported;
    protected int mMaxNumOfFocusAreas;
    protected int mMaxNumOfMeteringArea;
    protected float mMaxZoomRatio;
    protected float mHorizontalViewAngle;
    protected float mVerticalViewAngle;
    private final Stringifier mStringifier;

    /**
     * Focus modes.
     */
    public enum FocusMode {
        /**
         * Continuous auto focus mode intended for taking pictures.
         * @see {@link android.hardware.Camera.Parameters#FOCUS_MODE_AUTO}.
         */
        AUTO,
        /**
         * Continuous auto focus mode intended for taking pictures.
         * @see {@link android.hardware.Camera.Parameters#FOCUS_MODE_CONTINUOUS_PICTURE}.
         */
        CONTINUOUS_PICTURE,
        /**
         * Continuous auto focus mode intended for video recording.
         * @see {@link android.hardware.Camera.Parameters#FOCUS_MODE_CONTINUOUS_VIDEO}.
         */
        CONTINUOUS_VIDEO,
        /**
         * Extended depth of field (EDOF).
         * @see {@link android.hardware.Camera.Parameters#FOCUS_MODE_EDOF}.
         */
        EXTENDED_DOF,
        /**
         * Focus is fixed.
         * @see {@link android.hardware.Camera.Parameters#FOCUS_MODE_FIXED}.
         */
        FIXED,
        /**
         * Focus is set at infinity.
         * @see {@link android.hardware.Camera.Parameters#FOCUS_MODE_INFINITY}.
         */
        // TODO: Unsupported on API 2
        INFINITY,
        /**
         * Macro (close-up) focus mode.
         * @see {@link android.hardware.Camera.Parameters#FOCUS_MODE_MACRO}.
         */
        MACRO,
    }

    /**
     * Flash modes.
     */
    public enum FlashMode {
        /**
         * No flash.
         */
        NO_FLASH,
        /**
         * Flash will be fired automatically when required.
         * @see {@link android.hardware.Camera.Parameters#FLASH_MODE_OFF}.
         */
        AUTO,
        /**
         * Flash will not be fired.
         * @see {@link android.hardware.Camera.Parameters#FLASH_MODE_OFF}.
         */
        OFF,
        /**
         * Flash will always be fired during snapshot.
         * @see {@link android.hardware.Camera.Parameters#FLASH_MODE_ON}.
         */
        ON,
        /**
         * Constant emission of light during preview, auto-focus and snapshot.
         * @see {@link android.hardware.Camera.Parameters#FLASH_MODE_TORCH}.
         */
        TORCH,
        /**
         * Flash will be fired in red-eye reduction mode.
         * @see {@link android.hardware.Camera.Parameters#FLASH_MODE_RED_EYE}.
         */
        RED_EYE,
    }

    /**
     * Scene modes.
     */
    public enum SceneMode {
        /**
         * No supported scene mode.
         */
        NO_SCENE_MODE,
        /**
         * Scene mode is off.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_AUTO}.
         */
        AUTO,
        /**
         * Take photos of fast moving objects.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_ACTION}.
         */
        ACTION,
        /**
         * Applications are looking for a barcode.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_BARCODE}.
         */
        BARCODE,
        /**
         * Take pictures on the beach.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_BEACH}.
         */
        BEACH,
        /**
         * Capture the naturally warm color of scenes lit by candles.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_CANDLELIGHT}.
         */
        CANDLELIGHT,
        /**
         * For shooting firework displays.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_FIREWORKS}.
         */
        FIREWORKS,
        /**
         * Capture a scene using high dynamic range imaging techniques.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_HDR}.
         */
        // Note: Supported as a vendor tag on the Camera2 API for some LEGACY devices.
        HDR,
        /**
         * Take pictures on distant objects.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_LANDSCAPE}.
         */
        LANDSCAPE,
        /**
         * Take photos at night.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_NIGHT}.
         */
        NIGHT,
        /**
         * Take people pictures at night.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_NIGHT_PORTRAIT}.
         */
        // TODO: Unsupported on API 2
        NIGHT_PORTRAIT,
        /**
         * Take indoor low-light shot.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_PARTY}.
         */
        PARTY,
        /**
         * Take people pictures.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_PORTRAIT}.
         */
        PORTRAIT,
        /**
         * Take pictures on the snow.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_SNOW}.
         */
        SNOW,
        /**
         * Take photos of fast moving objects.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_SPORTS}.
         */
        SPORTS,
        /**
         * Avoid blurry pictures (for example, due to hand shake).
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_STEADYPHOTO}.
         */
        STEADYPHOTO,
        /**
         * Take sunset photos.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_SUNSET}.
         */
        SUNSET,
        /**
         * Take photos in a theater.
         * @see {@link android.hardware.Camera.Parameters#SCENE_MODE_THEATRE}.
         */
        THEATRE,
    }

    /**
     * White blances.
     */
    public enum WhiteBalance {
        /**
         * @see {@link android.hardware.Camera.Parameters#WHITE_BALANCE_AUTO}.
         */
        AUTO,
        /**
         * @see {@link android.hardware.Camera.Parameters#WHITE_BALANCE_CLOUDY_DAYLIGHT}.
         */
        CLOUDY_DAYLIGHT,
        /**
         * @see {@link android.hardware.Camera.Parameters#WHITE_BALANCE_DAYLIGHT}.
         */
        DAYLIGHT,
        /**
         * @see {@link android.hardware.Camera.Parameters#WHITE_BALANCE_FLUORESCENT}.
         */
        FLUORESCENT,
        /**
         * @see {@link android.hardware.Camera.Parameters#WHITE_BALANCE_INCANDESCENT}.
         */
        INCANDESCENT,
        /**
         * @see {@link android.hardware.Camera.Parameters#WHITE_BALANCE_SHADE}.
         */
        SHADE,
        /**
         * @see {@link android.hardware.Camera.Parameters#WHITE_BALANCE_TWILIGHT}.
         */
        TWILIGHT,
        /**
         * @see {@link android.hardware.Camera.Parameters#WHITE_BALANCE_WARM_FLUORESCENT}.
         */
        WARM_FLUORESCENT,
    }

    /**
     * Features.
     */
    public enum Feature {
        /**
         * Support zoom-related methods.
         */
        ZOOM,
        /**
         * Support for photo capturing during video recording.
         */
        VIDEO_SNAPSHOT,
        /**
         * Support for focus area settings.
         */
        FOCUS_AREA,
        /**
         * Support for metering area settings.
         */
        METERING_AREA,
        /**
         * Support for automatic exposure lock.
         */
        AUTO_EXPOSURE_LOCK,
        /**
         * Support for automatic white balance lock.
         */
        AUTO_WHITE_BALANCE_LOCK,
        /**
         * Support for video stabilization.
         */
        VIDEO_STABILIZATION,
    }

    /**
     * A interface stringifier to convert abstract representations to API
     * related string representation.
     */
    public static class Stringifier {
        /**
         * Converts the string to hyphen-delimited lowercase for compatibility with multiple APIs.
         *
         * @param enumCase The name of an enum constant.
         * @return The converted string.
         */
        private static String toApiCase(String enumCase) {
            return enumCase.toLowerCase(Locale.US).replaceAll("_", "-");
        }

        /**
         * Converts the string to underscore-delimited uppercase to match the enum constant names.
         *
         * @param apiCase An API-related string representation.
         * @return The converted string.
         */
        private static String toEnumCase(String apiCase) {
            return apiCase.toUpperCase(Locale.US).replaceAll("-", "_");
        }

        /**
         * Converts the focus mode to API-related string representation.
         *
         * @param focus The focus mode to convert.
         * @return The string used by the camera framework API to represent the
         *         focus mode.
         */
        public String stringify(FocusMode focus) {
            return toApiCase(focus.name());
        }

        /**
         * Converts the API-related string representation of the focus mode to the
         * abstract representation.
         *
         * @param val The string representation.
         * @return The focus mode represented by the input string, or the focus
         *         mode with the lowest ordinal if it cannot be converted.
         */
        public FocusMode focusModeFromString(String val) {
            if (val == null) {
                return FocusMode.values()[0];
            }
            try {
                return FocusMode.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return FocusMode.values()[0];
            }
        }

        /**
         * Converts the flash mode to API-related string representation.
         *
         * @param flash The focus mode to convert.
         * @return The string used by the camera framework API to represent the
         *         flash mode.
         */
        public String stringify(FlashMode flash) {
            return toApiCase(flash.name());
        }

        /**
         * Converts the API-related string representation of the flash mode to the
         * abstract representation.
         *
         * @param val The string representation.
         * @return The flash mode represented by the input string, or the flash
         *         mode with the lowest ordinal if it cannot be converted.
         */
        public FlashMode flashModeFromString(String val) {
            if (val == null) {
                return FlashMode.values()[0];
            }
            try {
                return FlashMode.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return FlashMode.values()[0];
            }
        }

        /**
         * Converts the scene mode to API-related string representation.
         *
         * @param scene The focus mode to convert.
         * @return The string used by the camera framework API to represent the
         *         scene mode.
         */
        public String stringify(SceneMode scene) {
            return toApiCase(scene.name());
        }

        /**
         * Converts the API-related string representation of the scene mode to the
         * abstract representation.
         *
         * @param val The string representation.
         * @return The scene mode represented by the input string, or the scene
         *         mode with the lowest ordinal if it cannot be converted.
         */
        public SceneMode sceneModeFromString(String val) {
            if (val == null) {
                return SceneMode.values()[0];
            }
            try {
                return SceneMode.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return SceneMode.values()[0];
            }
        }

        /**
         * Converts the white balance to API-related string representation.
         *
         * @param wb The focus mode to convert.
         * @return The string used by the camera framework API to represent the
         * white balance.
         */
        public String stringify(WhiteBalance wb) {
            return toApiCase(wb.name());
        }

        /**
         * Converts the API-related string representation of the white balance to
         * the abstract representation.
         *
         * @param val The string representation.
         * @return The white balance represented by the input string, or the
         *         white balance with the lowest ordinal if it cannot be
         *         converted.
         */
        public WhiteBalance whiteBalanceFromString(String val) {
            if (val == null) {
                return WhiteBalance.values()[0];
            }
            try {
                return WhiteBalance.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return WhiteBalance.values()[0];
            }
        }
    }

    /**
     * Constructor.
     * @param stringifier The API-specific stringifier for this instance.
     */
    CameraCapabilities(Stringifier stringifier) {
        mStringifier = stringifier;
    }

    /**
     * Copy constructor.
     * @param src The source instance.
     */
    public CameraCapabilities(CameraCapabilities src) {
        mSupportedPreviewFpsRange.addAll(src.mSupportedPreviewFpsRange);
        mSupportedPreviewSizes.addAll(src.mSupportedPreviewSizes);
        mSupportedPreviewFormats.addAll(src.mSupportedPreviewFormats);
        mSupportedVideoSizes.addAll(src.mSupportedVideoSizes);
        mSupportedPhotoSizes.addAll(src.mSupportedPhotoSizes);
        mSupportedPhotoFormats.addAll(src.mSupportedPhotoFormats);
        mSupportedSceneModes.addAll(src.mSupportedSceneModes);
        mSupportedFlashModes.addAll(src.mSupportedFlashModes);
        mSupportedFocusModes.addAll(src.mSupportedFocusModes);
        mSupportedWhiteBalances.addAll(src.mSupportedWhiteBalances);
        mSupportedFeatures.addAll(src.mSupportedFeatures);
        mPreferredPreviewSizeForVideo = src.mPreferredPreviewSizeForVideo;
        mMaxExposureCompensation = src.mMaxExposureCompensation;
        mMinExposureCompensation = src.mMinExposureCompensation;
        mExposureCompensationStep = src.mExposureCompensationStep;
        mMaxNumOfFacesSupported = src.mMaxNumOfFacesSupported;
        mMaxNumOfFocusAreas = src.mMaxNumOfFocusAreas;
        mMaxNumOfMeteringArea = src.mMaxNumOfMeteringArea;
        mMaxZoomRatio = src.mMaxZoomRatio;
        mHorizontalViewAngle = src.mHorizontalViewAngle;
        mVerticalViewAngle = src.mVerticalViewAngle;
        mStringifier = src.mStringifier;
    }

    public float getHorizontalViewAngle() {
        return mHorizontalViewAngle;
    }

    public float getVerticalViewAngle() {
        return mVerticalViewAngle;
    }

    /**
     * @return the supported picture formats. See {@link android.graphics.ImageFormat}.
     */
    public Set<Integer> getSupportedPhotoFormats() {
        return new TreeSet<Integer>(mSupportedPhotoFormats);
    }

    /**
     * Gets the supported preview formats.
     * @return The supported preview {@link android.graphics.ImageFormat}s.
     */
    public Set<Integer> getSupportedPreviewFormats() {
        return new TreeSet<Integer>(mSupportedPreviewFormats);
    }

    /**
     * Gets the supported picture sizes.
     */
    public List<Size> getSupportedPhotoSizes() {
        return new ArrayList<Size>(mSupportedPhotoSizes);
    }

    /**
     * @return The supported preview fps (frame-per-second) ranges. The returned
     * list is sorted by maximum fps then minimum fps in a descending order.
     * The values are multiplied by 1000.
     */
    public final List<int[]> getSupportedPreviewFpsRange() {
        return new ArrayList<int[]>(mSupportedPreviewFpsRange);
    }

    /**
     * @return The supported preview sizes. The list is sorted by width then
     * height in a descending order.
     */
    public final List<Size> getSupportedPreviewSizes() {
        return new ArrayList<Size>(mSupportedPreviewSizes);
    }

    public final Size getPreferredPreviewSizeForVideo() {
        return new Size(mPreferredPreviewSizeForVideo);
    }

    /**
     * @return The supported video frame sizes that can be used by MediaRecorder.
     *         The list is sorted by width then height in a descending order.
     */
    public final List<Size> getSupportedVideoSizes() {
        return new ArrayList<Size>(mSupportedVideoSizes);
    }

    /**
     * @return The supported scene modes.
     */
    public final Set<SceneMode> getSupportedSceneModes() {
        return new HashSet<SceneMode>(mSupportedSceneModes);
    }

    /**
     * @return Whether the scene mode is supported.
     */
    public final boolean supports(SceneMode scene) {
        return (scene != null && mSupportedSceneModes.contains(scene));
    }

    public boolean supports(final CameraSettings settings) {
        if (zoomCheck(settings) && exposureCheck(settings) && focusCheck(settings) &&
                flashCheck(settings) && photoSizeCheck(settings) && previewSizeCheck(settings) &&
                videoStabilizationCheck(settings)) {
            return true;
        }
        return false;
    }

    /**
     * @return The supported flash modes.
     */
    public final Set<FlashMode> getSupportedFlashModes() {
        return new HashSet<FlashMode>(mSupportedFlashModes);
    }

    /**
     * @return Whether the flash mode is supported.
     */
    public final boolean supports(FlashMode flash) {
        return (flash != null && mSupportedFlashModes.contains(flash));
    }

    /**
     * @return The supported focus modes.
     */
    public final Set<FocusMode> getSupportedFocusModes() {
        return new HashSet<FocusMode>(mSupportedFocusModes);
    }

    /**
     * @return Whether the focus mode is supported.
     */
    public final boolean supports(FocusMode focus) {
        return (focus != null && mSupportedFocusModes.contains(focus));
    }

    /**
     * @return The supported white balanceas.
     */
    public final Set<WhiteBalance> getSupportedWhiteBalance() {
        return new HashSet<WhiteBalance>(mSupportedWhiteBalances);
    }

    /**
     * @return Whether the white balance is supported.
     */
    public boolean supports(WhiteBalance wb) {
        return (wb != null && mSupportedWhiteBalances.contains(wb));
    }

    public final Set<Feature> getSupportedFeature() {
        return new HashSet<Feature>(mSupportedFeatures);
    }

    public boolean supports(Feature ft) {
        return (ft != null && mSupportedFeatures.contains(ft));
    }

    /**
     * @return The maximal supported zoom ratio.
     */
    public float getMaxZoomRatio() {
        return mMaxZoomRatio;
    }

    /**
     * @return The min exposure compensation index. The EV is the compensation
     * index multiplied by the step value. If unsupported, both this method and
     * {@link #getMaxExposureCompensation()} return 0.
     */
    public final int getMinExposureCompensation() {
        return mMinExposureCompensation;
    }

    /**
     * @return The max exposure compensation index. The EV is the compensation
     * index multiplied by the step value. If unsupported, both this method and
     * {@link #getMinExposureCompensation()} return 0.
     */
    public final int getMaxExposureCompensation() {
        return mMaxExposureCompensation;
    }

    /**
     * @return The exposure compensation step. The EV is the compensation index
     * multiplied by the step value.
     */
    public final float getExposureCompensationStep() {
        return mExposureCompensationStep;
    }

    /**
     * @return The max number of faces supported by the face detection. 0 if
     * unsupported.
     */
    public final int getMaxNumOfFacesSupported() {
        return mMaxNumOfFacesSupported;
    }

    /**
     * @return The stringifier used by this instance.
     */
    public Stringifier getStringifier() {
        return mStringifier;
    }

    private boolean zoomCheck(final CameraSettings settings) {
        final float ratio = settings.getCurrentZoomRatio();
        if (!supports(Feature.ZOOM)) {
            if (ratio != ZOOM_RATIO_UNZOOMED) {
                Log.v(TAG, "Zoom is not supported");
                return false;
            }
        } else {
            if (settings.getCurrentZoomRatio() > getMaxZoomRatio()) {
                Log.v(TAG, "Zoom ratio is not supported: ratio = " +
                        settings.getCurrentZoomRatio());
                return false;
            }
        }
        return true;
    }

    private boolean exposureCheck(final CameraSettings settings) {
        final int index = settings.getExposureCompensationIndex();
        if (index > getMaxExposureCompensation() || index < getMinExposureCompensation()) {
            Log.v(TAG, "Exposure compensation index is not supported. Min = " +
                    getMinExposureCompensation() + ", max = " + getMaxExposureCompensation() + "," +
                    " setting = " + index);
            return false;
        }
        return true;
    }

    private boolean focusCheck(final CameraSettings settings) {
        FocusMode focusMode = settings.getCurrentFocusMode();
        if (!supports(focusMode)) {
            if (supports(FocusMode.FIXED)) {
                // Workaround for devices whose templates define defaults they don't really support
                // TODO: Remove workaround (b/17177436)
                Log.w(TAG, "Focus mode not supported... trying FIXED");
                settings.setFocusMode(FocusMode.FIXED);
            } else {
                Log.v(TAG, "Focus mode not supported:" +
                        (focusMode != null ? focusMode.name() : "null"));
                return false;
            }
        }
        return true;
    }

    private boolean flashCheck(final CameraSettings settings) {
        FlashMode flashMode = settings.getCurrentFlashMode();
        if (!supports(flashMode)) {
            Log.v(TAG,
                    "Flash mode not supported:" + (flashMode != null ? flashMode.name() : "null"));
            return false;
        }
        return true;
    }

    private boolean photoSizeCheck(final CameraSettings settings) {
        Size photoSize = settings.getCurrentPhotoSize();
        if (mSupportedPhotoSizes.contains(photoSize)) {
            return true;
        }
        Log.v(TAG, "Unsupported photo size:" + photoSize);
        return false;
    }

    private boolean previewSizeCheck(final CameraSettings settings) {
        final Size previewSize = settings.getCurrentPreviewSize();
        if (mSupportedPreviewSizes.contains(previewSize)) {
            return true;
        }
        Log.v(TAG, "Unsupported preview size:" + previewSize);
        return false;
    }

    private boolean videoStabilizationCheck(final CameraSettings settings) {
        if (!settings.isVideoStabilizationEnabled() || supports(Feature.VIDEO_STABILIZATION)) {
            return true;
        }
        Log.v(TAG, "Video stabilization is not supported");
        return false;
    }
}
