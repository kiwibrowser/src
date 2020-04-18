/*
 * Copyright (C) 2015 The Android Open Source Project
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

/**
 * @addtogroup Camera
 * @{
 */

/**
 * @file NdkCameraMetadataTags.h
 */

/*
 * This file defines an NDK API.
 * Do not remove methods.
 * Do not change method signatures.
 * Do not change the value of constants.
 * Do not change the size of any of the classes defined in here.
 * Do not reference types that are not part of the NDK.
 * Do not #include files that aren't part of the NDK.
 */

#ifndef _NDK_CAMERA_METADATA_TAGS_H
#define _NDK_CAMERA_METADATA_TAGS_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#if __ANDROID_API__ >= 24

typedef enum acamera_metadata_section {
    ACAMERA_COLOR_CORRECTION,
    ACAMERA_CONTROL,
    ACAMERA_DEMOSAIC,
    ACAMERA_EDGE,
    ACAMERA_FLASH,
    ACAMERA_FLASH_INFO,
    ACAMERA_HOT_PIXEL,
    ACAMERA_JPEG,
    ACAMERA_LENS,
    ACAMERA_LENS_INFO,
    ACAMERA_NOISE_REDUCTION,
    ACAMERA_QUIRKS,
    ACAMERA_REQUEST,
    ACAMERA_SCALER,
    ACAMERA_SENSOR,
    ACAMERA_SENSOR_INFO,
    ACAMERA_SHADING,
    ACAMERA_STATISTICS,
    ACAMERA_STATISTICS_INFO,
    ACAMERA_TONEMAP,
    ACAMERA_LED,
    ACAMERA_INFO,
    ACAMERA_BLACK_LEVEL,
    ACAMERA_SYNC,
    ACAMERA_REPROCESS,
    ACAMERA_DEPTH,
    ACAMERA_SECTION_COUNT,

    ACAMERA_VENDOR = 0x8000
} acamera_metadata_section_t;

/**
 * Hierarchy positions in enum space.
 */
typedef enum acamera_metadata_section_start {
    ACAMERA_COLOR_CORRECTION_START = ACAMERA_COLOR_CORRECTION  << 16,
    ACAMERA_CONTROL_START          = ACAMERA_CONTROL           << 16,
    ACAMERA_DEMOSAIC_START         = ACAMERA_DEMOSAIC          << 16,
    ACAMERA_EDGE_START             = ACAMERA_EDGE              << 16,
    ACAMERA_FLASH_START            = ACAMERA_FLASH             << 16,
    ACAMERA_FLASH_INFO_START       = ACAMERA_FLASH_INFO        << 16,
    ACAMERA_HOT_PIXEL_START        = ACAMERA_HOT_PIXEL         << 16,
    ACAMERA_JPEG_START             = ACAMERA_JPEG              << 16,
    ACAMERA_LENS_START             = ACAMERA_LENS              << 16,
    ACAMERA_LENS_INFO_START        = ACAMERA_LENS_INFO         << 16,
    ACAMERA_NOISE_REDUCTION_START  = ACAMERA_NOISE_REDUCTION   << 16,
    ACAMERA_QUIRKS_START           = ACAMERA_QUIRKS            << 16,
    ACAMERA_REQUEST_START          = ACAMERA_REQUEST           << 16,
    ACAMERA_SCALER_START           = ACAMERA_SCALER            << 16,
    ACAMERA_SENSOR_START           = ACAMERA_SENSOR            << 16,
    ACAMERA_SENSOR_INFO_START      = ACAMERA_SENSOR_INFO       << 16,
    ACAMERA_SHADING_START          = ACAMERA_SHADING           << 16,
    ACAMERA_STATISTICS_START       = ACAMERA_STATISTICS        << 16,
    ACAMERA_STATISTICS_INFO_START  = ACAMERA_STATISTICS_INFO   << 16,
    ACAMERA_TONEMAP_START          = ACAMERA_TONEMAP           << 16,
    ACAMERA_LED_START              = ACAMERA_LED               << 16,
    ACAMERA_INFO_START             = ACAMERA_INFO              << 16,
    ACAMERA_BLACK_LEVEL_START      = ACAMERA_BLACK_LEVEL       << 16,
    ACAMERA_SYNC_START             = ACAMERA_SYNC              << 16,
    ACAMERA_REPROCESS_START        = ACAMERA_REPROCESS         << 16,
    ACAMERA_DEPTH_START            = ACAMERA_DEPTH             << 16,
    ACAMERA_VENDOR_START           = ACAMERA_VENDOR            << 16
} acamera_metadata_section_start_t;

/**
 * Main enum for camera metadata tags.
 */
