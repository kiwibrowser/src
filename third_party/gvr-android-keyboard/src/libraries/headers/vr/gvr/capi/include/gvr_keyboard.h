/* Copyright 2017 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VR_GVR_KEYBOARD_CAPI_INCLUDE_GVR_KEYBOARD_H_
#define VR_GVR_KEYBOARD_CAPI_INCLUDE_GVR_KEYBOARD_H_

#ifdef __ANDROID__
#include <jni.h>
#endif

#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr_types.h"
/// @defgroup keyboard GVR Keyboard API
/// @brief The Google VR Keyboard provides a virtual keyboard designed for VR
/// apps.
///
/// Any changes to this API must be backwards compatible (ie old clients should
/// be able to call a newer SDK). Any time a change is made, the API_VERSION in
/// GvrKeyboardLoaderClient.java should be updated.
///
/// ## API usage
///
/// *   `gvr_keyboard_initialize()` initializes the Android JVM. This must be
///     done only once during the entire lifecycle of your application,
///     and before calling any other GVR Keyboard API calls.
/// *   `gvr_keyboard_create()` creates a keyboard and returns a pointer to
///     its context. The keyboard is not shown.
/// *   Call `gvr_keyboard_advance_frame()` on every frame after a keyboard is
///     created with `gvr_create_keyboard()`. This is necessary even when the
///     keyboard is not visible as there may be events to process.
/// *   Call `gvr_keyboard_render()` after `gvr_keyboard_advance_frame()` twice
///     to render keyboard for both eyes.
/// *   `gvr_keyboard_show()` displays the keyboard at a specified location.
/// *   `gvr_keyboard_set_input_mode()` sets input mode of the keyboard.
/// *   Use `gvr_keyboard_on_event()` to send controller updates to the
///     keyboard. This should be done on every frame the keyboard is visible.
/// *   `gvr_keyboard_hide()` hides the keyboard.
/// *   Call `gvr_keyboard_destroy()` to destroy the keyboard when it is no
///     longer used.
///
/// This API is designed to be thread-safe, with some exceptions.
/// `gvr_keyboard_advance_frame()` and `gvr_keyboard_render()` must be called
/// from the GL thread.
/// `gvr_keyboard_destroy()` has potential threading issues as it destroys the
/// `gvr_keyboard_context` pointer. Make sure that when `gvr_keyboard_destroy()`
/// is called, `gvr_keyboard_context` is not used by another thread.

/// @{
#ifdef __cplusplus
extern "C" {
#endif

/// Events from the keyboard. To receive events, set a `gvr_keyboard_callback`
/// callback when you create the keyboard with `gvr_keyboard_create`.
typedef enum {
  /// Unknown error.
  GVR_KEYBOARD_ERROR_UNKNOWN = 0,
  /// The keyboard service could not be connected. This is usually due to the
  /// keyboard service not being installed.
  GVR_KEYBOARD_ERROR_SERVICE_NOT_CONNECTED = 1,
  /// No locale was found in the keyboard service.
  GVR_KEYBOARD_ERROR_NO_LOCALES_FOUND = 2,
  /// The keyboard SDK tried to load dynamically but failed. This is usually due
  /// to the keyboard service not being installed or being out of date.
  GVR_KEYBOARD_ERROR_SDK_LOAD_FAILED = 3,
  /// Keyboard becomes visible.
  GVR_KEYBOARD_SHOWN = 4,
  /// Keyboard becomes hidden.
  GVR_KEYBOARD_HIDDEN = 5,
  /// Text has been updated.
  GVR_KEYBOARD_TEXT_UPDATED = 6,
  /// Text has been committed.
  GVR_KEYBOARD_TEXT_COMMITTED = 7,
} gvr_keyboard_event;

/// The input mode of the keyboard.
typedef enum {
  /// Default keyboard layout.
  GVR_KEYBOARD_MODE_DEFAULT = 0,
  /// Keyboard layout for inputing numbers.
  GVR_KEYBOARD_MODE_NUMERIC = 1,
  /// Keyboard layout for inputing password.
  GVR_KEYBOARD_MODE_PASSWORD = 2,
} gvr_keyboard_input_mode;

typedef struct gvr_mat4f gvr_mat4f;

typedef struct gvr_keyboard_context_ gvr_keyboard_context;

/// Callback for various keyboard events.
/// @param void* Custom closure pointer.
typedef void (*gvr_keyboard_callback)(void* closure, int32_t event);

#ifdef __ANDROID__
/// Initializes the JVM for Android. This must be called exactly once, and
/// before using the Keyboard API.
///
/// @param env The JNIEnv associated with the current thread.
/// @param app_context The Android application context. This must be the
///     application context, NOT an Activity context. (Note: From any Android
///     Activity in your app, you can call getApplicationContext() to retrieve
///     the application context).
/// @param class_loader The class loader to use when loading Java classes.
///     This must be your app's main class loader, usually accessible through
///     activity.getClassLoader() on any of your Activities.
void gvr_keyboard_initialize(JNIEnv* env,
                             jobject app_context,
                             jobject class_loader);
#endif

/// Creates a keyboard and connects to to the Keyboard Service.
///
/// @param closure Custom closure pointer, passed to all the callbacks.
/// @param callback  A pointer to the callback function to receive events.
/// @return A pointer to the new keyboard's context.
gvr_keyboard_context* gvr_keyboard_create(void* closure,
                                          gvr_keyboard_callback callback);

/// Gets the current input mode.
///
/// @param context A pointer to the keyboard's context.
/// @return The input mode of the keyboard.
int32_t gvr_keyboard_get_input_mode(gvr_keyboard_context* context);

/// Sets the input mode of the keyboard.
///
/// @param context A pointer to the keyboard context.
/// @param input_mode The input mode to set.
void gvr_keyboard_set_input_mode(gvr_keyboard_context* context,
                                 int32_t input_mode);

/// Gets the recommended matrix for showing the keyboard. Use
/// `distance_from_eye` to specify the distance from the eyes to the keyboard.
///
/// @param distance_from_eye How far away to render the keyboard, in meters.
///     This number should normally be within [1.0f, 5.0f].
/// @param matrix The matrix to be filled in with the recommended matrix values.
void gvr_keyboard_get_recommended_world_from_keyboard_matrix(
    float distance_from_eye,
    gvr_mat4f* matrix);

/// Shows the keyboard, as specified by `keyboard_matrix`. The matrix should
/// generally be filled using `gvr_keyboard_get_recommended_matrix()`.
///
/// @param context A pointer to the keyboard's context.
/// @param keyboard_matrix A transformation matrix defining where and how to
///     show the keyboard.
void gvr_keyboard_show(gvr_keyboard_context* context);

/// Sets keyboard to world matrix.
///
/// @param context A pointer to the keyboard's context.
/// @param matrix The keyboard to world matrix.
void gvr_keyboard_set_world_from_keyboard_matrix(gvr_keyboard_context* context,
                                                 const gvr_mat4f* matrix);

/// Updates the keyboard with the controller's button state.
///
/// @param context A pointer to the keyboard's context.
/// @param button_index The controller's button defined in gvr.
/// @param pressed Whether button is being pressed.
void gvr_keyboard_update_button_state(gvr_keyboard_context* context,
                                      int32_t button_index,
                                      bool pressed);

/// Updates the ray of the controller.
///
/// @param context A pointer to the keyboard's context.
/// @param start Start position of the ray.
/// @param end End position of the ray.
/// @param hit Hit position if the ray intersects with the keyboard.
/// @return true if the ray hits the keyboard.
bool gvr_keyboard_update_controller_ray(gvr_keyboard_context* context,
                                        const gvr_vec3f* start,
                                        const gvr_vec3f* end,
                                        gvr_vec3f* hit);

/// Updates the touch state of the controller.
///
/// @param context A pointer to the keyboard's context.
/// @param touched Whether touch pad is being touched.
/// @param pos Touch position.
void gvr_keyboard_update_controller_touch(gvr_keyboard_context* context,
                                          bool touched,
                                          const gvr_vec2f* pos);

/// Gets the contents of the keyboard's text field.
///
/// @param context A pointer to the keyboard's context.
/// @return The current text field content of the keyboard. Caller must free
///     the returned pointer when it's not being used.
char* gvr_keyboard_get_text(gvr_keyboard_context* context);

/// Sets the contents of the keyboard's text field.
///
/// @param context A pointer to the keyboard's context.
/// @param edit_text The new text field content.
void gvr_keyboard_set_text(gvr_keyboard_context* context, const char* text);

/// Gets selection range of the keyboard's text field.
///
/// @param context A pointer to the keyboard's context.
/// @param start A pointer to receive the start index, must not be NULL.
/// @param start A pointer to receive the end index, must not be NULL.
void gvr_keyboard_get_selection_indices(gvr_keyboard_context* context,
                                        size_t* start,
                                        size_t* end);

/// Sets selection range of the keyboard's text field.
///
/// @param context A pointer to the keyboard's context.
/// @param start start index.
/// @param start end index.
void gvr_keyboard_set_selection_indices(gvr_keyboard_context* context,
                                        size_t start,
                                        size_t end);

/// Gets composing range of the keyboard's text field.
///
/// @param context A pointer to the keyboard's context.
/// @param start A pointer to receive the start index, must not be NULL.
/// @param start A pointer to receive the end index, must not be NULL.
void gvr_keyboard_get_composing_indices(gvr_keyboard_context* context,
                                        size_t* start,
                                        size_t* end);

/// Sets composing range of the keyboard's text field.
///
/// @param context A pointer to the keyboard's context.
/// @param start start index.
/// @param start end index.
void gvr_keyboard_set_composing_indices(gvr_keyboard_context* context,
                                        size_t start,
                                        size_t end);

/// Sets frame timestamp. This should be called once per frame.
///
/// @param context A pointer to the keyboard's context.
/// @param time The scheduled time of the next frame.
void gvr_keyboard_set_frame_time(gvr_keyboard_context* context,
                                 const gvr_clock_time_point* time);

/// Sets world to camera matrix.
///
/// @param context A pointer to the keyboard's context.
/// @param eye_type Left or right eye variable
/// @param matrix The eye from world matrix.
void gvr_keyboard_set_eye_from_world_matrix(gvr_keyboard_context* context,
                                            int32_t eye_type,
                                            const gvr_mat4f* matrix);

/// Sets projection matrix.
///
/// @param context A pointer to the keyboard's context.
/// @param eye_type Left or right eye variable
/// @param projection The current camera's projection matrix
void gvr_keyboard_set_projection_matrix(gvr_keyboard_context* context,
                                        int32_t eye_type,
                                        const gvr_mat4f* projection);
/// Sets viewport.
///
/// @param context A pointer to the keyboard's context.
/// @param eye_type Left or right eye variable
/// @param viewport The current camera's viewport
void gvr_keyboard_set_viewport(gvr_keyboard_context* context,
                               int32_t eye_type,
                               const gvr_recti* viewport);

/// Handles the new frame. This should be called once from the GL thread on
/// every frame after the keyboard context is created and before calling
/// `gvr_keyboard_render`.
///
/// @param context A pointer to the keyboard's context.
void gvr_keyboard_advance_frame(gvr_keyboard_context* context);

/// Renders the keyboard for a given eye. This should be called from the GL
/// thread.
///
/// @param eye_type gvr eye type.
void gvr_keyboard_render(gvr_keyboard_context* context, int32_t eye_type);

/// Hides the keyboard.
///
/// @param context A pointer to the keyboard's context.
void gvr_keyboard_hide(gvr_keyboard_context* context);

/// Destroys the keyboard. Resources related to the keyboard are released.
/// The `gvr_keyboard_context` pointer is also deleted. This part is not
/// thread-safe, so you must make sure that no other thread is using
/// `gvr_keyboard_context` when this method is getting called.
///
/// @param context A pointer to the keyboard context pointer to destroy.
void gvr_keyboard_destroy(gvr_keyboard_context** context);

#ifdef __cplusplus
}  // extern "C"
#endif

/// @}

#endif  // VR_GVR_KEYBOARD_CAPI_INCLUDE_GVR_KEYBOARD_H_
