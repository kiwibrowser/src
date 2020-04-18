/* Copyright (c) 2013 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From private/ppb_video_destination_private.idl,
 *   modified Thu Apr 25 11:51:30 2013.
 */

#ifndef PPAPI_C_PRIVATE_PPB_VIDEO_DESTINATION_PRIVATE_H_
#define PPAPI_C_PRIVATE_PPB_VIDEO_DESTINATION_PRIVATE_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/private/pp_video_frame_private.h"

#define PPB_VIDEODESTINATION_PRIVATE_INTERFACE_0_1 \
    "PPB_VideoDestination_Private;0.1"
#define PPB_VIDEODESTINATION_PRIVATE_INTERFACE \
    PPB_VIDEODESTINATION_PRIVATE_INTERFACE_0_1

/**
 * @file
 * This file defines the <code>PPB_VideoDestination_Private</code> interface
 * for a video destination resource, which sends video frames to a MediaStream
 * video track in the browser.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * The <code>PPB_VideoDestination_Private</code> interface contains pointers to
 * several functions for creating video destination resources and using them to
 * send video frames to a MediaStream video track in the browser.
 */
struct PPB_VideoDestination_Private_0_1 {
  /**
   * Creates a video destination resource.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying an instance of
   * a module.
   *
   * @return A <code>PP_Resource</code> with a nonzero ID on success or zero on
   * failure. Failure means the instance was invalid.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Determines if a resource is a video destination resource.
   *
   * @param[in] resource The <code>PP_Resource</code> to test.
   *
   * @return A <code>PP_Bool</code> with <code>PP_TRUE</code> if the given
   * resource is a video destination resource or <code>PP_FALSE</code>
   * otherwise.
   */
  PP_Bool (*IsVideoDestination)(PP_Resource resource);
  /**
   * Opens a video destination for putting frames.
   *
   * @param[in] destination A <code>PP_Resource</code> corresponding to a video
   * destination resource.
   * @param[in] stream_url A <code>PP_Var</code> string holding a URL
   * identifying a MediaStream.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion of Open().
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   * Returns PP_ERROR_BADRESOURCE if destination isn't a valid video
   * destination.
   * Returns PP_ERROR_INPROGRESS if destination is already open.
   * Returns PP_ERROR_FAILED if the MediaStream doesn't exist or if there is
   * some other browser error.
   */
  int32_t (*Open)(PP_Resource destination,
                  struct PP_Var stream_url,
                  struct PP_CompletionCallback callback);
  /**
   * Puts a frame to the video destination.
   *
   * After this call, you should take care to release your references to the
   * image embedded in the video frame. If you paint to the image after
   * PutFame(), there is the possibility of artifacts because the browser may
   * still be copying the frame to the stream.
   *
   * @param[in] destination A <code>PP_Resource</code> corresponding to a video
   * destination resource.
   * @param[in] frame A <code>PP_VideoFrame_Private</code> holding the video
   * frame to send to the destination.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   * Returns PP_ERROR_BADRESOURCE if destination isn't a valid video
   * destination.
   * Returns PP_ERROR_FAILED if destination is not open, if the video frame has
   * an invalid image data resource, or if some other browser error occurs.
   */
  int32_t (*PutFrame)(PP_Resource destination,
                      const struct PP_VideoFrame_Private* frame);
  /**
   * Closes the video destination.
   *
   * @param[in] destination A <code>PP_Resource</code> corresponding to a video
   * destination.
   */
  void (*Close)(PP_Resource destination);
};

typedef struct PPB_VideoDestination_Private_0_1 PPB_VideoDestination_Private;
/**
 * @}
 */

#endif  /* PPAPI_C_PRIVATE_PPB_VIDEO_DESTINATION_PRIVATE_H_ */

