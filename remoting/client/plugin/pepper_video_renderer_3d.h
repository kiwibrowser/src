// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_VIDEO_RENDERER_3D_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_VIDEO_RENDERER_3D_H_

#include <stdint.h>

#include <list>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "ppapi/cpp/graphics_3d.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/video_decoder.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "remoting/client/plugin/pepper_video_renderer.h"
#include "remoting/client/plugin/pepper_video_renderer_2d.h"
#include "remoting/protocol/video_stub.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"

struct PPB_OpenGLES2;

namespace remoting {

namespace protocol {
class FrameStatsConsumer;
}  // namespace protocol

// PepperVideoRenderer that uses the PPB_VideoDecoder interface for video
// decoding and Graphics3D for rendering.
class PepperVideoRenderer3D : public PepperVideoRenderer,
                              public protocol::VideoStub {
 public:
  PepperVideoRenderer3D();
  ~PepperVideoRenderer3D() override;

  // PepperVideoRenderer interface.
  void SetPepperContext(pp::Instance* instance,
                        EventHandler* event_handler) override;
  void OnViewChanged(const pp::View& view) override;
  void EnableDebugDirtyRegion(bool enable) override;

  // VideoRenderer interface.
  bool Initialize(const ClientContext& client_context,
                  protocol::FrameStatsConsumer* stats_consumer) override;
  void OnSessionConfig(const protocol::SessionConfig& config) override;
  protocol::VideoStub* GetVideoStub() override;
  protocol::FrameConsumer* GetFrameConsumer() override;
  protocol::FrameStatsConsumer* GetFrameStatsConsumer() override;

  // protocol::VideoStub interface.
  void ProcessVideoPacket(std::unique_ptr<VideoPacket> packet,
                          const base::Closure& done) override;

 private:
  // Class responsible for tracking state of a frame until it's rendered.
  class FrameTracker;

  class Picture;

  // Callback for pp::VideoDecoder::Initialize().
  void OnInitialized(int32_t result);

  // Passes one picture from |pending_frames_| to the |video_decoder_|.
  void DecodeNextPacket();

  // Callback for pp::VideoDecoder::Decode().
  void OnDecodeDone(int32_t result);

  // Fetches next picture from the |video_decoder_| if a decoded frame is ready.
  void GetNextPictureIfReady();

  // Callback for pp::VideoDecoder::GetPicture().
  void OnPictureReady(int32_t result, PP_VideoPicture picture);

  // Copies |next_picture_| to |current_picture_| if |next_picture_| is set and
  // then renders |current_picture_|. Doesn't do anything if |need_repaint_| is
  // false.
  void PaintIfNeeded();

  // Callback for pp::Graphics3D::SwapBuffers().
  void OnPaintDone(int32_t result);

  // Initializes |shader_program_| for |texture_target|.
  void EnsureProgramForTexture(uint32_t texture_target);

  // Initializes |shader_program_| with the given shaders.
  void CreateProgram(const char* vertex_shader, const char* fragment_shader);

  // Creates a new shader and compiles |source| for it.
  void CreateShaderProgram(int type, const char* source);

  // CHECKs that the last OpenGL call has completed successfully.
  void CheckGLError();

  pp::Instance* pp_instance_ = nullptr;
  EventHandler* event_handler_ = nullptr;
  protocol::FrameStatsConsumer* stats_consumer_ = nullptr;

  pp::Graphics3D graphics_;
  const PPB_OpenGLES2* gles2_if_;
  pp::VideoDecoder video_decoder_;

  webrtc::DesktopSize frame_size_;

  webrtc::DesktopSize view_size_;

  bool initialization_finished_ = false;
  bool decode_pending_ = false;
  bool get_picture_pending_ = false;
  bool paint_pending_ = false;

  // Frames that have been received, but haven't been passed to the decoder yet.
  std::list<std::unique_ptr<FrameTracker>> pending_frames_;

  // Frames that have been decoded but for which we haven't received the
  // pictures yet.
  std::list<std::unique_ptr<FrameTracker>> decoded_frames_;

  // The next picture to be rendered. PaintIfNeeded() will copy it to
  // |current_picture_| and render it after that. Must be deleted
  // before |video_decoder_|.
  std::unique_ptr<Picture> next_picture_;

  // FrameTracker instances in |next_picture_|.
  std::list<std::unique_ptr<FrameTracker>> next_picture_frames_;

  // The current picture shown on the screen or being rendered. Must be deleted
  // before |video_decoder_|.
  std::unique_ptr<Picture> current_picture_;

  // FrameTrackers for frames in |current_picture_|. The queue is emptied once
  // the |current_picture_| is rendered.
  std::list<std::unique_ptr<FrameTracker>> current_picture_frames_;

  // The fallback software renderer, if input video packet size is larger than
  // hardware limitation.
  PepperVideoRenderer2D fallback_renderer_;
  bool use_fallback_renderer_ = false;

  // Set to true if the screen has been resized and needs to be repainted.
  bool force_repaint_ = false;

  // The texture type for which |shader_program| was initialized. Can be either
  // 0, GL_TEXTURE_2D, GL_TEXTURE_RECTANGLE_ARB or GL_TEXTURE_EXTERNAL_OES. 0
  // indicates that |shader_program_| hasn't been intialized.
  uint32_t current_shader_program_texture_target_ = 0;

  // Shader program ID.
  unsigned int shader_program_ = 0;

  // Location of the scale value to be passed to the |shader_program_|.
  int shader_texcoord_scale_location_ = 0;

  // True if the renderer has received frame from the host.
  bool frame_received_ = false;

  // True if dirty regions are to be sent to |event_handler_| for debugging.
  bool debug_dirty_region_ = false;

  pp::CompletionCallbackFactory<PepperVideoRenderer3D> callback_factory_;

  // The hardware limitation.
  int gl_max_texture_size_;
  int gl_max_viewport_size_[2];

  DISALLOW_COPY_AND_ASSIGN(PepperVideoRenderer3D);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_VIDEO_RENDERER_3D_H_