typedef enum acamera_metadata_tag {
    /**
     * <p>The mode control selects how the image data is converted from the
     * sensor's native color into linear sRGB color.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_color_correction_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When auto-white balance (AWB) is enabled with ACAMERA_CONTROL_AWB_MODE, this
     * control is overridden by the AWB routine. When AWB is disabled, the
     * application controls how the color mapping is performed.</p>
     * <p>We define the expected processing pipeline below. For consistency
     * across devices, this is always the case with TRANSFORM_MATRIX.</p>
     * <p>When either FULL or HIGH_QUALITY is used, the camera device may
     * do additional processing but ACAMERA_COLOR_CORRECTION_GAINS and
     * ACAMERA_COLOR_CORRECTION_TRANSFORM will still be provided by the
     * camera device (in the results) and be roughly correct.</p>
     * <p>Switching to TRANSFORM_MATRIX and using the data provided from
     * FAST or HIGH_QUALITY will yield a picture with the same white point
     * as what was produced by the camera device in the earlier frame.</p>
     * <p>The expected processing pipeline is as follows:</p>
     * <p><img alt="White balance processing pipeline" src="../images/camera2/metadata/android.colorCorrection.mode/processing_pipeline.png" /></p>
     * <p>The white balance is encoded by two values, a 4-channel white-balance
     * gain vector (applied in the Bayer domain), and a 3x3 color transform
     * matrix (applied after demosaic).</p>
     * <p>The 4-channel white-balance gains are defined as:</p>
     * <pre><code>ACAMERA_COLOR_CORRECTION_GAINS = [ R G_even G_odd B ]
     * </code></pre>
     * <p>where <code>G_even</code> is the gain for green pixels on even rows of the
     * output, and <code>G_odd</code> is the gain for green pixels on the odd rows.
     * These may be identical for a given camera device implementation; if
     * the camera device does not support a separate gain for even/odd green
     * channels, it will use the <code>G_even</code> value, and write <code>G_odd</code> equal to
     * <code>G_even</code> in the output result metadata.</p>
     * <p>The matrices for color transforms are defined as a 9-entry vector:</p>
     * <pre><code>ACAMERA_COLOR_CORRECTION_TRANSFORM = [ I0 I1 I2 I3 I4 I5 I6 I7 I8 ]
     * </code></pre>
     * <p>which define a transform from input sensor colors, <code>P_in = [ r g b ]</code>,
     * to output linear sRGB, <code>P_out = [ r' g' b' ]</code>,</p>
     * <p>with colors as follows:</p>
     * <pre><code>r' = I0r + I1g + I2b
     * g' = I3r + I4g + I5b
     * b' = I6r + I7g + I8b
     * </code></pre>
     * <p>Both the input and output value ranges must match. Overflow/underflow
     * values are clipped to fit within the range.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     * @see ACAMERA_CONTROL_AWB_MODE
     */
    ACAMERA_COLOR_CORRECTION_MODE =                             // byte (acamera_metadata_enum_android_color_correction_mode_t)
            ACAMERA_COLOR_CORRECTION_START,
    /**
     * <p>A color transform matrix to use to transform
     * from sensor RGB color space to output linear sRGB color space.</p>
     *
     * <p>Type: rational[3*3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This matrix is either set by the camera device when the request
     * ACAMERA_COLOR_CORRECTION_MODE is not TRANSFORM_MATRIX, or
     * directly by the application in the request when the
     * ACAMERA_COLOR_CORRECTION_MODE is TRANSFORM_MATRIX.</p>
     * <p>In the latter case, the camera device may round the matrix to account
     * for precision issues; the final rounded matrix should be reported back
     * in this matrix result metadata. The transform should keep the magnitude
     * of the output color values within <code>[0, 1.0]</code> (assuming input color
     * values is within the normalized range <code>[0, 1.0]</code>), or clipping may occur.</p>
     * <p>The valid range of each matrix element varies on different devices, but
     * values within [-1.5, 3.0] are guaranteed not to be clipped.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_MODE
     */
    ACAMERA_COLOR_CORRECTION_TRANSFORM =                        // rational[3*3]
            ACAMERA_COLOR_CORRECTION_START + 1,
    /**
     * <p>Gains applying to Bayer raw color channels for
     * white-balance.</p>
     *
     * <p>Type: float[4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>These per-channel gains are either set by the camera device
     * when the request ACAMERA_COLOR_CORRECTION_MODE is not
     * TRANSFORM_MATRIX, or directly by the application in the
     * request when the ACAMERA_COLOR_CORRECTION_MODE is
     * TRANSFORM_MATRIX.</p>
     * <p>The gains in the result metadata are the gains actually
     * applied by the camera device to the current frame.</p>
     * <p>The valid range of gains varies on different devices, but gains
     * between [1.0, 3.0] are guaranteed not to be clipped. Even if a given
     * device allows gains below 1.0, this is usually not recommended because
     * this can create color artifacts.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_MODE
     */
    ACAMERA_COLOR_CORRECTION_GAINS =                            // float[4]
            ACAMERA_COLOR_CORRECTION_START + 2,
    /**
     * <p>Mode of operation for the chromatic aberration correction algorithm.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_color_correction_aberration_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Chromatic (color) aberration is caused by the fact that different wavelengths of light
     * can not focus on the same point after exiting from the lens. This metadata defines
     * the high level control of chromatic aberration correction algorithm, which aims to
     * minimize the chromatic artifacts that may occur along the object boundaries in an
     * image.</p>
     * <p>FAST/HIGH_QUALITY both mean that camera device determined aberration
     * correction will be applied. HIGH_QUALITY mode indicates that the camera device will
     * use the highest-quality aberration correction algorithms, even if it slows down
     * capture rate. FAST means the camera device will not slow down capture rate when
     * applying aberration correction.</p>
     * <p>LEGACY devices will always be in FAST mode.</p>
     */
    ACAMERA_COLOR_CORRECTION_ABERRATION_MODE =                  // byte (acamera_metadata_enum_android_color_correction_aberration_mode_t)
            ACAMERA_COLOR_CORRECTION_START + 3,
    /**
     * <p>List of aberration correction modes for ACAMERA_COLOR_CORRECTION_ABERRATION_MODE that are
     * supported by this camera device.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_ABERRATION_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This key lists the valid modes for ACAMERA_COLOR_CORRECTION_ABERRATION_MODE.  If no
     * aberration correction modes are available for a device, this list will solely include
     * OFF mode. All camera devices will support either OFF or FAST mode.</p>
     * <p>Camera devices that support the MANUAL_POST_PROCESSING capability will always list
     * OFF mode. This includes all FULL level devices.</p>
     * <p>LEGACY devices will always only support FAST mode.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_ABERRATION_MODE
     */
    ACAMERA_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES =       // byte[n]
            ACAMERA_COLOR_CORRECTION_START + 4,
    ACAMERA_COLOR_CORRECTION_END,

    /**
     * <p>The desired setting for the camera device's auto-exposure
     * algorithm's antibanding compensation.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_ae_antibanding_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Some kinds of lighting fixtures, such as some fluorescent
     * lights, flicker at the rate of the power supply frequency
     * (60Hz or 50Hz, depending on country). While this is
     * typically not noticeable to a person, it can be visible to
     * a camera device. If a camera sets its exposure time to the
     * wrong value, the flicker may become visible in the
     * viewfinder as flicker or in a final captured image, as a
     * set of variable-brightness bands across the image.</p>
     * <p>Therefore, the auto-exposure routines of camera devices
     * include antibanding routines that ensure that the chosen
     * exposure value will not cause such banding. The choice of
     * exposure time depends on the rate of flicker, which the
     * camera device can detect automatically, or the expected
     * rate can be selected by the application using this
     * control.</p>
     * <p>A given camera device may not support all of the possible
     * options for the antibanding mode. The
     * ACAMERA_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES key contains
     * the available modes for a given camera device.</p>
     * <p>AUTO mode is the default if it is available on given
     * camera device. When AUTO mode is not available, the
     * default will be either 50HZ or 60HZ, and both 50HZ
     * and 60HZ will be available.</p>
     * <p>If manual exposure control is enabled (by setting
     * ACAMERA_CONTROL_AE_MODE or ACAMERA_CONTROL_MODE to OFF),
     * then this setting has no effect, and the application must
     * ensure it selects exposure times that do not cause banding
     * issues. The ACAMERA_STATISTICS_SCENE_FLICKER key can assist
     * the application in this.</p>
     *
     * @see ACAMERA_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_STATISTICS_SCENE_FLICKER
     */
    ACAMERA_CONTROL_AE_ANTIBANDING_MODE =                       // byte (acamera_metadata_enum_android_control_ae_antibanding_mode_t)
            ACAMERA_CONTROL_START,
    /**
     * <p>Adjustment to auto-exposure (AE) target image
     * brightness.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>The adjustment is measured as a count of steps, with the
     * step size defined by ACAMERA_CONTROL_AE_COMPENSATION_STEP and the
     * allowed range by ACAMERA_CONTROL_AE_COMPENSATION_RANGE.</p>
     * <p>For example, if the exposure value (EV) step is 0.333, '6'
     * will mean an exposure compensation of +2 EV; -3 will mean an
     * exposure compensation of -1 EV. One EV represents a doubling
     * of image brightness. Note that this control will only be
     * effective if ACAMERA_CONTROL_AE_MODE <code>!=</code> OFF. This control
     * will take effect even when ACAMERA_CONTROL_AE_LOCK <code>== true</code>.</p>
     * <p>In the event of exposure compensation value being changed, camera device
     * may take several frames to reach the newly requested exposure target.
     * During that time, ACAMERA_CONTROL_AE_STATE field will be in the SEARCHING
     * state. Once the new exposure target is reached, ACAMERA_CONTROL_AE_STATE will
     * change from SEARCHING to either CONVERGED, LOCKED (if AE lock is enabled), or
     * FLASH_REQUIRED (if the scene is too dark for still capture).</p>
     *
     * @see ACAMERA_CONTROL_AE_COMPENSATION_RANGE
     * @see ACAMERA_CONTROL_AE_COMPENSATION_STEP
     * @see ACAMERA_CONTROL_AE_LOCK
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AE_STATE
     */
    ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION =                  // int32
            ACAMERA_CONTROL_START + 1,
    /**
     * <p>Whether auto-exposure (AE) is currently locked to its latest
     * calculated values.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_ae_lock_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When set to <code>true</code> (ON), the AE algorithm is locked to its latest parameters,
     * and will not change exposure settings until the lock is set to <code>false</code> (OFF).</p>
     * <p>Note that even when AE is locked, the flash may be fired if
     * the ACAMERA_CONTROL_AE_MODE is ON_AUTO_FLASH /
     * ON_ALWAYS_FLASH / ON_AUTO_FLASH_REDEYE.</p>
     * <p>When ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION is changed, even if the AE lock
     * is ON, the camera device will still adjust its exposure value.</p>
     * <p>If AE precapture is triggered (see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER)
     * when AE is already locked, the camera device will not change the exposure time
     * (ACAMERA_SENSOR_EXPOSURE_TIME) and sensitivity (ACAMERA_SENSOR_SENSITIVITY)
     * parameters. The flash may be fired if the ACAMERA_CONTROL_AE_MODE
     * is ON_AUTO_FLASH/ON_AUTO_FLASH_REDEYE and the scene is too dark. If the
     * ACAMERA_CONTROL_AE_MODE is ON_ALWAYS_FLASH, the scene may become overexposed.
     * Similarly, AE precapture trigger CANCEL has no effect when AE is already locked.</p>
     * <p>When an AE precapture sequence is triggered, AE unlock will not be able to unlock
     * the AE if AE is locked by the camera device internally during precapture metering
     * sequence In other words, submitting requests with AE unlock has no effect for an
     * ongoing precapture metering sequence. Otherwise, the precapture metering sequence
     * will never succeed in a sequence of preview requests where AE lock is always set
     * to <code>false</code>.</p>
     * <p>Since the camera device has a pipeline of in-flight requests, the settings that
     * get locked do not necessarily correspond to the settings that were present in the
     * latest capture result received from the camera device, since additional captures
     * and AE updates may have occurred even before the result was sent out. If an
     * application is switching between automatic and manual control and wishes to eliminate
     * any flicker during the switch, the following procedure is recommended:</p>
     * <ol>
     * <li>Starting in auto-AE mode:</li>
     * <li>Lock AE</li>
     * <li>Wait for the first result to be output that has the AE locked</li>
     * <li>Copy exposure settings from that result into a request, set the request to manual AE</li>
     * <li>Submit the capture request, proceed to run manual AE as desired.</li>
     * </ol>
     * <p>See ACAMERA_CONTROL_AE_STATE for AE lock related state transition details.</p>
     *
     * @see ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * @see ACAMERA_CONTROL_AE_STATE
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_CONTROL_AE_LOCK =                                   // byte (acamera_metadata_enum_android_control_ae_lock_t)
            ACAMERA_CONTROL_START + 2,
    /**
     * <p>The desired mode for the camera device's
     * auto-exposure routine.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_ae_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This control is only effective if ACAMERA_CONTROL_MODE is
     * AUTO.</p>
     * <p>When set to any of the ON modes, the camera device's
     * auto-exposure routine is enabled, overriding the
     * application's selected exposure time, sensor sensitivity,
     * and frame duration (ACAMERA_SENSOR_EXPOSURE_TIME,
     * ACAMERA_SENSOR_SENSITIVITY, and
     * ACAMERA_SENSOR_FRAME_DURATION). If one of the FLASH modes
     * is selected, the camera device's flash unit controls are
     * also overridden.</p>
     * <p>The FLASH modes are only available if the camera device
     * has a flash unit (ACAMERA_FLASH_INFO_AVAILABLE is <code>true</code>).</p>
     * <p>If flash TORCH mode is desired, this field must be set to
     * ON or OFF, and ACAMERA_FLASH_MODE set to TORCH.</p>
     * <p>When set to any of the ON modes, the values chosen by the
     * camera device auto-exposure routine for the overridden
     * fields for a given capture will be available in its
     * CaptureResult.</p>
     *
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_FLASH_INFO_AVAILABLE
     * @see ACAMERA_FLASH_MODE
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_FRAME_DURATION
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_CONTROL_AE_MODE =                                   // byte (acamera_metadata_enum_android_control_ae_mode_t)
            ACAMERA_CONTROL_START + 3,
    /**
     * <p>List of metering areas to use for auto-exposure adjustment.</p>
     *
     * <p>Type: int32[5*area_count]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Not available if android.control.maxRegionsAe is 0.
     * Otherwise will always be present.</p>
     * <p>The maximum number of regions supported by the device is determined by the value
     * of android.control.maxRegionsAe.</p>
     * <p>The data representation is int[5 * area_count].
     * Every five elements represent a metering region of (xmin, ymin, xmax, ymax, weight).
     * The rectangle is defined to be inclusive on xmin and ymin, but exclusive on xmax and
     * ymax.</p>
     * <p>The coordinate system is based on the active pixel array,
     * with (0,0) being the top-left pixel in the active pixel array, and
     * (ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.width - 1,
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.height - 1) being the
     * bottom-right pixel in the active pixel array.</p>
     * <p>The weight must be within <code>[0, 1000]</code>, and represents a weight
     * for every pixel in the area. This means that a large metering area
     * with the same weight as a smaller area will have more effect in
     * the metering result. Metering areas can partially overlap and the
     * camera device will add the weights in the overlap region.</p>
     * <p>The weights are relative to weights of other exposure metering regions, so if only one
     * region is used, all non-zero weights will have the same effect. A region with 0
     * weight is ignored.</p>
     * <p>If all regions have 0 weight, then no specific metering area needs to be used by the
     * camera device.</p>
     * <p>If the metering region is outside the used ACAMERA_SCALER_CROP_REGION returned in
     * capture result metadata, the camera device will ignore the sections outside the crop
     * region and output only the intersection rectangle as the metering region in the result
     * metadata.  If the region is entirely outside the crop region, it will be ignored and
     * not reported in the result metadata.</p>
     *
     * @see ACAMERA_SCALER_CROP_REGION
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_CONTROL_AE_REGIONS =                                // int32[5*area_count]
            ACAMERA_CONTROL_START + 4,
    /**
     * <p>Range over which the auto-exposure routine can
     * adjust the capture frame rate to maintain good
     * exposure.</p>
     *
     * <p>Type: int32[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Only constrains auto-exposure (AE) algorithm, not
     * manual control of ACAMERA_SENSOR_EXPOSURE_TIME and
     * ACAMERA_SENSOR_FRAME_DURATION.</p>
     *
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_FRAME_DURATION
     */
    ACAMERA_CONTROL_AE_TARGET_FPS_RANGE =                       // int32[2]
            ACAMERA_CONTROL_START + 5,
    /**
     * <p>Whether the camera device will trigger a precapture
     * metering sequence when it processes this request.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_ae_precapture_trigger_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This entry is normally set to IDLE, or is not
     * included at all in the request settings. When included and
     * set to START, the camera device will trigger the auto-exposure (AE)
     * precapture metering sequence.</p>
     * <p>When set to CANCEL, the camera device will cancel any active
     * precapture metering trigger, and return to its initial AE state.
     * If a precapture metering sequence is already completed, and the camera
     * device has implicitly locked the AE for subsequent still capture, the
     * CANCEL trigger will unlock the AE and return to its initial AE state.</p>
     * <p>The precapture sequence should be triggered before starting a
     * high-quality still capture for final metering decisions to
     * be made, and for firing pre-capture flash pulses to estimate
     * scene brightness and required final capture flash power, when
     * the flash is enabled.</p>
     * <p>Normally, this entry should be set to START for only a
     * single request, and the application should wait until the
     * sequence completes before starting a new one.</p>
     * <p>When a precapture metering sequence is finished, the camera device
     * may lock the auto-exposure routine internally to be able to accurately expose the
     * subsequent still capture image (<code>ACAMERA_CONTROL_CAPTURE_INTENT == STILL_CAPTURE</code>).
     * For this case, the AE may not resume normal scan if no subsequent still capture is
     * submitted. To ensure that the AE routine restarts normal scan, the application should
     * submit a request with <code>ACAMERA_CONTROL_AE_LOCK == true</code>, followed by a request
     * with <code>ACAMERA_CONTROL_AE_LOCK == false</code>, if the application decides not to submit a
     * still capture request after the precapture sequence completes. Alternatively, for
     * API level 23 or newer devices, the CANCEL can be used to unlock the camera device
     * internally locked AE if the application doesn't submit a still capture request after
     * the AE precapture trigger. Note that, the CANCEL was added in API level 23, and must not
     * be used in devices that have earlier API levels.</p>
     * <p>The exact effect of auto-exposure (AE) precapture trigger
     * depends on the current AE mode and state; see
     * ACAMERA_CONTROL_AE_STATE for AE precapture state transition
     * details.</p>
     * <p>On LEGACY-level devices, the precapture trigger is not supported;
     * capturing a high-resolution JPEG image will automatically trigger a
     * precapture sequence before the high-resolution capture, including
     * potentially firing a pre-capture flash.</p>
     * <p>Using the precapture trigger and the auto-focus trigger ACAMERA_CONTROL_AF_TRIGGER
     * simultaneously is allowed. However, since these triggers often require cooperation between
     * the auto-focus and auto-exposure routines (for example, the may need to be enabled for a
     * focus sweep), the camera device may delay acting on a later trigger until the previous
     * trigger has been fully handled. This may lead to longer intervals between the trigger and
     * changes to ACAMERA_CONTROL_AE_STATE indicating the start of the precapture sequence, for
     * example.</p>
     * <p>If both the precapture and the auto-focus trigger are activated on the same request, then
     * the camera device will complete them in the optimal order for that device.</p>
     *
     * @see ACAMERA_CONTROL_AE_LOCK
     * @see ACAMERA_CONTROL_AE_STATE
     * @see ACAMERA_CONTROL_AF_TRIGGER
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     */
    ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER =                     // byte (acamera_metadata_enum_android_control_ae_precapture_trigger_t)
            ACAMERA_CONTROL_START + 6,
    /**
     * <p>Whether auto-focus (AF) is currently enabled, and what
     * mode it is set to.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_af_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Only effective if ACAMERA_CONTROL_MODE = AUTO and the lens is not fixed focus
     * (i.e. <code>ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE &gt; 0</code>). Also note that
     * when ACAMERA_CONTROL_AE_MODE is OFF, the behavior of AF is device
     * dependent. It is recommended to lock AF by using ACAMERA_CONTROL_AF_TRIGGER before
     * setting ACAMERA_CONTROL_AE_MODE to OFF, or set AF mode to OFF when AE is OFF.</p>
     * <p>If the lens is controlled by the camera device auto-focus algorithm,
     * the camera device will report the current AF status in ACAMERA_CONTROL_AF_STATE
     * in result metadata.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AF_STATE
     * @see ACAMERA_CONTROL_AF_TRIGGER
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE
     */
    ACAMERA_CONTROL_AF_MODE =                                   // byte (acamera_metadata_enum_android_control_af_mode_t)
            ACAMERA_CONTROL_START + 7,
    /**
     * <p>List of metering areas to use for auto-focus.</p>
     *
     * <p>Type: int32[5*area_count]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Not available if android.control.maxRegionsAf is 0.
     * Otherwise will always be present.</p>
     * <p>The maximum number of focus areas supported by the device is determined by the value
     * of android.control.maxRegionsAf.</p>
     * <p>The data representation is int[5 * area_count].
     * Every five elements represent a metering region of (xmin, ymin, xmax, ymax, weight).
     * The rectangle is defined to be inclusive on xmin and ymin, but exclusive on xmax and
     * ymax.</p>
     * <p>The coordinate system is based on the active pixel array,
     * with (0,0) being the top-left pixel in the active pixel array, and
     * (ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.width - 1,
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.height - 1) being the
     * bottom-right pixel in the active pixel array.</p>
     * <p>The weight must be within <code>[0, 1000]</code>, and represents a weight
     * for every pixel in the area. This means that a large metering area
     * with the same weight as a smaller area will have more effect in
     * the metering result. Metering areas can partially overlap and the
     * camera device will add the weights in the overlap region.</p>
     * <p>The weights are relative to weights of other metering regions, so if only one region
     * is used, all non-zero weights will have the same effect. A region with 0 weight is
     * ignored.</p>
     * <p>If all regions have 0 weight, then no specific metering area needs to be used by the
     * camera device.</p>
     * <p>If the metering region is outside the used ACAMERA_SCALER_CROP_REGION returned in
     * capture result metadata, the camera device will ignore the sections outside the crop
     * region and output only the intersection rectangle as the metering region in the result
     * metadata. If the region is entirely outside the crop region, it will be ignored and
     * not reported in the result metadata.</p>
     *
     * @see ACAMERA_SCALER_CROP_REGION
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_CONTROL_AF_REGIONS =                                // int32[5*area_count]
            ACAMERA_CONTROL_START + 8,
    /**
     * <p>Whether the camera device will trigger autofocus for this request.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_af_trigger_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This entry is normally set to IDLE, or is not
     * included at all in the request settings.</p>
     * <p>When included and set to START, the camera device will trigger the
     * autofocus algorithm. If autofocus is disabled, this trigger has no effect.</p>
     * <p>When set to CANCEL, the camera device will cancel any active trigger,
     * and return to its initial AF state.</p>
     * <p>Generally, applications should set this entry to START or CANCEL for only a
     * single capture, and then return it to IDLE (or not set at all). Specifying
     * START for multiple captures in a row means restarting the AF operation over
     * and over again.</p>
     * <p>See ACAMERA_CONTROL_AF_STATE for what the trigger means for each AF mode.</p>
     * <p>Using the autofocus trigger and the precapture trigger ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * simultaneously is allowed. However, since these triggers often require cooperation between
     * the auto-focus and auto-exposure routines (for example, the may need to be enabled for a
     * focus sweep), the camera device may delay acting on a later trigger until the previous
     * trigger has been fully handled. This may lead to longer intervals between the trigger and
     * changes to ACAMERA_CONTROL_AF_STATE, for example.</p>
     *
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * @see ACAMERA_CONTROL_AF_STATE
     */
    ACAMERA_CONTROL_AF_TRIGGER =                                // byte (acamera_metadata_enum_android_control_af_trigger_t)
            ACAMERA_CONTROL_START + 9,
    /**
     * <p>Whether auto-white balance (AWB) is currently locked to its
     * latest calculated values.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_awb_lock_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When set to <code>true</code> (ON), the AWB algorithm is locked to its latest parameters,
     * and will not change color balance settings until the lock is set to <code>false</code> (OFF).</p>
     * <p>Since the camera device has a pipeline of in-flight requests, the settings that
     * get locked do not necessarily correspond to the settings that were present in the
     * latest capture result received from the camera device, since additional captures
     * and AWB updates may have occurred even before the result was sent out. If an
     * application is switching between automatic and manual control and wishes to eliminate
     * any flicker during the switch, the following procedure is recommended:</p>
     * <ol>
     * <li>Starting in auto-AWB mode:</li>
     * <li>Lock AWB</li>
     * <li>Wait for the first result to be output that has the AWB locked</li>
     * <li>Copy AWB settings from that result into a request, set the request to manual AWB</li>
     * <li>Submit the capture request, proceed to run manual AWB as desired.</li>
     * </ol>
     * <p>Note that AWB lock is only meaningful when
     * ACAMERA_CONTROL_AWB_MODE is in the AUTO mode; in other modes,
     * AWB is already fixed to a specific setting.</p>
     * <p>Some LEGACY devices may not support ON; the value is then overridden to OFF.</p>
     *
     * @see ACAMERA_CONTROL_AWB_MODE
     */
    ACAMERA_CONTROL_AWB_LOCK =                                  // byte (acamera_metadata_enum_android_control_awb_lock_t)
            ACAMERA_CONTROL_START + 10,
    /**
     * <p>Whether auto-white balance (AWB) is currently setting the color
     * transform fields, and what its illumination target
     * is.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_awb_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This control is only effective if ACAMERA_CONTROL_MODE is AUTO.</p>
     * <p>When set to the ON mode, the camera device's auto-white balance
     * routine is enabled, overriding the application's selected
     * ACAMERA_COLOR_CORRECTION_TRANSFORM, ACAMERA_COLOR_CORRECTION_GAINS and
     * ACAMERA_COLOR_CORRECTION_MODE. Note that when ACAMERA_CONTROL_AE_MODE
     * is OFF, the behavior of AWB is device dependent. It is recommened to
     * also set AWB mode to OFF or lock AWB by using ACAMERA_CONTROL_AWB_LOCK before
     * setting AE mode to OFF.</p>
     * <p>When set to the OFF mode, the camera device's auto-white balance
     * routine is disabled. The application manually controls the white
     * balance by ACAMERA_COLOR_CORRECTION_TRANSFORM, ACAMERA_COLOR_CORRECTION_GAINS
     * and ACAMERA_COLOR_CORRECTION_MODE.</p>
     * <p>When set to any other modes, the camera device's auto-white
     * balance routine is disabled. The camera device uses each
     * particular illumination target for white balance
     * adjustment. The application's values for
     * ACAMERA_COLOR_CORRECTION_TRANSFORM,
     * ACAMERA_COLOR_CORRECTION_GAINS and
     * ACAMERA_COLOR_CORRECTION_MODE are ignored.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_MODE
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AWB_LOCK
     * @see ACAMERA_CONTROL_MODE
     */
    ACAMERA_CONTROL_AWB_MODE =                                  // byte (acamera_metadata_enum_android_control_awb_mode_t)
            ACAMERA_CONTROL_START + 11,
    /**
     * <p>List of metering areas to use for auto-white-balance illuminant
     * estimation.</p>
     *
     * <p>Type: int32[5*area_count]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Not available if android.control.maxRegionsAwb is 0.
     * Otherwise will always be present.</p>
     * <p>The maximum number of regions supported by the device is determined by the value
     * of android.control.maxRegionsAwb.</p>
     * <p>The data representation is int[5 * area_count].
     * Every five elements represent a metering region of (xmin, ymin, xmax, ymax, weight).
     * The rectangle is defined to be inclusive on xmin and ymin, but exclusive on xmax and
     * ymax.</p>
     * <p>The coordinate system is based on the active pixel array,
     * with (0,0) being the top-left pixel in the active pixel array, and
     * (ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.width - 1,
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.height - 1) being the
     * bottom-right pixel in the active pixel array.</p>
     * <p>The weight must range from 0 to 1000, and represents a weight
     * for every pixel in the area. This means that a large metering area
     * with the same weight as a smaller area will have more effect in
     * the metering result. Metering areas can partially overlap and the
     * camera device will add the weights in the overlap region.</p>
     * <p>The weights are relative to weights of other white balance metering regions, so if
     * only one region is used, all non-zero weights will have the same effect. A region with
     * 0 weight is ignored.</p>
     * <p>If all regions have 0 weight, then no specific metering area needs to be used by the
     * camera device.</p>
     * <p>If the metering region is outside the used ACAMERA_SCALER_CROP_REGION returned in
     * capture result metadata, the camera device will ignore the sections outside the crop
     * region and output only the intersection rectangle as the metering region in the result
     * metadata.  If the region is entirely outside the crop region, it will be ignored and
     * not reported in the result metadata.</p>
     *
     * @see ACAMERA_SCALER_CROP_REGION
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_CONTROL_AWB_REGIONS =                               // int32[5*area_count]
            ACAMERA_CONTROL_START + 12,
    /**
     * <p>Information to the camera device 3A (auto-exposure,
     * auto-focus, auto-white balance) routines about the purpose
     * of this capture, to help the camera device to decide optimal 3A
     * strategy.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_capture_intent_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This control (except for MANUAL) is only effective if
     * <code>ACAMERA_CONTROL_MODE != OFF</code> and any 3A routine is active.</p>
     * <p>ZERO_SHUTTER_LAG will be supported if ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     * contains PRIVATE_REPROCESSING or YUV_REPROCESSING. MANUAL will be supported if
     * ACAMERA_REQUEST_AVAILABLE_CAPABILITIES contains MANUAL_SENSOR. Other intent values are
     * always supported.</p>
     *
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     */
    ACAMERA_CONTROL_CAPTURE_INTENT =                            // byte (acamera_metadata_enum_android_control_capture_intent_t)
            ACAMERA_CONTROL_START + 13,
    /**
     * <p>A special color effect to apply.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_effect_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When this mode is set, a color effect will be applied
     * to images produced by the camera device. The interpretation
     * and implementation of these color effects is left to the
     * implementor of the camera device, and should not be
     * depended on to be consistent (or present) across all
     * devices.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE =                               // byte (acamera_metadata_enum_android_control_effect_mode_t)
            ACAMERA_CONTROL_START + 14,
    /**
     * <p>Overall mode of 3A (auto-exposure, auto-white-balance, auto-focus) control
     * routines.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This is a top-level 3A control switch. When set to OFF, all 3A control
     * by the camera device is disabled. The application must set the fields for
     * capture parameters itself.</p>
     * <p>When set to AUTO, the individual algorithm controls in
     * ACAMERA_CONTROL_* are in effect, such as ACAMERA_CONTROL_AF_MODE.</p>
     * <p>When set to USE_SCENE_MODE, the individual controls in
     * ACAMERA_CONTROL_* are mostly disabled, and the camera device implements
     * one of the scene mode settings (such as ACTION, SUNSET, or PARTY)
     * as it wishes. The camera device scene mode 3A settings are provided by
     * capture results {@link ACameraMetadata} from
     * {@link ACameraCaptureSession_captureCallback_result}.</p>
     * <p>When set to OFF_KEEP_STATE, it is similar to OFF mode, the only difference
     * is that this frame will not be used by camera device background 3A statistics
     * update, as if this frame is never captured. This mode can be used in the scenario
     * where the application doesn't want a 3A manual control capture to affect
     * the subsequent auto 3A capture results.</p>
     *
     * @see ACAMERA_CONTROL_AF_MODE
     */
    ACAMERA_CONTROL_MODE =                                      // byte (acamera_metadata_enum_android_control_mode_t)
            ACAMERA_CONTROL_START + 15,
    /**
     * <p>Control for which scene mode is currently active.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_scene_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Scene modes are custom camera modes optimized for a certain set of conditions and
     * capture settings.</p>
     * <p>This is the mode that that is active when
     * <code>ACAMERA_CONTROL_MODE == USE_SCENE_MODE</code>. Aside from FACE_PRIORITY, these modes will
     * disable ACAMERA_CONTROL_AE_MODE, ACAMERA_CONTROL_AWB_MODE, and ACAMERA_CONTROL_AF_MODE
     * while in use.</p>
     * <p>The interpretation and implementation of these scene modes is left
     * to the implementor of the camera device. Their behavior will not be
     * consistent across all devices, and any given device may only implement
     * a subset of these modes.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AF_MODE
     * @see ACAMERA_CONTROL_AWB_MODE
     * @see ACAMERA_CONTROL_MODE
     */
    ACAMERA_CONTROL_SCENE_MODE =                                // byte (acamera_metadata_enum_android_control_scene_mode_t)
            ACAMERA_CONTROL_START + 16,
    /**
     * <p>Whether video stabilization is
     * active.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_video_stabilization_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Video stabilization automatically warps images from
     * the camera in order to stabilize motion between consecutive frames.</p>
     * <p>If enabled, video stabilization can modify the
     * ACAMERA_SCALER_CROP_REGION to keep the video stream stabilized.</p>
     * <p>Switching between different video stabilization modes may take several
     * frames to initialize, the camera device will report the current mode
     * in capture result metadata. For example, When "ON" mode is requested,
     * the video stabilization modes in the first several capture results may
     * still be "OFF", and it will become "ON" when the initialization is
     * done.</p>
     * <p>In addition, not all recording sizes or frame rates may be supported for
     * stabilization by a device that reports stabilization support. It is guaranteed
     * that an output targeting a MediaRecorder or MediaCodec will be stabilized if
     * the recording resolution is less than or equal to 1920 x 1080 (width less than
     * or equal to 1920, height less than or equal to 1080), and the recording
     * frame rate is less than or equal to 30fps.  At other sizes, the CaptureResult
     * ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE field will return
     * OFF if the recording output is not stabilized, or if there are no output
     * Surface types that can be stabilized.</p>
     * <p>If a camera device supports both this mode and OIS
     * (ACAMERA_LENS_OPTICAL_STABILIZATION_MODE), turning both modes on may
     * produce undesirable interaction, so it is recommended not to enable
     * both at the same time.</p>
     *
     * @see ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE
     * @see ACAMERA_LENS_OPTICAL_STABILIZATION_MODE
     * @see ACAMERA_SCALER_CROP_REGION
     */
    ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE =                  // byte (acamera_metadata_enum_android_control_video_stabilization_mode_t)
            ACAMERA_CONTROL_START + 17,
    /**
     * <p>List of auto-exposure antibanding modes for ACAMERA_CONTROL_AE_ANTIBANDING_MODE that are
     * supported by this camera device.</p>
     *
     * @see ACAMERA_CONTROL_AE_ANTIBANDING_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Not all of the auto-exposure anti-banding modes may be
     * supported by a given camera device. This field lists the
     * valid anti-banding modes that the application may request
     * for this camera device with the
     * ACAMERA_CONTROL_AE_ANTIBANDING_MODE control.</p>
     *
     * @see ACAMERA_CONTROL_AE_ANTIBANDING_MODE
     */
    ACAMERA_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES =            // byte[n]
            ACAMERA_CONTROL_START + 18,
    /**
     * <p>List of auto-exposure modes for ACAMERA_CONTROL_AE_MODE that are supported by this camera
     * device.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Not all the auto-exposure modes may be supported by a
     * given camera device, especially if no flash unit is
     * available. This entry lists the valid modes for
     * ACAMERA_CONTROL_AE_MODE for this camera device.</p>
     * <p>All camera devices support ON, and all camera devices with flash
     * units support ON_AUTO_FLASH and ON_ALWAYS_FLASH.</p>
     * <p>FULL mode camera devices always support OFF mode,
     * which enables application control of camera exposure time,
     * sensitivity, and frame duration.</p>
     * <p>LEGACY mode camera devices never support OFF mode.
     * LIMITED mode devices support OFF if they support the MANUAL_SENSOR
     * capability.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     */
    ACAMERA_CONTROL_AE_AVAILABLE_MODES =                        // byte[n]
            ACAMERA_CONTROL_START + 19,
    /**
     * <p>List of frame rate ranges for ACAMERA_CONTROL_AE_TARGET_FPS_RANGE supported by
     * this camera device.</p>
     *
     * @see ACAMERA_CONTROL_AE_TARGET_FPS_RANGE
     *
     * <p>Type: int32[2*n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>For devices at the LEGACY level or above:</p>
     * <ul>
     * <li>
     * <p>For constant-framerate recording, for each normal
     * <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html">CamcorderProfile</a>, that is, a
     * <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html">CamcorderProfile</a> that has
     * <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html#quality">quality</a>
     * in the range [
     * <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html#QUALITY_LOW">QUALITY_LOW</a>,
     * <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html#QUALITY_2160P">QUALITY_2160P</a>],
     * if the profile is supported by the device and has
     * <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html#videoFrameRate">videoFrameRate</a>
     * <code>x</code>, this list will always include (<code>x</code>,<code>x</code>).</p>
     * </li>
     * <li>
     * <p>Also, a camera device must either not support any
     * <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html">CamcorderProfile</a>,
     * or support at least one
     * normal <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html">CamcorderProfile</a>
     * that has
     * <a href="https://developer.android.com/reference/android/media/CamcorderProfile.html#videoFrameRate">videoFrameRate</a> <code>x</code> &gt;= 24.</p>
     * </li>
     * </ul>
     * <p>For devices at the LIMITED level or above:</p>
     * <ul>
     * <li>For YUV_420_888 burst capture use case, this list will always include (<code>min</code>, <code>max</code>)
     * and (<code>max</code>, <code>max</code>) where <code>min</code> &lt;= 15 and <code>max</code> = the maximum output frame rate of the
     * maximum YUV_420_888 output size.</li>
     * </ul>
     */
    ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES =            // int32[2*n]
            ACAMERA_CONTROL_START + 20,
    /**
     * <p>Maximum and minimum exposure compensation values for
     * ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION, in counts of ACAMERA_CONTROL_AE_COMPENSATION_STEP,
     * that are supported by this camera device.</p>
     *
     * @see ACAMERA_CONTROL_AE_COMPENSATION_STEP
     * @see ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION
     *
     * <p>Type: int32[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     */
    ACAMERA_CONTROL_AE_COMPENSATION_RANGE =                     // int32[2]
            ACAMERA_CONTROL_START + 21,
    /**
     * <p>Smallest step by which the exposure compensation
     * can be changed.</p>
     *
     * <p>Type: rational</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This is the unit for ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION. For example, if this key has
     * a value of <code>1/2</code>, then a setting of <code>-2</code> for ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION means
     * that the target EV offset for the auto-exposure routine is -1 EV.</p>
     * <p>One unit of EV compensation changes the brightness of the captured image by a factor
     * of two. +1 EV doubles the image brightness, while -1 EV halves the image brightness.</p>
     *
     * @see ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION
     */
    ACAMERA_CONTROL_AE_COMPENSATION_STEP =                      // rational
            ACAMERA_CONTROL_START + 22,
    /**
     * <p>List of auto-focus (AF) modes for ACAMERA_CONTROL_AF_MODE that are
     * supported by this camera device.</p>
     *
     * @see ACAMERA_CONTROL_AF_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Not all the auto-focus modes may be supported by a
     * given camera device. This entry lists the valid modes for
     * ACAMERA_CONTROL_AF_MODE for this camera device.</p>
     * <p>All LIMITED and FULL mode camera devices will support OFF mode, and all
     * camera devices with adjustable focuser units
     * (<code>ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE &gt; 0</code>) will support AUTO mode.</p>
     * <p>LEGACY devices will support OFF mode only if they support
     * focusing to infinity (by also setting ACAMERA_LENS_FOCUS_DISTANCE to
     * <code>0.0f</code>).</p>
     *
     * @see ACAMERA_CONTROL_AF_MODE
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     * @see ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE
     */
    ACAMERA_CONTROL_AF_AVAILABLE_MODES =                        // byte[n]
            ACAMERA_CONTROL_START + 23,
    /**
     * <p>List of color effects for ACAMERA_CONTROL_EFFECT_MODE that are supported by this camera
     * device.</p>
     *
     * @see ACAMERA_CONTROL_EFFECT_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This list contains the color effect modes that can be applied to
     * images produced by the camera device.
     * Implementations are not expected to be consistent across all devices.
     * If no color effect modes are available for a device, this will only list
     * OFF.</p>
     * <p>A color effect will only be applied if
     * ACAMERA_CONTROL_MODE != OFF.  OFF is always included in this list.</p>
     * <p>This control has no effect on the operation of other control routines such
     * as auto-exposure, white balance, or focus.</p>
     *
     * @see ACAMERA_CONTROL_MODE
     */
    ACAMERA_CONTROL_AVAILABLE_EFFECTS =                         // byte[n]
            ACAMERA_CONTROL_START + 24,
    /**
     * <p>List of scene modes for ACAMERA_CONTROL_SCENE_MODE that are supported by this camera
     * device.</p>
     *
     * @see ACAMERA_CONTROL_SCENE_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This list contains scene modes that can be set for the camera device.
     * Only scene modes that have been fully implemented for the
     * camera device may be included here. Implementations are not expected
     * to be consistent across all devices.</p>
     * <p>If no scene modes are supported by the camera device, this
     * will be set to DISABLED. Otherwise DISABLED will not be listed.</p>
     * <p>FACE_PRIORITY is always listed if face detection is
     * supported (i.e.<code>ACAMERA_STATISTICS_INFO_MAX_FACE_COUNT &gt;
     * 0</code>).</p>
     *
     * @see ACAMERA_STATISTICS_INFO_MAX_FACE_COUNT
     */
    ACAMERA_CONTROL_AVAILABLE_SCENE_MODES =                     // byte[n]
            ACAMERA_CONTROL_START + 25,
    /**
     * <p>List of video stabilization modes for ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE
     * that are supported by this camera device.</p>
     *
     * @see ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>OFF will always be listed.</p>
     */
    ACAMERA_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES =       // byte[n]
            ACAMERA_CONTROL_START + 26,
    /**
     * <p>List of auto-white-balance modes for ACAMERA_CONTROL_AWB_MODE that are supported by this
     * camera device.</p>
     *
     * @see ACAMERA_CONTROL_AWB_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Not all the auto-white-balance modes may be supported by a
     * given camera device. This entry lists the valid modes for
     * ACAMERA_CONTROL_AWB_MODE for this camera device.</p>
     * <p>All camera devices will support ON mode.</p>
     * <p>Camera devices that support the MANUAL_POST_PROCESSING capability will always support OFF
     * mode, which enables application control of white balance, by using
     * ACAMERA_COLOR_CORRECTION_TRANSFORM and ACAMERA_COLOR_CORRECTION_GAINS(ACAMERA_COLOR_CORRECTION_MODE must be set to TRANSFORM_MATRIX). This includes all FULL
     * mode camera devices.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_MODE
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     * @see ACAMERA_CONTROL_AWB_MODE
     */
    ACAMERA_CONTROL_AWB_AVAILABLE_MODES =                       // byte[n]
            ACAMERA_CONTROL_START + 27,
    /**
     * <p>List of the maximum number of regions that can be used for metering in
     * auto-exposure (AE), auto-white balance (AWB), and auto-focus (AF);
     * this corresponds to the the maximum number of elements in
     * ACAMERA_CONTROL_AE_REGIONS, ACAMERA_CONTROL_AWB_REGIONS,
     * and ACAMERA_CONTROL_AF_REGIONS.</p>
     *
     * @see ACAMERA_CONTROL_AE_REGIONS
     * @see ACAMERA_CONTROL_AF_REGIONS
     * @see ACAMERA_CONTROL_AWB_REGIONS
     *
     * <p>Type: int32[3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     */
    ACAMERA_CONTROL_MAX_REGIONS =                               // int32[3]
            ACAMERA_CONTROL_START + 28,
    /**
     * <p>Current state of the auto-exposure (AE) algorithm.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_ae_state_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Switching between or enabling AE modes (ACAMERA_CONTROL_AE_MODE) always
     * resets the AE state to INACTIVE. Similarly, switching between ACAMERA_CONTROL_MODE,
     * or ACAMERA_CONTROL_SCENE_MODE if <code>ACAMERA_CONTROL_MODE == USE_SCENE_MODE</code> resets all
     * the algorithm states to INACTIVE.</p>
     * <p>The camera device can do several state transitions between two results, if it is
     * allowed by the state transition table. For example: INACTIVE may never actually be
     * seen in a result.</p>
     * <p>The state in the result is the state for this image (in sync with this image): if
     * AE state becomes CONVERGED, then the image data associated with this result should
     * be good to use.</p>
     * <p>Below are state transition tables for different AE modes.</p>
     * <p>State       | Transition Cause | New State | Notes
     * :------------:|:----------------:|:---------:|:-----------------------:
     * INACTIVE      |                  | INACTIVE  | Camera device auto exposure algorithm is disabled</p>
     * <p>When ACAMERA_CONTROL_AE_MODE is AE_MODE_ON_*:</p>
     * <p>State        | Transition Cause                             | New State      | Notes
     * :-------------:|:--------------------------------------------:|:--------------:|:-----------------:
     * INACTIVE       | Camera device initiates AE scan              | SEARCHING      | Values changing
     * INACTIVE       | ACAMERA_CONTROL_AE_LOCK is ON                 | LOCKED         | Values locked
     * SEARCHING      | Camera device finishes AE scan               | CONVERGED      | Good values, not changing
     * SEARCHING      | Camera device finishes AE scan               | FLASH_REQUIRED | Converged but too dark w/o flash
     * SEARCHING      | ACAMERA_CONTROL_AE_LOCK is ON                 | LOCKED         | Values locked
     * CONVERGED      | Camera device initiates AE scan              | SEARCHING      | Values changing
     * CONVERGED      | ACAMERA_CONTROL_AE_LOCK is ON                 | LOCKED         | Values locked
     * FLASH_REQUIRED | Camera device initiates AE scan              | SEARCHING      | Values changing
     * FLASH_REQUIRED | ACAMERA_CONTROL_AE_LOCK is ON                 | LOCKED         | Values locked
     * LOCKED         | ACAMERA_CONTROL_AE_LOCK is OFF                | SEARCHING      | Values not good after unlock
     * LOCKED         | ACAMERA_CONTROL_AE_LOCK is OFF                | CONVERGED      | Values good after unlock
     * LOCKED         | ACAMERA_CONTROL_AE_LOCK is OFF                | FLASH_REQUIRED | Exposure good, but too dark
     * PRECAPTURE     | Sequence done. ACAMERA_CONTROL_AE_LOCK is OFF | CONVERGED      | Ready for high-quality capture
     * PRECAPTURE     | Sequence done. ACAMERA_CONTROL_AE_LOCK is ON  | LOCKED         | Ready for high-quality capture
     * LOCKED         | aeLock is ON and aePrecaptureTrigger is START | LOCKED        | Precapture trigger is ignored when AE is already locked
     * LOCKED         | aeLock is ON and aePrecaptureTrigger is CANCEL| LOCKED        | Precapture trigger is ignored when AE is already locked
     * Any state (excluding LOCKED) | ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER is START | PRECAPTURE     | Start AE precapture metering sequence
     * Any state (excluding LOCKED) | ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER is CANCEL| INACTIVE       | Currently active precapture metering sequence is canceled</p>
     * <p>For the above table, the camera device may skip reporting any state changes that happen
     * without application intervention (i.e. mode switch, trigger, locking). Any state that
     * can be skipped in that manner is called a transient state.</p>
     * <p>For example, for above AE modes (AE_MODE_ON_*), in addition to the state transitions
     * listed in above table, it is also legal for the camera device to skip one or more
     * transient states between two results. See below table for examples:</p>
     * <p>State        | Transition Cause                                            | New State      | Notes
     * :-------------:|:-----------------------------------------------------------:|:--------------:|:-----------------:
     * INACTIVE       | Camera device finished AE scan                              | CONVERGED      | Values are already good, transient states are skipped by camera device.
     * Any state (excluding LOCKED) | ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER is START, sequence done | FLASH_REQUIRED | Converged but too dark w/o flash after a precapture sequence, transient states are skipped by camera device.
     * Any state (excluding LOCKED) | ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER is START, sequence done | CONVERGED      | Converged after a precapture sequence, transient states are skipped by camera device.
     * Any state (excluding LOCKED) | ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER is CANCEL, converged    | FLASH_REQUIRED | Converged but too dark w/o flash after a precapture sequence is canceled, transient states are skipped by camera device.
     * Any state (excluding LOCKED) | ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER is CANCEL, converged    | CONVERGED      | Converged after a precapture sequenceis canceled, transient states are skipped by camera device.
     * CONVERGED      | Camera device finished AE scan                              | FLASH_REQUIRED | Converged but too dark w/o flash after a new scan, transient states are skipped by camera device.
     * FLASH_REQUIRED | Camera device finished AE scan                              | CONVERGED      | Converged after a new scan, transient states are skipped by camera device.</p>
     *
     * @see ACAMERA_CONTROL_AE_LOCK
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_CONTROL_SCENE_MODE
     */
    ACAMERA_CONTROL_AE_STATE =                                  // byte (acamera_metadata_enum_android_control_ae_state_t)
            ACAMERA_CONTROL_START + 31,
    /**
     * <p>Current state of auto-focus (AF) algorithm.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_af_state_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Switching between or enabling AF modes (ACAMERA_CONTROL_AF_MODE) always
     * resets the AF state to INACTIVE. Similarly, switching between ACAMERA_CONTROL_MODE,
     * or ACAMERA_CONTROL_SCENE_MODE if <code>ACAMERA_CONTROL_MODE == USE_SCENE_MODE</code> resets all
     * the algorithm states to INACTIVE.</p>
     * <p>The camera device can do several state transitions between two results, if it is
     * allowed by the state transition table. For example: INACTIVE may never actually be
     * seen in a result.</p>
     * <p>The state in the result is the state for this image (in sync with this image): if
     * AF state becomes FOCUSED, then the image data associated with this result should
     * be sharp.</p>
     * <p>Below are state transition tables for different AF modes.</p>
     * <p>When ACAMERA_CONTROL_AF_MODE is AF_MODE_OFF or AF_MODE_EDOF:</p>
     * <p>State       | Transition Cause | New State | Notes
     * :------------:|:----------------:|:---------:|:-----------:
     * INACTIVE      |                  | INACTIVE  | Never changes</p>
     * <p>When ACAMERA_CONTROL_AF_MODE is AF_MODE_AUTO or AF_MODE_MACRO:</p>
     * <p>State            | Transition Cause | New State          | Notes
     * :-----------------:|:----------------:|:------------------:|:--------------:
     * INACTIVE           | AF_TRIGGER       | ACTIVE_SCAN        | Start AF sweep, Lens now moving
     * ACTIVE_SCAN        | AF sweep done    | FOCUSED_LOCKED     | Focused, Lens now locked
     * ACTIVE_SCAN        | AF sweep done    | NOT_FOCUSED_LOCKED | Not focused, Lens now locked
     * ACTIVE_SCAN        | AF_CANCEL        | INACTIVE           | Cancel/reset AF, Lens now locked
     * FOCUSED_LOCKED     | AF_CANCEL        | INACTIVE           | Cancel/reset AF
     * FOCUSED_LOCKED     | AF_TRIGGER       | ACTIVE_SCAN        | Start new sweep, Lens now moving
     * NOT_FOCUSED_LOCKED | AF_CANCEL        | INACTIVE           | Cancel/reset AF
     * NOT_FOCUSED_LOCKED | AF_TRIGGER       | ACTIVE_SCAN        | Start new sweep, Lens now moving
     * Any state          | Mode change      | INACTIVE           |</p>
     * <p>For the above table, the camera device may skip reporting any state changes that happen
     * without application intervention (i.e. mode switch, trigger, locking). Any state that
     * can be skipped in that manner is called a transient state.</p>
     * <p>For example, for these AF modes (AF_MODE_AUTO and AF_MODE_MACRO), in addition to the
     * state transitions listed in above table, it is also legal for the camera device to skip
     * one or more transient states between two results. See below table for examples:</p>
     * <p>State            | Transition Cause | New State          | Notes
     * :-----------------:|:----------------:|:------------------:|:--------------:
     * INACTIVE           | AF_TRIGGER       | FOCUSED_LOCKED     | Focus is already good or good after a scan, lens is now locked.
     * INACTIVE           | AF_TRIGGER       | NOT_FOCUSED_LOCKED | Focus failed after a scan, lens is now locked.
     * FOCUSED_LOCKED     | AF_TRIGGER       | FOCUSED_LOCKED     | Focus is already good or good after a scan, lens is now locked.
     * NOT_FOCUSED_LOCKED | AF_TRIGGER       | FOCUSED_LOCKED     | Focus is good after a scan, lens is not locked.</p>
     * <p>When ACAMERA_CONTROL_AF_MODE is AF_MODE_CONTINUOUS_VIDEO:</p>
     * <p>State            | Transition Cause                    | New State          | Notes
     * :-----------------:|:-----------------------------------:|:------------------:|:--------------:
     * INACTIVE           | Camera device initiates new scan    | PASSIVE_SCAN       | Start AF scan, Lens now moving
     * INACTIVE           | AF_TRIGGER                          | NOT_FOCUSED_LOCKED | AF state query, Lens now locked
     * PASSIVE_SCAN       | Camera device completes current scan| PASSIVE_FOCUSED    | End AF scan, Lens now locked
     * PASSIVE_SCAN       | Camera device fails current scan    | PASSIVE_UNFOCUSED  | End AF scan, Lens now locked
     * PASSIVE_SCAN       | AF_TRIGGER                          | FOCUSED_LOCKED     | Immediate transition, if focus is good. Lens now locked
     * PASSIVE_SCAN       | AF_TRIGGER                          | NOT_FOCUSED_LOCKED | Immediate transition, if focus is bad. Lens now locked
     * PASSIVE_SCAN       | AF_CANCEL                           | INACTIVE           | Reset lens position, Lens now locked
     * PASSIVE_FOCUSED    | Camera device initiates new scan    | PASSIVE_SCAN       | Start AF scan, Lens now moving
     * PASSIVE_UNFOCUSED  | Camera device initiates new scan    | PASSIVE_SCAN       | Start AF scan, Lens now moving
     * PASSIVE_FOCUSED    | AF_TRIGGER                          | FOCUSED_LOCKED     | Immediate transition, lens now locked
     * PASSIVE_UNFOCUSED  | AF_TRIGGER                          | NOT_FOCUSED_LOCKED | Immediate transition, lens now locked
     * FOCUSED_LOCKED     | AF_TRIGGER                          | FOCUSED_LOCKED     | No effect
     * FOCUSED_LOCKED     | AF_CANCEL                           | INACTIVE           | Restart AF scan
     * NOT_FOCUSED_LOCKED | AF_TRIGGER                          | NOT_FOCUSED_LOCKED | No effect
     * NOT_FOCUSED_LOCKED | AF_CANCEL                           | INACTIVE           | Restart AF scan</p>
     * <p>When ACAMERA_CONTROL_AF_MODE is AF_MODE_CONTINUOUS_PICTURE:</p>
     * <p>State            | Transition Cause                     | New State          | Notes
     * :-----------------:|:------------------------------------:|:------------------:|:--------------:
     * INACTIVE           | Camera device initiates new scan     | PASSIVE_SCAN       | Start AF scan, Lens now moving
     * INACTIVE           | AF_TRIGGER                           | NOT_FOCUSED_LOCKED | AF state query, Lens now locked
     * PASSIVE_SCAN       | Camera device completes current scan | PASSIVE_FOCUSED    | End AF scan, Lens now locked
     * PASSIVE_SCAN       | Camera device fails current scan     | PASSIVE_UNFOCUSED  | End AF scan, Lens now locked
     * PASSIVE_SCAN       | AF_TRIGGER                           | FOCUSED_LOCKED     | Eventual transition once the focus is good. Lens now locked
     * PASSIVE_SCAN       | AF_TRIGGER                           | NOT_FOCUSED_LOCKED | Eventual transition if cannot find focus. Lens now locked
     * PASSIVE_SCAN       | AF_CANCEL                            | INACTIVE           | Reset lens position, Lens now locked
     * PASSIVE_FOCUSED    | Camera device initiates new scan     | PASSIVE_SCAN       | Start AF scan, Lens now moving
     * PASSIVE_UNFOCUSED  | Camera device initiates new scan     | PASSIVE_SCAN       | Start AF scan, Lens now moving
     * PASSIVE_FOCUSED    | AF_TRIGGER                           | FOCUSED_LOCKED     | Immediate trans. Lens now locked
     * PASSIVE_UNFOCUSED  | AF_TRIGGER                           | NOT_FOCUSED_LOCKED | Immediate trans. Lens now locked
     * FOCUSED_LOCKED     | AF_TRIGGER                           | FOCUSED_LOCKED     | No effect
     * FOCUSED_LOCKED     | AF_CANCEL                            | INACTIVE           | Restart AF scan
     * NOT_FOCUSED_LOCKED | AF_TRIGGER                           | NOT_FOCUSED_LOCKED | No effect
     * NOT_FOCUSED_LOCKED | AF_CANCEL                            | INACTIVE           | Restart AF scan</p>
     * <p>When switch between AF_MODE_CONTINUOUS_* (CAF modes) and AF_MODE_AUTO/AF_MODE_MACRO
     * (AUTO modes), the initial INACTIVE or PASSIVE_SCAN states may be skipped by the
     * camera device. When a trigger is included in a mode switch request, the trigger
     * will be evaluated in the context of the new mode in the request.
     * See below table for examples:</p>
     * <p>State      | Transition Cause                       | New State                                | Notes
     * :-----------:|:--------------------------------------:|:----------------------------------------:|:--------------:
     * any state    | CAF--&gt;AUTO mode switch                 | INACTIVE                                 | Mode switch without trigger, initial state must be INACTIVE
     * any state    | CAF--&gt;AUTO mode switch with AF_TRIGGER | trigger-reachable states from INACTIVE   | Mode switch with trigger, INACTIVE is skipped
     * any state    | AUTO--&gt;CAF mode switch                 | passively reachable states from INACTIVE | Mode switch without trigger, passive transient state is skipped</p>
     *
     * @see ACAMERA_CONTROL_AF_MODE
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_CONTROL_SCENE_MODE
     */
    ACAMERA_CONTROL_AF_STATE =                                  // byte (acamera_metadata_enum_android_control_af_state_t)
            ACAMERA_CONTROL_START + 32,
    /**
     * <p>Current state of auto-white balance (AWB) algorithm.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_awb_state_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Switching between or enabling AWB modes (ACAMERA_CONTROL_AWB_MODE) always
     * resets the AWB state to INACTIVE. Similarly, switching between ACAMERA_CONTROL_MODE,
     * or ACAMERA_CONTROL_SCENE_MODE if <code>ACAMERA_CONTROL_MODE == USE_SCENE_MODE</code> resets all
     * the algorithm states to INACTIVE.</p>
     * <p>The camera device can do several state transitions between two results, if it is
     * allowed by the state transition table. So INACTIVE may never actually be seen in
     * a result.</p>
     * <p>The state in the result is the state for this image (in sync with this image): if
     * AWB state becomes CONVERGED, then the image data associated with this result should
     * be good to use.</p>
     * <p>Below are state transition tables for different AWB modes.</p>
     * <p>When <code>ACAMERA_CONTROL_AWB_MODE != AWB_MODE_AUTO</code>:</p>
     * <p>State       | Transition Cause | New State | Notes
     * :------------:|:----------------:|:---------:|:-----------------------:
     * INACTIVE      |                  |INACTIVE   |Camera device auto white balance algorithm is disabled</p>
     * <p>When ACAMERA_CONTROL_AWB_MODE is AWB_MODE_AUTO:</p>
     * <p>State        | Transition Cause                 | New State     | Notes
     * :-------------:|:--------------------------------:|:-------------:|:-----------------:
     * INACTIVE       | Camera device initiates AWB scan | SEARCHING     | Values changing
     * INACTIVE       | ACAMERA_CONTROL_AWB_LOCK is ON    | LOCKED        | Values locked
     * SEARCHING      | Camera device finishes AWB scan  | CONVERGED     | Good values, not changing
     * SEARCHING      | ACAMERA_CONTROL_AWB_LOCK is ON    | LOCKED        | Values locked
     * CONVERGED      | Camera device initiates AWB scan | SEARCHING     | Values changing
     * CONVERGED      | ACAMERA_CONTROL_AWB_LOCK is ON    | LOCKED        | Values locked
     * LOCKED         | ACAMERA_CONTROL_AWB_LOCK is OFF   | SEARCHING     | Values not good after unlock</p>
     * <p>For the above table, the camera device may skip reporting any state changes that happen
     * without application intervention (i.e. mode switch, trigger, locking). Any state that
     * can be skipped in that manner is called a transient state.</p>
     * <p>For example, for this AWB mode (AWB_MODE_AUTO), in addition to the state transitions
     * listed in above table, it is also legal for the camera device to skip one or more
     * transient states between two results. See below table for examples:</p>
     * <p>State        | Transition Cause                 | New State     | Notes
     * :-------------:|:--------------------------------:|:-------------:|:-----------------:
     * INACTIVE       | Camera device finished AWB scan  | CONVERGED     | Values are already good, transient states are skipped by camera device.
     * LOCKED         | ACAMERA_CONTROL_AWB_LOCK is OFF   | CONVERGED     | Values good after unlock, transient states are skipped by camera device.</p>
     *
     * @see ACAMERA_CONTROL_AWB_LOCK
     * @see ACAMERA_CONTROL_AWB_MODE
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_CONTROL_SCENE_MODE
     */
    ACAMERA_CONTROL_AWB_STATE =                                 // byte (acamera_metadata_enum_android_control_awb_state_t)
            ACAMERA_CONTROL_START + 34,
    /**
     * <p>Whether the camera device supports ACAMERA_CONTROL_AE_LOCK</p>
     *
     * @see ACAMERA_CONTROL_AE_LOCK
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_ae_lock_available_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Devices with MANUAL_SENSOR capability or BURST_CAPTURE capability will always
     * list <code>true</code>. This includes FULL devices.</p>
     */
    ACAMERA_CONTROL_AE_LOCK_AVAILABLE =                         // byte (acamera_metadata_enum_android_control_ae_lock_available_t)
            ACAMERA_CONTROL_START + 36,
    /**
     * <p>Whether the camera device supports ACAMERA_CONTROL_AWB_LOCK</p>
     *
     * @see ACAMERA_CONTROL_AWB_LOCK
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_awb_lock_available_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Devices with MANUAL_POST_PROCESSING capability or BURST_CAPTURE capability will
     * always list <code>true</code>. This includes FULL devices.</p>
     */
    ACAMERA_CONTROL_AWB_LOCK_AVAILABLE =                        // byte (acamera_metadata_enum_android_control_awb_lock_available_t)
            ACAMERA_CONTROL_START + 37,
    /**
     * <p>List of control modes for ACAMERA_CONTROL_MODE that are supported by this camera
     * device.</p>
     *
     * @see ACAMERA_CONTROL_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This list contains control modes that can be set for the camera device.
     * LEGACY mode devices will always support AUTO mode. LIMITED and FULL
     * devices will always support OFF, AUTO modes.</p>
     */
    ACAMERA_CONTROL_AVAILABLE_MODES =                           // byte[n]
            ACAMERA_CONTROL_START + 38,
    /**
     * <p>Range of boosts for ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST supported
     * by this camera device.</p>
     *
     * @see ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST
     *
     * <p>Type: int32[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Devices support post RAW sensitivity boost  will advertise
     * ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST key for controling
     * post RAW sensitivity boost.</p>
     * <p>This key will be <code>null</code> for devices that do not support any RAW format
     * outputs. For devices that do support RAW format outputs, this key will always
     * present, and if a device does not support post RAW sensitivity boost, it will
     * list <code>(100, 100)</code> in this key.</p>
     *
     * @see ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST
     */
    ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST_RANGE =          // int32[2]
            ACAMERA_CONTROL_START + 39,
    /**
     * <p>The amount of additional sensitivity boost applied to output images
     * after RAW sensor data is captured.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Some camera devices support additional digital sensitivity boosting in the
     * camera processing pipeline after sensor RAW image is captured.
     * Such a boost will be applied to YUV/JPEG format output images but will not
     * have effect on RAW output formats like RAW_SENSOR, RAW10, RAW12 or RAW_OPAQUE.</p>
     * <p>This key will be <code>null</code> for devices that do not support any RAW format
     * outputs. For devices that do support RAW format outputs, this key will always
     * present, and if a device does not support post RAW sensitivity boost, it will
     * list <code>100</code> in this key.</p>
     * <p>If the camera device cannot apply the exact boost requested, it will reduce the
     * boost to the nearest supported value.
     * The final boost value used will be available in the output capture result.</p>
     * <p>For devices that support post RAW sensitivity boost, the YUV/JPEG output images
     * of such device will have the total sensitivity of
     * <code>ACAMERA_SENSOR_SENSITIVITY * ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST / 100</code>
     * The sensitivity of RAW format images will always be <code>ACAMERA_SENSOR_SENSITIVITY</code></p>
     * <p>This control is only effective if ACAMERA_CONTROL_AE_MODE or ACAMERA_CONTROL_MODE is set to
     * OFF; otherwise the auto-exposure algorithm will override this value.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_CONTROL_POST_RAW_SENSITIVITY_BOOST =                // int32
            ACAMERA_CONTROL_START + 40,
    /**
     * <p>Allow camera device to enable zero-shutter-lag mode for requests with
     * ACAMERA_CONTROL_CAPTURE_INTENT == STILL_CAPTURE.</p>
     *
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     *
     * <p>Type: byte (acamera_metadata_enum_android_control_enable_zsl_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>If enableZsl is <code>true</code>, the camera device may enable zero-shutter-lag mode for requests with
     * STILL_CAPTURE capture intent. The camera device may use images captured in the past to
     * produce output images for a zero-shutter-lag request. The result metadata including the
     * ACAMERA_SENSOR_TIMESTAMP reflects the source frames used to produce output images.
     * Therefore, the contents of the output images and the result metadata may be out of order
     * compared to previous regular requests. enableZsl does not affect requests with other
     * capture intents.</p>
     * <p>For example, when requests are submitted in the following order:
     *   Request A: enableZsl is <code>true</code>, ACAMERA_CONTROL_CAPTURE_INTENT is PREVIEW
     *   Request B: enableZsl is <code>true</code>, ACAMERA_CONTROL_CAPTURE_INTENT is STILL_CAPTURE</p>
     * <p>The output images for request B may have contents captured before the output images for
     * request A, and the result metadata for request B may be older than the result metadata for
     * request A.</p>
     * <p>Note that when enableZsl is <code>true</code>, it is not guaranteed to get output images captured in the
     * past for requests with STILL_CAPTURE capture intent.</p>
     * <p>For applications targeting SDK versions O and newer, the value of enableZsl in
     * TEMPLATE_STILL_CAPTURE template may be <code>true</code>. The value in other templates is always
     * <code>false</code> if present.</p>
     * <p>For applications targeting SDK versions older than O, the value of enableZsl in all
     * capture templates is always <code>false</code> if present.</p>
     * <p>For application-operated ZSL, use CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG template.</p>
     *
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     * @see ACAMERA_SENSOR_TIMESTAMP
     */
    ACAMERA_CONTROL_ENABLE_ZSL =                                // byte (acamera_metadata_enum_android_control_enable_zsl_t)
            ACAMERA_CONTROL_START + 41,
    ACAMERA_CONTROL_END,

    /**
     * <p>Operation mode for edge
     * enhancement.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_edge_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Edge enhancement improves sharpness and details in the captured image. OFF means
     * no enhancement will be applied by the camera device.</p>
     * <p>FAST/HIGH_QUALITY both mean camera device determined enhancement
     * will be applied. HIGH_QUALITY mode indicates that the
     * camera device will use the highest-quality enhancement algorithms,
     * even if it slows down capture rate. FAST means the camera device will
     * not slow down capture rate when applying edge enhancement. FAST may be the same as OFF if
     * edge enhancement will slow down capture rate. Every output stream will have a similar
     * amount of enhancement applied.</p>
     * <p>ZERO_SHUTTER_LAG is meant to be used by applications that maintain a continuous circular
     * buffer of high-resolution images during preview and reprocess image(s) from that buffer
     * into a final capture when triggered by the user. In this mode, the camera device applies
     * edge enhancement to low-resolution streams (below maximum recording resolution) to
     * maximize preview quality, but does not apply edge enhancement to high-resolution streams,
     * since those will be reprocessed later if necessary.</p>
     * <p>For YUV_REPROCESSING, these FAST/HIGH_QUALITY modes both mean that the camera
     * device will apply FAST/HIGH_QUALITY YUV-domain edge enhancement, respectively.
     * The camera device may adjust its internal edge enhancement parameters for best
     * image quality based on the android.reprocess.effectiveExposureFactor, if it is set.</p>
     */
    ACAMERA_EDGE_MODE =                                         // byte (acamera_metadata_enum_android_edge_mode_t)
            ACAMERA_EDGE_START,
    /**
     * <p>List of edge enhancement modes for ACAMERA_EDGE_MODE that are supported by this camera
     * device.</p>
     *
     * @see ACAMERA_EDGE_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Full-capability camera devices must always support OFF; camera devices that support
     * YUV_REPROCESSING or PRIVATE_REPROCESSING will list ZERO_SHUTTER_LAG; all devices will
     * list FAST.</p>
     */
    ACAMERA_EDGE_AVAILABLE_EDGE_MODES =                         // byte[n]
            ACAMERA_EDGE_START + 2,
    ACAMERA_EDGE_END,

    /**
     * <p>The desired mode for for the camera device's flash control.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_flash_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This control is only effective when flash unit is available
     * (<code>ACAMERA_FLASH_INFO_AVAILABLE == true</code>).</p>
     * <p>When this control is used, the ACAMERA_CONTROL_AE_MODE must be set to ON or OFF.
     * Otherwise, the camera device auto-exposure related flash control (ON_AUTO_FLASH,
     * ON_ALWAYS_FLASH, or ON_AUTO_FLASH_REDEYE) will override this control.</p>
     * <p>When set to OFF, the camera device will not fire flash for this capture.</p>
     * <p>When set to SINGLE, the camera device will fire flash regardless of the camera
     * device's auto-exposure routine's result. When used in still capture case, this
     * control should be used along with auto-exposure (AE) precapture metering sequence
     * (ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER), otherwise, the image may be incorrectly exposed.</p>
     * <p>When set to TORCH, the flash will be on continuously. This mode can be used
     * for use cases such as preview, auto-focus assist, still capture, or video recording.</p>
     * <p>The flash status will be reported by ACAMERA_FLASH_STATE in the capture result metadata.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * @see ACAMERA_FLASH_INFO_AVAILABLE
     * @see ACAMERA_FLASH_STATE
     */
    ACAMERA_FLASH_MODE =                                        // byte (acamera_metadata_enum_android_flash_mode_t)
            ACAMERA_FLASH_START + 2,
    /**
     * <p>Current state of the flash
     * unit.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_flash_state_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>When the camera device doesn't have flash unit
     * (i.e. <code>ACAMERA_FLASH_INFO_AVAILABLE == false</code>), this state will always be UNAVAILABLE.
     * Other states indicate the current flash status.</p>
     * <p>In certain conditions, this will be available on LEGACY devices:</p>
     * <ul>
     * <li>Flash-less cameras always return UNAVAILABLE.</li>
     * <li>Using ACAMERA_CONTROL_AE_MODE <code>==</code> ON_ALWAYS_FLASH
     *    will always return FIRED.</li>
     * <li>Using ACAMERA_FLASH_MODE <code>==</code> TORCH
     *    will always return FIRED.</li>
     * </ul>
     * <p>In all other conditions the state will not be available on
     * LEGACY devices (i.e. it will be <code>null</code>).</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_FLASH_INFO_AVAILABLE
     * @see ACAMERA_FLASH_MODE
     */
    ACAMERA_FLASH_STATE =                                       // byte (acamera_metadata_enum_android_flash_state_t)
            ACAMERA_FLASH_START + 5,
    ACAMERA_FLASH_END,

    /**
     * <p>Whether this camera device has a
     * flash unit.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_flash_info_available_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Will be <code>false</code> if no flash is available.</p>
     * <p>If there is no flash unit, none of the flash controls do
     * anything.</p>
     */
    ACAMERA_FLASH_INFO_AVAILABLE =                              // byte (acamera_metadata_enum_android_flash_info_available_t)
            ACAMERA_FLASH_INFO_START,
    ACAMERA_FLASH_INFO_END,

    /**
     * <p>Operational mode for hot pixel correction.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_hot_pixel_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Hotpixel correction interpolates out, or otherwise removes, pixels
     * that do not accurately measure the incoming light (i.e. pixels that
     * are stuck at an arbitrary value or are oversensitive).</p>
     */
    ACAMERA_HOT_PIXEL_MODE =                                    // byte (acamera_metadata_enum_android_hot_pixel_mode_t)
            ACAMERA_HOT_PIXEL_START,
    /**
     * <p>List of hot pixel correction modes for ACAMERA_HOT_PIXEL_MODE that are supported by this
     * camera device.</p>
     *
     * @see ACAMERA_HOT_PIXEL_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>FULL mode camera devices will always support FAST.</p>
     */
    ACAMERA_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES =               // byte[n]
            ACAMERA_HOT_PIXEL_START + 1,
    ACAMERA_HOT_PIXEL_END,

    /**
     * <p>GPS coordinates to include in output JPEG
     * EXIF.</p>
     *
     * <p>Type: double[3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     */
    ACAMERA_JPEG_GPS_COORDINATES =                              // double[3]
            ACAMERA_JPEG_START,
    /**
     * <p>32 characters describing GPS algorithm to
     * include in EXIF.</p>
     *
     * <p>Type: byte</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     */
    ACAMERA_JPEG_GPS_PROCESSING_METHOD =                        // byte
            ACAMERA_JPEG_START + 1,
    /**
     * <p>Time GPS fix was made to include in
     * EXIF.</p>
     *
     * <p>Type: int64</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     */
    ACAMERA_JPEG_GPS_TIMESTAMP =                                // int64
            ACAMERA_JPEG_START + 2,
    /**
     * <p>The orientation for a JPEG image.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>The clockwise rotation angle in degrees, relative to the orientation
     * to the camera, that the JPEG picture needs to be rotated by, to be viewed
     * upright.</p>
     * <p>Camera devices may either encode this value into the JPEG EXIF header, or
     * rotate the image data to match this orientation. When the image data is rotated,
     * the thumbnail data will also be rotated.</p>
     * <p>Note that this orientation is relative to the orientation of the camera sensor, given
     * by ACAMERA_SENSOR_ORIENTATION.</p>
     * <p>To translate from the device orientation given by the Android sensor APIs, the following
     * sample code may be used:</p>
     * <pre><code>private int getJpegOrientation(CameraCharacteristics c, int deviceOrientation) {
     *     if (deviceOrientation == android.view.OrientationEventListener.ORIENTATION_UNKNOWN) return 0;
     *     int sensorOrientation = c.get(CameraCharacteristics.SENSOR_ORIENTATION);
     *
     *     // Round device orientation to a multiple of 90
     *     deviceOrientation = (deviceOrientation + 45) / 90 * 90;
     *
     *     // Reverse device orientation for front-facing cameras
     *     boolean facingFront = c.get(CameraCharacteristics.LENS_FACING) == CameraCharacteristics.LENS_FACING_FRONT;
     *     if (facingFront) deviceOrientation = -deviceOrientation;
     *
     *     // Calculate desired JPEG orientation relative to camera orientation to make
     *     // the image upright relative to the device orientation
     *     int jpegOrientation = (sensorOrientation + deviceOrientation + 360) % 360;
     *
     *     return jpegOrientation;
     * }
     * </code></pre>
     *
     * @see ACAMERA_SENSOR_ORIENTATION
     */
    ACAMERA_JPEG_ORIENTATION =                                  // int32
            ACAMERA_JPEG_START + 3,
    /**
     * <p>Compression quality of the final JPEG
     * image.</p>
     *
     * <p>Type: byte</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>85-95 is typical usage range.</p>
     */
    ACAMERA_JPEG_QUALITY =                                      // byte
            ACAMERA_JPEG_START + 4,
    /**
     * <p>Compression quality of JPEG
     * thumbnail.</p>
     *
     * <p>Type: byte</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     */
    ACAMERA_JPEG_THUMBNAIL_QUALITY =                            // byte
            ACAMERA_JPEG_START + 5,
    /**
     * <p>Resolution of embedded JPEG thumbnail.</p>
     *
     * <p>Type: int32[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When set to (0, 0) value, the JPEG EXIF will not contain thumbnail,
     * but the captured JPEG will still be a valid image.</p>
     * <p>For best results, when issuing a request for a JPEG image, the thumbnail size selected
     * should have the same aspect ratio as the main JPEG output.</p>
     * <p>If the thumbnail image aspect ratio differs from the JPEG primary image aspect
     * ratio, the camera device creates the thumbnail by cropping it from the primary image.
     * For example, if the primary image has 4:3 aspect ratio, the thumbnail image has
     * 16:9 aspect ratio, the primary image will be cropped vertically (letterbox) to
     * generate the thumbnail image. The thumbnail image will always have a smaller Field
     * Of View (FOV) than the primary image when aspect ratios differ.</p>
     * <p>When an ACAMERA_JPEG_ORIENTATION of non-zero degree is requested,
     * the camera device will handle thumbnail rotation in one of the following ways:</p>
     * <ul>
     * <li>Set the
     *   <a href="https://developer.android.com/reference/android/media/ExifInterface.html#TAG_ORIENTATION">EXIF orientation flag</a>
     *   and keep jpeg and thumbnail image data unrotated.</li>
     * <li>Rotate the jpeg and thumbnail image data and not set
     *   <a href="https://developer.android.com/reference/android/media/ExifInterface.html#TAG_ORIENTATION">EXIF orientation flag</a>.
     *   In this case, LIMITED or FULL hardware level devices will report rotated thumnail size
     *   in capture result, so the width and height will be interchanged if 90 or 270 degree
     *   orientation is requested. LEGACY device will always report unrotated thumbnail size.</li>
     * </ul>
     *
     * @see ACAMERA_JPEG_ORIENTATION
     */
    ACAMERA_JPEG_THUMBNAIL_SIZE =                               // int32[2]
            ACAMERA_JPEG_START + 6,
    /**
     * <p>List of JPEG thumbnail sizes for ACAMERA_JPEG_THUMBNAIL_SIZE supported by this
     * camera device.</p>
     *
     * @see ACAMERA_JPEG_THUMBNAIL_SIZE
     *
     * <p>Type: int32[2*n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This list will include at least one non-zero resolution, plus <code>(0,0)</code> for indicating no
     * thumbnail should be generated.</p>
     * <p>Below condiditions will be satisfied for this size list:</p>
     * <ul>
     * <li>The sizes will be sorted by increasing pixel area (width x height).
     * If several resolutions have the same area, they will be sorted by increasing width.</li>
     * <li>The aspect ratio of the largest thumbnail size will be same as the
     * aspect ratio of largest JPEG output size in ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS.
     * The largest size is defined as the size that has the largest pixel area
     * in a given size list.</li>
     * <li>Each output JPEG size in ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS will have at least
     * one corresponding size that has the same aspect ratio in availableThumbnailSizes,
     * and vice versa.</li>
     * <li>All non-<code>(0, 0)</code> sizes will have non-zero widths and heights.</li>
     * </ul>
     *
     * @see ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS
     */
    ACAMERA_JPEG_AVAILABLE_THUMBNAIL_SIZES =                    // int32[2*n]
            ACAMERA_JPEG_START + 7,
    ACAMERA_JPEG_END,

    /**
     * <p>The desired lens aperture size, as a ratio of lens focal length to the
     * effective aperture diameter.</p>
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Setting this value is only supported on the camera devices that have a variable
     * aperture lens.</p>
     * <p>When this is supported and ACAMERA_CONTROL_AE_MODE is OFF,
     * this can be set along with ACAMERA_SENSOR_EXPOSURE_TIME,
     * ACAMERA_SENSOR_SENSITIVITY, and ACAMERA_SENSOR_FRAME_DURATION
     * to achieve manual exposure control.</p>
     * <p>The requested aperture value may take several frames to reach the
     * requested value; the camera device will report the current (intermediate)
     * aperture size in capture result metadata while the aperture is changing.
     * While the aperture is still changing, ACAMERA_LENS_STATE will be set to MOVING.</p>
     * <p>When this is supported and ACAMERA_CONTROL_AE_MODE is one of
     * the ON modes, this will be overridden by the camera device
     * auto-exposure algorithm, the overridden values are then provided
     * back to the user in the corresponding result.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_LENS_STATE
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_FRAME_DURATION
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_LENS_APERTURE =                                     // float
            ACAMERA_LENS_START,
    /**
     * <p>The desired setting for the lens neutral density filter(s).</p>
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This control will not be supported on most camera devices.</p>
     * <p>Lens filters are typically used to lower the amount of light the
     * sensor is exposed to (measured in steps of EV). As used here, an EV
     * step is the standard logarithmic representation, which are
     * non-negative, and inversely proportional to the amount of light
     * hitting the sensor.  For example, setting this to 0 would result
     * in no reduction of the incoming light, and setting this to 2 would
     * mean that the filter is set to reduce incoming light by two stops
     * (allowing 1/4 of the prior amount of light to the sensor).</p>
     * <p>It may take several frames before the lens filter density changes
     * to the requested value. While the filter density is still changing,
     * ACAMERA_LENS_STATE will be set to MOVING.</p>
     *
     * @see ACAMERA_LENS_STATE
     */
    ACAMERA_LENS_FILTER_DENSITY =                               // float
            ACAMERA_LENS_START + 1,
    /**
     * <p>The desired lens focal length; used for optical zoom.</p>
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This setting controls the physical focal length of the camera
     * device's lens. Changing the focal length changes the field of
     * view of the camera device, and is usually used for optical zoom.</p>
     * <p>Like ACAMERA_LENS_FOCUS_DISTANCE and ACAMERA_LENS_APERTURE, this
     * setting won't be applied instantaneously, and it may take several
     * frames before the lens can change to the requested focal length.
     * While the focal length is still changing, ACAMERA_LENS_STATE will
     * be set to MOVING.</p>
     * <p>Optical zoom will not be supported on most devices.</p>
     *
     * @see ACAMERA_LENS_APERTURE
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     * @see ACAMERA_LENS_STATE
     */
    ACAMERA_LENS_FOCAL_LENGTH =                                 // float
            ACAMERA_LENS_START + 2,
    /**
     * <p>Desired distance to plane of sharpest focus,
     * measured from frontmost surface of the lens.</p>
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Should be zero for fixed-focus cameras</p>
     */
    ACAMERA_LENS_FOCUS_DISTANCE =                               // float
            ACAMERA_LENS_START + 3,
    /**
     * <p>Sets whether the camera device uses optical image stabilization (OIS)
     * when capturing images.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_lens_optical_stabilization_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>OIS is used to compensate for motion blur due to small
     * movements of the camera during capture. Unlike digital image
     * stabilization (ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE), OIS
     * makes use of mechanical elements to stabilize the camera
     * sensor, and thus allows for longer exposure times before
     * camera shake becomes apparent.</p>
     * <p>Switching between different optical stabilization modes may take several
     * frames to initialize, the camera device will report the current mode in
     * capture result metadata. For example, When "ON" mode is requested, the
     * optical stabilization modes in the first several capture results may still
     * be "OFF", and it will become "ON" when the initialization is done.</p>
     * <p>If a camera device supports both OIS and digital image stabilization
     * (ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE), turning both modes on may produce undesirable
     * interaction, so it is recommended not to enable both at the same time.</p>
     * <p>Not all devices will support OIS; see
     * ACAMERA_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION for
     * available controls.</p>
     *
     * @see ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE
     * @see ACAMERA_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION
     */
    ACAMERA_LENS_OPTICAL_STABILIZATION_MODE =                   // byte (acamera_metadata_enum_android_lens_optical_stabilization_mode_t)
            ACAMERA_LENS_START + 4,
    /**
     * <p>Direction the camera faces relative to
     * device screen.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_lens_facing_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     */
    ACAMERA_LENS_FACING =                                       // byte (acamera_metadata_enum_android_lens_facing_t)
            ACAMERA_LENS_START + 5,
    /**
     * <p>The orientation of the camera relative to the sensor
     * coordinate system.</p>
     *
     * <p>Type: float[4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>The four coefficients that describe the quaternion
     * rotation from the Android sensor coordinate system to a
     * camera-aligned coordinate system where the X-axis is
     * aligned with the long side of the image sensor, the Y-axis
     * is aligned with the short side of the image sensor, and
     * the Z-axis is aligned with the optical axis of the sensor.</p>
     * <p>To convert from the quaternion coefficients <code>(x,y,z,w)</code>
     * to the axis of rotation <code>(a_x, a_y, a_z)</code> and rotation
     * amount <code>theta</code>, the following formulas can be used:</p>
     * <pre><code> theta = 2 * acos(w)
     * a_x = x / sin(theta/2)
     * a_y = y / sin(theta/2)
     * a_z = z / sin(theta/2)
     * </code></pre>
     * <p>To create a 3x3 rotation matrix that applies the rotation
     * defined by this quaternion, the following matrix can be
     * used:</p>
     * <pre><code>R = [ 1 - 2y^2 - 2z^2,       2xy - 2zw,       2xz + 2yw,
     *            2xy + 2zw, 1 - 2x^2 - 2z^2,       2yz - 2xw,
     *            2xz - 2yw,       2yz + 2xw, 1 - 2x^2 - 2y^2 ]
     * </code></pre>
     * <p>This matrix can then be used to apply the rotation to a
     *  column vector point with</p>
     * <p><code>p' = Rp</code></p>
     * <p>where <code>p</code> is in the device sensor coordinate system, and
     *  <code>p'</code> is in the camera-oriented coordinate system.</p>
     */
    ACAMERA_LENS_POSE_ROTATION =                                // float[4]
            ACAMERA_LENS_START + 6,
    /**
     * <p>Position of the camera optical center.</p>
     *
     * <p>Type: float[3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>The position of the camera device's lens optical center,
     * as a three-dimensional vector <code>(x,y,z)</code>, relative to the
     * optical center of the largest camera device facing in the
     * same direction as this camera, in the
     * <a href="https://developer.android.com/reference/android/hardware/SensorEvent.html">Android sensor coordinate axes</a>.
     * Note that only the axis definitions are shared with
     * the sensor coordinate system, but not the origin.</p>
     * <p>If this device is the largest or only camera device with a
     * given facing, then this position will be <code>(0, 0, 0)</code>; a
     * camera device with a lens optical center located 3 cm from
     * the main sensor along the +X axis (to the right from the
     * user's perspective) will report <code>(0.03, 0, 0)</code>.</p>
     * <p>To transform a pixel coordinates between two cameras
     * facing the same direction, first the source camera
     * ACAMERA_LENS_RADIAL_DISTORTION must be corrected for.  Then
     * the source camera ACAMERA_LENS_INTRINSIC_CALIBRATION needs
     * to be applied, followed by the ACAMERA_LENS_POSE_ROTATION
     * of the source camera, the translation of the source camera
     * relative to the destination camera, the
     * ACAMERA_LENS_POSE_ROTATION of the destination camera, and
     * finally the inverse of ACAMERA_LENS_INTRINSIC_CALIBRATION
     * of the destination camera. This obtains a
     * radial-distortion-free coordinate in the destination
     * camera pixel coordinates.</p>
     * <p>To compare this against a real image from the destination
     * camera, the destination camera image then needs to be
     * corrected for radial distortion before comparison or
     * sampling.</p>
     *
     * @see ACAMERA_LENS_INTRINSIC_CALIBRATION
     * @see ACAMERA_LENS_POSE_ROTATION
     * @see ACAMERA_LENS_RADIAL_DISTORTION
     */
    ACAMERA_LENS_POSE_TRANSLATION =                             // float[3]
            ACAMERA_LENS_START + 7,
    /**
     * <p>The range of scene distances that are in
     * sharp focus (depth of field).</p>
     *
     * <p>Type: float[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>If variable focus not supported, can still report
     * fixed depth of field range</p>
     */
    ACAMERA_LENS_FOCUS_RANGE =                                  // float[2]
            ACAMERA_LENS_START + 8,
    /**
     * <p>Current lens status.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_lens_state_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>For lens parameters ACAMERA_LENS_FOCAL_LENGTH, ACAMERA_LENS_FOCUS_DISTANCE,
     * ACAMERA_LENS_FILTER_DENSITY and ACAMERA_LENS_APERTURE, when changes are requested,
     * they may take several frames to reach the requested values. This state indicates
     * the current status of the lens parameters.</p>
     * <p>When the state is STATIONARY, the lens parameters are not changing. This could be
     * either because the parameters are all fixed, or because the lens has had enough
     * time to reach the most recently-requested values.
     * If all these lens parameters are not changable for a camera device, as listed below:</p>
     * <ul>
     * <li>Fixed focus (<code>ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE == 0</code>), which means
     * ACAMERA_LENS_FOCUS_DISTANCE parameter will always be 0.</li>
     * <li>Fixed focal length (ACAMERA_LENS_INFO_AVAILABLE_FOCAL_LENGTHS contains single value),
     * which means the optical zoom is not supported.</li>
     * <li>No ND filter (ACAMERA_LENS_INFO_AVAILABLE_FILTER_DENSITIES contains only 0).</li>
     * <li>Fixed aperture (ACAMERA_LENS_INFO_AVAILABLE_APERTURES contains single value).</li>
     * </ul>
     * <p>Then this state will always be STATIONARY.</p>
     * <p>When the state is MOVING, it indicates that at least one of the lens parameters
     * is changing.</p>
     *
     * @see ACAMERA_LENS_APERTURE
     * @see ACAMERA_LENS_FILTER_DENSITY
     * @see ACAMERA_LENS_FOCAL_LENGTH
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     * @see ACAMERA_LENS_INFO_AVAILABLE_APERTURES
     * @see ACAMERA_LENS_INFO_AVAILABLE_FILTER_DENSITIES
     * @see ACAMERA_LENS_INFO_AVAILABLE_FOCAL_LENGTHS
     * @see ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE
     */
    ACAMERA_LENS_STATE =                                        // byte (acamera_metadata_enum_android_lens_state_t)
            ACAMERA_LENS_START + 9,
    /**
     * <p>The parameters for this camera device's intrinsic
     * calibration.</p>
     *
     * <p>Type: float[5]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>The five calibration parameters that describe the
     * transform from camera-centric 3D coordinates to sensor
     * pixel coordinates:</p>
     * <pre><code>[f_x, f_y, c_x, c_y, s]
     * </code></pre>
     * <p>Where <code>f_x</code> and <code>f_y</code> are the horizontal and vertical
     * focal lengths, <code>[c_x, c_y]</code> is the position of the optical
     * axis, and <code>s</code> is a skew parameter for the sensor plane not
     * being aligned with the lens plane.</p>
     * <p>These are typically used within a transformation matrix K:</p>
     * <pre><code>K = [ f_x,   s, c_x,
     *        0, f_y, c_y,
     *        0    0,   1 ]
     * </code></pre>
     * <p>which can then be combined with the camera pose rotation
     * <code>R</code> and translation <code>t</code> (ACAMERA_LENS_POSE_ROTATION and
     * ACAMERA_LENS_POSE_TRANSLATION, respective) to calculate the
     * complete transform from world coordinates to pixel
     * coordinates:</p>
     * <pre><code>P = [ K 0   * [ R t
     *      0 1 ]     0 1 ]
     * </code></pre>
     * <p>and with <code>p_w</code> being a point in the world coordinate system
     * and <code>p_s</code> being a point in the camera active pixel array
     * coordinate system, and with the mapping including the
     * homogeneous division by z:</p>
     * <pre><code> p_h = (x_h, y_h, z_h) = P p_w
     * p_s = p_h / z_h
     * </code></pre>
     * <p>so <code>[x_s, y_s]</code> is the pixel coordinates of the world
     * point, <code>z_s = 1</code>, and <code>w_s</code> is a measurement of disparity
     * (depth) in pixel coordinates.</p>
     * <p>Note that the coordinate system for this transform is the
     * ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE system,
     * where <code>(0,0)</code> is the top-left of the
     * preCorrectionActiveArraySize rectangle. Once the pose and
     * intrinsic calibration transforms have been applied to a
     * world point, then the ACAMERA_LENS_RADIAL_DISTORTION
     * transform needs to be applied, and the result adjusted to
     * be in the ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE coordinate
     * system (where <code>(0, 0)</code> is the top-left of the
     * activeArraySize rectangle), to determine the final pixel
     * coordinate of the world point for processed (non-RAW)
     * output buffers.</p>
     *
     * @see ACAMERA_LENS_POSE_ROTATION
     * @see ACAMERA_LENS_POSE_TRANSLATION
     * @see ACAMERA_LENS_RADIAL_DISTORTION
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     * @see ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_LENS_INTRINSIC_CALIBRATION =                        // float[5]
            ACAMERA_LENS_START + 10,
    /**
     * <p>The correction coefficients to correct for this camera device's
     * radial and tangential lens distortion.</p>
     *
     * <p>Type: float[6]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Four radial distortion coefficients <code>[kappa_0, kappa_1, kappa_2,
     * kappa_3]</code> and two tangential distortion coefficients
     * <code>[kappa_4, kappa_5]</code> that can be used to correct the
     * lens's geometric distortion with the mapping equations:</p>
     * <pre><code> x_c = x_i * ( kappa_0 + kappa_1 * r^2 + kappa_2 * r^4 + kappa_3 * r^6 ) +
     *        kappa_4 * (2 * x_i * y_i) + kappa_5 * ( r^2 + 2 * x_i^2 )
     *  y_c = y_i * ( kappa_0 + kappa_1 * r^2 + kappa_2 * r^4 + kappa_3 * r^6 ) +
     *        kappa_5 * (2 * x_i * y_i) + kappa_4 * ( r^2 + 2 * y_i^2 )
     * </code></pre>
     * <p>Here, <code>[x_c, y_c]</code> are the coordinates to sample in the
     * input image that correspond to the pixel values in the
     * corrected image at the coordinate <code>[x_i, y_i]</code>:</p>
     * <pre><code> correctedImage(x_i, y_i) = sample_at(x_c, y_c, inputImage)
     * </code></pre>
     * <p>The pixel coordinates are defined in a normalized
     * coordinate system related to the
     * ACAMERA_LENS_INTRINSIC_CALIBRATION calibration fields.
     * Both <code>[x_i, y_i]</code> and <code>[x_c, y_c]</code> have <code>(0,0)</code> at the
     * lens optical center <code>[c_x, c_y]</code>. The maximum magnitudes
     * of both x and y coordinates are normalized to be 1 at the
     * edge further from the optical center, so the range
     * for both dimensions is <code>-1 &lt;= x &lt;= 1</code>.</p>
     * <p>Finally, <code>r</code> represents the radial distance from the
     * optical center, <code>r^2 = x_i^2 + y_i^2</code>, and its magnitude
     * is therefore no larger than <code>|r| &lt;= sqrt(2)</code>.</p>
     * <p>The distortion model used is the Brown-Conrady model.</p>
     *
     * @see ACAMERA_LENS_INTRINSIC_CALIBRATION
     */
    ACAMERA_LENS_RADIAL_DISTORTION =                            // float[6]
            ACAMERA_LENS_START + 11,
    ACAMERA_LENS_END,

    /**
     * <p>List of aperture size values for ACAMERA_LENS_APERTURE that are
     * supported by this camera device.</p>
     *
     * @see ACAMERA_LENS_APERTURE
     *
     * <p>Type: float[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If the camera device doesn't support a variable lens aperture,
     * this list will contain only one value, which is the fixed aperture size.</p>
     * <p>If the camera device supports a variable aperture, the aperture values
     * in this list will be sorted in ascending order.</p>
     */
    ACAMERA_LENS_INFO_AVAILABLE_APERTURES =                     // float[n]
            ACAMERA_LENS_INFO_START,
    /**
     * <p>List of neutral density filter values for
     * ACAMERA_LENS_FILTER_DENSITY that are supported by this camera device.</p>
     *
     * @see ACAMERA_LENS_FILTER_DENSITY
     *
     * <p>Type: float[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If a neutral density filter is not supported by this camera device,
     * this list will contain only 0. Otherwise, this list will include every
     * filter density supported by the camera device, in ascending order.</p>
     */
    ACAMERA_LENS_INFO_AVAILABLE_FILTER_DENSITIES =              // float[n]
            ACAMERA_LENS_INFO_START + 1,
    /**
     * <p>List of focal lengths for ACAMERA_LENS_FOCAL_LENGTH that are supported by this camera
     * device.</p>
     *
     * @see ACAMERA_LENS_FOCAL_LENGTH
     *
     * <p>Type: float[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If optical zoom is not supported, this list will only contain
     * a single value corresponding to the fixed focal length of the
     * device. Otherwise, this list will include every focal length supported
     * by the camera device, in ascending order.</p>
     */
    ACAMERA_LENS_INFO_AVAILABLE_FOCAL_LENGTHS =                 // float[n]
            ACAMERA_LENS_INFO_START + 2,
    /**
     * <p>List of optical image stabilization (OIS) modes for
     * ACAMERA_LENS_OPTICAL_STABILIZATION_MODE that are supported by this camera device.</p>
     *
     * @see ACAMERA_LENS_OPTICAL_STABILIZATION_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If OIS is not supported by a given camera device, this list will
     * contain only OFF.</p>
     */
    ACAMERA_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION =         // byte[n]
            ACAMERA_LENS_INFO_START + 3,
    /**
     * <p>Hyperfocal distance for this lens.</p>
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If the lens is not fixed focus, the camera device will report this
     * field when ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION is APPROXIMATE or CALIBRATED.</p>
     *
     * @see ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION
     */
    ACAMERA_LENS_INFO_HYPERFOCAL_DISTANCE =                     // float
            ACAMERA_LENS_INFO_START + 4,
    /**
     * <p>Shortest distance from frontmost surface
     * of the lens that can be brought into sharp focus.</p>
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If the lens is fixed-focus, this will be
     * 0.</p>
     */
    ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE =                  // float
            ACAMERA_LENS_INFO_START + 5,
    /**
     * <p>Dimensions of lens shading map.</p>
     *
     * <p>Type: int32[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The map should be on the order of 30-40 rows and columns, and
     * must be smaller than 64x64.</p>
     */
    ACAMERA_LENS_INFO_SHADING_MAP_SIZE =                        // int32[2]
            ACAMERA_LENS_INFO_START + 6,
    /**
     * <p>The lens focus distance calibration quality.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_lens_info_focus_distance_calibration_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The lens focus distance calibration quality determines the reliability of
     * focus related metadata entries, i.e. ACAMERA_LENS_FOCUS_DISTANCE,
     * ACAMERA_LENS_FOCUS_RANGE, ACAMERA_LENS_INFO_HYPERFOCAL_DISTANCE, and
     * ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE.</p>
     * <p>APPROXIMATE and CALIBRATED devices report the focus metadata in
     * units of diopters (1/meter), so <code>0.0f</code> represents focusing at infinity,
     * and increasing positive numbers represent focusing closer and closer
     * to the camera device. The focus distance control also uses diopters
     * on these devices.</p>
     * <p>UNCALIBRATED devices do not use units that are directly comparable
     * to any real physical measurement, but <code>0.0f</code> still represents farthest
     * focus, and ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE represents the
     * nearest focus the device can achieve.</p>
     *
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     * @see ACAMERA_LENS_FOCUS_RANGE
     * @see ACAMERA_LENS_INFO_HYPERFOCAL_DISTANCE
     * @see ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE
     */
    ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION =              // byte (acamera_metadata_enum_android_lens_info_focus_distance_calibration_t)
            ACAMERA_LENS_INFO_START + 7,
    ACAMERA_LENS_INFO_END,

    /**
     * <p>Mode of operation for the noise reduction algorithm.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_noise_reduction_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>The noise reduction algorithm attempts to improve image quality by removing
     * excessive noise added by the capture process, especially in dark conditions.</p>
     * <p>OFF means no noise reduction will be applied by the camera device, for both raw and
     * YUV domain.</p>
     * <p>MINIMAL means that only sensor raw domain basic noise reduction is enabled ,to remove
     * demosaicing or other processing artifacts. For YUV_REPROCESSING, MINIMAL is same as OFF.
     * This mode is optional, may not be support by all devices. The application should check
     * ACAMERA_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES before using it.</p>
     * <p>FAST/HIGH_QUALITY both mean camera device determined noise filtering
     * will be applied. HIGH_QUALITY mode indicates that the camera device
     * will use the highest-quality noise filtering algorithms,
     * even if it slows down capture rate. FAST means the camera device will not
     * slow down capture rate when applying noise filtering. FAST may be the same as MINIMAL if
     * MINIMAL is listed, or the same as OFF if any noise filtering will slow down capture rate.
     * Every output stream will have a similar amount of enhancement applied.</p>
     * <p>ZERO_SHUTTER_LAG is meant to be used by applications that maintain a continuous circular
     * buffer of high-resolution images during preview and reprocess image(s) from that buffer
     * into a final capture when triggered by the user. In this mode, the camera device applies
     * noise reduction to low-resolution streams (below maximum recording resolution) to maximize
     * preview quality, but does not apply noise reduction to high-resolution streams, since
     * those will be reprocessed later if necessary.</p>
     * <p>For YUV_REPROCESSING, these FAST/HIGH_QUALITY modes both mean that the camera device
     * will apply FAST/HIGH_QUALITY YUV domain noise reduction, respectively. The camera device
     * may adjust the noise reduction parameters for best image quality based on the
     * android.reprocess.effectiveExposureFactor if it is set.</p>
     *
     * @see ACAMERA_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES
     */
    ACAMERA_NOISE_REDUCTION_MODE =                              // byte (acamera_metadata_enum_android_noise_reduction_mode_t)
            ACAMERA_NOISE_REDUCTION_START,
    /**
     * <p>List of noise reduction modes for ACAMERA_NOISE_REDUCTION_MODE that are supported
     * by this camera device.</p>
     *
     * @see ACAMERA_NOISE_REDUCTION_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Full-capability camera devices will always support OFF and FAST.</p>
     * <p>Camera devices that support YUV_REPROCESSING or PRIVATE_REPROCESSING will support
     * ZERO_SHUTTER_LAG.</p>
     * <p>Legacy-capability camera devices will only support FAST mode.</p>
     */
    ACAMERA_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES =   // byte[n]
            ACAMERA_NOISE_REDUCTION_START + 2,
    ACAMERA_NOISE_REDUCTION_END,

    /**
     * <p>The maximum numbers of different types of output streams
     * that can be configured and used simultaneously by a camera device.</p>
     *
     * <p>Type: int32[3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This is a 3 element tuple that contains the max number of output simultaneous
     * streams for raw sensor, processed (but not stalling), and processed (and stalling)
     * formats respectively. For example, assuming that JPEG is typically a processed and
     * stalling stream, if max raw sensor format output stream number is 1, max YUV streams
     * number is 3, and max JPEG stream number is 2, then this tuple should be <code>(1, 3, 2)</code>.</p>
     * <p>This lists the upper bound of the number of output streams supported by
     * the camera device. Using more streams simultaneously may require more hardware and
     * CPU resources that will consume more power. The image format for an output stream can
     * be any supported format provided by ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS.
     * The formats defined in ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS can be catergorized
     * into the 3 stream types as below:</p>
     * <ul>
     * <li>Processed (but stalling): any non-RAW format with a stallDurations &gt; 0.
     *   Typically {@link AIMAGE_FORMAT_JPEG} format.</li>
     * <li>Raw formats: {@link AIMAGE_FORMAT_RAW16}, {@link AIMAGE_FORMAT_RAW10}, or
     *   {@link AIMAGE_FORMAT_RAW12}.</li>
     * <li>Processed (but not-stalling): any non-RAW format without a stall duration.
     *   Typically {@link AIMAGE_FORMAT_YUV_420_888}.</li>
     * </ul>
     *
     * @see ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS
     */
    ACAMERA_REQUEST_MAX_NUM_OUTPUT_STREAMS =                    // int32[3]
            ACAMERA_REQUEST_START + 6,
    /**
     * <p>Specifies the number of pipeline stages the frame went
     * through from when it was exposed to when the final completed result
     * was available to the framework.</p>
     *
     * <p>Type: byte</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Depending on what settings are used in the request, and
     * what streams are configured, the data may undergo less processing,
     * and some pipeline stages skipped.</p>
     * <p>See ACAMERA_REQUEST_PIPELINE_MAX_DEPTH for more details.</p>
     *
     * @see ACAMERA_REQUEST_PIPELINE_MAX_DEPTH
     */
    ACAMERA_REQUEST_PIPELINE_DEPTH =                            // byte
            ACAMERA_REQUEST_START + 9,
    /**
     * <p>Specifies the number of maximum pipeline stages a frame
     * has to go through from when it's exposed to when it's available
     * to the framework.</p>
     *
     * <p>Type: byte</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>A typical minimum value for this is 2 (one stage to expose,
     * one stage to readout) from the sensor. The ISP then usually adds
     * its own stages to do custom HW processing. Further stages may be
     * added by SW processing.</p>
     * <p>Depending on what settings are used (e.g. YUV, JPEG) and what
     * processing is enabled (e.g. face detection), the actual pipeline
     * depth (specified by ACAMERA_REQUEST_PIPELINE_DEPTH) may be less than
     * the max pipeline depth.</p>
     * <p>A pipeline depth of X stages is equivalent to a pipeline latency of
     * X frame intervals.</p>
     * <p>This value will normally be 8 or less, however, for high speed capture session,
     * the max pipeline depth will be up to 8 x size of high speed capture request list.</p>
     *
     * @see ACAMERA_REQUEST_PIPELINE_DEPTH
     */
    ACAMERA_REQUEST_PIPELINE_MAX_DEPTH =                        // byte
            ACAMERA_REQUEST_START + 10,
    /**
     * <p>Defines how many sub-components
     * a result will be composed of.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>In order to combat the pipeline latency, partial results
     * may be delivered to the application layer from the camera device as
     * soon as they are available.</p>
     * <p>Optional; defaults to 1. A value of 1 means that partial
     * results are not supported, and only the final TotalCaptureResult will
     * be produced by the camera device.</p>
     * <p>A typical use case for this might be: after requesting an
     * auto-focus (AF) lock the new AF state might be available 50%
     * of the way through the pipeline.  The camera device could
     * then immediately dispatch this state via a partial result to
     * the application, and the rest of the metadata via later
     * partial results.</p>
     */
    ACAMERA_REQUEST_PARTIAL_RESULT_COUNT =                      // int32
            ACAMERA_REQUEST_START + 11,
    /**
     * <p>List of capabilities that this camera device
     * advertises as fully supporting.</p>
     *
     * <p>Type: byte[n] (acamera_metadata_enum_android_request_available_capabilities_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>A capability is a contract that the camera device makes in order
     * to be able to satisfy one or more use cases.</p>
     * <p>Listing a capability guarantees that the whole set of features
     * required to support a common use will all be available.</p>
     * <p>Using a subset of the functionality provided by an unsupported
     * capability may be possible on a specific camera device implementation;
     * to do this query each of ACAMERA_REQUEST_AVAILABLE_REQUEST_KEYS,
     * ACAMERA_REQUEST_AVAILABLE_RESULT_KEYS,
     * ACAMERA_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS.</p>
     * <p>The following capabilities are guaranteed to be available on
     * ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL <code>==</code> FULL devices:</p>
     * <ul>
     * <li>MANUAL_SENSOR</li>
     * <li>MANUAL_POST_PROCESSING</li>
     * </ul>
     * <p>Other capabilities may be available on either FULL or LIMITED
     * devices, but the application should query this key to be sure.</p>
     *
     * @see ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL
     * @see ACAMERA_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS
     * @see ACAMERA_REQUEST_AVAILABLE_REQUEST_KEYS
     * @see ACAMERA_REQUEST_AVAILABLE_RESULT_KEYS
     */
    ACAMERA_REQUEST_AVAILABLE_CAPABILITIES =                    // byte[n] (acamera_metadata_enum_android_request_available_capabilities_t)
            ACAMERA_REQUEST_START + 12,
    /**
     * <p>A list of all keys that the camera device has available
     * to use with {@link ACaptureRequest}.</p>
     *
     * <p>Type: int32[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Attempting to set a key into a CaptureRequest that is not
     * listed here will result in an invalid request and will be rejected
     * by the camera device.</p>
     * <p>This field can be used to query the feature set of a camera device
     * at a more granular level than capabilities. This is especially
     * important for optional keys that are not listed under any capability
     * in ACAMERA_REQUEST_AVAILABLE_CAPABILITIES.</p>
     *
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     */
    ACAMERA_REQUEST_AVAILABLE_REQUEST_KEYS =                    // int32[n]
            ACAMERA_REQUEST_START + 13,
    /**
     * <p>A list of all keys that the camera device has available
     * to query with {@link ACameraMetadata} from
     * {@link ACameraCaptureSession_captureCallback_result}.</p>
     *
     * <p>Type: int32[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Attempting to get a key from a CaptureResult that is not
     * listed here will always return a <code>null</code> value. Getting a key from
     * a CaptureResult that is listed here will generally never return a <code>null</code>
     * value.</p>
     * <p>The following keys may return <code>null</code> unless they are enabled:</p>
     * <ul>
     * <li>ACAMERA_STATISTICS_LENS_SHADING_MAP (non-null iff ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE == ON)</li>
     * </ul>
     * <p>(Those sometimes-null keys will nevertheless be listed here
     * if they are available.)</p>
     * <p>This field can be used to query the feature set of a camera device
     * at a more granular level than capabilities. This is especially
     * important for optional keys that are not listed under any capability
     * in ACAMERA_REQUEST_AVAILABLE_CAPABILITIES.</p>
     *
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     * @see ACAMERA_STATISTICS_LENS_SHADING_MAP
     * @see ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE
     */
    ACAMERA_REQUEST_AVAILABLE_RESULT_KEYS =                     // int32[n]
            ACAMERA_REQUEST_START + 14,
    /**
     * <p>A list of all keys that the camera device has available
     * to query with {@link ACameraMetadata} from
     * {@link ACameraManager_getCameraCharacteristics}.</p>
     *
     * <p>Type: int32[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This entry follows the same rules as
     * ACAMERA_REQUEST_AVAILABLE_RESULT_KEYS (except that it applies for
     * CameraCharacteristics instead of CaptureResult). See above for more
     * details.</p>
     *
     * @see ACAMERA_REQUEST_AVAILABLE_RESULT_KEYS
     */
    ACAMERA_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS =            // int32[n]
            ACAMERA_REQUEST_START + 15,
    ACAMERA_REQUEST_END,

    /**
     * <p>The desired region of the sensor to read out for this capture.</p>
     *
     * <p>Type: int32[4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>This control can be used to implement digital zoom.</p>
     * <p>The data representation is int[4], which maps to (left, top, width, height).</p>
     * <p>The crop region coordinate system is based off
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE, with <code>(0, 0)</code> being the
     * top-left corner of the sensor active array.</p>
     * <p>Output streams use this rectangle to produce their output,
     * cropping to a smaller region if necessary to maintain the
     * stream's aspect ratio, then scaling the sensor input to
     * match the output's configured resolution.</p>
     * <p>The crop region is applied after the RAW to other color
     * space (e.g. YUV) conversion. Since raw streams
     * (e.g. RAW16) don't have the conversion stage, they are not
     * croppable. The crop region will be ignored by raw streams.</p>
     * <p>For non-raw streams, any additional per-stream cropping will
     * be done to maximize the final pixel area of the stream.</p>
     * <p>For example, if the crop region is set to a 4:3 aspect
     * ratio, then 4:3 streams will use the exact crop
     * region. 16:9 streams will further crop vertically
     * (letterbox).</p>
     * <p>Conversely, if the crop region is set to a 16:9, then 4:3
     * outputs will crop horizontally (pillarbox), and 16:9
     * streams will match exactly. These additional crops will
     * be centered within the crop region.</p>
     * <p>The width and height of the crop region cannot
     * be set to be smaller than
     * <code>floor( activeArraySize.width / ACAMERA_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM )</code> and
     * <code>floor( activeArraySize.height / ACAMERA_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM )</code>, respectively.</p>
     * <p>The camera device may adjust the crop region to account
     * for rounding and other hardware requirements; the final
     * crop region used will be included in the output capture
     * result.</p>
     *
     * @see ACAMERA_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_SCALER_CROP_REGION =                                // int32[4]
            ACAMERA_SCALER_START,
    /**
     * <p>The maximum ratio between both active area width
     * and crop region width, and active area height and
     * crop region height, for ACAMERA_SCALER_CROP_REGION.</p>
     *
     * @see ACAMERA_SCALER_CROP_REGION
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This represents the maximum amount of zooming possible by
     * the camera device, or equivalently, the minimum cropping
     * window size.</p>
     * <p>Crop regions that have a width or height that is smaller
     * than this ratio allows will be rounded up to the minimum
     * allowed size by the camera device.</p>
     */
    ACAMERA_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM =                 // float
            ACAMERA_SCALER_START + 4,
    /**
     * <p>The available stream configurations that this
     * camera device supports
     * (i.e. format, width, height, output/input stream).</p>
     *
     * <p>Type: int32[n*4] (acamera_metadata_enum_android_scaler_available_stream_configurations_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The configurations are listed as <code>(format, width, height, input?)</code>
     * tuples.</p>
     * <p>For a given use case, the actual maximum supported resolution
     * may be lower than what is listed here, depending on the destination
     * Surface for the image data. For example, for recording video,
     * the video encoder chosen may have a maximum size limit (e.g. 1080p)
     * smaller than what the camera (e.g. maximum resolution is 3264x2448)
     * can provide.</p>
     * <p>Please reference the documentation for the image data destination to
     * check if it limits the maximum size for image data.</p>
     * <p>Not all output formats may be supported in a configuration with
     * an input stream of a particular format. For more details, see
     * android.scaler.availableInputOutputFormatsMap.</p>
     * <p>The following table describes the minimum required output stream
     * configurations based on the hardware level
     * (ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL):</p>
     * <p>Format         | Size                                         | Hardware Level | Notes
     * :-------------:|:--------------------------------------------:|:--------------:|:--------------:
     * JPEG           | ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE          | Any            |
     * JPEG           | 1920x1080 (1080p)                            | Any            | if 1080p &lt;= activeArraySize
     * JPEG           | 1280x720 (720)                               | Any            | if 720p &lt;= activeArraySize
     * JPEG           | 640x480 (480p)                               | Any            | if 480p &lt;= activeArraySize
     * JPEG           | 320x240 (240p)                               | Any            | if 240p &lt;= activeArraySize
     * YUV_420_888    | all output sizes available for JPEG          | FULL           |
     * YUV_420_888    | all output sizes available for JPEG, up to the maximum video size | LIMITED        |
     * IMPLEMENTATION_DEFINED | same as YUV_420_888                  | Any            |</p>
     * <p>Refer to ACAMERA_REQUEST_AVAILABLE_CAPABILITIES for additional
     * mandatory stream configurations on a per-capability basis.</p>
     *
     * @see ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS =            // int32[n*4] (acamera_metadata_enum_android_scaler_available_stream_configurations_t)
            ACAMERA_SCALER_START + 10,
    /**
     * <p>This lists the minimum frame duration for each
     * format/size combination.</p>
     *
     * <p>Type: int64[4*n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This should correspond to the frame duration when only that
     * stream is active, with all processing (typically in android.*.mode)
     * set to either OFF or FAST.</p>
     * <p>When multiple streams are used in a request, the minimum frame
     * duration will be max(individual stream min durations).</p>
     * <p>The minimum frame duration of a stream (of a particular format, size)
     * is the same regardless of whether the stream is input or output.</p>
     * <p>See ACAMERA_SENSOR_FRAME_DURATION and
     * ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS for more details about
     * calculating the max frame rate.</p>
     *
     * @see ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS
     * @see ACAMERA_SENSOR_FRAME_DURATION
     */
    ACAMERA_SCALER_AVAILABLE_MIN_FRAME_DURATIONS =              // int64[4*n]
            ACAMERA_SCALER_START + 11,
    /**
     * <p>This lists the maximum stall duration for each
     * output format/size combination.</p>
     *
     * <p>Type: int64[4*n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>A stall duration is how much extra time would get added
     * to the normal minimum frame duration for a repeating request
     * that has streams with non-zero stall.</p>
     * <p>For example, consider JPEG captures which have the following
     * characteristics:</p>
     * <ul>
     * <li>JPEG streams act like processed YUV streams in requests for which
     * they are not included; in requests in which they are directly
     * referenced, they act as JPEG streams. This is because supporting a
     * JPEG stream requires the underlying YUV data to always be ready for
     * use by a JPEG encoder, but the encoder will only be used (and impact
     * frame duration) on requests that actually reference a JPEG stream.</li>
     * <li>The JPEG processor can run concurrently to the rest of the camera
     * pipeline, but cannot process more than 1 capture at a time.</li>
     * </ul>
     * <p>In other words, using a repeating YUV request would result
     * in a steady frame rate (let's say it's 30 FPS). If a single
     * JPEG request is submitted periodically, the frame rate will stay
     * at 30 FPS (as long as we wait for the previous JPEG to return each
     * time). If we try to submit a repeating YUV + JPEG request, then
     * the frame rate will drop from 30 FPS.</p>
     * <p>In general, submitting a new request with a non-0 stall time
     * stream will <em>not</em> cause a frame rate drop unless there are still
     * outstanding buffers for that stream from previous requests.</p>
     * <p>Submitting a repeating request with streams (call this <code>S</code>)
     * is the same as setting the minimum frame duration from
     * the normal minimum frame duration corresponding to <code>S</code>, added with
     * the maximum stall duration for <code>S</code>.</p>
     * <p>If interleaving requests with and without a stall duration,
     * a request will stall by the maximum of the remaining times
     * for each can-stall stream with outstanding buffers.</p>
     * <p>This means that a stalling request will not have an exposure start
     * until the stall has completed.</p>
     * <p>This should correspond to the stall duration when only that stream is
     * active, with all processing (typically in android.*.mode) set to FAST
     * or OFF. Setting any of the processing modes to HIGH_QUALITY
     * effectively results in an indeterminate stall duration for all
     * streams in a request (the regular stall calculation rules are
     * ignored).</p>
     * <p>The following formats may always have a stall duration:</p>
     * <ul>
     * <li>{@link AIMAGE_FORMAT_JPEG}</li>
     * <li>{@link AIMAGE_FORMAT_RAW16}</li>
     * </ul>
     * <p>The following formats will never have a stall duration:</p>
     * <ul>
     * <li>{@link AIMAGE_FORMAT_YUV_420_888}</li>
     * <li>{@link AIMAGE_FORMAT_RAW10}</li>
     * </ul>
     * <p>All other formats may or may not have an allowed stall duration on
     * a per-capability basis; refer to ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     * for more details.</p>
     * <p>See ACAMERA_SENSOR_FRAME_DURATION for more information about
     * calculating the max frame rate (absent stalls).</p>
     *
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     * @see ACAMERA_SENSOR_FRAME_DURATION
     */
    ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS =                  // int64[4*n]
            ACAMERA_SCALER_START + 12,
    /**
     * <p>The crop type that this camera device supports.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_scaler_cropping_type_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>When passing a non-centered crop region (ACAMERA_SCALER_CROP_REGION) to a camera
     * device that only supports CENTER_ONLY cropping, the camera device will move the
     * crop region to the center of the sensor active array (ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE)
     * and keep the crop region width and height unchanged. The camera device will return the
     * final used crop region in metadata result ACAMERA_SCALER_CROP_REGION.</p>
     * <p>Camera devices that support FREEFORM cropping will support any crop region that
     * is inside of the active array. The camera device will apply the same crop region and
     * return the final used crop region in capture result metadata ACAMERA_SCALER_CROP_REGION.</p>
     * <p>LEGACY capability devices will only support CENTER_ONLY cropping.</p>
     *
     * @see ACAMERA_SCALER_CROP_REGION
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_SCALER_CROPPING_TYPE =                              // byte (acamera_metadata_enum_android_scaler_cropping_type_t)
            ACAMERA_SCALER_START + 13,
    ACAMERA_SCALER_END,

    /**
     * <p>Duration each pixel is exposed to
     * light.</p>
     *
     * <p>Type: int64</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>If the sensor can't expose this exact duration, it will shorten the
     * duration exposed to the nearest possible value (rather than expose longer).
     * The final exposure time used will be available in the output capture result.</p>
     * <p>This control is only effective if ACAMERA_CONTROL_AE_MODE or ACAMERA_CONTROL_MODE is set to
     * OFF; otherwise the auto-exposure algorithm will override this value.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_MODE
     */
    ACAMERA_SENSOR_EXPOSURE_TIME =                              // int64
            ACAMERA_SENSOR_START,
    /**
     * <p>Duration from start of frame exposure to
     * start of next frame exposure.</p>
     *
     * <p>Type: int64</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>The maximum frame rate that can be supported by a camera subsystem is
     * a function of many factors:</p>
     * <ul>
     * <li>Requested resolutions of output image streams</li>
     * <li>Availability of binning / skipping modes on the imager</li>
     * <li>The bandwidth of the imager interface</li>
     * <li>The bandwidth of the various ISP processing blocks</li>
     * </ul>
     * <p>Since these factors can vary greatly between different ISPs and
     * sensors, the camera abstraction tries to represent the bandwidth
     * restrictions with as simple a model as possible.</p>
     * <p>The model presented has the following characteristics:</p>
     * <ul>
     * <li>The image sensor is always configured to output the smallest
     * resolution possible given the application's requested output stream
     * sizes.  The smallest resolution is defined as being at least as large
     * as the largest requested output stream size; the camera pipeline must
     * never digitally upsample sensor data when the crop region covers the
     * whole sensor. In general, this means that if only small output stream
     * resolutions are configured, the sensor can provide a higher frame
     * rate.</li>
     * <li>Since any request may use any or all the currently configured
     * output streams, the sensor and ISP must be configured to support
     * scaling a single capture to all the streams at the same time.  This
     * means the camera pipeline must be ready to produce the largest
     * requested output size without any delay.  Therefore, the overall
     * frame rate of a given configured stream set is governed only by the
     * largest requested stream resolution.</li>
     * <li>Using more than one output stream in a request does not affect the
     * frame duration.</li>
     * <li>Certain format-streams may need to do additional background processing
     * before data is consumed/produced by that stream. These processors
     * can run concurrently to the rest of the camera pipeline, but
     * cannot process more than 1 capture at a time.</li>
     * </ul>
     * <p>The necessary information for the application, given the model above,
     * is provided via
     * {@link ACAMERA_SCALER_AVAILABLE_MIN_FRAME_DURATIONS}.
     * These are used to determine the maximum frame rate / minimum frame
     * duration that is possible for a given stream configuration.</p>
     * <p>Specifically, the application can use the following rules to
     * determine the minimum frame duration it can request from the camera
     * device:</p>
     * <ol>
     * <li>Let the set of currently configured input/output streams
     * be called <code>S</code>.</li>
     * <li>Find the minimum frame durations for each stream in <code>S</code>, by looking
     * it up in {@link ACAMERA_SCALER_AVAILABLE_MIN_FRAME_DURATIONS}
     * (with its respective size/format). Let this set of frame durations be
     * called <code>F</code>.</li>
     * <li>For any given request <code>R</code>, the minimum frame duration allowed
     * for <code>R</code> is the maximum out of all values in <code>F</code>. Let the streams
     * used in <code>R</code> be called <code>S_r</code>.</li>
     * </ol>
     * <p>If none of the streams in <code>S_r</code> have a stall time (listed in {@link
     * ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS}
     * using its respective size/format), then the frame duration in <code>F</code>
     * determines the steady state frame rate that the application will get
     * if it uses <code>R</code> as a repeating request. Let this special kind of
     * request be called <code>Rsimple</code>.</p>
     * <p>A repeating request <code>Rsimple</code> can be <em>occasionally</em> interleaved
     * by a single capture of a new request <code>Rstall</code> (which has at least
     * one in-use stream with a non-0 stall time) and if <code>Rstall</code> has the
     * same minimum frame duration this will not cause a frame rate loss
     * if all buffers from the previous <code>Rstall</code> have already been
     * delivered.</p>
     * <p>For more details about stalling, see
     * {@link ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS}.</p>
     * <p>This control is only effective if ACAMERA_CONTROL_AE_MODE or ACAMERA_CONTROL_MODE is set to
     * OFF; otherwise the auto-exposure algorithm will override this value.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_MODE
     */
    ACAMERA_SENSOR_FRAME_DURATION =                             // int64
            ACAMERA_SENSOR_START + 1,
    /**
     * <p>The amount of gain applied to sensor data
     * before processing.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>The sensitivity is the standard ISO sensitivity value,
     * as defined in ISO 12232:2006.</p>
     * <p>The sensitivity must be within ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, and
     * if if it less than ACAMERA_SENSOR_MAX_ANALOG_SENSITIVITY, the camera device
     * is guaranteed to use only analog amplification for applying the gain.</p>
     * <p>If the camera device cannot apply the exact sensitivity
     * requested, it will reduce the gain to the nearest supported
     * value. The final sensitivity used will be available in the
     * output capture result.</p>
     * <p>This control is only effective if ACAMERA_CONTROL_AE_MODE or ACAMERA_CONTROL_MODE is set to
     * OFF; otherwise the auto-exposure algorithm will override this value.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_MODE
     * @see ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE
     * @see ACAMERA_SENSOR_MAX_ANALOG_SENSITIVITY
     */
    ACAMERA_SENSOR_SENSITIVITY =                                // int32
            ACAMERA_SENSOR_START + 2,
    /**
     * <p>The standard reference illuminant used as the scene light source when
     * calculating the ACAMERA_SENSOR_COLOR_TRANSFORM1,
     * ACAMERA_SENSOR_CALIBRATION_TRANSFORM1, and
     * ACAMERA_SENSOR_FORWARD_MATRIX1 matrices.</p>
     *
     * @see ACAMERA_SENSOR_CALIBRATION_TRANSFORM1
     * @see ACAMERA_SENSOR_COLOR_TRANSFORM1
     * @see ACAMERA_SENSOR_FORWARD_MATRIX1
     *
     * <p>Type: byte (acamera_metadata_enum_android_sensor_reference_illuminant1_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The values in this key correspond to the values defined for the
     * EXIF LightSource tag. These illuminants are standard light sources
     * that are often used calibrating camera devices.</p>
     * <p>If this key is present, then ACAMERA_SENSOR_COLOR_TRANSFORM1,
     * ACAMERA_SENSOR_CALIBRATION_TRANSFORM1, and
     * ACAMERA_SENSOR_FORWARD_MATRIX1 will also be present.</p>
     * <p>Some devices may choose to provide a second set of calibration
     * information for improved quality, including
     * ACAMERA_SENSOR_REFERENCE_ILLUMINANT2 and its corresponding matrices.</p>
     *
     * @see ACAMERA_SENSOR_CALIBRATION_TRANSFORM1
     * @see ACAMERA_SENSOR_COLOR_TRANSFORM1
     * @see ACAMERA_SENSOR_FORWARD_MATRIX1
     * @see ACAMERA_SENSOR_REFERENCE_ILLUMINANT2
     */
    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1 =                      // byte (acamera_metadata_enum_android_sensor_reference_illuminant1_t)
            ACAMERA_SENSOR_START + 3,
    /**
     * <p>The standard reference illuminant used as the scene light source when
     * calculating the ACAMERA_SENSOR_COLOR_TRANSFORM2,
     * ACAMERA_SENSOR_CALIBRATION_TRANSFORM2, and
     * ACAMERA_SENSOR_FORWARD_MATRIX2 matrices.</p>
     *
     * @see ACAMERA_SENSOR_CALIBRATION_TRANSFORM2
     * @see ACAMERA_SENSOR_COLOR_TRANSFORM2
     * @see ACAMERA_SENSOR_FORWARD_MATRIX2
     *
     * <p>Type: byte</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>See ACAMERA_SENSOR_REFERENCE_ILLUMINANT1 for more details.</p>
     * <p>If this key is present, then ACAMERA_SENSOR_COLOR_TRANSFORM2,
     * ACAMERA_SENSOR_CALIBRATION_TRANSFORM2, and
     * ACAMERA_SENSOR_FORWARD_MATRIX2 will also be present.</p>
     *
     * @see ACAMERA_SENSOR_CALIBRATION_TRANSFORM2
     * @see ACAMERA_SENSOR_COLOR_TRANSFORM2
     * @see ACAMERA_SENSOR_FORWARD_MATRIX2
     * @see ACAMERA_SENSOR_REFERENCE_ILLUMINANT1
     */
    ACAMERA_SENSOR_REFERENCE_ILLUMINANT2 =                      // byte
            ACAMERA_SENSOR_START + 4,
    /**
     * <p>A per-device calibration transform matrix that maps from the
     * reference sensor colorspace to the actual device sensor colorspace.</p>
     *
     * <p>Type: rational[3*3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This matrix is used to correct for per-device variations in the
     * sensor colorspace, and is used for processing raw buffer data.</p>
     * <p>The matrix is expressed as a 3x3 matrix in row-major-order, and
     * contains a per-device calibration transform that maps colors
     * from reference sensor color space (i.e. the "golden module"
     * colorspace) into this camera device's native sensor color
     * space under the first reference illuminant
     * (ACAMERA_SENSOR_REFERENCE_ILLUMINANT1).</p>
     *
     * @see ACAMERA_SENSOR_REFERENCE_ILLUMINANT1
     */
    ACAMERA_SENSOR_CALIBRATION_TRANSFORM1 =                     // rational[3*3]
            ACAMERA_SENSOR_START + 5,
    /**
     * <p>A per-device calibration transform matrix that maps from the
     * reference sensor colorspace to the actual device sensor colorspace
     * (this is the colorspace of the raw buffer data).</p>
     *
     * <p>Type: rational[3*3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This matrix is used to correct for per-device variations in the
     * sensor colorspace, and is used for processing raw buffer data.</p>
     * <p>The matrix is expressed as a 3x3 matrix in row-major-order, and
     * contains a per-device calibration transform that maps colors
     * from reference sensor color space (i.e. the "golden module"
     * colorspace) into this camera device's native sensor color
     * space under the second reference illuminant
     * (ACAMERA_SENSOR_REFERENCE_ILLUMINANT2).</p>
     * <p>This matrix will only be present if the second reference
     * illuminant is present.</p>
     *
     * @see ACAMERA_SENSOR_REFERENCE_ILLUMINANT2
     */
    ACAMERA_SENSOR_CALIBRATION_TRANSFORM2 =                     // rational[3*3]
            ACAMERA_SENSOR_START + 6,
    /**
     * <p>A matrix that transforms color values from CIE XYZ color space to
     * reference sensor color space.</p>
     *
     * <p>Type: rational[3*3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This matrix is used to convert from the standard CIE XYZ color
     * space to the reference sensor colorspace, and is used when processing
     * raw buffer data.</p>
     * <p>The matrix is expressed as a 3x3 matrix in row-major-order, and
     * contains a color transform matrix that maps colors from the CIE
     * XYZ color space to the reference sensor color space (i.e. the
     * "golden module" colorspace) under the first reference illuminant
     * (ACAMERA_SENSOR_REFERENCE_ILLUMINANT1).</p>
     * <p>The white points chosen in both the reference sensor color space
     * and the CIE XYZ colorspace when calculating this transform will
     * match the standard white point for the first reference illuminant
     * (i.e. no chromatic adaptation will be applied by this transform).</p>
     *
     * @see ACAMERA_SENSOR_REFERENCE_ILLUMINANT1
     */
    ACAMERA_SENSOR_COLOR_TRANSFORM1 =                           // rational[3*3]
            ACAMERA_SENSOR_START + 7,
    /**
     * <p>A matrix that transforms color values from CIE XYZ color space to
     * reference sensor color space.</p>
     *
     * <p>Type: rational[3*3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This matrix is used to convert from the standard CIE XYZ color
     * space to the reference sensor colorspace, and is used when processing
     * raw buffer data.</p>
     * <p>The matrix is expressed as a 3x3 matrix in row-major-order, and
     * contains a color transform matrix that maps colors from the CIE
     * XYZ color space to the reference sensor color space (i.e. the
     * "golden module" colorspace) under the second reference illuminant
     * (ACAMERA_SENSOR_REFERENCE_ILLUMINANT2).</p>
     * <p>The white points chosen in both the reference sensor color space
     * and the CIE XYZ colorspace when calculating this transform will
     * match the standard white point for the second reference illuminant
     * (i.e. no chromatic adaptation will be applied by this transform).</p>
     * <p>This matrix will only be present if the second reference
     * illuminant is present.</p>
     *
     * @see ACAMERA_SENSOR_REFERENCE_ILLUMINANT2
     */
    ACAMERA_SENSOR_COLOR_TRANSFORM2 =                           // rational[3*3]
            ACAMERA_SENSOR_START + 8,
    /**
     * <p>A matrix that transforms white balanced camera colors from the reference
     * sensor colorspace to the CIE XYZ colorspace with a D50 whitepoint.</p>
     *
     * <p>Type: rational[3*3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This matrix is used to convert to the standard CIE XYZ colorspace, and
     * is used when processing raw buffer data.</p>
     * <p>This matrix is expressed as a 3x3 matrix in row-major-order, and contains
     * a color transform matrix that maps white balanced colors from the
     * reference sensor color space to the CIE XYZ color space with a D50 white
     * point.</p>
     * <p>Under the first reference illuminant (ACAMERA_SENSOR_REFERENCE_ILLUMINANT1)
     * this matrix is chosen so that the standard white point for this reference
     * illuminant in the reference sensor colorspace is mapped to D50 in the
     * CIE XYZ colorspace.</p>
     *
     * @see ACAMERA_SENSOR_REFERENCE_ILLUMINANT1
     */
    ACAMERA_SENSOR_FORWARD_MATRIX1 =                            // rational[3*3]
            ACAMERA_SENSOR_START + 9,
    /**
     * <p>A matrix that transforms white balanced camera colors from the reference
     * sensor colorspace to the CIE XYZ colorspace with a D50 whitepoint.</p>
     *
     * <p>Type: rational[3*3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This matrix is used to convert to the standard CIE XYZ colorspace, and
     * is used when processing raw buffer data.</p>
     * <p>This matrix is expressed as a 3x3 matrix in row-major-order, and contains
     * a color transform matrix that maps white balanced colors from the
     * reference sensor color space to the CIE XYZ color space with a D50 white
     * point.</p>
     * <p>Under the second reference illuminant (ACAMERA_SENSOR_REFERENCE_ILLUMINANT2)
     * this matrix is chosen so that the standard white point for this reference
     * illuminant in the reference sensor colorspace is mapped to D50 in the
     * CIE XYZ colorspace.</p>
     * <p>This matrix will only be present if the second reference
     * illuminant is present.</p>
     *
     * @see ACAMERA_SENSOR_REFERENCE_ILLUMINANT2
     */
    ACAMERA_SENSOR_FORWARD_MATRIX2 =                            // rational[3*3]
            ACAMERA_SENSOR_START + 10,
    /**
     * <p>A fixed black level offset for each of the color filter arrangement
     * (CFA) mosaic channels.</p>
     *
     * <p>Type: int32[4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This key specifies the zero light value for each of the CFA mosaic
     * channels in the camera sensor.  The maximal value output by the
     * sensor is represented by the value in ACAMERA_SENSOR_INFO_WHITE_LEVEL.</p>
     * <p>The values are given in the same order as channels listed for the CFA
     * layout key (see ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT), i.e. the
     * nth value given corresponds to the black level offset for the nth
     * color channel listed in the CFA.</p>
     * <p>The black level values of captured images may vary for different
     * capture settings (e.g., ACAMERA_SENSOR_SENSITIVITY). This key
     * represents a coarse approximation for such case. It is recommended to
     * use ACAMERA_SENSOR_DYNAMIC_BLACK_LEVEL or use pixels from
     * ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS directly for captures when
     * supported by the camera device, which provides more accurate black
     * level values. For raw capture in particular, it is recommended to use
     * pixels from ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS to calculate black
     * level values for each frame.</p>
     *
     * @see ACAMERA_SENSOR_DYNAMIC_BLACK_LEVEL
     * @see ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT
     * @see ACAMERA_SENSOR_INFO_WHITE_LEVEL
     * @see ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_SENSOR_BLACK_LEVEL_PATTERN =                        // int32[4]
            ACAMERA_SENSOR_START + 12,
    /**
     * <p>Maximum sensitivity that is implemented
     * purely through analog gain.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>For ACAMERA_SENSOR_SENSITIVITY values less than or
     * equal to this, all applied gain must be analog. For
     * values above this, the gain applied can be a mix of analog and
     * digital.</p>
     *
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_SENSOR_MAX_ANALOG_SENSITIVITY =                     // int32
            ACAMERA_SENSOR_START + 13,
    /**
     * <p>Clockwise angle through which the output image needs to be rotated to be
     * upright on the device screen in its native orientation.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Also defines the direction of rolling shutter readout, which is from top to bottom in
     * the sensor's coordinate system.</p>
     */
    ACAMERA_SENSOR_ORIENTATION =                                // int32
            ACAMERA_SENSOR_START + 14,
    /**
     * <p>Time at start of exposure of first
     * row of the image sensor active array, in nanoseconds.</p>
     *
     * <p>Type: int64</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>The timestamps are also included in all image
     * buffers produced for the same capture, and will be identical
     * on all the outputs.</p>
     * <p>When ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE <code>==</code> UNKNOWN,
     * the timestamps measure time since an unspecified starting point,
     * and are monotonically increasing. They can be compared with the
     * timestamps for other captures from the same camera device, but are
     * not guaranteed to be comparable to any other time source.</p>
     * <p>When ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE <code>==</code> REALTIME, the
     * timestamps measure time in the same timebase as
     * <a href="https://developer.android.com/reference/android/os/SystemClock.html#elapsedRealtimeNanos">elapsedRealtimeNanos</a>
     * (or CLOCK_BOOTTIME), and they can
     * be compared to other timestamps from other subsystems that
     * are using that base.</p>
     * <p>For reprocessing, the timestamp will match the start of exposure of
     * the input image, i.e. {@link CaptureResult#SENSOR_TIMESTAMP the
     * timestamp} in the TotalCaptureResult that was used to create the
     * reprocess capture request.</p>
     *
     * @see ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE
     */
    ACAMERA_SENSOR_TIMESTAMP =                                  // int64
            ACAMERA_SENSOR_START + 16,
    /**
     * <p>The estimated camera neutral color in the native sensor colorspace at
     * the time of capture.</p>
     *
     * <p>Type: rational[3]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>This value gives the neutral color point encoded as an RGB value in the
     * native sensor color space.  The neutral color point indicates the
     * currently estimated white point of the scene illumination.  It can be
     * used to interpolate between the provided color transforms when
     * processing raw sensor data.</p>
     * <p>The order of the values is R, G, B; where R is in the lowest index.</p>
     */
    ACAMERA_SENSOR_NEUTRAL_COLOR_POINT =                        // rational[3]
            ACAMERA_SENSOR_START + 18,
    /**
     * <p>Noise model coefficients for each CFA mosaic channel.</p>
     *
     * <p>Type: double[2*CFA Channels]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>This key contains two noise model coefficients for each CFA channel
     * corresponding to the sensor amplification (S) and sensor readout
     * noise (O).  These are given as pairs of coefficients for each channel
     * in the same order as channels listed for the CFA layout key
     * (see ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT).  This is
     * represented as an array of Pair&lt;Double, Double&gt;, where
     * the first member of the Pair at index n is the S coefficient and the
     * second member is the O coefficient for the nth color channel in the CFA.</p>
     * <p>These coefficients are used in a two parameter noise model to describe
     * the amount of noise present in the image for each CFA channel.  The
     * noise model used here is:</p>
     * <p>N(x) = sqrt(Sx + O)</p>
     * <p>Where x represents the recorded signal of a CFA channel normalized to
     * the range [0, 1], and S and O are the noise model coeffiecients for
     * that channel.</p>
     * <p>A more detailed description of the noise model can be found in the
     * Adobe DNG specification for the NoiseProfile tag.</p>
     *
     * @see ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT
     */
    ACAMERA_SENSOR_NOISE_PROFILE =                              // double[2*CFA Channels]
            ACAMERA_SENSOR_START + 19,
    /**
     * <p>The worst-case divergence between Bayer green channels.</p>
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>This value is an estimate of the worst case split between the
     * Bayer green channels in the red and blue rows in the sensor color
     * filter array.</p>
     * <p>The green split is calculated as follows:</p>
     * <ol>
     * <li>A 5x5 pixel (or larger) window W within the active sensor array is
     * chosen. The term 'pixel' here is taken to mean a group of 4 Bayer
     * mosaic channels (R, Gr, Gb, B).  The location and size of the window
     * chosen is implementation defined, and should be chosen to provide a
     * green split estimate that is both representative of the entire image
     * for this camera sensor, and can be calculated quickly.</li>
     * <li>The arithmetic mean of the green channels from the red
     * rows (mean_Gr) within W is computed.</li>
     * <li>The arithmetic mean of the green channels from the blue
     * rows (mean_Gb) within W is computed.</li>
     * <li>The maximum ratio R of the two means is computed as follows:
     * <code>R = max((mean_Gr + 1)/(mean_Gb + 1), (mean_Gb + 1)/(mean_Gr + 1))</code></li>
     * </ol>
     * <p>The ratio R is the green split divergence reported for this property,
     * which represents how much the green channels differ in the mosaic
     * pattern.  This value is typically used to determine the treatment of
     * the green mosaic channels when demosaicing.</p>
     * <p>The green split value can be roughly interpreted as follows:</p>
     * <ul>
     * <li>R &lt; 1.03 is a negligible split (&lt;3% divergence).</li>
     * <li>1.20 &lt;= R &gt;= 1.03 will require some software
     * correction to avoid demosaic errors (3-20% divergence).</li>
     * <li>R &gt; 1.20 will require strong software correction to produce
     * a usuable image (&gt;20% divergence).</li>
     * </ul>
     */
    ACAMERA_SENSOR_GREEN_SPLIT =                                // float
            ACAMERA_SENSOR_START + 22,
    /**
     * <p>A pixel <code>[R, G_even, G_odd, B]</code> that supplies the test pattern
     * when ACAMERA_SENSOR_TEST_PATTERN_MODE is SOLID_COLOR.</p>
     *
     * @see ACAMERA_SENSOR_TEST_PATTERN_MODE
     *
     * <p>Type: int32[4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Each color channel is treated as an unsigned 32-bit integer.
     * The camera device then uses the most significant X bits
     * that correspond to how many bits are in its Bayer raw sensor
     * output.</p>
     * <p>For example, a sensor with RAW10 Bayer output would use the
     * 10 most significant bits from each color channel.</p>
     */
    ACAMERA_SENSOR_TEST_PATTERN_DATA =                          // int32[4]
            ACAMERA_SENSOR_START + 23,
    /**
     * <p>When enabled, the sensor sends a test pattern instead of
     * doing a real exposure from the camera.</p>
     *
     * <p>Type: int32 (acamera_metadata_enum_android_sensor_test_pattern_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When a test pattern is enabled, all manual sensor controls specified
     * by ACAMERA_SENSOR_* will be ignored. All other controls should
     * work as normal.</p>
     * <p>For example, if manual flash is enabled, flash firing should still
     * occur (and that the test pattern remain unmodified, since the flash
     * would not actually affect it).</p>
     * <p>Defaults to OFF.</p>
     */
    ACAMERA_SENSOR_TEST_PATTERN_MODE =                          // int32 (acamera_metadata_enum_android_sensor_test_pattern_mode_t)
            ACAMERA_SENSOR_START + 24,
    /**
     * <p>List of sensor test pattern modes for ACAMERA_SENSOR_TEST_PATTERN_MODE
     * supported by this camera device.</p>
     *
     * @see ACAMERA_SENSOR_TEST_PATTERN_MODE
     *
     * <p>Type: int32[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Defaults to OFF, and always includes OFF if defined.</p>
     */
    ACAMERA_SENSOR_AVAILABLE_TEST_PATTERN_MODES =               // int32[n]
            ACAMERA_SENSOR_START + 25,
    /**
     * <p>Duration between the start of first row exposure
     * and the start of last row exposure.</p>
     *
     * <p>Type: int64</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>This is the exposure time skew between the first and last
     * row exposure start times. The first row and the last row are
     * the first and last rows inside of the
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.</p>
     * <p>For typical camera sensors that use rolling shutters, this is also equivalent
     * to the frame readout time.</p>
     *
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_SENSOR_ROLLING_SHUTTER_SKEW =                       // int64
            ACAMERA_SENSOR_START + 26,
    /**
     * <p>List of disjoint rectangles indicating the sensor
     * optically shielded black pixel regions.</p>
     *
     * <p>Type: int32[4*num_regions]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>In most camera sensors, the active array is surrounded by some
     * optically shielded pixel areas. By blocking light, these pixels
     * provides a reliable black reference for black level compensation
     * in active array region.</p>
     * <p>The data representation is int[4], which maps to (left, top, width, height).</p>
     * <p>This key provides a list of disjoint rectangles specifying the
     * regions of optically shielded (with metal shield) black pixel
     * regions if the camera device is capable of reading out these black
     * pixels in the output raw images. In comparison to the fixed black
     * level values reported by ACAMERA_SENSOR_BLACK_LEVEL_PATTERN, this key
     * may provide a more accurate way for the application to calculate
     * black level of each captured raw images.</p>
     * <p>When this key is reported, the ACAMERA_SENSOR_DYNAMIC_BLACK_LEVEL and
     * ACAMERA_SENSOR_DYNAMIC_WHITE_LEVEL will also be reported.</p>
     *
     * @see ACAMERA_SENSOR_BLACK_LEVEL_PATTERN
     * @see ACAMERA_SENSOR_DYNAMIC_BLACK_LEVEL
     * @see ACAMERA_SENSOR_DYNAMIC_WHITE_LEVEL
     */
    ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS =                      // int32[4*num_regions]
            ACAMERA_SENSOR_START + 27,
    /**
     * <p>A per-frame dynamic black level offset for each of the color filter
     * arrangement (CFA) mosaic channels.</p>
     *
     * <p>Type: float[4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Camera sensor black levels may vary dramatically for different
     * capture settings (e.g. ACAMERA_SENSOR_SENSITIVITY). The fixed black
     * level reported by ACAMERA_SENSOR_BLACK_LEVEL_PATTERN may be too
     * inaccurate to represent the actual value on a per-frame basis. The
     * camera device internal pipeline relies on reliable black level values
     * to process the raw images appropriately. To get the best image
     * quality, the camera device may choose to estimate the per frame black
     * level values either based on optically shielded black regions
     * (ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS) or its internal model.</p>
     * <p>This key reports the camera device estimated per-frame zero light
     * value for each of the CFA mosaic channels in the camera sensor. The
     * ACAMERA_SENSOR_BLACK_LEVEL_PATTERN may only represent a coarse
     * approximation of the actual black level values. This value is the
     * black level used in camera device internal image processing pipeline
     * and generally more accurate than the fixed black level values.
     * However, since they are estimated values by the camera device, they
     * may not be as accurate as the black level values calculated from the
     * optical black pixels reported by ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS.</p>
     * <p>The values are given in the same order as channels listed for the CFA
     * layout key (see ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT), i.e. the
     * nth value given corresponds to the black level offset for the nth
     * color channel listed in the CFA.</p>
     * <p>This key will be available if ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS is
     * available or the camera device advertises this key via
     * {@link ACAMERA_REQUEST_AVAILABLE_RESULT_KEYS}.</p>
     *
     * @see ACAMERA_SENSOR_BLACK_LEVEL_PATTERN
     * @see ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT
     * @see ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_SENSOR_DYNAMIC_BLACK_LEVEL =                        // float[4]
            ACAMERA_SENSOR_START + 28,
    /**
     * <p>Maximum raw value output by sensor for this frame.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Since the ACAMERA_SENSOR_BLACK_LEVEL_PATTERN may change for different
     * capture settings (e.g., ACAMERA_SENSOR_SENSITIVITY), the white
     * level will change accordingly. This key is similar to
     * ACAMERA_SENSOR_INFO_WHITE_LEVEL, but specifies the camera device
     * estimated white level for each frame.</p>
     * <p>This key will be available if ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS is
     * available or the camera device advertises this key via
     * {@link ACAMERA_REQUEST_AVAILABLE_RESULT_KEYS}.</p>
     *
     * @see ACAMERA_SENSOR_BLACK_LEVEL_PATTERN
     * @see ACAMERA_SENSOR_INFO_WHITE_LEVEL
     * @see ACAMERA_SENSOR_OPTICAL_BLACK_REGIONS
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_SENSOR_DYNAMIC_WHITE_LEVEL =                        // int32
            ACAMERA_SENSOR_START + 29,
    ACAMERA_SENSOR_END,

    /**
     * <p>The area of the image sensor which corresponds to active pixels after any geometric
     * distortion correction has been applied.</p>
     *
     * <p>Type: int32[4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This is the rectangle representing the size of the active region of the sensor (i.e.
     * the region that actually receives light from the scene) after any geometric correction
     * has been applied, and should be treated as the maximum size in pixels of any of the
     * image output formats aside from the raw formats.</p>
     * <p>This rectangle is defined relative to the full pixel array; (0,0) is the top-left of
     * the full pixel array, and the size of the full pixel array is given by
     * ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE.</p>
     * <p>The data representation is int[4], which maps to (left, top, width, height).</p>
     * <p>The coordinate system for most other keys that list pixel coordinates, including
     * ACAMERA_SCALER_CROP_REGION, is defined relative to the active array rectangle given in
     * this field, with <code>(0, 0)</code> being the top-left of this rectangle.</p>
     * <p>The active array may be smaller than the full pixel array, since the full array may
     * include black calibration pixels or other inactive regions, and geometric correction
     * resulting in scaling or cropping may have been applied.</p>
     *
     * @see ACAMERA_SCALER_CROP_REGION
     * @see ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE
     */
    ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE =                     // int32[4]
            ACAMERA_SENSOR_INFO_START,
    /**
     * <p>Range of sensitivities for ACAMERA_SENSOR_SENSITIVITY supported by this
     * camera device.</p>
     *
     * @see ACAMERA_SENSOR_SENSITIVITY
     *
     * <p>Type: int32[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The values are the standard ISO sensitivity values,
     * as defined in ISO 12232:2006.</p>
     */
    ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE =                     // int32[2]
            ACAMERA_SENSOR_INFO_START + 1,
    /**
     * <p>The arrangement of color filters on sensor;
     * represents the colors in the top-left 2x2 section of
     * the sensor, in reading order.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_sensor_info_color_filter_arrangement_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     */
    ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT =              // byte (acamera_metadata_enum_android_sensor_info_color_filter_arrangement_t)
            ACAMERA_SENSOR_INFO_START + 2,
    /**
     * <p>The range of image exposure times for ACAMERA_SENSOR_EXPOSURE_TIME supported
     * by this camera device.</p>
     *
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     *
     * <p>Type: int64[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     */
    ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE =                   // int64[2]
            ACAMERA_SENSOR_INFO_START + 3,
    /**
     * <p>The maximum possible frame duration (minimum frame rate) for
     * ACAMERA_SENSOR_FRAME_DURATION that is supported this camera device.</p>
     *
     * @see ACAMERA_SENSOR_FRAME_DURATION
     *
     * <p>Type: int64</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Attempting to use frame durations beyond the maximum will result in the frame
     * duration being clipped to the maximum. See that control for a full definition of frame
     * durations.</p>
     * <p>Refer to {@link
     * ACAMERA_SCALER_AVAILABLE_MIN_FRAME_DURATIONS}
     * for the minimum frame duration values.</p>
     */
    ACAMERA_SENSOR_INFO_MAX_FRAME_DURATION =                    // int64
            ACAMERA_SENSOR_INFO_START + 4,
    /**
     * <p>The physical dimensions of the full pixel
     * array.</p>
     *
     * <p>Type: float[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This is the physical size of the sensor pixel
     * array defined by ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE.</p>
     *
     * @see ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE
     */
    ACAMERA_SENSOR_INFO_PHYSICAL_SIZE =                         // float[2]
            ACAMERA_SENSOR_INFO_START + 5,
    /**
     * <p>Dimensions of the full pixel array, possibly
     * including black calibration pixels.</p>
     *
     * <p>Type: int32[2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The pixel count of the full pixel array of the image sensor, which covers
     * ACAMERA_SENSOR_INFO_PHYSICAL_SIZE area.  This represents the full pixel dimensions of
     * the raw buffers produced by this sensor.</p>
     * <p>If a camera device supports raw sensor formats, either this or
     * ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE is the maximum dimensions for the raw
     * output formats listed in ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS (this depends on
     * whether or not the image sensor returns buffers containing pixels that are not
     * part of the active array region for blacklevel calibration or other purposes).</p>
     * <p>Some parts of the full pixel array may not receive light from the scene,
     * or be otherwise inactive.  The ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE key
     * defines the rectangle of active pixels that will be included in processed image
     * formats.</p>
     *
     * @see ACAMERA_SENSOR_INFO_PHYSICAL_SIZE
     * @see ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE =                      // int32[2]
            ACAMERA_SENSOR_INFO_START + 6,
    /**
     * <p>Maximum raw value output by sensor.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This specifies the fully-saturated encoding level for the raw
     * sample values from the sensor.  This is typically caused by the
     * sensor becoming highly non-linear or clipping. The minimum for
     * each channel is specified by the offset in the
     * ACAMERA_SENSOR_BLACK_LEVEL_PATTERN key.</p>
     * <p>The white level is typically determined either by sensor bit depth
     * (8-14 bits is expected), or by the point where the sensor response
     * becomes too non-linear to be useful.  The default value for this is
     * maximum representable value for a 16-bit raw sample (2^16 - 1).</p>
     * <p>The white level values of captured images may vary for different
     * capture settings (e.g., ACAMERA_SENSOR_SENSITIVITY). This key
     * represents a coarse approximation for such case. It is recommended
     * to use ACAMERA_SENSOR_DYNAMIC_WHITE_LEVEL for captures when supported
     * by the camera device, which provides more accurate white level values.</p>
     *
     * @see ACAMERA_SENSOR_BLACK_LEVEL_PATTERN
     * @see ACAMERA_SENSOR_DYNAMIC_WHITE_LEVEL
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_SENSOR_INFO_WHITE_LEVEL =                           // int32
            ACAMERA_SENSOR_INFO_START + 7,
    /**
     * <p>The time base source for sensor capture start timestamps.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_sensor_info_timestamp_source_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The timestamps provided for captures are always in nanoseconds and monotonic, but
     * may not based on a time source that can be compared to other system time sources.</p>
     * <p>This characteristic defines the source for the timestamps, and therefore whether they
     * can be compared against other system time sources/timestamps.</p>
     */
    ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE =                      // byte (acamera_metadata_enum_android_sensor_info_timestamp_source_t)
            ACAMERA_SENSOR_INFO_START + 8,
    /**
     * <p>Whether the RAW images output from this camera device are subject to
     * lens shading correction.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_sensor_info_lens_shading_applied_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If TRUE, all images produced by the camera device in the RAW image formats will
     * have lens shading correction already applied to it. If FALSE, the images will
     * not be adjusted for lens shading correction.
     * See android.request.maxNumOutputRaw for a list of RAW image formats.</p>
     * <p>This key will be <code>null</code> for all devices do not report this information.
     * Devices with RAW capability will always report this information in this key.</p>
     */
    ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED =                  // byte (acamera_metadata_enum_android_sensor_info_lens_shading_applied_t)
            ACAMERA_SENSOR_INFO_START + 9,
    /**
     * <p>The area of the image sensor which corresponds to active pixels prior to the
     * application of any geometric distortion correction.</p>
     *
     * <p>Type: int32[4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The data representation is int[4], which maps to (left, top, width, height).</p>
     * <p>This is the rectangle representing the size of the active region of the sensor (i.e.
     * the region that actually receives light from the scene) before any geometric correction
     * has been applied, and should be treated as the active region rectangle for any of the
     * raw formats.  All metadata associated with raw processing (e.g. the lens shading
     * correction map, and radial distortion fields) treats the top, left of this rectangle as
     * the origin, (0,0).</p>
     * <p>The size of this region determines the maximum field of view and the maximum number of
     * pixels that an image from this sensor can contain, prior to the application of
     * geometric distortion correction. The effective maximum pixel dimensions of a
     * post-distortion-corrected image is given by the ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     * field, and the effective maximum field of view for a post-distortion-corrected image
     * can be calculated by applying the geometric distortion correction fields to this
     * rectangle, and cropping to the rectangle given in ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.</p>
     * <p>E.g. to calculate position of a pixel, (x,y), in a processed YUV output image with the
     * dimensions in ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE given the position of a pixel,
     * (x', y'), in the raw pixel array with dimensions give in
     * ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE:</p>
     * <ol>
     * <li>Choose a pixel (x', y') within the active array region of the raw buffer given in
     * ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE, otherwise this pixel is considered
     * to be outside of the FOV, and will not be shown in the processed output image.</li>
     * <li>Apply geometric distortion correction to get the post-distortion pixel coordinate,
     * (x_i, y_i). When applying geometric correction metadata, note that metadata for raw
     * buffers is defined relative to the top, left of the
     * ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE rectangle.</li>
     * <li>If the resulting corrected pixel coordinate is within the region given in
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE, then the position of this pixel in the
     * processed output image buffer is <code>(x_i - activeArray.left, y_i - activeArray.top)</code>,
     * when the top, left coordinate of that buffer is treated as (0, 0).</li>
     * </ol>
     * <p>Thus, for pixel x',y' = (25, 25) on a sensor where ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE
     * is (100,100), ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE is (10, 10, 100, 100),
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE is (20, 20, 80, 80), and the geometric distortion
     * correction doesn't change the pixel coordinate, the resulting pixel selected in
     * pixel coordinates would be x,y = (25, 25) relative to the top,left of the raw buffer
     * with dimensions given in ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE, and would be (5, 5)
     * relative to the top,left of post-processed YUV output buffer with dimensions given in
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.</p>
     * <p>The currently supported fields that correct for geometric distortion are:</p>
     * <ol>
     * <li>ACAMERA_LENS_RADIAL_DISTORTION.</li>
     * </ol>
     * <p>If all of the geometric distortion fields are no-ops, this rectangle will be the same
     * as the post-distortion-corrected rectangle given in
     * ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.</p>
     * <p>This rectangle is defined relative to the full pixel array; (0,0) is the top-left of
     * the full pixel array, and the size of the full pixel array is given by
     * ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE.</p>
     * <p>The pre-correction active array may be smaller than the full pixel array, since the
     * full array may include black calibration pixels or other inactive regions.</p>
     *
     * @see ACAMERA_LENS_RADIAL_DISTORTION
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     * @see ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE
     * @see ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE =      // int32[4]
            ACAMERA_SENSOR_INFO_START + 10,
    ACAMERA_SENSOR_INFO_END,

    /**
     * <p>Quality of lens shading correction applied
     * to the image data.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_shading_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When set to OFF mode, no lens shading correction will be applied by the
     * camera device, and an identity lens shading map data will be provided
     * if <code>ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE == ON</code>. For example, for lens
     * shading map with size of <code>[ 4, 3 ]</code>,
     * the output android.statistics.lensShadingCorrectionMap for this case will be an identity
     * map shown below:</p>
     * <pre><code>[ 1.0, 1.0, 1.0, 1.0,  1.0, 1.0, 1.0, 1.0,
     *  1.0, 1.0, 1.0, 1.0,  1.0, 1.0, 1.0, 1.0,
     *  1.0, 1.0, 1.0, 1.0,  1.0, 1.0, 1.0, 1.0,
     *  1.0, 1.0, 1.0, 1.0,  1.0, 1.0, 1.0, 1.0,
     *  1.0, 1.0, 1.0, 1.0,  1.0, 1.0, 1.0, 1.0,
     *  1.0, 1.0, 1.0, 1.0,  1.0, 1.0, 1.0, 1.0 ]
     * </code></pre>
     * <p>When set to other modes, lens shading correction will be applied by the camera
     * device. Applications can request lens shading map data by setting
     * ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE to ON, and then the camera device will provide lens
     * shading map data in android.statistics.lensShadingCorrectionMap; the returned shading map
     * data will be the one applied by the camera device for this capture request.</p>
     * <p>The shading map data may depend on the auto-exposure (AE) and AWB statistics, therefore
     * the reliability of the map data may be affected by the AE and AWB algorithms. When AE and
     * AWB are in AUTO modes(ACAMERA_CONTROL_AE_MODE <code>!=</code> OFF and ACAMERA_CONTROL_AWB_MODE <code>!=</code>
     * OFF), to get best results, it is recommended that the applications wait for the AE and AWB
     * to be converged before using the returned shading map data.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AWB_MODE
     * @see ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE
     */
    ACAMERA_SHADING_MODE =                                      // byte (acamera_metadata_enum_android_shading_mode_t)
            ACAMERA_SHADING_START,
    /**
     * <p>List of lens shading modes for ACAMERA_SHADING_MODE that are supported by this camera device.</p>
     *
     * @see ACAMERA_SHADING_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This list contains lens shading modes that can be set for the camera device.
     * Camera devices that support the MANUAL_POST_PROCESSING capability will always
     * list OFF and FAST mode. This includes all FULL level devices.
     * LEGACY devices will always only support FAST mode.</p>
     */
    ACAMERA_SHADING_AVAILABLE_MODES =                           // byte[n]
            ACAMERA_SHADING_START + 2,
    ACAMERA_SHADING_END,

    /**
     * <p>Operating mode for the face detector
     * unit.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_statistics_face_detect_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Whether face detection is enabled, and whether it
     * should output just the basic fields or the full set of
     * fields.</p>
     */
    ACAMERA_STATISTICS_FACE_DETECT_MODE =                       // byte (acamera_metadata_enum_android_statistics_face_detect_mode_t)
            ACAMERA_STATISTICS_START,
    /**
     * <p>Operating mode for hot pixel map generation.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_statistics_hot_pixel_map_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>If set to <code>true</code>, a hot pixel map is returned in ACAMERA_STATISTICS_HOT_PIXEL_MAP.
     * If set to <code>false</code>, no hot pixel map will be returned.</p>
     *
     * @see ACAMERA_STATISTICS_HOT_PIXEL_MAP
     */
    ACAMERA_STATISTICS_HOT_PIXEL_MAP_MODE =                     // byte (acamera_metadata_enum_android_statistics_hot_pixel_map_mode_t)
            ACAMERA_STATISTICS_START + 3,
    /**
     * <p>List of unique IDs for detected faces.</p>
     *
     * <p>Type: int32[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Each detected face is given a unique ID that is valid for as long as the face is visible
     * to the camera device.  A face that leaves the field of view and later returns may be
     * assigned a new ID.</p>
     * <p>Only available if ACAMERA_STATISTICS_FACE_DETECT_MODE == FULL</p>
     *
     * @see ACAMERA_STATISTICS_FACE_DETECT_MODE
     */
    ACAMERA_STATISTICS_FACE_IDS =                               // int32[n]
            ACAMERA_STATISTICS_START + 4,
    /**
     * <p>List of landmarks for detected
     * faces.</p>
     *
     * <p>Type: int32[n*6]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>The coordinate system is that of ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE, with
     * <code>(0, 0)</code> being the top-left pixel of the active array.</p>
     * <p>Only available if ACAMERA_STATISTICS_FACE_DETECT_MODE == FULL</p>
     *
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     * @see ACAMERA_STATISTICS_FACE_DETECT_MODE
     */
    ACAMERA_STATISTICS_FACE_LANDMARKS =                         // int32[n*6]
            ACAMERA_STATISTICS_START + 5,
    /**
     * <p>List of the bounding rectangles for detected
     * faces.</p>
     *
     * <p>Type: int32[n*4]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>The data representation is int[4], which maps to (left, top, width, height).</p>
     * <p>The coordinate system is that of ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE, with
     * <code>(0, 0)</code> being the top-left pixel of the active array.</p>
     * <p>Only available if ACAMERA_STATISTICS_FACE_DETECT_MODE != OFF</p>
     *
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     * @see ACAMERA_STATISTICS_FACE_DETECT_MODE
     */
    ACAMERA_STATISTICS_FACE_RECTANGLES =                        // int32[n*4]
            ACAMERA_STATISTICS_START + 6,
    /**
     * <p>List of the face confidence scores for
     * detected faces</p>
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Only available if ACAMERA_STATISTICS_FACE_DETECT_MODE != OFF.</p>
     *
     * @see ACAMERA_STATISTICS_FACE_DETECT_MODE
     */
    ACAMERA_STATISTICS_FACE_SCORES =                            // byte[n]
            ACAMERA_STATISTICS_START + 7,
    /**
     * <p>The shading map is a low-resolution floating-point map
     * that lists the coefficients used to correct for vignetting and color shading,
     * for each Bayer color channel of RAW image data.</p>
     *
     * <p>Type: float[4*n*m]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>The map provided here is the same map that is used by the camera device to
     * correct both color shading and vignetting for output non-RAW images.</p>
     * <p>When there is no lens shading correction applied to RAW
     * output images (ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED <code>==</code>
     * false), this map is the complete lens shading correction
     * map; when there is some lens shading correction applied to
     * the RAW output image (ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED<code>==</code> true), this map reports the remaining lens shading
     * correction map that needs to be applied to get shading
     * corrected images that match the camera device's output for
     * non-RAW formats.</p>
     * <p>For a complete shading correction map, the least shaded
     * section of the image will have a gain factor of 1; all
     * other sections will have gains above 1.</p>
     * <p>When ACAMERA_COLOR_CORRECTION_MODE = TRANSFORM_MATRIX, the map
     * will take into account the colorCorrection settings.</p>
     * <p>The shading map is for the entire active pixel array, and is not
     * affected by the crop region specified in the request. Each shading map
     * entry is the value of the shading compensation map over a specific
     * pixel on the sensor.  Specifically, with a (N x M) resolution shading
     * map, and an active pixel array size (W x H), shading map entry
     * (x,y) ϵ (0 ... N-1, 0 ... M-1) is the value of the shading map at
     * pixel ( ((W-1)/(N-1)) * x, ((H-1)/(M-1)) * y) for the four color channels.
     * The map is assumed to be bilinearly interpolated between the sample points.</p>
     * <p>The channel order is [R, Geven, Godd, B], where Geven is the green
     * channel for the even rows of a Bayer pattern, and Godd is the odd rows.
     * The shading map is stored in a fully interleaved format, and its size
     * is provided in the camera static metadata by ACAMERA_LENS_INFO_SHADING_MAP_SIZE.</p>
     * <p>The shading map will generally have on the order of 30-40 rows and columns,
     * and will be smaller than 64x64.</p>
     * <p>As an example, given a very small map defined as:</p>
     * <pre><code>ACAMERA_LENS_INFO_SHADING_MAP_SIZE = [ 4, 3 ]
     * ACAMERA_STATISTICS_LENS_SHADING_MAP =
     * [ 1.3, 1.2, 1.15, 1.2,  1.2, 1.2, 1.15, 1.2,
     *     1.1, 1.2, 1.2, 1.2,  1.3, 1.2, 1.3, 1.3,
     *   1.2, 1.2, 1.25, 1.1,  1.1, 1.1, 1.1, 1.0,
     *     1.0, 1.0, 1.0, 1.0,  1.2, 1.3, 1.25, 1.2,
     *   1.3, 1.2, 1.2, 1.3,   1.2, 1.15, 1.1, 1.2,
     *     1.2, 1.1, 1.0, 1.2,  1.3, 1.15, 1.2, 1.3 ]
     * </code></pre>
     * <p>The low-resolution scaling map images for each channel are
     * (displayed using nearest-neighbor interpolation):</p>
     * <p><img alt="Red lens shading map" src="../images/camera2/metadata/android.statistics.lensShadingMap/red_shading.png" />
     * <img alt="Green (even rows) lens shading map" src="../images/camera2/metadata/android.statistics.lensShadingMap/green_e_shading.png" />
     * <img alt="Green (odd rows) lens shading map" src="../images/camera2/metadata/android.statistics.lensShadingMap/green_o_shading.png" />
     * <img alt="Blue lens shading map" src="../images/camera2/metadata/android.statistics.lensShadingMap/blue_shading.png" /></p>
     * <p>As a visualization only, inverting the full-color map to recover an
     * image of a gray wall (using bicubic interpolation for visual quality)
     * as captured by the sensor gives:</p>
     * <p><img alt="Image of a uniform white wall (inverse shading map)" src="../images/camera2/metadata/android.statistics.lensShadingMap/inv_shading.png" /></p>
     * <p>Note that the RAW image data might be subject to lens shading
     * correction not reported on this map. Query
     * ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED to see if RAW image data has subject
     * to lens shading correction. If ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED
     * is TRUE, the RAW image data is subject to partial or full lens shading
     * correction. In the case full lens shading correction is applied to RAW
     * images, the gain factor map reported in this key will contain all 1.0 gains.
     * In other words, the map reported in this key is the remaining lens shading
     * that needs to be applied on the RAW image to get images without lens shading
     * artifacts. See android.request.maxNumOutputRaw for a list of RAW image
     * formats.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_MODE
     * @see ACAMERA_LENS_INFO_SHADING_MAP_SIZE
     * @see ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED
     * @see ACAMERA_STATISTICS_LENS_SHADING_MAP
     */
    ACAMERA_STATISTICS_LENS_SHADING_MAP =                       // float[4*n*m]
            ACAMERA_STATISTICS_START + 11,
    /**
     * <p>The camera device estimated scene illumination lighting
     * frequency.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_statistics_scene_flicker_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>Many light sources, such as most fluorescent lights, flicker at a rate
     * that depends on the local utility power standards. This flicker must be
     * accounted for by auto-exposure routines to avoid artifacts in captured images.
     * The camera device uses this entry to tell the application what the scene
     * illuminant frequency is.</p>
     * <p>When manual exposure control is enabled
     * (<code>ACAMERA_CONTROL_AE_MODE == OFF</code> or <code>ACAMERA_CONTROL_MODE ==
     * OFF</code>), the ACAMERA_CONTROL_AE_ANTIBANDING_MODE doesn't perform
     * antibanding, and the application can ensure it selects
     * exposure times that do not cause banding issues by looking
     * into this metadata field. See
     * ACAMERA_CONTROL_AE_ANTIBANDING_MODE for more details.</p>
     * <p>Reports NONE if there doesn't appear to be flickering illumination.</p>
     *
     * @see ACAMERA_CONTROL_AE_ANTIBANDING_MODE
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_MODE
     */
    ACAMERA_STATISTICS_SCENE_FLICKER =                          // byte (acamera_metadata_enum_android_statistics_scene_flicker_t)
            ACAMERA_STATISTICS_START + 14,
    /**
     * <p>List of <code>(x, y)</code> coordinates of hot/defective pixels on the sensor.</p>
     *
     * <p>Type: int32[2*n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>A coordinate <code>(x, y)</code> must lie between <code>(0, 0)</code>, and
     * <code>(width - 1, height - 1)</code> (inclusive), which are the top-left and
     * bottom-right of the pixel array, respectively. The width and
     * height dimensions are given in ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE.
     * This may include hot pixels that lie outside of the active array
     * bounds given by ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE.</p>
     *
     * @see ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE
     * @see ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE
     */
    ACAMERA_STATISTICS_HOT_PIXEL_MAP =                          // int32[2*n]
            ACAMERA_STATISTICS_START + 15,
    /**
     * <p>Whether the camera device will output the lens
     * shading map in output result metadata.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_statistics_lens_shading_map_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When set to ON,
     * ACAMERA_STATISTICS_LENS_SHADING_MAP will be provided in
     * the output result metadata.</p>
     * <p>ON is always supported on devices with the RAW capability.</p>
     *
     * @see ACAMERA_STATISTICS_LENS_SHADING_MAP
     */
    ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE =                  // byte (acamera_metadata_enum_android_statistics_lens_shading_map_mode_t)
            ACAMERA_STATISTICS_START + 16,
    ACAMERA_STATISTICS_END,

    /**
     * <p>List of face detection modes for ACAMERA_STATISTICS_FACE_DETECT_MODE that are
     * supported by this camera device.</p>
     *
     * @see ACAMERA_STATISTICS_FACE_DETECT_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>OFF is always supported.</p>
     */
    ACAMERA_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES =       // byte[n]
            ACAMERA_STATISTICS_INFO_START,
    /**
     * <p>The maximum number of simultaneously detectable
     * faces.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     */
    ACAMERA_STATISTICS_INFO_MAX_FACE_COUNT =                    // int32
            ACAMERA_STATISTICS_INFO_START + 2,
    /**
     * <p>List of hot pixel map output modes for ACAMERA_STATISTICS_HOT_PIXEL_MAP_MODE that are
     * supported by this camera device.</p>
     *
     * @see ACAMERA_STATISTICS_HOT_PIXEL_MAP_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If no hotpixel map output is available for this camera device, this will contain only
     * <code>false</code>.</p>
     * <p>ON is always supported on devices with the RAW capability.</p>
     */
    ACAMERA_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES =     // byte[n]
            ACAMERA_STATISTICS_INFO_START + 6,
    /**
     * <p>List of lens shading map output modes for ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE that
     * are supported by this camera device.</p>
     *
     * @see ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If no lens shading map output is available for this camera device, this key will
     * contain only OFF.</p>
     * <p>ON is always supported on devices with the RAW capability.
     * LEGACY mode devices will always only support OFF.</p>
     */
    ACAMERA_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES =  // byte[n]
            ACAMERA_STATISTICS_INFO_START + 7,
    ACAMERA_STATISTICS_INFO_END,

    /**
     * <p>Tonemapping / contrast / gamma curve for the blue
     * channel, to use when ACAMERA_TONEMAP_MODE is
     * CONTRAST_CURVE.</p>
     *
     * @see ACAMERA_TONEMAP_MODE
     *
     * <p>Type: float[n*2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>See ACAMERA_TONEMAP_CURVE_RED for more details.</p>
     *
     * @see ACAMERA_TONEMAP_CURVE_RED
     */
    ACAMERA_TONEMAP_CURVE_BLUE =                                // float[n*2]
            ACAMERA_TONEMAP_START,
    /**
     * <p>Tonemapping / contrast / gamma curve for the green
     * channel, to use when ACAMERA_TONEMAP_MODE is
     * CONTRAST_CURVE.</p>
     *
     * @see ACAMERA_TONEMAP_MODE
     *
     * <p>Type: float[n*2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>See ACAMERA_TONEMAP_CURVE_RED for more details.</p>
     *
     * @see ACAMERA_TONEMAP_CURVE_RED
     */
    ACAMERA_TONEMAP_CURVE_GREEN =                               // float[n*2]
            ACAMERA_TONEMAP_START + 1,
    /**
     * <p>Tonemapping / contrast / gamma curve for the red
     * channel, to use when ACAMERA_TONEMAP_MODE is
     * CONTRAST_CURVE.</p>
     *
     * @see ACAMERA_TONEMAP_MODE
     *
     * <p>Type: float[n*2]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Each channel's curve is defined by an array of control points:</p>
     * <pre><code>ACAMERA_TONEMAP_CURVE_RED =
     *   [ P0in, P0out, P1in, P1out, P2in, P2out, P3in, P3out, ..., PNin, PNout ]
     * 2 &lt;= N &lt;= ACAMERA_TONEMAP_MAX_CURVE_POINTS</code></pre>
     * <p>These are sorted in order of increasing <code>Pin</code>; it is
     * required that input values 0.0 and 1.0 are included in the list to
     * define a complete mapping. For input values between control points,
     * the camera device must linearly interpolate between the control
     * points.</p>
     * <p>Each curve can have an independent number of points, and the number
     * of points can be less than max (that is, the request doesn't have to
     * always provide a curve with number of points equivalent to
     * ACAMERA_TONEMAP_MAX_CURVE_POINTS).</p>
     * <p>A few examples, and their corresponding graphical mappings; these
     * only specify the red channel and the precision is limited to 4
     * digits, for conciseness.</p>
     * <p>Linear mapping:</p>
     * <pre><code>ACAMERA_TONEMAP_CURVE_RED = [ 0, 0, 1.0, 1.0 ]
     * </code></pre>
     * <p><img alt="Linear mapping curve" src="../images/camera2/metadata/android.tonemap.curveRed/linear_tonemap.png" /></p>
     * <p>Invert mapping:</p>
     * <pre><code>ACAMERA_TONEMAP_CURVE_RED = [ 0, 1.0, 1.0, 0 ]
     * </code></pre>
     * <p><img alt="Inverting mapping curve" src="../images/camera2/metadata/android.tonemap.curveRed/inverse_tonemap.png" /></p>
     * <p>Gamma 1/2.2 mapping, with 16 control points:</p>
     * <pre><code>ACAMERA_TONEMAP_CURVE_RED = [
     *   0.0000, 0.0000, 0.0667, 0.2920, 0.1333, 0.4002, 0.2000, 0.4812,
     *   0.2667, 0.5484, 0.3333, 0.6069, 0.4000, 0.6594, 0.4667, 0.7072,
     *   0.5333, 0.7515, 0.6000, 0.7928, 0.6667, 0.8317, 0.7333, 0.8685,
     *   0.8000, 0.9035, 0.8667, 0.9370, 0.9333, 0.9691, 1.0000, 1.0000 ]
     * </code></pre>
     * <p><img alt="Gamma = 1/2.2 tonemapping curve" src="../images/camera2/metadata/android.tonemap.curveRed/gamma_tonemap.png" /></p>
     * <p>Standard sRGB gamma mapping, per IEC 61966-2-1:1999, with 16 control points:</p>
     * <pre><code>ACAMERA_TONEMAP_CURVE_RED = [
     *   0.0000, 0.0000, 0.0667, 0.2864, 0.1333, 0.4007, 0.2000, 0.4845,
     *   0.2667, 0.5532, 0.3333, 0.6125, 0.4000, 0.6652, 0.4667, 0.7130,
     *   0.5333, 0.7569, 0.6000, 0.7977, 0.6667, 0.8360, 0.7333, 0.8721,
     *   0.8000, 0.9063, 0.8667, 0.9389, 0.9333, 0.9701, 1.0000, 1.0000 ]
     * </code></pre>
     * <p><img alt="sRGB tonemapping curve" src="../images/camera2/metadata/android.tonemap.curveRed/srgb_tonemap.png" /></p>
     *
     * @see ACAMERA_TONEMAP_CURVE_RED
     * @see ACAMERA_TONEMAP_MAX_CURVE_POINTS
     */
    ACAMERA_TONEMAP_CURVE_RED =                                 // float[n*2]
            ACAMERA_TONEMAP_START + 2,
    /**
     * <p>High-level global contrast/gamma/tonemapping control.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_tonemap_mode_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>When switching to an application-defined contrast curve by setting
     * ACAMERA_TONEMAP_MODE to CONTRAST_CURVE, the curve is defined
     * per-channel with a set of <code>(in, out)</code> points that specify the
     * mapping from input high-bit-depth pixel value to the output
     * low-bit-depth value.  Since the actual pixel ranges of both input
     * and output may change depending on the camera pipeline, the values
     * are specified by normalized floating-point numbers.</p>
     * <p>More-complex color mapping operations such as 3D color look-up
     * tables, selective chroma enhancement, or other non-linear color
     * transforms will be disabled when ACAMERA_TONEMAP_MODE is
     * CONTRAST_CURVE.</p>
     * <p>When using either FAST or HIGH_QUALITY, the camera device will
     * emit its own tonemap curve in android.tonemap.curve.
     * These values are always available, and as close as possible to the
     * actually used nonlinear/nonglobal transforms.</p>
     * <p>If a request is sent with CONTRAST_CURVE with the camera device's
     * provided curve in FAST or HIGH_QUALITY, the image's tonemap will be
     * roughly the same.</p>
     *
     * @see ACAMERA_TONEMAP_MODE
     */
    ACAMERA_TONEMAP_MODE =                                      // byte (acamera_metadata_enum_android_tonemap_mode_t)
            ACAMERA_TONEMAP_START + 3,
    /**
     * <p>Maximum number of supported points in the
     * tonemap curve that can be used for android.tonemap.curve.</p>
     *
     * <p>Type: int32</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If the actual number of points provided by the application (in ACAMERA_TONEMAPCURVE_*) is
     * less than this maximum, the camera device will resample the curve to its internal
     * representation, using linear interpolation.</p>
     * <p>The output curves in the result metadata may have a different number
     * of points than the input curves, and will represent the actual
     * hardware curves used as closely as possible when linearly interpolated.</p>
     */
    ACAMERA_TONEMAP_MAX_CURVE_POINTS =                          // int32
            ACAMERA_TONEMAP_START + 4,
    /**
     * <p>List of tonemapping modes for ACAMERA_TONEMAP_MODE that are supported by this camera
     * device.</p>
     *
     * @see ACAMERA_TONEMAP_MODE
     *
     * <p>Type: byte[n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>Camera devices that support the MANUAL_POST_PROCESSING capability will always contain
     * at least one of below mode combinations:</p>
     * <ul>
     * <li>CONTRAST_CURVE, FAST and HIGH_QUALITY</li>
     * <li>GAMMA_VALUE, PRESET_CURVE, FAST and HIGH_QUALITY</li>
     * </ul>
     * <p>This includes all FULL level devices.</p>
     */
    ACAMERA_TONEMAP_AVAILABLE_TONE_MAP_MODES =                  // byte[n]
            ACAMERA_TONEMAP_START + 5,
    /**
     * <p>Tonemapping curve to use when ACAMERA_TONEMAP_MODE is
     * GAMMA_VALUE</p>
     *
     * @see ACAMERA_TONEMAP_MODE
     *
     * <p>Type: float</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>The tonemap curve will be defined the following formula:
     * * OUT = pow(IN, 1.0 / gamma)
     * where IN and OUT is the input pixel value scaled to range [0.0, 1.0],
     * pow is the power function and gamma is the gamma value specified by this
     * key.</p>
     * <p>The same curve will be applied to all color channels. The camera device
     * may clip the input gamma value to its supported range. The actual applied
     * value will be returned in capture result.</p>
     * <p>The valid range of gamma value varies on different devices, but values
     * within [1.0, 5.0] are guaranteed not to be clipped.</p>
     */
    ACAMERA_TONEMAP_GAMMA =                                     // float
            ACAMERA_TONEMAP_START + 6,
    /**
     * <p>Tonemapping curve to use when ACAMERA_TONEMAP_MODE is
     * PRESET_CURVE</p>
     *
     * @see ACAMERA_TONEMAP_MODE
     *
     * <p>Type: byte (acamera_metadata_enum_android_tonemap_preset_curve_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>The tonemap curve will be defined by specified standard.</p>
     * <p>sRGB (approximated by 16 control points):</p>
     * <p><img alt="sRGB tonemapping curve" src="../images/camera2/metadata/android.tonemap.curveRed/srgb_tonemap.png" /></p>
     * <p>Rec. 709 (approximated by 16 control points):</p>
     * <p><img alt="Rec. 709 tonemapping curve" src="../images/camera2/metadata/android.tonemap.curveRed/rec709_tonemap.png" /></p>
     * <p>Note that above figures show a 16 control points approximation of preset
     * curves. Camera devices may apply a different approximation to the curve.</p>
     */
    ACAMERA_TONEMAP_PRESET_CURVE =                              // byte (acamera_metadata_enum_android_tonemap_preset_curve_t)
            ACAMERA_TONEMAP_START + 7,
    ACAMERA_TONEMAP_END,

    /**
     * <p>Generally classifies the overall set of the camera device functionality.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_info_supported_hardware_level_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>The supported hardware level is a high-level description of the camera device's
     * capabilities, summarizing several capabilities into one field.  Each level adds additional
     * features to the previous one, and is always a strict superset of the previous level.
     * The ordering is <code>LEGACY &lt; LIMITED &lt; FULL &lt; LEVEL_3</code>.</p>
     * <p>Starting from <code>LEVEL_3</code>, the level enumerations are guaranteed to be in increasing
     * numerical value as well. To check if a given device is at least at a given hardware level,
     * the following code snippet can be used:</p>
     * <pre><code>// Returns true if the device supports the required hardware level, or better.
     * boolean isHardwareLevelSupported(CameraCharacteristics c, int requiredLevel) {
     *     int deviceLevel = c.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL);
     *     if (deviceLevel == CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
     *         return requiredLevel == deviceLevel;
     *     }
     *     // deviceLevel is not LEGACY, can use numerical sort
     *     return requiredLevel &lt;= deviceLevel;
     * }
     * </code></pre>
     * <p>At a high level, the levels are:</p>
     * <ul>
     * <li><code>LEGACY</code> devices operate in a backwards-compatibility mode for older
     *   Android devices, and have very limited capabilities.</li>
     * <li><code>LIMITED</code> devices represent the
     *   baseline feature set, and may also include additional capabilities that are
     *   subsets of <code>FULL</code>.</li>
     * <li><code>FULL</code> devices additionally support per-frame manual control of sensor, flash, lens and
     *   post-processing settings, and image capture at a high rate.</li>
     * <li><code>LEVEL_3</code> devices additionally support YUV reprocessing and RAW image capture, along
     *   with additional output stream configurations.</li>
     * </ul>
     * <p>See the individual level enums for full descriptions of the supported capabilities.  The
     * ACAMERA_REQUEST_AVAILABLE_CAPABILITIES entry describes the device's capabilities at a
     * finer-grain level, if needed. In addition, many controls have their available settings or
     * ranges defined in individual metadata tag entries in this document.</p>
     * <p>Some features are not part of any particular hardware level or capability and must be
     * queried separately. These include:</p>
     * <ul>
     * <li>Calibrated timestamps (ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE <code>==</code> REALTIME)</li>
     * <li>Precision lens control (ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION <code>==</code> CALIBRATED)</li>
     * <li>Face detection (ACAMERA_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES)</li>
     * <li>Optical or electrical image stabilization
     *   (ACAMERA_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
     *    ACAMERA_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES)</li>
     * </ul>
     *
     * @see ACAMERA_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES
     * @see ACAMERA_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION
     * @see ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     * @see ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE
     * @see ACAMERA_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES
     */
    ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL =                     // byte (acamera_metadata_enum_android_info_supported_hardware_level_t)
            ACAMERA_INFO_START,
    ACAMERA_INFO_END,

    /**
     * <p>Whether black-level compensation is locked
     * to its current values, or is free to vary.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_black_level_lock_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     *   <li>ACaptureRequest</li>
     * </ul></p>
     *
     * <p>Whether the black level offset was locked for this frame.  Should be
     * ON if ACAMERA_BLACK_LEVEL_LOCK was ON in the capture request, unless
     * a change in other capture settings forced the camera device to
     * perform a black level reset.</p>
     *
     * @see ACAMERA_BLACK_LEVEL_LOCK
     */
    ACAMERA_BLACK_LEVEL_LOCK =                                  // byte (acamera_metadata_enum_android_black_level_lock_t)
            ACAMERA_BLACK_LEVEL_START,
    ACAMERA_BLACK_LEVEL_END,

    /**
     * <p>The frame number corresponding to the last request
     * with which the output result (metadata + buffers) has been fully
     * synchronized.</p>
     *
     * <p>Type: int64 (acamera_metadata_enum_android_sync_frame_number_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraCaptureSession_captureCallback_result callbacks</li>
     * </ul></p>
     *
     * <p>When a request is submitted to the camera device, there is usually a
     * delay of several frames before the controls get applied. A camera
     * device may either choose to account for this delay by implementing a
     * pipeline and carefully submit well-timed atomic control updates, or
     * it may start streaming control changes that span over several frame
     * boundaries.</p>
     * <p>In the latter case, whenever a request's settings change relative to
     * the previous submitted request, the full set of changes may take
     * multiple frame durations to fully take effect. Some settings may
     * take effect sooner (in less frame durations) than others.</p>
     * <p>While a set of control changes are being propagated, this value
     * will be CONVERGING.</p>
     * <p>Once it is fully known that a set of control changes have been
     * finished propagating, and the resulting updated control settings
     * have been read back by the camera device, this value will be set
     * to a non-negative frame number (corresponding to the request to
     * which the results have synchronized to).</p>
     * <p>Older camera device implementations may not have a way to detect
     * when all camera controls have been applied, and will always set this
     * value to UNKNOWN.</p>
     * <p>FULL capability devices will always have this value set to the
     * frame number of the request corresponding to this result.</p>
     * <p><em>Further details</em>:</p>
     * <ul>
     * <li>Whenever a request differs from the last request, any future
     * results not yet returned may have this value set to CONVERGING (this
     * could include any in-progress captures not yet returned by the camera
     * device, for more details see pipeline considerations below).</li>
     * <li>Submitting a series of multiple requests that differ from the
     * previous request (e.g. r1, r2, r3 s.t. r1 != r2 != r3)
     * moves the new synchronization frame to the last non-repeating
     * request (using the smallest frame number from the contiguous list of
     * repeating requests).</li>
     * <li>Submitting the same request repeatedly will not change this value
     * to CONVERGING, if it was already a non-negative value.</li>
     * <li>When this value changes to non-negative, that means that all of the
     * metadata controls from the request have been applied, all of the
     * metadata controls from the camera device have been read to the
     * updated values (into the result), and all of the graphics buffers
     * corresponding to this result are also synchronized to the request.</li>
     * </ul>
     * <p><em>Pipeline considerations</em>:</p>
     * <p>Submitting a request with updated controls relative to the previously
     * submitted requests may also invalidate the synchronization state
     * of all the results corresponding to currently in-flight requests.</p>
     * <p>In other words, results for this current request and up to
     * ACAMERA_REQUEST_PIPELINE_MAX_DEPTH prior requests may have their
     * ACAMERA_SYNC_FRAME_NUMBER change to CONVERGING.</p>
     *
     * @see ACAMERA_REQUEST_PIPELINE_MAX_DEPTH
     * @see ACAMERA_SYNC_FRAME_NUMBER
     */
    ACAMERA_SYNC_FRAME_NUMBER =                                 // int64 (acamera_metadata_enum_android_sync_frame_number_t)
            ACAMERA_SYNC_START,
    /**
     * <p>The maximum number of frames that can occur after a request
     * (different than the previous) has been submitted, and before the
     * result's state becomes synchronized.</p>
     *
     * <p>Type: int32 (acamera_metadata_enum_android_sync_max_latency_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This defines the maximum distance (in number of metadata results),
     * between the frame number of the request that has new controls to apply
     * and the frame number of the result that has all the controls applied.</p>
     * <p>In other words this acts as an upper boundary for how many frames
     * must occur before the camera device knows for a fact that the new
     * submitted camera settings have been applied in outgoing frames.</p>
     */
    ACAMERA_SYNC_MAX_LATENCY =                                  // int32 (acamera_metadata_enum_android_sync_max_latency_t)
            ACAMERA_SYNC_START + 1,
    ACAMERA_SYNC_END,

    /**
     * <p>The available depth dataspace stream
     * configurations that this camera device supports
     * (i.e. format, width, height, output/input stream).</p>
     *
     * <p>Type: int32[n*4] (acamera_metadata_enum_android_depth_available_depth_stream_configurations_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>These are output stream configurations for use with
     * dataSpace HAL_DATASPACE_DEPTH. The configurations are
     * listed as <code>(format, width, height, input?)</code> tuples.</p>
     * <p>Only devices that support depth output for at least
     * the HAL_PIXEL_FORMAT_Y16 dense depth map may include
     * this entry.</p>
     * <p>A device that also supports the HAL_PIXEL_FORMAT_BLOB
     * sparse depth point cloud must report a single entry for
     * the format in this list as <code>(HAL_PIXEL_FORMAT_BLOB,
     * android.depth.maxDepthSamples, 1, OUTPUT)</code> in addition to
     * the entries for HAL_PIXEL_FORMAT_Y16.</p>
     */
    ACAMERA_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS =       // int32[n*4] (acamera_metadata_enum_android_depth_available_depth_stream_configurations_t)
            ACAMERA_DEPTH_START + 1,
    /**
     * <p>This lists the minimum frame duration for each
     * format/size combination for depth output formats.</p>
     *
     * <p>Type: int64[4*n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>This should correspond to the frame duration when only that
     * stream is active, with all processing (typically in android.*.mode)
     * set to either OFF or FAST.</p>
     * <p>When multiple streams are used in a request, the minimum frame
     * duration will be max(individual stream min durations).</p>
     * <p>The minimum frame duration of a stream (of a particular format, size)
     * is the same regardless of whether the stream is input or output.</p>
     * <p>See ACAMERA_SENSOR_FRAME_DURATION and
     * ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS for more details about
     * calculating the max frame rate.</p>
     *
     * @see ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS
     * @see ACAMERA_SENSOR_FRAME_DURATION
     */
    ACAMERA_DEPTH_AVAILABLE_DEPTH_MIN_FRAME_DURATIONS =         // int64[4*n]
            ACAMERA_DEPTH_START + 2,
    /**
     * <p>This lists the maximum stall duration for each
     * output format/size combination for depth streams.</p>
     *
     * <p>Type: int64[4*n]</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>A stall duration is how much extra time would get added
     * to the normal minimum frame duration for a repeating request
     * that has streams with non-zero stall.</p>
     * <p>This functions similarly to
     * ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS for depth
     * streams.</p>
     * <p>All depth output stream formats may have a nonzero stall
     * duration.</p>
     *
     * @see ACAMERA_SCALER_AVAILABLE_STALL_DURATIONS
     */
    ACAMERA_DEPTH_AVAILABLE_DEPTH_STALL_DURATIONS =             // int64[4*n]
            ACAMERA_DEPTH_START + 3,
    /**
     * <p>Indicates whether a capture request may target both a
     * DEPTH16 / DEPTH_POINT_CLOUD output, and normal color outputs (such as
     * YUV_420_888, JPEG, or RAW) simultaneously.</p>
     *
     * <p>Type: byte (acamera_metadata_enum_android_depth_depth_is_exclusive_t)</p>
     *
     * <p>This tag may appear in:
     * <ul>
     *   <li>ACameraMetadata from ACameraManager_getCameraCharacteristics</li>
     * </ul></p>
     *
     * <p>If TRUE, including both depth and color outputs in a single
     * capture request is not supported. An application must interleave color
     * and depth requests.  If FALSE, a single request can target both types
     * of output.</p>
     * <p>Typically, this restriction exists on camera devices that
     * need to emit a specific pattern or wavelength of light to
     * measure depth values, which causes the color image to be
     * corrupted during depth measurement.</p>
     */
    ACAMERA_DEPTH_DEPTH_IS_EXCLUSIVE =                          // byte (acamera_metadata_enum_android_depth_depth_is_exclusive_t)
            ACAMERA_DEPTH_START + 4,
    ACAMERA_DEPTH_END,

} acamera_metadata_tag_t;

