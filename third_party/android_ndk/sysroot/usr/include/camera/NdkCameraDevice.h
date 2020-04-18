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
 * @file NdkCameraDevice.h
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
#include <sys/cdefs.h>

#include <android/native_window.h>
#include "NdkCameraError.h"
#include "NdkCaptureRequest.h"
#include "NdkCameraCaptureSession.h"

#ifndef _NDK_CAMERA_DEVICE_H
#define _NDK_CAMERA_DEVICE_H

__BEGIN_DECLS

#if __ANDROID_API__ >= 24

/**
 * ACameraDevice is opaque type that provides access to a camera device.
 *
 * A pointer can be obtained using {@link ACameraManager_openCamera} method.
 */
typedef struct ACameraDevice ACameraDevice;

/// Enum for ACameraDevice_ErrorStateCallback error code
enum {
    /**
     * The camera device is in use already.
     */
    ERROR_CAMERA_IN_USE = 1,

    /**
     * The system-wide limit for number of open cameras or camera resources has
     * been reached, and more camera devices cannot be opened until previous
     * instances are closed.
     */
    ERROR_MAX_CAMERAS_IN_USE = 2,

    /**
     * The camera is disabled due to a device policy, and cannot be opened.
     */
    ERROR_CAMERA_DISABLED = 3,

    /**
     * The camera device has encountered a fatal error.
     * <p>The camera device needs to be re-opened to be used again.</p>
     */
    ERROR_CAMERA_DEVICE = 4,

    /**
     * The camera service has encountered a fatal error.
     * <p>The Android device may need to be shut down and restarted to restore
     * camera function, or there may be a persistent hardware problem.
     * An attempt at recovery may be possible by closing the
     * CameraDevice and the CameraManager, and trying to acquire all resources
     * again from scratch.</p>
     */
    ERROR_CAMERA_SERVICE = 5
};

/**
 * Camera device state callbacks to be used in {@link ACameraDevice_stateCallbacks}.
 *
 * @param context The optional context in {@link ACameraDevice_stateCallbacks} will be
 *                passed to this callback.
 * @param device The {@link ACameraDevice} that is being disconnected.
 */
typedef void (*ACameraDevice_StateCallback)(void* context, ACameraDevice* device);

/**
 * Camera device error state callbacks to be used in {@link ACameraDevice_stateCallbacks}.
 *
 * @param context The optional context in {@link ACameraDevice_stateCallbacks} will be
 *                passed to this callback.
 * @param device The {@link ACameraDevice} that is being disconnected.
 * @param error The error code describes the cause of this error callback. See the folowing
 *              links for more detail.
 *
 * @see ERROR_CAMERA_IN_USE
 * @see ERROR_MAX_CAMERAS_IN_USE
 * @see ERROR_CAMERA_DISABLED
 * @see ERROR_CAMERA_DEVICE
 * @see ERROR_CAMERA_SERVICE
 */
typedef void (*ACameraDevice_ErrorStateCallback)(void* context, ACameraDevice* device, int error);

typedef struct ACameraDevice_StateCallbacks {
    /// optional application context.
    void*                             context;

    /**
     * The function is  called when a camera device is no longer available for use.
     *
     * <p>Any attempt to call API methods on this ACameraDevice will return
     * {@link ACAMERA_ERROR_CAMERA_DISCONNECTED}. The disconnection could be due to a
     * change in security policy or permissions; the physical disconnection
     * of a removable camera device; or the camera being needed for a
     * higher-priority camera API client.</p>
     *
     * <p>Application should clean up the camera with {@link ACameraDevice_close} after
     * this happens, as it is not recoverable until the camera can be opened
     * again.</p>
     *
     */
    ACameraDevice_StateCallback       onDisconnected;

    /**
     * The function called when a camera device has encountered a serious error.
     *
     * <p>This indicates a failure of the camera device or camera service in some way.
     * Any attempt to call API methods on this ACameraDevice in the future will return
     * {@link ACAMERA_ERROR_CAMERA_DISCONNECTED}.</p>
     *
     * <p>There may still be capture completion or camera stream callbacks that will be called
     * after this error is received.</p>
     *
     * <p>Application should clean up the camera with {@link ACameraDevice_close} after this
     * happens. Further attempts at recovery are error-code specific.</p>
     *
     */
    ACameraDevice_ErrorStateCallback  onError;
} ACameraDevice_stateCallbacks;

