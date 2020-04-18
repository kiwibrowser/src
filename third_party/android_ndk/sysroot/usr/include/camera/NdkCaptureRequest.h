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
 * @file NdkCaptureRequest.h
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
#include "NdkCameraMetadata.h"

#ifndef _NDK_CAPTURE_REQUEST_H
#define _NDK_CAPTURE_REQUEST_H

__BEGIN_DECLS

#if __ANDROID_API__ >= 24

// Container for output targets
typedef struct ACameraOutputTargets ACameraOutputTargets;

// Container for a single output target
typedef struct ACameraOutputTarget ACameraOutputTarget;

/**
 * ACaptureRequest is an opaque type that contains settings and output targets needed to capture
 * a single image from camera device.
 *
 * <p>ACaptureRequest contains the configuration for the capture hardware (sensor, lens, flash),
 * the processing pipeline, the control algorithms, and the output buffers. Also
 * contains the list of target {@link ANativeWindow}s to send image data to for this
 * capture.</p>
 *
 * <p>ACaptureRequest is created by {@link ACameraDevice_createCaptureRequest}.
 *
 * <p>ACaptureRequest is given to {@link ACameraCaptureSession_capture} or
 * {@link ACameraCaptureSession_setRepeatingRequest} to capture images from a camera.</p>
 *
 * <p>Each request can specify a different subset of target {@link ANativeWindow}s for the
 * camera to send the captured data to. All the {@link ANativeWindow}s used in a request must
 * be part of the {@link ANativeWindow} list given to the last call to
 * {@link ACameraDevice_createCaptureSession}, when the request is submitted to the
 * session.</p>
 *
 * <p>For example, a request meant for repeating preview might only include the
 * {@link ANativeWindow} for the preview SurfaceView or SurfaceTexture, while a
 * high-resolution still capture would also include a {@link ANativeWindow} from a
 * {@link AImageReader} configured for high-resolution JPEG images.</p>
 *
 * @see ACameraDevice_createCaptureRequest
 * @see ACameraCaptureSession_capture
 * @see ACameraCaptureSession_setRepeatingRequest
 */
typedef struct ACaptureRequest ACaptureRequest;

/**
 * Create a ACameraOutputTarget object.
 *
 * <p>The ACameraOutputTarget is used in {@link ACaptureRequest_addTarget} method to add an output
 * {@link ANativeWindow} to ACaptureRequest. Use {@link ACameraOutputTarget_free} to free the object
 * and its memory after application no longer needs the {@link ACameraOutputTarget}.</p>
 *
 * @param window the {@link ANativeWindow} to be associated with the {@link ACameraOutputTarget}
 * @param output the output {@link ACameraOutputTarget} will be stored here if the
 *                  method call succeeds.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds. The created ACameraOutputTarget will
 *                                be filled in the output argument.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if window or output is NULL.</li></ul>
 *
 * @see ACaptureRequest_addTarget
 */
camera_status_t ACameraOutputTarget_create(ANativeWindow* window, ACameraOutputTarget** output);

/**
 * Free a ACameraOutputTarget object.
 *
 * @param output the {@link ACameraOutputTarget} to be freed.
 *
 * @see ACameraOutputTarget_create
 */
void ACameraOutputTarget_free(ACameraOutputTarget* output);

/**
 * Add an {@link ACameraOutputTarget} object to {@link ACaptureRequest}.
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param output the output {@link ACameraOutputTarget} to be added to capture request.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request or output is NULL.</li></ul>
 */
camera_status_t ACaptureRequest_addTarget(ACaptureRequest* request,
        const ACameraOutputTarget* output);

/**
 * Remove an {@link ACameraOutputTarget} object from {@link ACaptureRequest}.
 *
 * <p>This method has no effect if the ACameraOutputTarget does not exist in ACaptureRequest.</p>
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param output the output {@link ACameraOutputTarget} to be removed from capture request.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request or output is NULL.</li></ul>
 */
camera_status_t ACaptureRequest_removeTarget(ACaptureRequest* request,
        const ACameraOutputTarget* output);

/**
 * Get a metadata entry from input {@link ACaptureRequest}.
 *
 * <p>The memory of the data field in returned entry is managed by camera framework. Do not
 * attempt to free it.</p>
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param tag the tag value of the camera metadata entry to be get.
 * @param entry the output {@link ACameraMetadata_const_entry} will be filled here if the method
 *        call succeeeds.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if metadata or entry is NULL.</li>
 *         <li>{@link ACAMERA_ERROR_METADATA_NOT_FOUND} if the capture request does not contain an
 *             entry of input tag value.</li></ul>
 */
camera_status_t ACaptureRequest_getConstEntry(
        const ACaptureRequest* request, uint32_t tag, ACameraMetadata_const_entry* entry);

