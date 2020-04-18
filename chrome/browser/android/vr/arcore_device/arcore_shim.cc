// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/arcore-android-sdk/src/libraries/include/arcore_c_api.h"

#include <dlfcn.h>

#include "base/logging.h"

namespace {

// Run CALL macro for every function defined in the API.
#define FOR_EACH_API_FN                  \
  CALL(ArCamera_getDisplayOrientedPose)  \
  CALL(ArCamera_getProjectionMatrix)     \
  CALL(ArCamera_getTrackingState)        \
  CALL(ArConfig_create)                  \
  CALL(ArConfig_destroy)                 \
  CALL(ArFrame_acquireCamera)            \
  CALL(ArFrame_create)                   \
  CALL(ArFrame_destroy)                  \
  CALL(ArFrame_transformDisplayUvCoords) \
  CALL(ArPose_create)                    \
  CALL(ArPose_destroy)                   \
  CALL(ArPose_getPoseRaw)                \
  CALL(ArSession_checkSupported)         \
  CALL(ArSession_configure)              \
  CALL(ArSession_create)                 \
  CALL(ArSession_destroy)                \
  CALL(ArSession_pause)                  \
  CALL(ArSession_resume)                 \
  CALL(ArSession_setCameraTextureName)   \
  CALL(ArSession_setDisplayGeometry)     \
  CALL(ArSession_update)

#define CALL(fn) decltype(&fn) impl_##fn = nullptr;
struct ArCoreApi {
  FOR_EACH_API_FN
};
#undef CALL

static void* sdk_handle = nullptr;
static ArCoreApi* arcore_api = nullptr;

template <typename Fn>
void LoadFunction(void* handle, const char* function_name, Fn* fn_out) {
  void* fn = dlsym(handle, function_name);
  if (!fn)
    return;

  *fn_out = reinterpret_cast<Fn>(fn);
}

}  // namespace

namespace vr {

bool LoadArCoreSdk() {
  if (arcore_api)
    return true;

  sdk_handle = dlopen("libarcore_sdk_c_minimal.so", RTLD_GLOBAL | RTLD_NOW);
  if (!sdk_handle) {
    DLOG(ERROR) << "could not open libarcore_sdk_c_minimal.so";
    return false;
  }

  // TODO(vollick): check SDK version.
  arcore_api = new ArCoreApi();

#define CALL(fn) LoadFunction(sdk_handle, #fn, &arcore_api->impl_##fn);
  FOR_EACH_API_FN
#undef CALL

  return true;
}

}  // namespace vr

#undef FOR_EACH_API_FN

void ArCamera_getDisplayOrientedPose(const ArSession* session,
                                     const ArCamera* camera,
                                     ArPose* out_pose) {
  arcore_api->impl_ArCamera_getDisplayOrientedPose(session, camera, out_pose);
}

void ArCamera_getProjectionMatrix(const ArSession* session,
                                  const ArCamera* camera,
                                  float near,
                                  float far,
                                  float* dest_col_major_4x4) {
  arcore_api->impl_ArCamera_getProjectionMatrix(session, camera, near, far,
                                                dest_col_major_4x4);
}
void ArCamera_getTrackingState(const ArSession* session,
                               const ArCamera* camera,
                               ArTrackingState* out_tracking_state) {
  arcore_api->impl_ArCamera_getTrackingState(session, camera,
                                             out_tracking_state);
}

void ArConfig_create(const ArSession* session, ArConfig** out_config) {
  arcore_api->impl_ArConfig_create(session, out_config);
}

void ArConfig_destroy(ArConfig* config) {
  arcore_api->impl_ArConfig_destroy(config);
}

void ArFrame_acquireCamera(const ArSession* session,
                           const ArFrame* frame,
                           ArCamera** out_camera) {
  arcore_api->impl_ArFrame_acquireCamera(session, frame, out_camera);
}

void ArFrame_create(const ArSession* session, ArFrame** out_frame) {
  arcore_api->impl_ArFrame_create(session, out_frame);
}

void ArFrame_destroy(ArFrame* frame) {
  arcore_api->impl_ArFrame_destroy(frame);
}

void ArFrame_transformDisplayUvCoords(const ArSession* session,
                                      const ArFrame* frame,
                                      int32_t num_elements,
                                      const float* uvs_in,
                                      float* uvs_out) {
  arcore_api->impl_ArFrame_transformDisplayUvCoords(
      session, frame, num_elements, uvs_in, uvs_out);
}

void ArPose_create(const ArSession* session,
                   const float* pose_raw,
                   ArPose** out_pose) {
  arcore_api->impl_ArPose_create(session, pose_raw, out_pose);
}

void ArPose_destroy(ArPose* pose) {
  arcore_api->impl_ArPose_destroy(pose);
}

void ArPose_getPoseRaw(const ArSession* session,
                       const ArPose* pose,
                       float* out_pose_raw) {
  arcore_api->impl_ArPose_getPoseRaw(session, pose, out_pose_raw);
}

ArStatus ArSession_checkSupported(const ArSession* session,
                                  const ArConfig* config) {
  return arcore_api->impl_ArSession_checkSupported(session, config);
}

ArStatus ArSession_configure(ArSession* session, const ArConfig* config) {
  return arcore_api->impl_ArSession_configure(session, config);
}

ArStatus ArSession_create(void* env,
                          void* application_context,
                          ArSession** out_session_pointer) {
  return arcore_api->impl_ArSession_create(env, application_context,
                                           out_session_pointer);
}

void ArSession_destroy(ArSession* session) {
  arcore_api->impl_ArSession_destroy(session);
}

ArStatus ArSession_pause(ArSession* session) {
  return arcore_api->impl_ArSession_pause(session);
}

ArStatus ArSession_resume(ArSession* session) {
  return arcore_api->impl_ArSession_resume(session);
}

void ArSession_setCameraTextureName(ArSession* session, uint32_t texture_id) {
  return arcore_api->impl_ArSession_setCameraTextureName(session, texture_id);
}

void ArSession_setDisplayGeometry(ArSession* session,
                                  int32_t rotation,
                                  int32_t width,
                                  int32_t height) {
  return arcore_api->impl_ArSession_setDisplayGeometry(session, rotation, width,
                                                       height);
}

ArStatus ArSession_update(ArSession* session, ArFrame* out_frame) {
  return arcore_api->impl_ArSession_update(session, out_frame);
}