/**
 * Close the connection and free this ACameraDevice synchronously. Access to the ACameraDevice
 * after calling this method will cause a crash.
 *
 * <p>After this call, all calls to the active ACameraCaptureSession associated to this
 * ACameraDevice will return {@link ACAMERA_ERROR_SESSION_CLOSED} except for calls to
 * {@link ACameraCaptureSession_close}.</p>
 *
 * <p>This method will stop all repeating captures sent via
 * {@link ACameraCaptureSession_setRepeatingRequest} and block until all capture requests sent via
 * {@link ACameraCaptureSession_capture} is complete. Once the method returns, the camera device
 * will be removed from memory and access to the closed camera device pointer will cause a crash.</p>
 *
 * @param device the camera device to be closed
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if device is NULL.</li></ul>
 */
camera_status_t ACameraDevice_close(ACameraDevice* device);

/**
 * Return the camera id associated with this camera device.
 *
 * @param device the camera device to be closed
 *
 * @return camera ID string. The returned string is managed by framework and should not be
 * delete/free by the application. Also the returned string must not be used after the device
 * has been closed.
 */
const char* ACameraDevice_getId(const ACameraDevice* device);

typedef enum {
    /**
     * Create a request suitable for a camera preview window. Specifically, this
     * means that high frame rate is given priority over the highest-quality
     * post-processing. These requests would normally be used with the
     * {@link ACameraCaptureSession_setRepeatingRequest} method.
     * This template is guaranteed to be supported on all camera devices.
     *
     * @see ACameraDevice_createCaptureRequest
     */
    TEMPLATE_PREVIEW = 1,

    /**
     * Create a request suitable for still image capture. Specifically, this
     * means prioritizing image quality over frame rate. These requests would
     * commonly be used with the {@link ACameraCaptureSession_capture} method.
     * This template is guaranteed to be supported on all camera devices.
     *
     * @see ACameraDevice_createCaptureRequest
     */
    TEMPLATE_STILL_CAPTURE = 2,

    /**
     * Create a request suitable for video recording. Specifically, this means
     * that a stable frame rate is used, and post-processing is set for
     * recording quality. These requests would commonly be used with the
     * {@link ACameraCaptureSession_setRepeatingRequest} method.
     * This template is guaranteed to be supported on all camera devices.
     *
     * @see ACameraDevice_createCaptureRequest
     */
    TEMPLATE_RECORD = 3,

    /**
     * Create a request suitable for still image capture while recording
     * video. Specifically, this means maximizing image quality without
     * disrupting the ongoing recording. These requests would commonly be used
     * with the {@link ACameraCaptureSession_capture} method while a request based on
     * {@link TEMPLATE_RECORD} is is in use with {@link ACameraCaptureSession_setRepeatingRequest}.
     * This template is guaranteed to be supported on all camera devices.
     *
     * @see ACameraDevice_createCaptureRequest
     */
    TEMPLATE_VIDEO_SNAPSHOT = 4,

    /**
     * Create a request suitable for zero shutter lag still capture. This means
     * means maximizing image quality without compromising preview frame rate.
     * AE/AWB/AF should be on auto mode.
     *
     * @see ACameraDevice_createCaptureRequest
     */
    TEMPLATE_ZERO_SHUTTER_LAG = 5,

    /**
     * A basic template for direct application control of capture
     * parameters. All automatic control is disabled (auto-exposure, auto-white
     * balance, auto-focus), and post-processing parameters are set to preview
     * quality. The manual capture parameters (exposure, sensitivity, and so on)
     * are set to reasonable defaults, but should be overriden by the
     * application depending on the intended use case.
     * This template is guaranteed to be supported on camera devices that support the
     * {@link ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR} capability.
     *
     * @see ACameraDevice_createCaptureRequest
     */
    TEMPLATE_MANUAL = 6,
} ACameraDevice_request_template;