/*
 * List all the entry tags in input {@link ACaptureRequest}.
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param numEntries number of metadata entries in input {@link ACaptureRequest}
 * @param tags the tag values of the metadata entries. Length of tags is returned in numEntries
 *             argument. The memory is managed by ACaptureRequest itself and must NOT be free/delete
 *             by application. Calling ACaptureRequest_setEntry_* methods will invalidate previous
 *             output of ACaptureRequest_getAllTags. Do not access tags after calling
 *             ACaptureRequest_setEntry_*. To get new list of tags after updating capture request,
 *             application must call ACaptureRequest_getAllTags again. Do NOT access tags after
 *             calling ACaptureRequest_free.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request, numEntries or tags is NULL.</li>
 *         <li>{@link ACAMERA_ERROR_UNKNOWN} if the method fails for some other reasons.</li></ul>
 */
camera_status_t ACaptureRequest_getAllTags(
        const ACaptureRequest* request, /*out*/int32_t* numTags, /*out*/const uint32_t** tags);

/**
 * Set/change a camera capture control entry with unsigned 8 bits data type.
 *
 * <p>Set count to 0 and data to NULL to remove a tag from the capture request.</p>
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param tag the tag value of the camera metadata entry to be set.
 * @param count number of elements to be set in data argument
 * @param data the entries to be set/change in the capture request.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request is NULL, count is larger than
 *             zero while data is NULL, the data type of the tag is not unsigned 8 bits, or
 *             the tag is not controllable by application.</li></ul>
 */
camera_status_t ACaptureRequest_setEntry_u8(
        ACaptureRequest* request, uint32_t tag, uint32_t count, const uint8_t* data);

/**
 * Set/change a camera capture control entry with signed 32 bits data type.
 *
 * <p>Set count to 0 and data to NULL to remove a tag from the capture request.</p>
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param tag the tag value of the camera metadata entry to be set.
 * @param count number of elements to be set in data argument
 * @param data the entries to be set/change in the capture request.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request is NULL, count is larger than
 *             zero while data is NULL, the data type of the tag is not signed 32 bits, or
 *             the tag is not controllable by application.</li></ul>
 */
camera_status_t ACaptureRequest_setEntry_i32(
        ACaptureRequest* request, uint32_t tag, uint32_t count, const int32_t* data);

/**
 * Set/change a camera capture control entry with float data type.
 *
 * <p>Set count to 0 and data to NULL to remove a tag from the capture request.</p>
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param tag the tag value of the camera metadata entry to be set.
 * @param count number of elements to be set in data argument
 * @param data the entries to be set/change in the capture request.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request is NULL, count is larger than
 *             zero while data is NULL, the data type of the tag is not float, or
 *             the tag is not controllable by application.</li></ul>
 */
camera_status_t ACaptureRequest_setEntry_float(
        ACaptureRequest* request, uint32_t tag, uint32_t count, const float* data);

/**
 * Set/change a camera capture control entry with signed 64 bits data type.
 *
 * <p>Set count to 0 and data to NULL to remove a tag from the capture request.</p>
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param tag the tag value of the camera metadata entry to be set.
 * @param count number of elements to be set in data argument
 * @param data the entries to be set/change in the capture request.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request is NULL, count is larger than
 *             zero while data is NULL, the data type of the tag is not signed 64 bits, or
 *             the tag is not controllable by application.</li></ul>
 */
camera_status_t ACaptureRequest_setEntry_i64(
        ACaptureRequest* request, uint32_t tag, uint32_t count, const int64_t* data);

/**
 * Set/change a camera capture control entry with double data type.
 *
 * <p>Set count to 0 and data to NULL to remove a tag from the capture request.</p>
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param tag the tag value of the camera metadata entry to be set.
 * @param count number of elements to be set in data argument
 * @param data the entries to be set/change in the capture request.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request is NULL, count is larger than
 *             zero while data is NULL, the data type of the tag is not double, or
 *             the tag is not controllable by application.</li></ul>
 */
camera_status_t ACaptureRequest_setEntry_double(
        ACaptureRequest* request, uint32_t tag, uint32_t count, const double* data);

/**
 * Set/change a camera capture control entry with rational data type.
 *
 * <p>Set count to 0 and data to NULL to remove a tag from the capture request.</p>
 *
 * @param request the {@link ACaptureRequest} of interest.
 * @param tag the tag value of the camera metadata entry to be set.
 * @param count number of elements to be set in data argument
 * @param data the entries to be set/change in the capture request.
 *
 * @return <ul>
 *         <li>{@link ACAMERA_OK} if the method call succeeds.</li>
 *         <li>{@link ACAMERA_ERROR_INVALID_PARAMETER} if request is NULL, count is larger than
 *             zero while data is NULL, the data type of the tag is not rational, or
 *             the tag is not controllable by application.</li></ul>
 */
camera_status_t ACaptureRequest_setEntry_rational(
        ACaptureRequest* request, uint32_t tag, uint32_t count,
        const ACameraMetadata_rational* data);

/**
 * Free a {@link ACaptureRequest} structure.
 *
 * @param request the {@link ACaptureRequest} to be freed.
 */
void ACaptureRequest_free(ACaptureRequest* request);

#endif /* __ANDROID_API__ >= 24 */

__END_DECLS

#endif /* _NDK_CAPTURE_REQUEST_H */

/** @} */