/**
 * Enumeration definitions for the various entries that need them
 */

// ACAMERA_COLOR_CORRECTION_MODE
typedef enum acamera_metadata_enum_acamera_color_correction_mode {
    /**
     * <p>Use the ACAMERA_COLOR_CORRECTION_TRANSFORM matrix
     * and ACAMERA_COLOR_CORRECTION_GAINS to do color conversion.</p>
     * <p>All advanced white balance adjustments (not specified
     * by our white balance pipeline) must be disabled.</p>
     * <p>If AWB is enabled with <code>ACAMERA_CONTROL_AWB_MODE != OFF</code>, then
     * TRANSFORM_MATRIX is ignored. The camera device will override
     * this value to either FAST or HIGH_QUALITY.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     * @see ACAMERA_CONTROL_AWB_MODE
     */
    ACAMERA_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX                   = 0,

    /**
     * <p>Color correction processing must not slow down
     * capture rate relative to sensor raw output.</p>
     * <p>Advanced white balance adjustments above and beyond
     * the specified white balance pipeline may be applied.</p>
     * <p>If AWB is enabled with <code>ACAMERA_CONTROL_AWB_MODE != OFF</code>, then
     * the camera device uses the last frame's AWB values
     * (or defaults if AWB has never been run).</p>
     *
     * @see ACAMERA_CONTROL_AWB_MODE
     */
    ACAMERA_COLOR_CORRECTION_MODE_FAST                               = 1,

    /**
     * <p>Color correction processing operates at improved
     * quality but the capture rate might be reduced (relative to sensor
     * raw output rate)</p>
     * <p>Advanced white balance adjustments above and beyond
     * the specified white balance pipeline may be applied.</p>
     * <p>If AWB is enabled with <code>ACAMERA_CONTROL_AWB_MODE != OFF</code>, then
     * the camera device uses the last frame's AWB values
     * (or defaults if AWB has never been run).</p>
     *
     * @see ACAMERA_CONTROL_AWB_MODE
     */
    ACAMERA_COLOR_CORRECTION_MODE_HIGH_QUALITY                       = 2,

} acamera_metadata_enum_android_color_correction_mode_t;