/**
 * Create a ACaptureRequest for capturing images, initialized with template
 * for a target use case.
 *
 * <p>The settings are chosen to be the best options for this camera device,
 * so it is not recommended to reuse the same request for a different camera device.</p>
 *
 * @param device the camera device of interest
 * @param templateId the type of capture request to be created.
 *        See {@link ACameraDevice_request_template}.
 * @param request the output request will be stored here if the method call succeeds.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds. The created capture request will be
 *                                filled in request argument.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if device or request is NULL, templateId
 *                                is undefined or camera device does not support requested template.
 *                                </li>
 *         <li>{@link ACAMERA_ERROR_CAMERA_DISCONNECTED} if the camera device is closed.</li>
 *         <li>{@link ACAMERA_ERROR_CAMERA_DEVICE} if the camera device encounters fatal error.</li>
 *         <li>{@link ACAMERA_ERROR_CAMERA_SERVICE} if the camera service encounters fatal error.</li>
 *         <li>{@link ACAMERA_ERROR_UNKNOWN} if the method fails for some other reasons.</li></ul>
 *
 * @see TEMPLATE_PREVIEW
 * @see TEMPLATE_RECORD
 * @see TEMPLATE_STILL_CAPTURE
 * @see TEMPLATE_VIDEO_SNAPSHOT
 * @see TEMPLATE_MANUAL
 */
camera_status_t ACameraDevice_createCaptureRequest(
        const ACameraDevice* device, ACameraDevice_request_template templateId,
        /*out*/ACaptureRequest** request);


typedef struct ACaptureSessionOutputContainer ACaptureSessionOutputContainer;

typedef struct ACaptureSessionOutput ACaptureSessionOutput;

/**
 * Create a capture session output container.
 *
 * <p>The container is used in {@link ACameraDevice_createCaptureSession} method to create a capture
 * session. Use {@link ACaptureSessionOutputContainer_free} to free the container and its memory
 * after application no longer needs the ACaptureSessionOutputContainer.</p>
 *
 * @param container the output {@link ACaptureSessionOutputContainer} will be stored here if the
 *                  method call succeeds.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds. The created container will be
 *                                filled in container argument.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if container is NULL.</li></ul>
 */
camera_status_t ACaptureSessionOutputContainer_create(
        /*out*/ACaptureSessionOutputContainer** container);

/**
 * Free a capture session output container.
 *
 * @param container the {@link ACaptureSessionOutputContainer} to be freed.
 *
 * @see ACaptureSessionOutputContainer_create
 */
void            ACaptureSessionOutputContainer_free(ACaptureSessionOutputContainer* container);

/**
 * Create a ACaptureSessionOutput object.
 *
 * <p>The ACaptureSessionOutput is used in {@link ACaptureSessionOutputContainer_add} method to add
 * an output {@link ANativeWindow} to ACaptureSessionOutputContainer. Use
 * {@link ACaptureSessionOutput_free} to free the object and its memory after application no longer
 * needs the {@link ACaptureSessionOutput}.</p>
 *
 * @param anw the {@link ANativeWindow} to be associated with the {@link ACaptureSessionOutput}
 * @param output the output {@link ACaptureSessionOutput} will be stored here if the
 *                  method call succeeds.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds. The created container will be
 *                                filled in the output argument.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if anw or output is NULL.</li></ul>
 *
 * @see ACaptureSessionOutputContainer_add
 */
camera_status_t ACaptureSessionOutput_create(
        ANativeWindow* anw, /*out*/ACaptureSessionOutput** output);

