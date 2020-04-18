/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_video_source_private.idl,
 *   modified Mon Oct 27 16:13:24 2014.
 */

#ifndef PPAPI_C_PRIVATE_PPB_VIDEO_SOURCE_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_VIDEO_SOURCE_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/private/pp_video_frame_private.h"

#define PPB_VIDEOSOURCE_PRIVATE_INTERFACE_0_1 "PPB_VideoSource_Private;0.1"
#define PPB_VIDEOSOURCE_PRIVATE_INTERFACE PPB_VIDEOSOURCE_PRIVATE_INTERFACE_0_1

/**
 * @file
 * This file defines the <code>PPB_VideoSource_Private</code> interface for a
 * video source resource, which receives video frames from a MediaStream video
 * track in the browser.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_VideoSource_Private</code> interface contains pointers to
 * several functions for creating video source resources and using them to
 * receive video frames from a MediaStream video track in the browser.
 */
struct PPB_VideoSource_Private_0_1 {
  /**
   * Creates a video source resource.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying an instance of
   * a module.
   *
   * @return A <code>PP_Resource</code> with a nonzero ID on success or zero on
   * failure. Failure means the instance was invalid.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Determines if a resource is a video source resource.
   *
   * @param[in] resource The <code>PP_Resource</code> to test.
   *
   * @return A <code>PP_Bool</code> with <code>PP_TRUE</code> if the given
   * resource is a video source resource or <code>PP_FALSE</code> otherwise.
   */
  PP_Bool (*IsVideoSource)(PP_Resource resource);
  /**
   * Opens a video source for getting frames.
   *
   * @param[in] source A <code>PP_Resource</code> corresponding to a video
   * source resource.
   * @param[in] stream_url A <code>PP_Var</code> string holding a URL
   * identifying a MediaStream.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion of Open().
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   * Returns PP_ERROR_BADRESOURCE if source isn't a valid video source.
   * Returns PP_ERROR_INPROGRESS if source is already open.
   * Returns PP_ERROR_FAILED if the MediaStream doesn't exist or if there is
   * some other browser error.
   */
  int32_t (*Open)(PP_Resource source,
                  struct PP_Var stream_url,
                  struct PP_CompletionCallback callback);
  /**
   * Gets a frame from the video source. The returned image data is only valid
   * until the next call to GetFrame.
   * The image data resource inside the returned frame will have its reference
   * count incremented by one and must be managed by the plugin.
   *
   * @param[in] source A <code>PP_Resource</code> corresponding to a video
   * source resource.
   * @param[out] frame A <code>PP_VideoFrame_Private</code> to hold a video
   * frame from the source.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion of GetNextFrame().
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   * Returns PP_ERROR_BADRESOURCE if source isn't a valid video source.
   * Returns PP_ERROR_FAILED if the source is not open, or if some other
   * browser error occurs.
   */
  int32_t (*GetFrame)(PP_Resource source,
                      struct PP_VideoFrame_Private* frame,
                      struct PP_CompletionCallback callback);
  /**
   * Closes the video source.
   *
   * @param[in] source A <code>PP_Resource</code> corresponding to a video
   * source resource.
   */
  void (*Close)(PP_Resource source);
};

typedef struct PPB_VideoSource_Private_0_1 PPB_VideoSource_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_VIDEO_SOURCE_PRIVATE_H_ */