// ACAMERA_COLOR_CORRECTION_ABERRATION_MODE
typedef enum acamera_metadata_enum_acamera_color_correction_aberration_mode {
    /**
     * <p>No aberration correction is applied.</p>
     */
    ACAMERA_COLOR_CORRECTION_ABERRATION_MODE_OFF                     = 0,

    /**
     * <p>Aberration correction will not slow down capture rate
     * relative to sensor raw output.</p>
     */
    ACAMERA_COLOR_CORRECTION_ABERRATION_MODE_FAST                    = 1,

    /**
     * <p>Aberration correction operates at improved quality but the capture rate might be
     * reduced (relative to sensor raw output rate)</p>
     */
    ACAMERA_COLOR_CORRECTION_ABERRATION_MODE_HIGH_QUALITY            = 2,

} acamera_metadata_enum_android_color_correction_aberration_mode_t;


// ACAMERA_CONTROL_AE_ANTIBANDING_MODE
typedef enum acamera_metadata_enum_acamera_control_ae_antibanding_mode {
    /**
     * <p>The camera device will not adjust exposure duration to
     * avoid banding problems.</p>
     */
    ACAMERA_CONTROL_AE_ANTIBANDING_MODE_OFF                          = 0,

    /**
     * <p>The camera device will adjust exposure duration to
     * avoid banding problems with 50Hz illumination sources.</p>
     */
    ACAMERA_CONTROL_AE_ANTIBANDING_MODE_50HZ                         = 1,

    /**
     * <p>The camera device will adjust exposure duration to
     * avoid banding problems with 60Hz illumination
     * sources.</p>
     */
    ACAMERA_CONTROL_AE_ANTIBANDING_MODE_60HZ                         = 2,

    /**
     * <p>The camera device will automatically adapt its
     * antibanding routine to the current illumination
     * condition. This is the default mode if AUTO is
     * available on given camera device.</p>
     */
    ACAMERA_CONTROL_AE_ANTIBANDING_MODE_AUTO                         = 3,

} acamera_metadata_enum_android_control_ae_antibanding_mode_t;