/**
 * Free a ACaptureSessionOutput object.
 *
 * @param output the {@link ACaptureSessionOutput} to be freed.
 *
 * @see ACaptureSessionOutput_create
 */
void            ACaptureSessionOutput_free(ACaptureSessionOutput* output);

/**
 * Add an {@link ACaptureSessionOutput} object to {@link ACaptureSessionOutputContainer}.
 *
 * @param container the {@link ACaptureSessionOutputContainer} of interest.
 * @param output the output {@link ACaptureSessionOutput} to be added to container.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if container or output is NULL.</li></ul>
 */
camera_status_t ACaptureSessionOutputContainer_add(
        ACaptureSessionOutputContainer* container, const ACaptureSessionOutput* output);

/**
 * Remove an {@link ACaptureSessionOutput} object from {@link ACaptureSessionOutputContainer}.
 *
 * <p>This method has no effect if the ACaptureSessionOutput does not exist in
 * ACaptureSessionOutputContainer.</p>
 *
 * @param container the {@link ACaptureSessionOutputContainer} of interest.
 * @param output the output {@link ACaptureSessionOutput} to be removed from container.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if container or output is NULL.</li></ul>
 */
camera_status_t ACaptureSessionOutputContainer_remove(
        ACaptureSessionOutputContainer* container, const ACaptureSessionOutput* output);