// ACAMERA_CONTROL_AE_LOCK
typedef enum acamera_metadata_enum_acamera_control_ae_lock {
    /**
     * <p>Auto-exposure lock is disabled; the AE algorithm
     * is free to update its parameters.</p>
     */
    ACAMERA_CONTROL_AE_LOCK_OFF                                      = 0,

    /**
     * <p>Auto-exposure lock is enabled; the AE algorithm
     * must not update the exposure and sensitivity parameters
     * while the lock is active.</p>
     * <p>ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION setting changes
     * will still take effect while auto-exposure is locked.</p>
     * <p>Some rare LEGACY devices may not support
     * this, in which case the value will always be overridden to OFF.</p>
     *
     * @see ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION
     */
    ACAMERA_CONTROL_AE_LOCK_ON                                       = 1,

} acamera_metadata_enum_android_control_ae_lock_t;

// ACAMERA_CONTROL_AE_MODE
typedef enum acamera_metadata_enum_acamera_control_ae_mode {
    /**
     * <p>The camera device's autoexposure routine is disabled.</p>
     * <p>The application-selected ACAMERA_SENSOR_EXPOSURE_TIME,
     * ACAMERA_SENSOR_SENSITIVITY and
     * ACAMERA_SENSOR_FRAME_DURATION are used by the camera
     * device, along with ACAMERA_FLASH_* fields, if there's
     * a flash unit for this camera device.</p>
     * <p>Note that auto-white balance (AWB) and auto-focus (AF)
     * behavior is device dependent when AE is in OFF mode.
     * To have consistent behavior across different devices,
     * it is recommended to either set AWB and AF to OFF mode
     * or lock AWB and AF before setting AE to OFF.
     * See ACAMERA_CONTROL_AWB_MODE, ACAMERA_CONTROL_AF_MODE,
     * ACAMERA_CONTROL_AWB_LOCK, and ACAMERA_CONTROL_AF_TRIGGER
     * for more details.</p>
     * <p>LEGACY devices do not support the OFF mode and will
     * override attempts to use this value to ON.</p>
     *
     * @see ACAMERA_CONTROL_AF_MODE
     * @see ACAMERA_CONTROL_AF_TRIGGER
     * @see ACAMERA_CONTROL_AWB_LOCK
     * @see ACAMERA_CONTROL_AWB_MODE
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_FRAME_DURATION
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_CONTROL_AE_MODE_OFF                                      = 0,

    /**
     * <p>The camera device's autoexposure routine is active,
     * with no flash control.</p>
     * <p>The application's values for
     * ACAMERA_SENSOR_EXPOSURE_TIME,
     * ACAMERA_SENSOR_SENSITIVITY, and
     * ACAMERA_SENSOR_FRAME_DURATION are ignored. The
     * application has control over the various
     * ACAMERA_FLASH_* fields.</p>
     *
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_FRAME_DURATION
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_CONTROL_AE_MODE_ON                                       = 1,

    /**
     * <p>Like ON, except that the camera device also controls
     * the camera's flash unit, firing it in low-light
     * conditions.</p>
     * <p>The flash may be fired during a precapture sequence
     * (triggered by ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER) and
     * may be fired for captures for which the
     * ACAMERA_CONTROL_CAPTURE_INTENT field is set to
     * STILL_CAPTURE</p>
     *
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     */
    ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH                            = 2,

    /**
     * <p>Like ON, except that the camera device also controls
     * the camera's flash unit, always firing it for still
     * captures.</p>
     * <p>The flash may be fired during a precapture sequence
     * (triggered by ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER) and
     * will always be fired for captures for which the
     * ACAMERA_CONTROL_CAPTURE_INTENT field is set to
     * STILL_CAPTURE</p>
     *
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     */
    ACAMERA_CONTROL_AE_MODE_ON_ALWAYS_FLASH                          = 3,

    /**
     * <p>Like ON_AUTO_FLASH, but with automatic red eye
     * reduction.</p>
     * <p>If deemed necessary by the camera device, a red eye
     * reduction flash will fire during the precapture
     * sequence.</p>
     */
    ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE                     = 4,

} acamera_metadata_enum_android_control_ae_mode_t;

// ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
typedef enum acamera_metadata_enum_acamera_control_ae_precapture_trigger {
    /**
     * <p>The trigger is idle.</p>
     */
    ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE                       = 0,

    /**
     * <p>The precapture metering sequence will be started
     * by the camera device.</p>
     * <p>The exact effect of the precapture trigger depends on
     * the current AE mode and state.</p>
     */
    ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER_START                      = 1,

    /**
     * <p>The camera device will cancel any currently active or completed
     * precapture metering sequence, the auto-exposure routine will return to its
     * initial state.</p>
     */
    ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL                     = 2,

} acamera_metadata_enum_android_control_ae_precapture_trigger_t;

// ACAMERA_CONTROL_AF_MODE
typedef enum acamera_metadata_enum_acamera_control_af_mode {
    /**
     * <p>The auto-focus routine does not control the lens;
     * ACAMERA_LENS_FOCUS_DISTANCE is controlled by the
     * application.</p>
     *
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     */
    ACAMERA_CONTROL_AF_MODE_OFF                                      = 0,

    /**
     * <p>Basic automatic focus mode.</p>
     * <p>In this mode, the lens does not move unless
     * the autofocus trigger action is called. When that trigger
     * is activated, AF will transition to ACTIVE_SCAN, then to
     * the outcome of the scan (FOCUSED or NOT_FOCUSED).</p>
     * <p>Always supported if lens is not fixed focus.</p>
     * <p>Use ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE to determine if lens
     * is fixed-focus.</p>
     * <p>Triggering AF_CANCEL resets the lens position to default,
     * and sets the AF state to INACTIVE.</p>
     *
     * @see ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE
     */
    ACAMERA_CONTROL_AF_MODE_AUTO                                     = 1,

    /**
     * <p>Close-up focusing mode.</p>
     * <p>In this mode, the lens does not move unless the
     * autofocus trigger action is called. When that trigger is
     * activated, AF will transition to ACTIVE_SCAN, then to
     * the outcome of the scan (FOCUSED or NOT_FOCUSED). This
     * mode is optimized for focusing on objects very close to
     * the camera.</p>
     * <p>When that trigger is activated, AF will transition to
     * ACTIVE_SCAN, then to the outcome of the scan (FOCUSED or
     * NOT_FOCUSED). Triggering cancel AF resets the lens
     * position to default, and sets the AF state to
     * INACTIVE.</p>
     */
    ACAMERA_CONTROL_AF_MODE_MACRO                                    = 2,

    /**
     * <p>In this mode, the AF algorithm modifies the lens
     * position continually to attempt to provide a
     * constantly-in-focus image stream.</p>
     * <p>The focusing behavior should be suitable for good quality
     * video recording; typically this means slower focus
     * movement and no overshoots. When the AF trigger is not
     * involved, the AF algorithm should start in INACTIVE state,
     * and then transition into PASSIVE_SCAN and PASSIVE_FOCUSED
     * states as appropriate. When the AF trigger is activated,
     * the algorithm should immediately transition into
     * AF_FOCUSED or AF_NOT_FOCUSED as appropriate, and lock the
     * lens position until a cancel AF trigger is received.</p>
     * <p>Once cancel is received, the algorithm should transition
     * back to INACTIVE and resume passive scan. Note that this
     * behavior is not identical to CONTINUOUS_PICTURE, since an
     * ongoing PASSIVE_SCAN must immediately be
     * canceled.</p>
     */
    ACAMERA_CONTROL_AF_MODE_CONTINUOUS_VIDEO                         = 3,

    /**
     * <p>In this mode, the AF algorithm modifies the lens
     * position continually to attempt to provide a
     * constantly-in-focus image stream.</p>
     * <p>The focusing behavior should be suitable for still image
     * capture; typically this means focusing as fast as
     * possible. When the AF trigger is not involved, the AF
     * algorithm should start in INACTIVE state, and then
     * transition into PASSIVE_SCAN and PASSIVE_FOCUSED states as
     * appropriate as it attempts to maintain focus. When the AF
     * trigger is activated, the algorithm should finish its
     * PASSIVE_SCAN if active, and then transition into
     * AF_FOCUSED or AF_NOT_FOCUSED as appropriate, and lock the
     * lens position until a cancel AF trigger is received.</p>
     * <p>When the AF cancel trigger is activated, the algorithm
     * should transition back to INACTIVE and then act as if it
     * has just been started.</p>
     */
    ACAMERA_CONTROL_AF_MODE_CONTINUOUS_PICTURE                       = 4,

    /**
     * <p>Extended depth of field (digital focus) mode.</p>
     * <p>The camera device will produce images with an extended
     * depth of field automatically; no special focusing
     * operations need to be done before taking a picture.</p>
     * <p>AF triggers are ignored, and the AF state will always be
     * INACTIVE.</p>
     */
    ACAMERA_CONTROL_AF_MODE_EDOF                                     = 5,

} acamera_metadata_enum_android_control_af_mode_t;

// ACAMERA_CONTROL_AF_TRIGGER
typedef enum acamera_metadata_enum_acamera_control_af_trigger {
    /**
     * <p>The trigger is idle.</p>
     */
    ACAMERA_CONTROL_AF_TRIGGER_IDLE                                  = 0,

    /**
     * <p>Autofocus will trigger now.</p>
     */
    ACAMERA_CONTROL_AF_TRIGGER_START                                 = 1,

    /**
     * <p>Autofocus will return to its initial
     * state, and cancel any currently active trigger.</p>
     */
    ACAMERA_CONTROL_AF_TRIGGER_CANCEL                                = 2,

} acamera_metadata_enum_android_control_af_trigger_t;

// ACAMERA_CONTROL_AWB_LOCK
typedef enum acamera_metadata_enum_acamera_control_awb_lock {
    /**
     * <p>Auto-white balance lock is disabled; the AWB
     * algorithm is free to update its parameters if in AUTO
     * mode.</p>
     */
    ACAMERA_CONTROL_AWB_LOCK_OFF                                     = 0,

    /**
     * <p>Auto-white balance lock is enabled; the AWB
     * algorithm will not update its parameters while the lock
     * is active.</p>
     */
    ACAMERA_CONTROL_AWB_LOCK_ON                                      = 1,

} acamera_metadata_enum_android_control_awb_lock_t;

// ACAMERA_CONTROL_AWB_MODE
typedef enum acamera_metadata_enum_acamera_control_awb_mode {
    /**
     * <p>The camera device's auto-white balance routine is disabled.</p>
     * <p>The application-selected color transform matrix
     * (ACAMERA_COLOR_CORRECTION_TRANSFORM) and gains
     * (ACAMERA_COLOR_CORRECTION_GAINS) are used by the camera
     * device for manual white balance control.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_OFF                                     = 0,

    /**
     * <p>The camera device's auto-white balance routine is active.</p>
     * <p>The application's values for ACAMERA_COLOR_CORRECTION_TRANSFORM
     * and ACAMERA_COLOR_CORRECTION_GAINS are ignored.
     * For devices that support the MANUAL_POST_PROCESSING capability, the
     * values used by the camera device for the transform and gains
     * will be available in the capture result for this request.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_AUTO                                    = 1,

    /**
     * <p>The camera device's auto-white balance routine is disabled;
     * the camera device uses incandescent light as the assumed scene
     * illumination for white balance.</p>
     * <p>While the exact white balance transforms are up to the
     * camera device, they will approximately match the CIE
     * standard illuminant A.</p>
     * <p>The application's values for ACAMERA_COLOR_CORRECTION_TRANSFORM
     * and ACAMERA_COLOR_CORRECTION_GAINS are ignored.
     * For devices that support the MANUAL_POST_PROCESSING capability, the
     * values used by the camera device for the transform and gains
     * will be available in the capture result for this request.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_INCANDESCENT                            = 2,

    /**
     * <p>The camera device's auto-white balance routine is disabled;
     * the camera device uses fluorescent light as the assumed scene
     * illumination for white balance.</p>
     * <p>While the exact white balance transforms are up to the
     * camera device, they will approximately match the CIE
     * standard illuminant F2.</p>
     * <p>The application's values for ACAMERA_COLOR_CORRECTION_TRANSFORM
     * and ACAMERA_COLOR_CORRECTION_GAINS are ignored.
     * For devices that support the MANUAL_POST_PROCESSING capability, the
     * values used by the camera device for the transform and gains
     * will be available in the capture result for this request.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_FLUORESCENT                             = 3,

    /**
     * <p>The camera device's auto-white balance routine is disabled;
     * the camera device uses warm fluorescent light as the assumed scene
     * illumination for white balance.</p>
     * <p>While the exact white balance transforms are up to the
     * camera device, they will approximately match the CIE
     * standard illuminant F4.</p>
     * <p>The application's values for ACAMERA_COLOR_CORRECTION_TRANSFORM
     * and ACAMERA_COLOR_CORRECTION_GAINS are ignored.
     * For devices that support the MANUAL_POST_PROCESSING capability, the
     * values used by the camera device for the transform and gains
     * will be available in the capture result for this request.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_WARM_FLUORESCENT                        = 4,

    /**
     * <p>The camera device's auto-white balance routine is disabled;
     * the camera device uses daylight light as the assumed scene
     * illumination for white balance.</p>
     * <p>While the exact white balance transforms are up to the
     * camera device, they will approximately match the CIE
     * standard illuminant D65.</p>
     * <p>The application's values for ACAMERA_COLOR_CORRECTION_TRANSFORM
     * and ACAMERA_COLOR_CORRECTION_GAINS are ignored.
     * For devices that support the MANUAL_POST_PROCESSING capability, the
     * values used by the camera device for the transform and gains
     * will be available in the capture result for this request.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_DAYLIGHT                                = 5,

    /**
     * <p>The camera device's auto-white balance routine is disabled;
     * the camera device uses cloudy daylight light as the assumed scene
     * illumination for white balance.</p>
     * <p>The application's values for ACAMERA_COLOR_CORRECTION_TRANSFORM
     * and ACAMERA_COLOR_CORRECTION_GAINS are ignored.
     * For devices that support the MANUAL_POST_PROCESSING capability, the
     * values used by the camera device for the transform and gains
     * will be available in the capture result for this request.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT                         = 6,

    /**
     * <p>The camera device's auto-white balance routine is disabled;
     * the camera device uses twilight light as the assumed scene
     * illumination for white balance.</p>
     * <p>The application's values for ACAMERA_COLOR_CORRECTION_TRANSFORM
     * and ACAMERA_COLOR_CORRECTION_GAINS are ignored.
     * For devices that support the MANUAL_POST_PROCESSING capability, the
     * values used by the camera device for the transform and gains
     * will be available in the capture result for this request.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_TWILIGHT                                = 7,

    /**
     * <p>The camera device's auto-white balance routine is disabled;
     * the camera device uses shade light as the assumed scene
     * illumination for white balance.</p>
     * <p>The application's values for ACAMERA_COLOR_CORRECTION_TRANSFORM
     * and ACAMERA_COLOR_CORRECTION_GAINS are ignored.
     * For devices that support the MANUAL_POST_PROCESSING capability, the
     * values used by the camera device for the transform and gains
     * will be available in the capture result for this request.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     */
    ACAMERA_CONTROL_AWB_MODE_SHADE                                   = 8,

} acamera_metadata_enum_android_control_awb_mode_t;

// ACAMERA_CONTROL_CAPTURE_INTENT
typedef enum acamera_metadata_enum_acamera_control_capture_intent {
    /**
     * <p>The goal of this request doesn't fall into the other
     * categories. The camera device will default to preview-like
     * behavior.</p>
     */
    ACAMERA_CONTROL_CAPTURE_INTENT_CUSTOM                            = 0,

    /**
     * <p>This request is for a preview-like use case.</p>
     * <p>The precapture trigger may be used to start off a metering
     * w/flash sequence.</p>
     */
    ACAMERA_CONTROL_CAPTURE_INTENT_PREVIEW                           = 1,

    /**
     * <p>This request is for a still capture-type
     * use case.</p>
     * <p>If the flash unit is under automatic control, it may fire as needed.</p>
     */
    ACAMERA_CONTROL_CAPTURE_INTENT_STILL_CAPTURE                     = 2,

    /**
     * <p>This request is for a video recording
     * use case.</p>
     */
    ACAMERA_CONTROL_CAPTURE_INTENT_VIDEO_RECORD                      = 3,

    /**
     * <p>This request is for a video snapshot (still
     * image while recording video) use case.</p>
     * <p>The camera device should take the highest-quality image
     * possible (given the other settings) without disrupting the
     * frame rate of video recording.  </p>
     */
    ACAMERA_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT                    = 4,

    /**
     * <p>This request is for a ZSL usecase; the
     * application will stream full-resolution images and
     * reprocess one or several later for a final
     * capture.</p>
     */
    ACAMERA_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG                  = 5,

    /**
     * <p>This request is for manual capture use case where
     * the applications want to directly control the capture parameters.</p>
     * <p>For example, the application may wish to manually control
     * ACAMERA_SENSOR_EXPOSURE_TIME, ACAMERA_SENSOR_SENSITIVITY, etc.</p>
     *
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_CONTROL_CAPTURE_INTENT_MANUAL                            = 6,

} acamera_metadata_enum_android_control_capture_intent_t;

// ACAMERA_CONTROL_EFFECT_MODE
typedef enum acamera_metadata_enum_acamera_control_effect_mode {
    /**
     * <p>No color effect will be applied.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_OFF                                  = 0,

    /**
     * <p>A "monocolor" effect where the image is mapped into
     * a single color.</p>
     * <p>This will typically be grayscale.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_MONO                                 = 1,

    /**
     * <p>A "photo-negative" effect where the image's colors
     * are inverted.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_NEGATIVE                             = 2,

    /**
     * <p>A "solarisation" effect (Sabattier effect) where the
     * image is wholly or partially reversed in
     * tone.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_SOLARIZE                             = 3,

    /**
     * <p>A "sepia" effect where the image is mapped into warm
     * gray, red, and brown tones.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_SEPIA                                = 4,

    /**
     * <p>A "posterization" effect where the image uses
     * discrete regions of tone rather than a continuous
     * gradient of tones.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_POSTERIZE                            = 5,

    /**
     * <p>A "whiteboard" effect where the image is typically displayed
     * as regions of white, with black or grey details.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_WHITEBOARD                           = 6,

    /**
     * <p>A "blackboard" effect where the image is typically displayed
     * as regions of black, with white or grey details.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_BLACKBOARD                           = 7,

    /**
     * <p>An "aqua" effect where a blue hue is added to the image.</p>
     */
    ACAMERA_CONTROL_EFFECT_MODE_AQUA                                 = 8,

} acamera_metadata_enum_android_control_effect_mode_t;

// ACAMERA_CONTROL_MODE
typedef enum acamera_metadata_enum_acamera_control_mode {
    /**
     * <p>Full application control of pipeline.</p>
     * <p>All control by the device's metering and focusing (3A)
     * routines is disabled, and no other settings in
     * ACAMERA_CONTROL_* have any effect, except that
     * ACAMERA_CONTROL_CAPTURE_INTENT may be used by the camera
     * device to select post-processing values for processing
     * blocks that do not allow for manual control, or are not
     * exposed by the camera API.</p>
     * <p>However, the camera device's 3A routines may continue to
     * collect statistics and update their internal state so that
     * when control is switched to AUTO mode, good control values
     * can be immediately applied.</p>
     *
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     */
    ACAMERA_CONTROL_MODE_OFF                                         = 0,

    /**
     * <p>Use settings for each individual 3A routine.</p>
     * <p>Manual control of capture parameters is disabled. All
     * controls in ACAMERA_CONTROL_* besides sceneMode take
     * effect.</p>
     */
    ACAMERA_CONTROL_MODE_AUTO                                        = 1,

    /**
     * <p>Use a specific scene mode.</p>
     * <p>Enabling this disables control.aeMode, control.awbMode and
     * control.afMode controls; the camera device will ignore
     * those settings while USE_SCENE_MODE is active (except for
     * FACE_PRIORITY scene mode). Other control entries are still active.
     * This setting can only be used if scene mode is supported (i.e.
     * ACAMERA_CONTROL_AVAILABLE_SCENE_MODES
     * contain some modes other than DISABLED).</p>
     *
     * @see ACAMERA_CONTROL_AVAILABLE_SCENE_MODES
     */
    ACAMERA_CONTROL_MODE_USE_SCENE_MODE                              = 2,

    /**
     * <p>Same as OFF mode, except that this capture will not be
     * used by camera device background auto-exposure, auto-white balance and
     * auto-focus algorithms (3A) to update their statistics.</p>
     * <p>Specifically, the 3A routines are locked to the last
     * values set from a request with AUTO, OFF, or
     * USE_SCENE_MODE, and any statistics or state updates
     * collected from manual captures with OFF_KEEP_STATE will be
     * discarded by the camera device.</p>
     */
    ACAMERA_CONTROL_MODE_OFF_KEEP_STATE                              = 3,

} acamera_metadata_enum_android_control_mode_t;

// ACAMERA_CONTROL_SCENE_MODE
typedef enum acamera_metadata_enum_acamera_control_scene_mode {
    /**
     * <p>Indicates that no scene modes are set for a given capture request.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_DISABLED                              = 0,

    /**
     * <p>If face detection support exists, use face
     * detection data for auto-focus, auto-white balance, and
     * auto-exposure routines.</p>
     * <p>If face detection statistics are disabled
     * (i.e. ACAMERA_STATISTICS_FACE_DETECT_MODE is set to OFF),
     * this should still operate correctly (but will not return
     * face detection statistics to the framework).</p>
     * <p>Unlike the other scene modes, ACAMERA_CONTROL_AE_MODE,
     * ACAMERA_CONTROL_AWB_MODE, and ACAMERA_CONTROL_AF_MODE
     * remain active when FACE_PRIORITY is set.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AF_MODE
     * @see ACAMERA_CONTROL_AWB_MODE
     * @see ACAMERA_STATISTICS_FACE_DETECT_MODE
     */
    ACAMERA_CONTROL_SCENE_MODE_FACE_PRIORITY                         = 1,

    /**
     * <p>Optimized for photos of quickly moving objects.</p>
     * <p>Similar to SPORTS.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_ACTION                                = 2,

    /**
     * <p>Optimized for still photos of people.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_PORTRAIT                              = 3,

    /**
     * <p>Optimized for photos of distant macroscopic objects.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_LANDSCAPE                             = 4,

    /**
     * <p>Optimized for low-light settings.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_NIGHT                                 = 5,

    /**
     * <p>Optimized for still photos of people in low-light
     * settings.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_NIGHT_PORTRAIT                        = 6,

    /**
     * <p>Optimized for dim, indoor settings where flash must
     * remain off.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_THEATRE                               = 7,

    /**
     * <p>Optimized for bright, outdoor beach settings.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_BEACH                                 = 8,

    /**
     * <p>Optimized for bright, outdoor settings containing snow.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_SNOW                                  = 9,

    /**
     * <p>Optimized for scenes of the setting sun.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_SUNSET                                = 10,

    /**
     * <p>Optimized to avoid blurry photos due to small amounts of
     * device motion (for example: due to hand shake).</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_STEADYPHOTO                           = 11,

    /**
     * <p>Optimized for nighttime photos of fireworks.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_FIREWORKS                             = 12,

    /**
     * <p>Optimized for photos of quickly moving people.</p>
     * <p>Similar to ACTION.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_SPORTS                                = 13,

    /**
     * <p>Optimized for dim, indoor settings with multiple moving
     * people.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_PARTY                                 = 14,

    /**
     * <p>Optimized for dim settings where the main light source
     * is a flame.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_CANDLELIGHT                           = 15,

    /**
     * <p>Optimized for accurately capturing a photo of barcode
     * for use by camera applications that wish to read the
     * barcode value.</p>
     */
    ACAMERA_CONTROL_SCENE_MODE_BARCODE                               = 16,

    /**
     * <p>Turn on a device-specific high dynamic range (HDR) mode.</p>
     * <p>In this scene mode, the camera device captures images
     * that keep a larger range of scene illumination levels
     * visible in the final image. For example, when taking a
     * picture of a object in front of a bright window, both
     * the object and the scene through the window may be
     * visible when using HDR mode, while in normal AUTO mode,
     * one or the other may be poorly exposed. As a tradeoff,
     * HDR mode generally takes much longer to capture a single
     * image, has no user control, and may have other artifacts
     * depending on the HDR method used.</p>
     * <p>Therefore, HDR captures operate at a much slower rate
     * than regular captures.</p>
     * <p>In this mode, on LIMITED or FULL devices, when a request
     * is made with a ACAMERA_CONTROL_CAPTURE_INTENT of
     * STILL_CAPTURE, the camera device will capture an image
     * using a high dynamic range capture technique.  On LEGACY
     * devices, captures that target a JPEG-format output will
     * be captured with HDR, and the capture intent is not
     * relevant.</p>
     * <p>The HDR capture may involve the device capturing a burst
     * of images internally and combining them into one, or it
     * may involve the device using specialized high dynamic
     * range capture hardware. In all cases, a single image is
     * produced in response to a capture request submitted
     * while in HDR mode.</p>
     * <p>Since substantial post-processing is generally needed to
     * produce an HDR image, only YUV, PRIVATE, and JPEG
     * outputs are supported for LIMITED/FULL device HDR
     * captures, and only JPEG outputs are supported for LEGACY
     * HDR captures. Using a RAW output for HDR capture is not
     * supported.</p>
     * <p>Some devices may also support always-on HDR, which
     * applies HDR processing at full frame rate.  For these
     * devices, intents other than STILL_CAPTURE will also
     * produce an HDR output with no frame rate impact compared
     * to normal operation, though the quality may be lower
     * than for STILL_CAPTURE intents.</p>
     * <p>If SCENE_MODE_HDR is used with unsupported output types
     * or capture intents, the images captured will be as if
     * the SCENE_MODE was not enabled at all.</p>
     *
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     */
    ACAMERA_CONTROL_SCENE_MODE_HDR                                   = 18,

} acamera_metadata_enum_android_control_scene_mode_t;

// ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE
typedef enum acamera_metadata_enum_acamera_control_video_stabilization_mode {
    /**
     * <p>Video stabilization is disabled.</p>
     */
    ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE_OFF                     = 0,

    /**
     * <p>Video stabilization is enabled.</p>
     */
    ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE_ON                      = 1,

} acamera_metadata_enum_android_control_video_stabilization_mode_t;

// ACAMERA_CONTROL_AE_STATE
typedef enum acamera_metadata_enum_acamera_control_ae_state {
    /**
     * <p>AE is off or recently reset.</p>
     * <p>When a camera device is opened, it starts in
     * this state. This is a transient state, the camera device may skip reporting
     * this state in capture result.</p>
     */
    ACAMERA_CONTROL_AE_STATE_INACTIVE                                = 0,

    /**
     * <p>AE doesn't yet have a good set of control values
     * for the current scene.</p>
     * <p>This is a transient state, the camera device may skip
     * reporting this state in capture result.</p>
     */
    ACAMERA_CONTROL_AE_STATE_SEARCHING                               = 1,

    /**
     * <p>AE has a good set of control values for the
     * current scene.</p>
     */
    ACAMERA_CONTROL_AE_STATE_CONVERGED                               = 2,

    /**
     * <p>AE has been locked.</p>
     */
    ACAMERA_CONTROL_AE_STATE_LOCKED                                  = 3,

    /**
     * <p>AE has a good set of control values, but flash
     * needs to be fired for good quality still
     * capture.</p>
     */
    ACAMERA_CONTROL_AE_STATE_FLASH_REQUIRED                          = 4,

    /**
     * <p>AE has been asked to do a precapture sequence
     * and is currently executing it.</p>
     * <p>Precapture can be triggered through setting
     * ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER to START. Currently
     * active and completed (if it causes camera device internal AE lock) precapture
     * metering sequence can be canceled through setting
     * ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER to CANCEL.</p>
     * <p>Once PRECAPTURE completes, AE will transition to CONVERGED
     * or FLASH_REQUIRED as appropriate. This is a transient
     * state, the camera device may skip reporting this state in
     * capture result.</p>
     *
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     */
    ACAMERA_CONTROL_AE_STATE_PRECAPTURE                              = 5,

} acamera_metadata_enum_android_control_ae_state_t;

// ACAMERA_CONTROL_AF_STATE
typedef enum acamera_metadata_enum_acamera_control_af_state {
    /**
     * <p>AF is off or has not yet tried to scan/been asked
     * to scan.</p>
     * <p>When a camera device is opened, it starts in this
     * state. This is a transient state, the camera device may
     * skip reporting this state in capture
     * result.</p>
     */
    ACAMERA_CONTROL_AF_STATE_INACTIVE                                = 0,

    /**
     * <p>AF is currently performing an AF scan initiated the
     * camera device in a continuous autofocus mode.</p>
     * <p>Only used by CONTINUOUS_* AF modes. This is a transient
     * state, the camera device may skip reporting this state in
     * capture result.</p>
     */
    ACAMERA_CONTROL_AF_STATE_PASSIVE_SCAN                            = 1,

    /**
     * <p>AF currently believes it is in focus, but may
     * restart scanning at any time.</p>
     * <p>Only used by CONTINUOUS_* AF modes. This is a transient
     * state, the camera device may skip reporting this state in
     * capture result.</p>
     */
    ACAMERA_CONTROL_AF_STATE_PASSIVE_FOCUSED                         = 2,

    /**
     * <p>AF is performing an AF scan because it was
     * triggered by AF trigger.</p>
     * <p>Only used by AUTO or MACRO AF modes. This is a transient
     * state, the camera device may skip reporting this state in
     * capture result.</p>
     */
    ACAMERA_CONTROL_AF_STATE_ACTIVE_SCAN                             = 3,

    /**
     * <p>AF believes it is focused correctly and has locked
     * focus.</p>
     * <p>This state is reached only after an explicit START AF trigger has been
     * sent (ACAMERA_CONTROL_AF_TRIGGER), when good focus has been obtained.</p>
     * <p>The lens will remain stationary until the AF mode (ACAMERA_CONTROL_AF_MODE) is changed or
     * a new AF trigger is sent to the camera device (ACAMERA_CONTROL_AF_TRIGGER).</p>
     *
     * @see ACAMERA_CONTROL_AF_MODE
     * @see ACAMERA_CONTROL_AF_TRIGGER
     */
    ACAMERA_CONTROL_AF_STATE_FOCUSED_LOCKED                          = 4,

    /**
     * <p>AF has failed to focus successfully and has locked
     * focus.</p>
     * <p>This state is reached only after an explicit START AF trigger has been
     * sent (ACAMERA_CONTROL_AF_TRIGGER), when good focus cannot be obtained.</p>
     * <p>The lens will remain stationary until the AF mode (ACAMERA_CONTROL_AF_MODE) is changed or
     * a new AF trigger is sent to the camera device (ACAMERA_CONTROL_AF_TRIGGER).</p>
     *
     * @see ACAMERA_CONTROL_AF_MODE
     * @see ACAMERA_CONTROL_AF_TRIGGER
     */
    ACAMERA_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED                      = 5,

    /**
     * <p>AF finished a passive scan without finding focus,
     * and may restart scanning at any time.</p>
     * <p>Only used by CONTINUOUS_* AF modes. This is a transient state, the camera
     * device may skip reporting this state in capture result.</p>
     * <p>LEGACY camera devices do not support this state. When a passive
     * scan has finished, it will always go to PASSIVE_FOCUSED.</p>
     */
    ACAMERA_CONTROL_AF_STATE_PASSIVE_UNFOCUSED                       = 6,

} acamera_metadata_enum_android_control_af_state_t;

// ACAMERA_CONTROL_AWB_STATE
typedef enum acamera_metadata_enum_acamera_control_awb_state {
    /**
     * <p>AWB is not in auto mode, or has not yet started metering.</p>
     * <p>When a camera device is opened, it starts in this
     * state. This is a transient state, the camera device may
     * skip reporting this state in capture
     * result.</p>
     */
    ACAMERA_CONTROL_AWB_STATE_INACTIVE                               = 0,

    /**
     * <p>AWB doesn't yet have a good set of control
     * values for the current scene.</p>
     * <p>This is a transient state, the camera device
     * may skip reporting this state in capture result.</p>
     */
    ACAMERA_CONTROL_AWB_STATE_SEARCHING                              = 1,

    /**
     * <p>AWB has a good set of control values for the
     * current scene.</p>
     */
    ACAMERA_CONTROL_AWB_STATE_CONVERGED                              = 2,

    /**
     * <p>AWB has been locked.</p>
     */
    ACAMERA_CONTROL_AWB_STATE_LOCKED                                 = 3,

} acamera_metadata_enum_android_control_awb_state_t;

// ACAMERA_CONTROL_AE_LOCK_AVAILABLE
typedef enum acamera_metadata_enum_acamera_control_ae_lock_available {
    ACAMERA_CONTROL_AE_LOCK_AVAILABLE_FALSE                          = 0,

    ACAMERA_CONTROL_AE_LOCK_AVAILABLE_TRUE                           = 1,

} acamera_metadata_enum_android_control_ae_lock_available_t;

// ACAMERA_CONTROL_AWB_LOCK_AVAILABLE
typedef enum acamera_metadata_enum_acamera_control_awb_lock_available {
    ACAMERA_CONTROL_AWB_LOCK_AVAILABLE_FALSE                         = 0,

    ACAMERA_CONTROL_AWB_LOCK_AVAILABLE_TRUE                          = 1,

} acamera_metadata_enum_android_control_awb_lock_available_t;

// ACAMERA_CONTROL_ENABLE_ZSL
typedef enum acamera_metadata_enum_acamera_control_enable_zsl {
    /**
     * <p>Requests with ACAMERA_CONTROL_CAPTURE_INTENT == STILL_CAPTURE must be captured
     * after previous requests.</p>
     *
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     */
    ACAMERA_CONTROL_ENABLE_ZSL_FALSE                                 = 0,

    /**
     * <p>Requests with ACAMERA_CONTROL_CAPTURE_INTENT == STILL_CAPTURE may or may not be
     * captured before previous requests.</p>
     *
     * @see ACAMERA_CONTROL_CAPTURE_INTENT
     */
    ACAMERA_CONTROL_ENABLE_ZSL_TRUE                                  = 1,

} acamera_metadata_enum_android_control_enable_zsl_t;



// ACAMERA_EDGE_MODE
typedef enum acamera_metadata_enum_acamera_edge_mode {
    /**
     * <p>No edge enhancement is applied.</p>
     */
    ACAMERA_EDGE_MODE_OFF                                            = 0,

    /**
     * <p>Apply edge enhancement at a quality level that does not slow down frame rate
     * relative to sensor output. It may be the same as OFF if edge enhancement will
     * slow down frame rate relative to sensor.</p>
     */
    ACAMERA_EDGE_MODE_FAST                                           = 1,

    /**
     * <p>Apply high-quality edge enhancement, at a cost of possibly reduced output frame rate.</p>
     */
    ACAMERA_EDGE_MODE_HIGH_QUALITY                                   = 2,

    /**
     * <p>Edge enhancement is applied at different levels for different output streams,
     * based on resolution. Streams at maximum recording resolution (see {@link
     * ACameraDevice_createCaptureSession}) or below have
     * edge enhancement applied, while higher-resolution streams have no edge enhancement
     * applied. The level of edge enhancement for low-resolution streams is tuned so that
     * frame rate is not impacted, and the quality is equal to or better than FAST (since it
     * is only applied to lower-resolution outputs, quality may improve from FAST).</p>
     * <p>This mode is intended to be used by applications operating in a zero-shutter-lag mode
     * with YUV or PRIVATE reprocessing, where the application continuously captures
     * high-resolution intermediate buffers into a circular buffer, from which a final image is
     * produced via reprocessing when a user takes a picture.  For such a use case, the
     * high-resolution buffers must not have edge enhancement applied to maximize efficiency of
     * preview and to avoid double-applying enhancement when reprocessed, while low-resolution
     * buffers (used for recording or preview, generally) need edge enhancement applied for
     * reasonable preview quality.</p>
     * <p>This mode is guaranteed to be supported by devices that support either the
     * YUV_REPROCESSING or PRIVATE_REPROCESSING capabilities
     * (ACAMERA_REQUEST_AVAILABLE_CAPABILITIES lists either of those capabilities) and it will
     * be the default mode for CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG template.</p>
     *
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     */
    ACAMERA_EDGE_MODE_ZERO_SHUTTER_LAG                               = 3,

} acamera_metadata_enum_android_edge_mode_t;


// ACAMERA_FLASH_MODE
typedef enum acamera_metadata_enum_acamera_flash_mode {
    /**
     * <p>Do not fire the flash for this capture.</p>
     */
    ACAMERA_FLASH_MODE_OFF                                           = 0,

    /**
     * <p>If the flash is available and charged, fire flash
     * for this capture.</p>
     */
    ACAMERA_FLASH_MODE_SINGLE                                        = 1,

    /**
     * <p>Transition flash to continuously on.</p>
     */
    ACAMERA_FLASH_MODE_TORCH                                         = 2,

} acamera_metadata_enum_android_flash_mode_t;

// ACAMERA_FLASH_STATE
typedef enum acamera_metadata_enum_acamera_flash_state {
    /**
     * <p>No flash on camera.</p>
     */
    ACAMERA_FLASH_STATE_UNAVAILABLE                                  = 0,

    /**
     * <p>Flash is charging and cannot be fired.</p>
     */
    ACAMERA_FLASH_STATE_CHARGING                                     = 1,

    /**
     * <p>Flash is ready to fire.</p>
     */
    ACAMERA_FLASH_STATE_READY                                        = 2,

    /**
     * <p>Flash fired for this capture.</p>
     */
    ACAMERA_FLASH_STATE_FIRED                                        = 3,

    /**
     * <p>Flash partially illuminated this frame.</p>
     * <p>This is usually due to the next or previous frame having
     * the flash fire, and the flash spilling into this capture
     * due to hardware limitations.</p>
     */
    ACAMERA_FLASH_STATE_PARTIAL                                      = 4,

} acamera_metadata_enum_android_flash_state_t;


// ACAMERA_FLASH_INFO_AVAILABLE
typedef enum acamera_metadata_enum_acamera_flash_info_available {
    ACAMERA_FLASH_INFO_AVAILABLE_FALSE                               = 0,

    ACAMERA_FLASH_INFO_AVAILABLE_TRUE                                = 1,

} acamera_metadata_enum_android_flash_info_available_t;


// ACAMERA_HOT_PIXEL_MODE
typedef enum acamera_metadata_enum_acamera_hot_pixel_mode {
    /**
     * <p>No hot pixel correction is applied.</p>
     * <p>The frame rate must not be reduced relative to sensor raw output
     * for this option.</p>
     * <p>The hotpixel map may be returned in ACAMERA_STATISTICS_HOT_PIXEL_MAP.</p>
     *
     * @see ACAMERA_STATISTICS_HOT_PIXEL_MAP
     */
    ACAMERA_HOT_PIXEL_MODE_OFF                                       = 0,

    /**
     * <p>Hot pixel correction is applied, without reducing frame
     * rate relative to sensor raw output.</p>
     * <p>The hotpixel map may be returned in ACAMERA_STATISTICS_HOT_PIXEL_MAP.</p>
     *
     * @see ACAMERA_STATISTICS_HOT_PIXEL_MAP
     */
    ACAMERA_HOT_PIXEL_MODE_FAST                                      = 1,

    /**
     * <p>High-quality hot pixel correction is applied, at a cost
     * of possibly reduced frame rate relative to sensor raw output.</p>
     * <p>The hotpixel map may be returned in ACAMERA_STATISTICS_HOT_PIXEL_MAP.</p>
     *
     * @see ACAMERA_STATISTICS_HOT_PIXEL_MAP
     */
    ACAMERA_HOT_PIXEL_MODE_HIGH_QUALITY                              = 2,

} acamera_metadata_enum_android_hot_pixel_mode_t;



// ACAMERA_LENS_OPTICAL_STABILIZATION_MODE
typedef enum acamera_metadata_enum_acamera_lens_optical_stabilization_mode {
    /**
     * <p>Optical stabilization is unavailable.</p>
     */
    ACAMERA_LENS_OPTICAL_STABILIZATION_MODE_OFF                      = 0,

    /**
     * <p>Optical stabilization is enabled.</p>
     */
    ACAMERA_LENS_OPTICAL_STABILIZATION_MODE_ON                       = 1,

} acamera_metadata_enum_android_lens_optical_stabilization_mode_t;

// ACAMERA_LENS_FACING
typedef enum acamera_metadata_enum_acamera_lens_facing {
    /**
     * <p>The camera device faces the same direction as the device's screen.</p>
     */
    ACAMERA_LENS_FACING_FRONT                                        = 0,

    /**
     * <p>The camera device faces the opposite direction as the device's screen.</p>
     */
    ACAMERA_LENS_FACING_BACK                                         = 1,

    /**
     * <p>The camera device is an external camera, and has no fixed facing relative to the
     * device's screen.</p>
     */
    ACAMERA_LENS_FACING_EXTERNAL                                     = 2,

} acamera_metadata_enum_android_lens_facing_t;

// ACAMERA_LENS_STATE
typedef enum acamera_metadata_enum_acamera_lens_state {
    /**
     * <p>The lens parameters (ACAMERA_LENS_FOCAL_LENGTH, ACAMERA_LENS_FOCUS_DISTANCE,
     * ACAMERA_LENS_FILTER_DENSITY and ACAMERA_LENS_APERTURE) are not changing.</p>
     *
     * @see ACAMERA_LENS_APERTURE
     * @see ACAMERA_LENS_FILTER_DENSITY
     * @see ACAMERA_LENS_FOCAL_LENGTH
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     */
    ACAMERA_LENS_STATE_STATIONARY                                    = 0,

    /**
     * <p>One or several of the lens parameters
     * (ACAMERA_LENS_FOCAL_LENGTH, ACAMERA_LENS_FOCUS_DISTANCE,
     * ACAMERA_LENS_FILTER_DENSITY or ACAMERA_LENS_APERTURE) is
     * currently changing.</p>
     *
     * @see ACAMERA_LENS_APERTURE
     * @see ACAMERA_LENS_FILTER_DENSITY
     * @see ACAMERA_LENS_FOCAL_LENGTH
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     */
    ACAMERA_LENS_STATE_MOVING                                        = 1,

} acamera_metadata_enum_android_lens_state_t;


// ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION
typedef enum acamera_metadata_enum_acamera_lens_info_focus_distance_calibration {
    /**
     * <p>The lens focus distance is not accurate, and the units used for
     * ACAMERA_LENS_FOCUS_DISTANCE do not correspond to any physical units.</p>
     * <p>Setting the lens to the same focus distance on separate occasions may
     * result in a different real focus distance, depending on factors such
     * as the orientation of the device, the age of the focusing mechanism,
     * and the device temperature. The focus distance value will still be
     * in the range of <code>[0, ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE]</code>, where 0
     * represents the farthest focus.</p>
     *
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     * @see ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE
     */
    ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED        = 0,

    /**
     * <p>The lens focus distance is measured in diopters.</p>
     * <p>However, setting the lens to the same focus distance
     * on separate occasions may result in a different real
     * focus distance, depending on factors such as the
     * orientation of the device, the age of the focusing
     * mechanism, and the device temperature.</p>
     */
    ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_APPROXIMATE         = 1,

    /**
     * <p>The lens focus distance is measured in diopters, and
     * is calibrated.</p>
     * <p>The lens mechanism is calibrated so that setting the
     * same focus distance is repeatable on multiple
     * occasions with good accuracy, and the focus distance
     * corresponds to the real physical distance to the plane
     * of best focus.</p>
     */
    ACAMERA_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED          = 2,

} acamera_metadata_enum_android_lens_info_focus_distance_calibration_t;


// ACAMERA_NOISE_REDUCTION_MODE
typedef enum acamera_metadata_enum_acamera_noise_reduction_mode {
    /**
     * <p>No noise reduction is applied.</p>
     */
    ACAMERA_NOISE_REDUCTION_MODE_OFF                                 = 0,

    /**
     * <p>Noise reduction is applied without reducing frame rate relative to sensor
     * output. It may be the same as OFF if noise reduction will reduce frame rate
     * relative to sensor.</p>
     */
    ACAMERA_NOISE_REDUCTION_MODE_FAST                                = 1,

    /**
     * <p>High-quality noise reduction is applied, at the cost of possibly reduced frame
     * rate relative to sensor output.</p>
     */
    ACAMERA_NOISE_REDUCTION_MODE_HIGH_QUALITY                        = 2,

    /**
     * <p>MINIMAL noise reduction is applied without reducing frame rate relative to
     * sensor output. </p>
     */
    ACAMERA_NOISE_REDUCTION_MODE_MINIMAL                             = 3,

    /**
     * <p>Noise reduction is applied at different levels for different output streams,
     * based on resolution. Streams at maximum recording resolution (see {@link
     * ACameraDevice_createCaptureSession}) or below have noise
     * reduction applied, while higher-resolution streams have MINIMAL (if supported) or no
     * noise reduction applied (if MINIMAL is not supported.) The degree of noise reduction
     * for low-resolution streams is tuned so that frame rate is not impacted, and the quality
     * is equal to or better than FAST (since it is only applied to lower-resolution outputs,
     * quality may improve from FAST).</p>
     * <p>This mode is intended to be used by applications operating in a zero-shutter-lag mode
     * with YUV or PRIVATE reprocessing, where the application continuously captures
     * high-resolution intermediate buffers into a circular buffer, from which a final image is
     * produced via reprocessing when a user takes a picture.  For such a use case, the
     * high-resolution buffers must not have noise reduction applied to maximize efficiency of
     * preview and to avoid over-applying noise filtering when reprocessing, while
     * low-resolution buffers (used for recording or preview, generally) need noise reduction
     * applied for reasonable preview quality.</p>
     * <p>This mode is guaranteed to be supported by devices that support either the
     * YUV_REPROCESSING or PRIVATE_REPROCESSING capabilities
     * (ACAMERA_REQUEST_AVAILABLE_CAPABILITIES lists either of those capabilities) and it will
     * be the default mode for CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG template.</p>
     *
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     */
    ACAMERA_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG                    = 4,

} acamera_metadata_enum_android_noise_reduction_mode_t;



// ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
typedef enum acamera_metadata_enum_acamera_request_available_capabilities {
    /**
     * <p>The minimal set of capabilities that every camera
     * device (regardless of ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL)
     * supports.</p>
     * <p>This capability is listed by all normal devices, and
     * indicates that the camera device has a feature set
     * that's comparable to the baseline requirements for the
     * older android.hardware.Camera API.</p>
     * <p>Devices with the DEPTH_OUTPUT capability might not list this
     * capability, indicating that they support only depth measurement,
     * not standard color output.</p>
     *
     * @see ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL
     */
    ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE       = 0,

    /**
     * <p>The camera device can be manually controlled (3A algorithms such
     * as auto-exposure, and auto-focus can be bypassed).
     * The camera device supports basic manual control of the sensor image
     * acquisition related stages. This means the following controls are
     * guaranteed to be supported:</p>
     * <ul>
     * <li>Manual frame duration control<ul>
     * <li>ACAMERA_SENSOR_FRAME_DURATION</li>
     * <li>ACAMERA_SENSOR_INFO_MAX_FRAME_DURATION</li>
     * </ul>
     * </li>
     * <li>Manual exposure control<ul>
     * <li>ACAMERA_SENSOR_EXPOSURE_TIME</li>
     * <li>ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE</li>
     * </ul>
     * </li>
     * <li>Manual sensitivity control<ul>
     * <li>ACAMERA_SENSOR_SENSITIVITY</li>
     * <li>ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE</li>
     * </ul>
     * </li>
     * <li>Manual lens control (if the lens is adjustable)<ul>
     * <li>ACAMERA_LENS_*</li>
     * </ul>
     * </li>
     * <li>Manual flash control (if a flash unit is present)<ul>
     * <li>ACAMERA_FLASH_*</li>
     * </ul>
     * </li>
     * <li>Manual black level locking<ul>
     * <li>ACAMERA_BLACK_LEVEL_LOCK</li>
     * </ul>
     * </li>
     * <li>Auto exposure lock<ul>
     * <li>ACAMERA_CONTROL_AE_LOCK</li>
     * </ul>
     * </li>
     * </ul>
     * <p>If any of the above 3A algorithms are enabled, then the camera
     * device will accurately report the values applied by 3A in the
     * result.</p>
     * <p>A given camera device may also support additional manual sensor controls,
     * but this capability only covers the above list of controls.</p>
     * <p>If this is supported, android.scaler.streamConfigurationMap will
     * additionally return a min frame duration that is greater than
     * zero for each supported size-format combination.</p>
     *
     * @see ACAMERA_BLACK_LEVEL_LOCK
     * @see ACAMERA_CONTROL_AE_LOCK
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_FRAME_DURATION
     * @see ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE
     * @see ACAMERA_SENSOR_INFO_MAX_FRAME_DURATION
     * @see ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR             = 1,

    /**
     * <p>The camera device post-processing stages can be manually controlled.
     * The camera device supports basic manual control of the image post-processing
     * stages. This means the following controls are guaranteed to be supported:</p>
     * <ul>
     * <li>
     * <p>Manual tonemap control</p>
     * <ul>
     * <li>android.tonemap.curve</li>
     * <li>ACAMERA_TONEMAP_MODE</li>
     * <li>ACAMERA_TONEMAP_MAX_CURVE_POINTS</li>
     * <li>ACAMERA_TONEMAP_GAMMA</li>
     * <li>ACAMERA_TONEMAP_PRESET_CURVE</li>
     * </ul>
     * </li>
     * <li>
     * <p>Manual white balance control</p>
     * <ul>
     * <li>ACAMERA_COLOR_CORRECTION_TRANSFORM</li>
     * <li>ACAMERA_COLOR_CORRECTION_GAINS</li>
     * </ul>
     * </li>
     * <li>Manual lens shading map control<ul>
     * <li>ACAMERA_SHADING_MODE</li>
     * <li>ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE</li>
     * <li>ACAMERA_STATISTICS_LENS_SHADING_MAP</li>
     * <li>ACAMERA_LENS_INFO_SHADING_MAP_SIZE</li>
     * </ul>
     * </li>
     * <li>Manual aberration correction control (if aberration correction is supported)<ul>
     * <li>ACAMERA_COLOR_CORRECTION_ABERRATION_MODE</li>
     * <li>ACAMERA_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES</li>
     * </ul>
     * </li>
     * <li>Auto white balance lock<ul>
     * <li>ACAMERA_CONTROL_AWB_LOCK</li>
     * </ul>
     * </li>
     * </ul>
     * <p>If auto white balance is enabled, then the camera device
     * will accurately report the values applied by AWB in the result.</p>
     * <p>A given camera device may also support additional post-processing
     * controls, but this capability only covers the above list of controls.</p>
     *
     * @see ACAMERA_COLOR_CORRECTION_ABERRATION_MODE
     * @see ACAMERA_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES
     * @see ACAMERA_COLOR_CORRECTION_GAINS
     * @see ACAMERA_COLOR_CORRECTION_TRANSFORM
     * @see ACAMERA_CONTROL_AWB_LOCK
     * @see ACAMERA_LENS_INFO_SHADING_MAP_SIZE
     * @see ACAMERA_SHADING_MODE
     * @see ACAMERA_STATISTICS_LENS_SHADING_MAP
     * @see ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE
     * @see ACAMERA_TONEMAP_GAMMA
     * @see ACAMERA_TONEMAP_MAX_CURVE_POINTS
     * @see ACAMERA_TONEMAP_MODE
     * @see ACAMERA_TONEMAP_PRESET_CURVE
     */
    ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING    = 2,

    /**
     * <p>The camera device supports outputting RAW buffers and
     * metadata for interpreting them.</p>
     * <p>Devices supporting the RAW capability allow both for
     * saving DNG files, and for direct application processing of
     * raw sensor images.</p>
     * <ul>
     * <li>RAW_SENSOR is supported as an output format.</li>
     * <li>The maximum available resolution for RAW_SENSOR streams
     *   will match either the value in
     *   ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE or
     *   ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE.</li>
     * <li>All DNG-related optional metadata entries are provided
     *   by the camera device.</li>
     * </ul>
     *
     * @see ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE
     * @see ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE
     */
    ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_RAW                       = 3,

    /**
     * <p>The camera device supports accurately reporting the sensor settings for many of
     * the sensor controls while the built-in 3A algorithm is running.  This allows
     * reporting of sensor settings even when these settings cannot be manually changed.</p>
     * <p>The values reported for the following controls are guaranteed to be available
     * in the CaptureResult, including when 3A is enabled:</p>
     * <ul>
     * <li>Exposure control<ul>
     * <li>ACAMERA_SENSOR_EXPOSURE_TIME</li>
     * </ul>
     * </li>
     * <li>Sensitivity control<ul>
     * <li>ACAMERA_SENSOR_SENSITIVITY</li>
     * </ul>
     * </li>
     * <li>Lens controls (if the lens is adjustable)<ul>
     * <li>ACAMERA_LENS_FOCUS_DISTANCE</li>
     * <li>ACAMERA_LENS_APERTURE</li>
     * </ul>
     * </li>
     * </ul>
     * <p>This capability is a subset of the MANUAL_SENSOR control capability, and will
     * always be included if the MANUAL_SENSOR capability is available.</p>
     *
     * @see ACAMERA_LENS_APERTURE
     * @see ACAMERA_LENS_FOCUS_DISTANCE
     * @see ACAMERA_SENSOR_EXPOSURE_TIME
     * @see ACAMERA_SENSOR_SENSITIVITY
     */
    ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_READ_SENSOR_SETTINGS      = 5,

    /**
     * <p>The camera device supports capturing high-resolution images at &gt;= 20 frames per
     * second, in at least the uncompressed YUV format, when post-processing settings are set
     * to FAST. Additionally, maximum-resolution images can be captured at &gt;= 10 frames
     * per second.  Here, 'high resolution' means at least 8 megapixels, or the maximum
     * resolution of the device, whichever is smaller.</p>
     * <p>More specifically, this means that at least one output {@link
     * AIMAGE_FORMAT_YUV_420_888} size listed in
     * {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS} is larger or equal to the
     * 'high resolution' defined above, and can be captured at at least 20 fps.
     * For the largest {@link AIMAGE_FORMAT_YUV_420_888} size listed in
     * {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS}, camera device can capture this
     * size for at least 10 frames per second.
     * Also the ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES entry lists at least one FPS range
     * where the minimum FPS is &gt;= 1 / minimumFrameDuration for the largest YUV_420_888 size.</p>
     * <p>If the device supports the {@link AIMAGE_FORMAT_RAW10}, {@link
     * AIMAGE_FORMAT_RAW12}, then those can also be captured at the same rate
     * as the maximum-size YUV_420_888 resolution is.</p>
     * <p>In addition, the ACAMERA_SYNC_MAX_LATENCY field is guaranted to have a value between 0
     * and 4, inclusive. ACAMERA_CONTROL_AE_LOCK_AVAILABLE and ACAMERA_CONTROL_AWB_LOCK_AVAILABLE
     * are also guaranteed to be <code>true</code> so burst capture with these two locks ON yields
     * consistent image output.</p>
     *
     * @see ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES
     * @see ACAMERA_CONTROL_AE_LOCK_AVAILABLE
     * @see ACAMERA_CONTROL_AWB_LOCK_AVAILABLE
     * @see ACAMERA_SYNC_MAX_LATENCY
     */
    ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE             = 6,

    /**
     * <p>The camera device can produce depth measurements from its field of view.</p>
     * <p>This capability requires the camera device to support the following:</p>
     * <ul>
     * <li>{@link AIMAGE_FORMAT_DEPTH16} is supported as an output format.</li>
     * <li>{@link AIMAGE_FORMAT_DEPTH_POINT_CLOUD} is optionally supported as an
     *   output format.</li>
     * <li>This camera device, and all camera devices with the same ACAMERA_LENS_FACING,
     *   will list the following calibration entries in {@link ACameraMetadata} from both
     *   {@link ACameraManager_getCameraCharacteristics} and
     *   {@link ACameraCaptureSession_captureCallback_result}:<ul>
     * <li>ACAMERA_LENS_POSE_TRANSLATION</li>
     * <li>ACAMERA_LENS_POSE_ROTATION</li>
     * <li>ACAMERA_LENS_INTRINSIC_CALIBRATION</li>
     * <li>ACAMERA_LENS_RADIAL_DISTORTION</li>
     * </ul>
     * </li>
     * <li>The ACAMERA_DEPTH_DEPTH_IS_EXCLUSIVE entry is listed by this device.</li>
     * <li>A LIMITED camera with only the DEPTH_OUTPUT capability does not have to support
     *   normal YUV_420_888, JPEG, and PRIV-format outputs. It only has to support the DEPTH16
     *   format.</li>
     * </ul>
     * <p>Generally, depth output operates at a slower frame rate than standard color capture,
     * so the DEPTH16 and DEPTH_POINT_CLOUD formats will commonly have a stall duration that
     * should be accounted for (see
     * {@link ACAMERA_DEPTH_AVAILABLE_DEPTH_STALL_DURATIONS}).
     * On a device that supports both depth and color-based output, to enable smooth preview,
     * using a repeating burst is recommended, where a depth-output target is only included
     * once every N frames, where N is the ratio between preview output rate and depth output
     * rate, including depth stall time.</p>
     *
     * @see ACAMERA_DEPTH_DEPTH_IS_EXCLUSIVE
     * @see ACAMERA_LENS_FACING
     * @see ACAMERA_LENS_INTRINSIC_CALIBRATION
     * @see ACAMERA_LENS_POSE_ROTATION
     * @see ACAMERA_LENS_POSE_TRANSLATION
     * @see ACAMERA_LENS_RADIAL_DISTORTION
     */
    ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_DEPTH_OUTPUT              = 8,

} acamera_metadata_enum_android_request_available_capabilities_t;


// ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS
typedef enum acamera_metadata_enum_acamera_scaler_available_stream_configurations {
    ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT            = 0,

    ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT             = 1,

} acamera_metadata_enum_android_scaler_available_stream_configurations_t;

// ACAMERA_SCALER_CROPPING_TYPE
typedef enum acamera_metadata_enum_acamera_scaler_cropping_type {
    /**
     * <p>The camera device only supports centered crop regions.</p>
     */
    ACAMERA_SCALER_CROPPING_TYPE_CENTER_ONLY                         = 0,

    /**
     * <p>The camera device supports arbitrarily chosen crop regions.</p>
     */
    ACAMERA_SCALER_CROPPING_TYPE_FREEFORM                            = 1,

} acamera_metadata_enum_android_scaler_cropping_type_t;


// ACAMERA_SENSOR_REFERENCE_ILLUMINANT1
typedef enum acamera_metadata_enum_acamera_sensor_reference_illuminant1 {
    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT                    = 1,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_FLUORESCENT                 = 2,

    /**
     * <p>Incandescent light</p>
     */
    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_TUNGSTEN                    = 3,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_FLASH                       = 4,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_FINE_WEATHER                = 9,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_CLOUDY_WEATHER              = 10,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_SHADE                       = 11,

    /**
     * <p>D 5700 - 7100K</p>
     */
    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT_FLUORESCENT        = 12,

    /**
     * <p>N 4600 - 5400K</p>
     */
    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_DAY_WHITE_FLUORESCENT       = 13,

    /**
     * <p>W 3900 - 4500K</p>
     */
    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_COOL_WHITE_FLUORESCENT      = 14,

    /**
     * <p>WW 3200 - 3700K</p>
     */
    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_WHITE_FLUORESCENT           = 15,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A                  = 17,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_B                  = 18,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_C                  = 19,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_D55                         = 20,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_D65                         = 21,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_D75                         = 22,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_D50                         = 23,

    ACAMERA_SENSOR_REFERENCE_ILLUMINANT1_ISO_STUDIO_TUNGSTEN         = 24,

} acamera_metadata_enum_android_sensor_reference_illuminant1_t;

// ACAMERA_SENSOR_TEST_PATTERN_MODE
typedef enum acamera_metadata_enum_acamera_sensor_test_pattern_mode {
    /**
     * <p>No test pattern mode is used, and the camera
     * device returns captures from the image sensor.</p>
     * <p>This is the default if the key is not set.</p>
     */
    ACAMERA_SENSOR_TEST_PATTERN_MODE_OFF                             = 0,

    /**
     * <p>Each pixel in <code>[R, G_even, G_odd, B]</code> is replaced by its
     * respective color channel provided in
     * ACAMERA_SENSOR_TEST_PATTERN_DATA.</p>
     * <p>For example:</p>
     * <pre><code>android.testPatternData = [0, 0xFFFFFFFF, 0xFFFFFFFF, 0]
     * </code></pre>
     * <p>All green pixels are 100% green. All red/blue pixels are black.</p>
     * <pre><code>android.testPatternData = [0xFFFFFFFF, 0, 0xFFFFFFFF, 0]
     * </code></pre>
     * <p>All red pixels are 100% red. Only the odd green pixels
     * are 100% green. All blue pixels are 100% black.</p>
     *
     * @see ACAMERA_SENSOR_TEST_PATTERN_DATA
     */
    ACAMERA_SENSOR_TEST_PATTERN_MODE_SOLID_COLOR                     = 1,

    /**
     * <p>All pixel data is replaced with an 8-bar color pattern.</p>
     * <p>The vertical bars (left-to-right) are as follows:</p>
     * <ul>
     * <li>100% white</li>
     * <li>yellow</li>
     * <li>cyan</li>
     * <li>green</li>
     * <li>magenta</li>
     * <li>red</li>
     * <li>blue</li>
     * <li>black</li>
     * </ul>
     * <p>In general the image would look like the following:</p>
     * <pre><code>W Y C G M R B K
     * W Y C G M R B K
     * W Y C G M R B K
     * W Y C G M R B K
     * W Y C G M R B K
     * . . . . . . . .
     * . . . . . . . .
     * . . . . . . . .
     *
     * (B = Blue, K = Black)
     * </code></pre>
     * <p>Each bar should take up 1/8 of the sensor pixel array width.
     * When this is not possible, the bar size should be rounded
     * down to the nearest integer and the pattern can repeat
     * on the right side.</p>
     * <p>Each bar's height must always take up the full sensor
     * pixel array height.</p>
     * <p>Each pixel in this test pattern must be set to either
     * 0% intensity or 100% intensity.</p>
     */
    ACAMERA_SENSOR_TEST_PATTERN_MODE_COLOR_BARS                      = 2,

    /**
     * <p>The test pattern is similar to COLOR_BARS, except that
     * each bar should start at its specified color at the top,
     * and fade to gray at the bottom.</p>
     * <p>Furthermore each bar is further subdivided into a left and
     * right half. The left half should have a smooth gradient,
     * and the right half should have a quantized gradient.</p>
     * <p>In particular, the right half's should consist of blocks of the
     * same color for 1/16th active sensor pixel array width.</p>
     * <p>The least significant bits in the quantized gradient should
     * be copied from the most significant bits of the smooth gradient.</p>
     * <p>The height of each bar should always be a multiple of 128.
     * When this is not the case, the pattern should repeat at the bottom
     * of the image.</p>
     */
    ACAMERA_SENSOR_TEST_PATTERN_MODE_COLOR_BARS_FADE_TO_GRAY         = 3,

    /**
     * <p>All pixel data is replaced by a pseudo-random sequence
     * generated from a PN9 512-bit sequence (typically implemented
     * in hardware with a linear feedback shift register).</p>
     * <p>The generator should be reset at the beginning of each frame,
     * and thus each subsequent raw frame with this test pattern should
     * be exactly the same as the last.</p>
     */
    ACAMERA_SENSOR_TEST_PATTERN_MODE_PN9                             = 4,

    /**
     * <p>The first custom test pattern. All custom patterns that are
     * available only on this camera device are at least this numeric
     * value.</p>
     * <p>All of the custom test patterns will be static
     * (that is the raw image must not vary from frame to frame).</p>
     */
    ACAMERA_SENSOR_TEST_PATTERN_MODE_CUSTOM1                         = 256,

} acamera_metadata_enum_android_sensor_test_pattern_mode_t;


// ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT
typedef enum acamera_metadata_enum_acamera_sensor_info_color_filter_arrangement {
    ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB                = 0,

    ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG                = 1,

    ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG                = 2,

    ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR                = 3,

    /**
     * <p>Sensor is not Bayer; output has 3 16-bit
     * values for each pixel, instead of just 1 16-bit value
     * per pixel.</p>
     */
    ACAMERA_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB                 = 4,

} acamera_metadata_enum_android_sensor_info_color_filter_arrangement_t;

// ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE
typedef enum acamera_metadata_enum_acamera_sensor_info_timestamp_source {
    /**
     * <p>Timestamps from ACAMERA_SENSOR_TIMESTAMP are in nanoseconds and monotonic,
     * but can not be compared to timestamps from other subsystems
     * (e.g. accelerometer, gyro etc.), or other instances of the same or different
     * camera devices in the same system. Timestamps between streams and results for
     * a single camera instance are comparable, and the timestamps for all buffers
     * and the result metadata generated by a single capture are identical.</p>
     *
     * @see ACAMERA_SENSOR_TIMESTAMP
     */
    ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN                     = 0,

    /**
     * <p>Timestamps from ACAMERA_SENSOR_TIMESTAMP are in the same timebase as
     * <a href="https://developer.android.com/reference/android/os/SystemClock.html#elapsedRealtimeNanos">elapsedRealtimeNanos</a>
     * (or CLOCK_BOOTTIME), and they can be compared to other timestamps using that base.</p>
     *
     * @see ACAMERA_SENSOR_TIMESTAMP
     */
    ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME                    = 1,

} acamera_metadata_enum_android_sensor_info_timestamp_source_t;

// ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED
typedef enum acamera_metadata_enum_acamera_sensor_info_lens_shading_applied {
    ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED_FALSE                   = 0,

    ACAMERA_SENSOR_INFO_LENS_SHADING_APPLIED_TRUE                    = 1,

} acamera_metadata_enum_android_sensor_info_lens_shading_applied_t;


// ACAMERA_SHADING_MODE
typedef enum acamera_metadata_enum_acamera_shading_mode {
    /**
     * <p>No lens shading correction is applied.</p>
     */
    ACAMERA_SHADING_MODE_OFF                                         = 0,

    /**
     * <p>Apply lens shading corrections, without slowing
     * frame rate relative to sensor raw output</p>
     */
    ACAMERA_SHADING_MODE_FAST                                        = 1,

    /**
     * <p>Apply high-quality lens shading correction, at the
     * cost of possibly reduced frame rate.</p>
     */
    ACAMERA_SHADING_MODE_HIGH_QUALITY                                = 2,

} acamera_metadata_enum_android_shading_mode_t;


// ACAMERA_STATISTICS_FACE_DETECT_MODE
typedef enum acamera_metadata_enum_acamera_statistics_face_detect_mode {
    /**
     * <p>Do not include face detection statistics in capture
     * results.</p>
     */
    ACAMERA_STATISTICS_FACE_DETECT_MODE_OFF                          = 0,

    /**
     * <p>Return face rectangle and confidence values only.</p>
     */
    ACAMERA_STATISTICS_FACE_DETECT_MODE_SIMPLE                       = 1,

    /**
     * <p>Return all face
     * metadata.</p>
     * <p>In this mode, face rectangles, scores, landmarks, and face IDs are all valid.</p>
     */
    ACAMERA_STATISTICS_FACE_DETECT_MODE_FULL                         = 2,

} acamera_metadata_enum_android_statistics_face_detect_mode_t;

// ACAMERA_STATISTICS_HOT_PIXEL_MAP_MODE
typedef enum acamera_metadata_enum_acamera_statistics_hot_pixel_map_mode {
    /**
     * <p>Hot pixel map production is disabled.</p>
     */
    ACAMERA_STATISTICS_HOT_PIXEL_MAP_MODE_OFF                        = 0,

    /**
     * <p>Hot pixel map production is enabled.</p>
     */
    ACAMERA_STATISTICS_HOT_PIXEL_MAP_MODE_ON                         = 1,

} acamera_metadata_enum_android_statistics_hot_pixel_map_mode_t;

// ACAMERA_STATISTICS_SCENE_FLICKER
typedef enum acamera_metadata_enum_acamera_statistics_scene_flicker {
    /**
     * <p>The camera device does not detect any flickering illumination
     * in the current scene.</p>
     */
    ACAMERA_STATISTICS_SCENE_FLICKER_NONE                            = 0,

    /**
     * <p>The camera device detects illumination flickering at 50Hz
     * in the current scene.</p>
     */
    ACAMERA_STATISTICS_SCENE_FLICKER_50HZ                            = 1,

    /**
     * <p>The camera device detects illumination flickering at 60Hz
     * in the current scene.</p>
     */
    ACAMERA_STATISTICS_SCENE_FLICKER_60HZ                            = 2,

} acamera_metadata_enum_android_statistics_scene_flicker_t;

// ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE
typedef enum acamera_metadata_enum_acamera_statistics_lens_shading_map_mode {
    /**
     * <p>Do not include a lens shading map in the capture result.</p>
     */
    ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE_OFF                     = 0,

    /**
     * <p>Include a lens shading map in the capture result.</p>
     */
    ACAMERA_STATISTICS_LENS_SHADING_MAP_MODE_ON                      = 1,

} acamera_metadata_enum_android_statistics_lens_shading_map_mode_t;



// ACAMERA_TONEMAP_MODE
typedef enum acamera_metadata_enum_acamera_tonemap_mode {
    /**
     * <p>Use the tone mapping curve specified in
     * the ACAMERA_TONEMAPCURVE_* entries.</p>
     * <p>All color enhancement and tonemapping must be disabled, except
     * for applying the tonemapping curve specified by
     * android.tonemap.curve.</p>
     * <p>Must not slow down frame rate relative to raw
     * sensor output.</p>
     */
    ACAMERA_TONEMAP_MODE_CONTRAST_CURVE                              = 0,

    /**
     * <p>Advanced gamma mapping and color enhancement may be applied, without
     * reducing frame rate compared to raw sensor output.</p>
     */
    ACAMERA_TONEMAP_MODE_FAST                                        = 1,

    /**
     * <p>High-quality gamma mapping and color enhancement will be applied, at
     * the cost of possibly reduced frame rate compared to raw sensor output.</p>
     */
    ACAMERA_TONEMAP_MODE_HIGH_QUALITY                                = 2,

    /**
     * <p>Use the gamma value specified in ACAMERA_TONEMAP_GAMMA to peform
     * tonemapping.</p>
     * <p>All color enhancement and tonemapping must be disabled, except
     * for applying the tonemapping curve specified by ACAMERA_TONEMAP_GAMMA.</p>
     * <p>Must not slow down frame rate relative to raw sensor output.</p>
     *
     * @see ACAMERA_TONEMAP_GAMMA
     */
    ACAMERA_TONEMAP_MODE_GAMMA_VALUE                                 = 3,

    /**
     * <p>Use the preset tonemapping curve specified in
     * ACAMERA_TONEMAP_PRESET_CURVE to peform tonemapping.</p>
     * <p>All color enhancement and tonemapping must be disabled, except
     * for applying the tonemapping curve specified by
     * ACAMERA_TONEMAP_PRESET_CURVE.</p>
     * <p>Must not slow down frame rate relative to raw sensor output.</p>
     *
     * @see ACAMERA_TONEMAP_PRESET_CURVE
     */
    ACAMERA_TONEMAP_MODE_PRESET_CURVE                                = 4,

} acamera_metadata_enum_android_tonemap_mode_t;

// ACAMERA_TONEMAP_PRESET_CURVE
typedef enum acamera_metadata_enum_acamera_tonemap_preset_curve {
    /**
     * <p>Tonemapping curve is defined by sRGB</p>
     */
    ACAMERA_TONEMAP_PRESET_CURVE_SRGB                                = 0,

    /**
     * <p>Tonemapping curve is defined by ITU-R BT.709</p>
     */
    ACAMERA_TONEMAP_PRESET_CURVE_REC709                              = 1,

} acamera_metadata_enum_android_tonemap_preset_curve_t;



// ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL
typedef enum acamera_metadata_enum_acamera_info_supported_hardware_level {
    /**
     * <p>This camera device does not have enough capabilities to qualify as a <code>FULL</code> device or
     * better.</p>
     * <p>Only the stream configurations listed in the <code>LEGACY</code> and <code>LIMITED</code> tables in the
     * {@link ACameraDevice_createCaptureSession} documentation are guaranteed to be supported.</p>
     * <p>All <code>LIMITED</code> devices support the <code>BACKWARDS_COMPATIBLE</code> capability, indicating basic
     * support for color image capture. The only exception is that the device may
     * alternatively support only the <code>DEPTH_OUTPUT</code> capability, if it can only output depth
     * measurements and not color images.</p>
     * <p><code>LIMITED</code> devices and above require the use of ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * to lock exposure metering (and calculate flash power, for cameras with flash) before
     * capturing a high-quality still image.</p>
     * <p>A <code>LIMITED</code> device that only lists the <code>BACKWARDS_COMPATIBLE</code> capability is only
     * required to support full-automatic operation and post-processing (<code>OFF</code> is not
     * supported for ACAMERA_CONTROL_AE_MODE, ACAMERA_CONTROL_AF_MODE, or
     * ACAMERA_CONTROL_AWB_MODE)</p>
     * <p>Additional capabilities may optionally be supported by a <code>LIMITED</code>-level device, and
     * can be checked for in ACAMERA_REQUEST_AVAILABLE_CAPABILITIES.</p>
     *
     * @see ACAMERA_CONTROL_AE_MODE
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * @see ACAMERA_CONTROL_AF_MODE
     * @see ACAMERA_CONTROL_AWB_MODE
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     */
    ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED                    = 0,

    /**
     * <p>This camera device is capable of supporting advanced imaging applications.</p>
     * <p>The stream configurations listed in the <code>FULL</code>, <code>LEGACY</code> and <code>LIMITED</code> tables in the
     * {@link ACameraDevice_createCaptureSession} documentation are guaranteed to be supported.</p>
     * <p>A <code>FULL</code> device will support below capabilities:</p>
     * <ul>
     * <li><code>BURST_CAPTURE</code> capability (ACAMERA_REQUEST_AVAILABLE_CAPABILITIES contains
     *   <code>BURST_CAPTURE</code>)</li>
     * <li>Per frame control (ACAMERA_SYNC_MAX_LATENCY <code>==</code> PER_FRAME_CONTROL)</li>
     * <li>Manual sensor control (ACAMERA_REQUEST_AVAILABLE_CAPABILITIES contains <code>MANUAL_SENSOR</code>)</li>
     * <li>Manual post-processing control (ACAMERA_REQUEST_AVAILABLE_CAPABILITIES contains
     *   <code>MANUAL_POST_PROCESSING</code>)</li>
     * <li>The required exposure time range defined in ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE</li>
     * <li>The required maxFrameDuration defined in ACAMERA_SENSOR_INFO_MAX_FRAME_DURATION</li>
     * </ul>
     * <p>Note:
     * Pre-API level 23, FULL devices also supported arbitrary cropping region
     * (ACAMERA_SCALER_CROPPING_TYPE <code>== FREEFORM</code>); this requirement was relaxed in API level
     * 23, and <code>FULL</code> devices may only support <code>CENTERED</code> cropping.</p>
     *
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     * @see ACAMERA_SCALER_CROPPING_TYPE
     * @see ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE
     * @see ACAMERA_SENSOR_INFO_MAX_FRAME_DURATION
     * @see ACAMERA_SYNC_MAX_LATENCY
     */
    ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_FULL                       = 1,

    /**
     * <p>This camera device is running in backward compatibility mode.</p>
     * <p>Only the stream configurations listed in the <code>LEGACY</code> table in the {@link
     * ACameraDevice_createCaptureSession} documentation are supported.</p>
     * <p>A <code>LEGACY</code> device does not support per-frame control, manual sensor control, manual
     * post-processing, arbitrary cropping regions, and has relaxed performance constraints.
     * No additional capabilities beyond <code>BACKWARD_COMPATIBLE</code> will ever be listed by a
     * <code>LEGACY</code> device in ACAMERA_REQUEST_AVAILABLE_CAPABILITIES.</p>
     * <p>In addition, the ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER is not functional on <code>LEGACY</code>
     * devices. Instead, every request that includes a JPEG-format output target is treated
     * as triggering a still capture, internally executing a precapture trigger.  This may
     * fire the flash for flash power metering during precapture, and then fire the flash
     * for the final capture, if a flash is available on the device and the AE mode is set to
     * enable the flash.</p>
     *
     * @see ACAMERA_CONTROL_AE_PRECAPTURE_TRIGGER
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     */
    ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY                     = 2,

    /**
     * <p>This camera device is capable of YUV reprocessing and RAW data capture, in addition to
     * FULL-level capabilities.</p>
     * <p>The stream configurations listed in the <code>LEVEL_3</code>, <code>RAW</code>, <code>FULL</code>, <code>LEGACY</code> and
     * <code>LIMITED</code> tables in the {@link
     * ACameraDevice_createCaptureSession}
     * documentation are guaranteed to be supported.</p>
     * <p>The following additional capabilities are guaranteed to be supported:</p>
     * <ul>
     * <li><code>YUV_REPROCESSING</code> capability (ACAMERA_REQUEST_AVAILABLE_CAPABILITIES contains
     *   <code>YUV_REPROCESSING</code>)</li>
     * <li><code>RAW</code> capability (ACAMERA_REQUEST_AVAILABLE_CAPABILITIES contains
     *   <code>RAW</code>)</li>
     * </ul>
     *
     * @see ACAMERA_REQUEST_AVAILABLE_CAPABILITIES
     */
    ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_3                          = 3,

} acamera_metadata_enum_android_info_supported_hardware_level_t;


// ACAMERA_BLACK_LEVEL_LOCK
typedef enum acamera_metadata_enum_acamera_black_level_lock {
    ACAMERA_BLACK_LEVEL_LOCK_OFF                                     = 0,

    ACAMERA_BLACK_LEVEL_LOCK_ON                                      = 1,

} acamera_metadata_enum_android_black_level_lock_t;


// ACAMERA_SYNC_FRAME_NUMBER
typedef enum acamera_metadata_enum_acamera_sync_frame_number {
    /**
     * <p>The current result is not yet fully synchronized to any request.</p>
     * <p>Synchronization is in progress, and reading metadata from this
     * result may include a mix of data that have taken effect since the
     * last synchronization time.</p>
     * <p>In some future result, within ACAMERA_SYNC_MAX_LATENCY frames,
     * this value will update to the actual frame number frame number
     * the result is guaranteed to be synchronized to (as long as the
     * request settings remain constant).</p>
     *
     * @see ACAMERA_SYNC_MAX_LATENCY
     */
    ACAMERA_SYNC_FRAME_NUMBER_CONVERGING                             = -1,

    /**
     * <p>The current result's synchronization status is unknown.</p>
     * <p>The result may have already converged, or it may be in
     * progress.  Reading from this result may include some mix
     * of settings from past requests.</p>
     * <p>After a settings change, the new settings will eventually all
     * take effect for the output buffers and results. However, this
     * value will not change when that happens. Altering settings
     * rapidly may provide outcomes using mixes of settings from recent
     * requests.</p>
     * <p>This value is intended primarily for backwards compatibility with
     * the older camera implementations (for android.hardware.Camera).</p>
     */
    ACAMERA_SYNC_FRAME_NUMBER_UNKNOWN                                = -2,

} acamera_metadata_enum_android_sync_frame_number_t;

// ACAMERA_SYNC_MAX_LATENCY
typedef enum acamera_metadata_enum_acamera_sync_max_latency {
    /**
     * <p>Every frame has the requests immediately applied.</p>
     * <p>Changing controls over multiple requests one after another will
     * produce results that have those controls applied atomically
     * each frame.</p>
     * <p>All FULL capability devices will have this as their maxLatency.</p>
     */
    ACAMERA_SYNC_MAX_LATENCY_PER_FRAME_CONTROL                       = 0,

    /**
     * <p>Each new frame has some subset (potentially the entire set)
     * of the past requests applied to the camera settings.</p>
     * <p>By submitting a series of identical requests, the camera device
     * will eventually have the camera settings applied, but it is
     * unknown when that exact point will be.</p>
     * <p>All LEGACY capability devices will have this as their maxLatency.</p>
     */
    ACAMERA_SYNC_MAX_LATENCY_UNKNOWN                                 = -1,

} acamera_metadata_enum_android_sync_max_latency_t;



// ACAMERA_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS
typedef enum acamera_metadata_enum_acamera_depth_available_depth_stream_configurations {
    ACAMERA_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS_OUTPUT       = 0,

    ACAMERA_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS_INPUT        = 1,

} acamera_metadata_enum_android_depth_available_depth_stream_configurations_t;

// ACAMERA_DEPTH_DEPTH_IS_EXCLUSIVE
typedef enum acamera_metadata_enum_acamera_depth_depth_is_exclusive {
    ACAMERA_DEPTH_DEPTH_IS_EXCLUSIVE_FALSE                           = 0,

    ACAMERA_DEPTH_DEPTH_IS_EXCLUSIVE_TRUE                            = 1,

} acamera_metadata_enum_android_depth_depth_is_exclusive_t;


#endif /* __ANDROID_API__ >= 24 */

__END_DECLS

#endif /* _NDK_CAMERA_METADATA_TAGS_H */

/** @} */