/**
 * Create a new camera capture session by providing the target output set of {@link ANativeWindow}
 * to the camera device.
 *
 * <p>If there is a preexisting session, the previous session will be closed
 * automatically. However, app still needs to call {@link ACameraCaptureSession_close} on previous
 * session. Otherwise the resources held by previous session will NOT be freed.</p>
 *
 * <p>The active capture session determines the set of potential output {@link ANativeWindow}s for
 * the camera device for each capture request. A given request may use all
 * or only some of the outputs. Once the ACameraCaptureSession is created, requests can be
 * submitted with {@link ACameraCaptureSession_capture} or
 * {@link ACameraCaptureSession_setRepeatingRequest}.</p>
 *
 * <p>Often the {@link ANativeWindow} used with this method can be obtained from a <a href=
 * "http://developer.android.com/reference/android/view/Surface.html">Surface</a> java object by
 * {@link ANativeWindow_fromSurface} NDK method. Surfaces or ANativeWindow suitable for inclusion as a camera
 * output can be created for various use cases and targets:</p>
 *
 * <ul>
 *
 * <li>For drawing to a
 *   <a href="http://developer.android.com/reference/android/view/SurfaceView.html">SurfaceView</a>:
 *   Once the SurfaceView's Surface is created, set the size
 *   of the Surface with
 *   <a href="http://developer.android.com/reference/android/view/SurfaceHolder.html#setFixedSize(int, int)">
 *    android.view.SurfaceHolder\#setFixedSize</a> to be one of the PRIVATE output sizes
 *   returned by {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS}
 *   and then obtain the Surface by calling <a href=
 *   "http://developer.android.com/reference/android/view/SurfaceHolder.html#getSurface()">
 *   android.view.SurfaceHolder\#getSurface</a>. If the size is not set by the application, it will
 *   be rounded to the nearest supported size less than 1080p, by the camera device.</li>
 *
 * <li>For accessing through an OpenGL texture via a <a href=
 *   "http://developer.android.com/reference/android/graphics/SurfaceTexture.html">SurfaceTexture</a>:
 *   Set the size of the SurfaceTexture with <a href=
 *   "http://developer.android.com/reference/android/graphics/SurfaceTexture.html#setDefaultBufferSize(int, int)">
 *   setDefaultBufferSize</a> to be one of the PRIVATE output sizes
 *   returned by {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS}
 *   before creating a Surface from the SurfaceTexture with <a href=
 *   "http://developer.android.com/reference/android/view/Surface.html#Surface(android.graphics.SurfaceTexture)">
 *   Surface\#Surface(SurfaceTextrue)</a>. If the size is not set by the application, it will be set to be the
 *   smallest supported size less than 1080p, by the camera device.</li>
 *
 * <li>For recording with <a href=
 *     "http://developer.android.com/reference/android/media/MediaCodec.html">
 *     MediaCodec</a>: Call
 *   <a href=
 *     "http://developer.android.com/reference/android/media/MediaCodec.html#createInputSurface()">
 *     android.media.MediaCodec\#createInputSurface</a> after configuring
 *   the media codec to use one of the PRIVATE output sizes
 *   returned by {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS}.
 *   </li>
 *
 * <li>For recording with <a href=
 *    "http://developer.android.com/reference/android/media/MediaRecorder.html">
 *    MediaRecorder</a>: Call
 *   <a href="http://developer.android.com/reference/android/media/MediaRecorder.html#getSurface()">
 *    android.media.MediaRecorder\#getSurface</a> after configuring the media recorder to use
 *   one of the PRIVATE output sizes returned by
 *   {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS}, or configuring it to use one of the supported
 *   <a href="http://developer.android.com/reference/android/media/CamcorderProfile.html">
 *    CamcorderProfiles</a>.</li>
 *
 * <li>For efficient YUV processing with <a href=
 *   "http://developer.android.com/reference/android/renderscript/package-summary.html">
 *   RenderScript</a>:
 *   Create a RenderScript
 *   <a href="http://developer.android.com/reference/android/renderscript/Allocation.html">
 *   Allocation</a> with a supported YUV
 *   type, the IO_INPUT flag, and one of the YUV output sizes returned by
 *   {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS},
 *   Then obtain the Surface with
 *   <a href="http://developer.android.com/reference/android/renderscript/Allocation.html#getSurface()">
 *   Allocation#getSurface}</a>.</li>
 *
 * <li>For access to RAW, uncompressed YUV, or compressed JPEG data in the application: Create an
 *   {@link AImageReader} object using the {@link AImageReader_new} method with one of the supported
 *   output formats given by {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS}. Then obtain a
 *   ANativeWindow from it with {@link AImageReader_getWindow}.
 *   If the AImageReader size is not set to a supported size, it will be rounded to a supported
 *   size less than 1080p by the camera device.
 *   </li>
 *
 * </ul>
 *
 * <p>The camera device will query each ANativeWindow's size and formats upon this
 * call, so they must be set to a valid setting at this time.</p>
 *
 * <p>It can take several hundred milliseconds for the session's configuration to complete,
 * since camera hardware may need to be powered on or reconfigured.</p>
 *
 * <p>If a prior ACameraCaptureSession already exists when this method is called, the previous
 * session will no longer be able to accept new capture requests and will be closed. Any
 * in-progress capture requests made on the prior session will be completed before it's closed.
 * To minimize the transition time,
 * the ACameraCaptureSession_abortCaptures method can be used to discard the remaining
 * requests for the prior capture session before a new one is created. Note that once the new
 * session is created, the old one can no longer have its captures aborted.</p>
 *
 * <p>Using larger resolution outputs, or more outputs, can result in slower
 * output rate from the device.</p>
 *
 * <p>Configuring a session with an empty list will close the current session, if
 * any. This can be used to release the current session's target surfaces for another use.</p>
 *
 * <p>While any of the sizes from {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS} can be used when
 * a single output stream is configured, a given camera device may not be able to support all
 * combination of sizes, formats, and targets when multiple outputs are configured at once.  The
 * tables below list the maximum guaranteed resolutions for combinations of streams and targets,
 * given the capabilities of the camera device.</p>
 *
 * <p>If an application tries to create a session using a set of targets that exceed the limits
 * described in the below tables, one of three possibilities may occur. First, the session may
 * be successfully created and work normally. Second, the session may be successfully created,
 * but the camera device won't meet the frame rate guarantees as described in
 * {@link ACAMERA_SCALER_AVAILABLE_MIN_FRAME_DURATIONS}. Or third, if the output set
 * cannot be used at all, session creation will fail entirely, with
 * {@link ACAMERA_ERROR_STREAM_CONFIGURE_FAIL} being returned.</p>
 *
 * <p>For the type column `PRIV` refers to output format {@link AIMAGE_FORMAT_PRIVATE},
 * `YUV` refers to output format {@link AIMAGE_FORMAT_YUV_420_888},
 * `JPEG` refers to output format {@link AIMAGE_FORMAT_JPEG},
 * and `RAW` refers to output format {@link AIMAGE_FORMAT_RAW16}
 *
 *
 * <p>For the maximum size column, `PREVIEW` refers to the best size match to the
 * device's screen resolution, or to 1080p `(1920x1080)`, whichever is
 * smaller. `RECORD` refers to the camera device's maximum supported recording resolution,
 * as determined by <a href="http://developer.android.com/reference/android/media/CamcorderProfile.html">
 * android.media.CamcorderProfiles</a>. And `MAXIMUM` refers to the
 * camera device's maximum output resolution for that format or target from
 * {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS}.</p>
 *
 * <p>To use these tables, determine the number and the formats/targets of outputs needed, and
 * find the row(s) of the table with those targets. The sizes indicate the maximum set of sizes
 * that can be used; it is guaranteed that for those targets, the listed sizes and anything
 * smaller from the list given by {@link ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS} can be
 * successfully used to create a session.  For example, if a row indicates that a 8 megapixel
 * (MP) YUV_420_888 output can be used together with a 2 MP `PRIV` output, then a session
 * can be created with targets `[8 MP YUV, 2 MP PRIV]` or targets `[2 MP YUV, 2 MP PRIV]`;
 * but a session with targets `[8 MP YUV, 4 MP PRIV]`, targets `[4 MP YUV, 4 MP PRIV]`,
 * or targets `[8 MP PRIV, 2 MP YUV]` would not be guaranteed to work, unless
 * some other row of the table lists such a combination.</p>
 *
 * <p>Legacy devices ({@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL}
 * `== `{@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY LEGACY}) support at
 * least the following stream combinations:
 *
 * <table>
 * <tr><th colspan="7">LEGACY-level guaranteed configurations</th></tr>
 * <tr> <th colspan="2" id="rb">Target 1</th> <th colspan="2" id="rb">Target 2</th>  <th colspan="2" id="rb">Target 3</th> <th rowspan="2">Sample use case(s)</th> </tr>
 * <tr> <th>Type</th><th id="rb">Max size</th> <th>Type</th><th id="rb">Max size</th> <th>Type</th><th id="rb">Max size</th></tr>
 * <tr> <td>`PRIV`</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td colspan="2" id="rb"></td> <td>Simple preview, GPU video processing, or no-preview video recording.</td> </tr>
 * <tr> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td colspan="2" id="rb"></td> <td>No-viewfinder still image capture.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td colspan="2" id="rb"></td> <td>In-application video/image processing.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td>Standard still imaging.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td>In-app processing plus still capture.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td colspan="2" id="rb"></td> <td>Standard recording.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td colspan="2" id="rb"></td> <td>Preview plus in-app processing.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td>Still capture plus in-app processing.</td> </tr>
 * </table><br>
 * </p>
 *
 * <p>Limited-level ({@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL}
 * `== `{@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED LIMITED}) devices
 * support at least the following stream combinations in addition to those for
 * {@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY LEGACY} devices:
 *
 * <table>
 * <tr><th colspan="7">LIMITED-level additional guaranteed configurations</th></tr>
 * <tr><th colspan="2" id="rb">Target 1</th><th colspan="2" id="rb">Target 2</th><th colspan="2" id="rb">Target 3</th> <th rowspan="2">Sample use case(s)</th> </tr>
 * <tr><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th></tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`RECORD `</td> <td colspan="2" id="rb"></td> <td>High-resolution video recording with preview.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`RECORD `</td> <td colspan="2" id="rb"></td> <td>High-resolution in-app video processing with preview.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`RECORD `</td> <td colspan="2" id="rb"></td> <td>Two-input in-app video processing.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`RECORD `</td> <td>`JPEG`</td><td id="rb">`RECORD `</td> <td>High-resolution recording with video snapshot.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`RECORD `</td> <td>`JPEG`</td><td id="rb">`RECORD `</td> <td>High-resolution in-app processing with video snapshot.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td>Two-input in-app processing with still capture.</td> </tr>
 * </table><br>
 * </p>
 *
 * <p>FULL-level ({@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL}
 * `== `{@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_FULL FULL}) devices
 * support at least the following stream combinations in addition to those for
 * {@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED LIMITED} devices:
 *
 * <table>
 * <tr><th colspan="7">FULL-level additional guaranteed configurations</th></tr>
 * <tr><th colspan="2" id="rb">Target 1</th><th colspan="2" id="rb">Target 2</th><th colspan="2" id="rb">Target 3</th> <th rowspan="2">Sample use case(s)</th> </tr>
 * <tr><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td>Maximum-resolution GPU processing with preview.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td>Maximum-resolution in-app processing with preview.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td>Maximum-resolution two-input in-app processsing.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td>Video recording with maximum-size video snapshot</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`640x480`</td> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`MAXIMUM`</td> <td>Standard video recording plus maximum-resolution in-app processing.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`640x480`</td> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`MAXIMUM`</td> <td>Preview plus two-input maximum-resolution in-app processing.</td> </tr>
 * </table><br>
 * </p>
 *
 * <p>RAW-capability ({@link ACAMERA_REQUEST_AVAILABLE_CAPABILITIES} includes
 * {@link ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_RAW RAW}) devices additionally support
 * at least the following stream combinations on both
 * {@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_FULL FULL} and
 * {@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED LIMITED} devices:
 *
 * <table>
 * <tr><th colspan="7">RAW-capability additional guaranteed configurations</th></tr>
 * <tr><th colspan="2" id="rb">Target 1</th><th colspan="2" id="rb">Target 2</th><th colspan="2" id="rb">Target 3</th> <th rowspan="2">Sample use case(s)</th> </tr>
 * <tr><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th> </tr>
 * <tr> <td>`RAW `</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td colspan="2" id="rb"></td> <td>No-preview DNG capture.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`RAW `</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td>Standard DNG capture.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`RAW `</td><td id="rb">`MAXIMUM`</td> <td colspan="2" id="rb"></td> <td>In-app processing plus DNG capture.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`RAW `</td><td id="rb">`MAXIMUM`</td> <td>Video recording with DNG capture.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`RAW `</td><td id="rb">`MAXIMUM`</td> <td>Preview with in-app processing and DNG capture.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`RAW `</td><td id="rb">`MAXIMUM`</td> <td>Two-input in-app processing plus DNG capture.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td>`RAW `</td><td id="rb">`MAXIMUM`</td> <td>Still capture with simultaneous JPEG and DNG.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td>`RAW `</td><td id="rb">`MAXIMUM`</td> <td>In-app processing with simultaneous JPEG and DNG.</td> </tr>
 * </table><br>
 * </p>
 *
 * <p>BURST-capability ({@link ACAMERA_REQUEST_AVAILABLE_CAPABILITIES} includes
 * {@link ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE BURST_CAPTURE}) devices
 * support at least the below stream combinations in addition to those for
 * {@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED LIMITED} devices. Note that all
 * FULL-level devices support the BURST capability, and the below list is a strict subset of the
 * list for FULL-level devices, so this table is only relevant for LIMITED-level devices that
 * support the BURST_CAPTURE capability.
 *
 * <table>
 * <tr><th colspan="5">BURST-capability additional guaranteed configurations</th></tr>
 * <tr><th colspan="2" id="rb">Target 1</th><th colspan="2" id="rb">Target 2</th><th rowspan="2">Sample use case(s)</th> </tr>
 * <tr><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`MAXIMUM`</td> <td>Maximum-resolution GPU processing with preview.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`MAXIMUM`</td> <td>Maximum-resolution in-app processing with preview.</td> </tr>
 * <tr> <td>`YUV `</td><td id="rb">`PREVIEW`</td> <td>`YUV `</td><td id="rb">`MAXIMUM`</td> <td>Maximum-resolution two-input in-app processsing.</td> </tr>
 * </table><br>
 * </p>
 *
 * <p>LEVEL-3 ({@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL}
 * `== `{@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_3 LEVEL_3})
 * support at least the following stream combinations in addition to the combinations for
 * {@link ACAMERA_INFO_SUPPORTED_HARDWARE_LEVEL_FULL FULL} and for
 * RAW capability ({@link ACAMERA_REQUEST_AVAILABLE_CAPABILITIES} includes
 * {@link ACAMERA_REQUEST_AVAILABLE_CAPABILITIES_RAW RAW}):
 *
 * <table>
 * <tr><th colspan="11">LEVEL-3 additional guaranteed configurations</th></tr>
 * <tr><th colspan="2" id="rb">Target 1</th><th colspan="2" id="rb">Target 2</th><th colspan="2" id="rb">Target 3</th><th colspan="2" id="rb">Target 4</th><th rowspan="2">Sample use case(s)</th> </tr>
 * <tr><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th><th>Type</th><th id="rb">Max size</th> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`640x480`</td> <td>`YUV`</td><td id="rb">`MAXIMUM`</td> <td>`RAW`</td><td id="rb">`MAXIMUM`</td> <td>In-app viewfinder analysis with dynamic selection of output format.</td> </tr>
 * <tr> <td>`PRIV`</td><td id="rb">`PREVIEW`</td> <td>`PRIV`</td><td id="rb">`640x480`</td> <td>`JPEG`</td><td id="rb">`MAXIMUM`</td> <td>`RAW`</td><td id="rb">`MAXIMUM`</td> <td>In-app viewfinder analysis with dynamic selection of output format.</td> </tr>
 * </table><br>
 * </p>
 *
 * <p>Since the capabilities of camera devices vary greatly, a given camera device may support
 * target combinations with sizes outside of these guarantees, but this can only be tested for
 * by attempting to create a session with such targets.</p>
 *
 * @param device the camera device of interest.
 * @param outputs the {@link ACaptureSessionOutputContainer} describes all output streams.
 * @param callbacks the {@link ACameraCaptureSession_stateCallbacks capture session state callbacks}.
 * @param session the created {@link ACameraCaptureSession} will be filled here if the method call
 *        succeeds.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds. The created capture session will be
 *                                filled in session argument.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if any of device, outputs, callbacks or
 *                                session is NULL.</li>
 *         <li>{@link ACAMERA_ERROR_CAMERA_DISCONNECTED} if the camera device is closed.</li>
 *         <li>{@link ACAMERA_ERROR_CAMERA_DEVICE} if the camera device encounters fatal error.</li>
 *         <li>{@link ACAMERA_ERROR_CAMERA_SERVICE} if the camera service encounters fatal error.</li>
 *         <li>{@link ACAMERA_ERROR_UNKNOWN} if the method fails for some other reasons.</li></ul>
 */
camera_status_t ACameraDevice_createCaptureSession(
        ACameraDevice* device,
        const ACaptureSessionOutputContainer*       outputs,
        const ACameraCaptureSession_stateCallbacks* callbacks,
        /*out*/ACameraCaptureSession** session);

#endif /* __ANDROID_API__ >= 24 */

__END_DECLS

#endif /* _NDK_CAMERA_DEVICE_H */

/** @} */

