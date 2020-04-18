// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_video_renderer_3d.h"

#include <math.h>

#include <algorithm>
#include <utility>

#include "ppapi/c/pp_codecs.h"
#include "ppapi/c/ppb_opengles2.h"
#include "ppapi/c/ppb_video_decoder.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/lib/gl/include/GLES2/gl2.h"
#include "ppapi/lib/gl/include/GLES2/gl2ext.h"
#include "remoting/proto/video.pb.h"
#include "remoting/protocol/frame_stats.h"
#include "remoting/protocol/performance_tracker.h"
#include "remoting/protocol/session_config.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_region.h"

namespace remoting {

namespace {
// The implementation here requires that the decoder allocates at least 3
// pictures. PPB_VideoDecoder didn't support this parameter prior to
// 1.1, so we have to pass 0 for backwards compatibility with older versions of
// the browser. Currently all API implementations allocate more than 3 buffers
// by default.
const uint32_t kMinimumPictureCount = 3;
}  // namespace

class PepperVideoRenderer3D::FrameTracker {
 public:
  FrameTracker(std::unique_ptr<VideoPacket> packet,
               protocol::FrameStatsConsumer* stats_consumer,
               const base::Closure& done)
      : packet_(std::move(packet)),
        stats_consumer_(stats_consumer),
        done_(done) {
    stats_.host_stats = protocol::HostFrameStats::GetForVideoPacket(*packet_);
    stats_.client_stats.time_received = base::TimeTicks::Now();
  }

  ~FrameTracker() {
    if (stats_consumer_)
      stats_consumer_->OnVideoFrameStats(stats_);
    if (!done_.is_null())
      done_.Run();
  }

  void OnDecoded() {
    stats_.client_stats.time_decoded = base::TimeTicks::Now();
  }
  void OnRendered() {
    stats_.client_stats.time_rendered = base::TimeTicks::Now();
  }

  VideoPacket* packet() { return packet_.get(); }

 private:
  std::unique_ptr<VideoPacket> packet_;
  protocol::FrameStatsConsumer* stats_consumer_;
  protocol::FrameStats stats_;
  base::Closure done_;
};

class PepperVideoRenderer3D::Picture {
 public:
  Picture(pp::VideoDecoder* decoder, PP_VideoPicture picture)
      : decoder_(decoder), picture_(picture) {}
  ~Picture() { decoder_->RecyclePicture(picture_); }

  const PP_VideoPicture& picture() { return picture_; }

 private:
  pp::VideoDecoder* decoder_;
  PP_VideoPicture picture_;
};

PepperVideoRenderer3D::PepperVideoRenderer3D() : callback_factory_(this) {}

PepperVideoRenderer3D::~PepperVideoRenderer3D() {
  if (shader_program_)
    gles2_if_->DeleteProgram(graphics_.pp_resource(), shader_program_);
}

void PepperVideoRenderer3D::SetPepperContext(
    pp::Instance* instance,
    EventHandler* event_handler) {
  DCHECK(event_handler);
  DCHECK(!event_handler_);

  fallback_renderer_.SetPepperContext(instance, event_handler);

  event_handler_ = event_handler;
  pp_instance_ = instance;
}

void PepperVideoRenderer3D::OnViewChanged(const pp::View& view) {
  fallback_renderer_.OnViewChanged(view);

  pp::Size size = view.GetRect().size();
  float scale = view.GetDeviceScale();
  DCHECK_GT(scale, 0.0);
  view_size_.set(std::min<int>(ceilf(size.width() * scale),
                               gl_max_viewport_size_[0]),
                 std::min<int>(ceilf(size.height() * scale),
                               gl_max_viewport_size_[1]));
  graphics_.ResizeBuffers(view_size_.width(), view_size_.height());

  force_repaint_ = true;
  PaintIfNeeded();
}

void PepperVideoRenderer3D::EnableDebugDirtyRegion(bool enable) {
  fallback_renderer_.EnableDebugDirtyRegion(enable);
  debug_dirty_region_ = enable;
}

bool PepperVideoRenderer3D::Initialize(
    const ClientContext& context,
    protocol::FrameStatsConsumer* stats_consumer) {
  if (!fallback_renderer_.Initialize(context, stats_consumer)) {
    LOG(FATAL) << "Failed to initialize fallback_renderer_";
  }

  stats_consumer_ = stats_consumer;

  const int32_t context_attributes[] = {
      PP_GRAPHICS3DATTRIB_ALPHA_SIZE,     8,
      PP_GRAPHICS3DATTRIB_BLUE_SIZE,      8,
      PP_GRAPHICS3DATTRIB_GREEN_SIZE,     8,
      PP_GRAPHICS3DATTRIB_RED_SIZE,       8,
      PP_GRAPHICS3DATTRIB_DEPTH_SIZE,     0,
      PP_GRAPHICS3DATTRIB_STENCIL_SIZE,   0,
      PP_GRAPHICS3DATTRIB_SAMPLES,        0,
      PP_GRAPHICS3DATTRIB_SAMPLE_BUFFERS, 0,
      PP_GRAPHICS3DATTRIB_WIDTH,          640,
      PP_GRAPHICS3DATTRIB_HEIGHT,         480,
      PP_GRAPHICS3DATTRIB_NONE,
  };
  graphics_ = pp::Graphics3D(pp_instance_, context_attributes);

  if (graphics_.is_null()) {
    LOG(WARNING) << "Graphics3D interface is not available.";
    return false;
  }
  if (!pp_instance_->BindGraphics(graphics_)) {
    LOG(WARNING) << "Failed to bind Graphics3D.";
    return false;
  }

  // Fetch the GLES2 interface to use to render frames.
  gles2_if_ = static_cast<const PPB_OpenGLES2*>(
      pp::Module::Get()->GetBrowserInterface(PPB_OPENGLES2_INTERFACE));
  CHECK(gles2_if_);

  video_decoder_ = pp::VideoDecoder(pp_instance_);
  if (video_decoder_.is_null()) {
    LOG(WARNING) << "VideoDecoder interface is not available.";
    return false;
  }

  PP_Resource graphics_3d = graphics_.pp_resource();

  gles2_if_->ClearColor(graphics_3d, 1, 0, 0, 1);
  gles2_if_->Clear(graphics_3d, GL_COLOR_BUFFER_BIT);

  // Assign vertex positions and texture coordinates to buffers for use in
  // shader program.
  static const float kVertices[] = {
      -1, -1, -1, 1, 1, -1, 1, 1,  // Position coordinates.
      0,  1,  0,  0, 1, 1,  1, 0,  // Texture coordinates.
  };

  GLuint buffer;
  gles2_if_->GenBuffers(graphics_3d, 1, &buffer);
  gles2_if_->BindBuffer(graphics_3d, GL_ARRAY_BUFFER, buffer);
  gles2_if_->BufferData(graphics_3d, GL_ARRAY_BUFFER, sizeof(kVertices),
                        kVertices, GL_STATIC_DRAW);

  gles2_if_->GetIntegerv(
      graphics_3d, GL_MAX_TEXTURE_SIZE, &gl_max_texture_size_);
  gles2_if_->GetIntegerv(
      graphics_3d, GL_MAX_VIEWPORT_DIMS, gl_max_viewport_size_);

  CheckGLError();

  return true;
}

void PepperVideoRenderer3D::OnSessionConfig(
    const protocol::SessionConfig& config) {
  fallback_renderer_.OnSessionConfig(config);

  PP_VideoProfile video_profile = PP_VIDEOPROFILE_VP8_ANY;
  switch (config.video_config().codec) {
    case protocol::ChannelConfig::CODEC_VP8:
      video_profile = PP_VIDEOPROFILE_VP8_ANY;
      break;
    case protocol::ChannelConfig::CODEC_VP9:
      video_profile = PP_VIDEOPROFILE_VP9_ANY;
      break;
    default:
      NOTREACHED();
  }

  int32_t result = video_decoder_.Initialize(
      graphics_, video_profile, PP_HARDWAREACCELERATION_WITHFALLBACK,
      kMinimumPictureCount,
      callback_factory_.NewCallback(&PepperVideoRenderer3D::OnInitialized));
  CHECK_EQ(result, PP_OK_COMPLETIONPENDING)
      << "video_decoder_.Initialize() returned " << result;
}

protocol::VideoStub* PepperVideoRenderer3D::GetVideoStub() {
  return this;
}

protocol::FrameConsumer* PepperVideoRenderer3D::GetFrameConsumer() {
  // GetFrameConsumer() is used only for WebRTC-based connections which are not
  // supported by the plugin.
  NOTREACHED();
  return nullptr;
}

protocol::FrameStatsConsumer* PepperVideoRenderer3D::GetFrameStatsConsumer() {
  return stats_consumer_;
}

void PepperVideoRenderer3D::ProcessVideoPacket(
    std::unique_ptr<VideoPacket> packet,
    const base::Closure& done) {
  if (!use_fallback_renderer_ &&
      packet->format().has_screen_width() &&
      packet->format().has_screen_height() &&
      (packet->format().screen_width() > gl_max_texture_size_ ||
       packet->format().screen_height() > gl_max_texture_size_)) {
    use_fallback_renderer_ = true;
    // Clear current instance and use fallback_renderer_.
    current_picture_frames_.clear();
    current_picture_.reset();
    next_picture_frames_.clear();
    next_picture_.reset();
    decoded_frames_.clear();
    pending_frames_.clear();
    graphics_ = pp::Graphics3D();
    video_decoder_ = pp::VideoDecoder();
  }

  if (use_fallback_renderer_) {
    fallback_renderer_.GetVideoStub()->ProcessVideoPacket(
        std::move(packet), done);
    return;
  }

  VideoPacket* packet_ptr = packet.get();
  std::unique_ptr<FrameTracker> frame_tracker(
      new FrameTracker(std::move(packet), stats_consumer_, done));

  // Don't need to do anything if the packet is empty. Host sends empty video
  // packets when the screen is not changing.
  if (packet_ptr->data().empty())
    return;

  if (!frame_received_) {
    event_handler_->OnVideoFirstFrameReceived();
    frame_received_ = true;
  }

  if (packet_ptr->format().has_screen_width() &&
      packet_ptr->format().has_screen_height()) {
    frame_size_.set(packet_ptr->format().screen_width(),
                    packet_ptr->format().screen_height());
  }

  // Report the dirty region, for debugging, if requested.
  if (debug_dirty_region_) {
    webrtc::DesktopRegion dirty_region;
    for (int i = 0; i < packet_ptr->dirty_rects_size(); ++i) {
      Rect remoting_rect = packet_ptr->dirty_rects(i);
      dirty_region.AddRect(webrtc::DesktopRect::MakeXYWH(
          remoting_rect.x(), remoting_rect.y(), remoting_rect.width(),
          remoting_rect.height()));
    }
    event_handler_->OnVideoFrameDirtyRegion(dirty_region);
  }

  pending_frames_.push_back(std::move(frame_tracker));
  DecodeNextPacket();
}

void PepperVideoRenderer3D::OnInitialized(int32_t result) {
  // Assume that VP8 and VP9 codecs are always supported by the browser.
  CHECK_EQ(result, PP_OK) << "VideoDecoder::Initialize() failed: " << result;
  initialization_finished_ = true;

  // Start decoding in case a frame was received during decoder initialization.
  DecodeNextPacket();
}

void PepperVideoRenderer3D::DecodeNextPacket() {
  if (!initialization_finished_ || decode_pending_ || pending_frames_.empty())
    return;

  const VideoPacket* packet = pending_frames_.front()->packet();

  int32_t result = video_decoder_.Decode(
      packet->frame_id(), packet->data().size(), packet->data().data(),
      callback_factory_.NewCallback(&PepperVideoRenderer3D::OnDecodeDone));
  CHECK_EQ(result, PP_OK_COMPLETIONPENDING);
  decode_pending_ = true;
}

void PepperVideoRenderer3D::OnDecodeDone(int32_t result) {
  DCHECK(decode_pending_);
  decode_pending_ = false;

  if (result != PP_OK) {
    LOG(ERROR) << "VideoDecoder::Decode() returned " << result;
    event_handler_->OnVideoDecodeError();
    return;
  }

  pending_frames_.front()->OnDecoded();
  // Move the frame from |pending_frames_| to |decoded_frames_|.
  decoded_frames_.splice(decoded_frames_.end(), pending_frames_,
                         pending_frames_.begin());

  DecodeNextPacket();
  GetNextPictureIfReady();
}

void PepperVideoRenderer3D::GetNextPictureIfReady() {
  // Return early if |decoded_frames_| is empty or the decoder is already
  // preparing a picture.  If we call GetPicture() before a new frame has been
  // prepared (i.e. |decoded_frames_| is populated), the OnPictureReady callback
  // could be called before OnDecodeDone() is called which will cause a crash.
  // See crbug.com/689229 for more details.
  if (get_picture_pending_ || decoded_frames_.empty()) {
    return;
  }

  int32_t result =
      video_decoder_.GetPicture(callback_factory_.NewCallbackWithOutput(
          &PepperVideoRenderer3D::OnPictureReady));
  CHECK_EQ(result, PP_OK_COMPLETIONPENDING);
  get_picture_pending_ = true;
}

void PepperVideoRenderer3D::OnPictureReady(int32_t result,
                                           PP_VideoPicture picture) {
  DCHECK(get_picture_pending_);
  get_picture_pending_ = false;

  if (result != PP_OK) {
    LOG(ERROR) << "VideoDecoder::GetPicture() returned " << result;
    event_handler_->OnVideoDecodeError();
    return;
  }

  // Workaround crbug.com/542945 by filling in visible_rect if it isn't set.
  if (picture.visible_rect.size.width == 0 ||
      picture.visible_rect.size.height == 0) {
    static bool warning_logged = false;
    if (!warning_logged) {
      LOG(WARNING) << "PPB_VideoDecoder doesn't set visible_rect.";
      warning_logged = true;
    }

    picture.visible_rect.size.width =
        std::min(frame_size_.width(), picture.texture_size.width);
    picture.visible_rect.size.height =
        std::min(frame_size_.height(), picture.texture_size.height);
  }

  DCHECK_EQ(static_cast<int32_t>(picture.decode_id),
            decoded_frames_.front()->packet()->frame_id());

  // Move the frame from |decoded_frames_| to |next_picture_frames_|.
  next_picture_frames_.splice(next_picture_frames_.end(), decoded_frames_,
                              decoded_frames_.begin());

  next_picture_.reset(new Picture(&video_decoder_, picture));

  PaintIfNeeded();
  GetNextPictureIfReady();
}

void PepperVideoRenderer3D::PaintIfNeeded() {
  bool need_repaint = next_picture_ || (force_repaint_ && current_picture_);
  if (paint_pending_ || !need_repaint)
    return;

  if (next_picture_) {
    current_picture_ = std::move(next_picture_);
    current_picture_frames_.splice(current_picture_frames_.end(),
                                   next_picture_frames_);
  }

  force_repaint_ = false;

  const PP_VideoPicture& picture = current_picture_->picture();
  PP_Resource graphics_3d = graphics_.pp_resource();

  EnsureProgramForTexture(picture.texture_target);

  gles2_if_->UseProgram(graphics_3d, shader_program_);

  // Calculate v_scale passed to the vertex shader.
  double scale_x = picture.visible_rect.size.width;
  double scale_y = picture.visible_rect.size.height;
  if (picture.texture_target != GL_TEXTURE_RECTANGLE_ARB) {
    CHECK(picture.texture_size.width > 0 && picture.texture_size.height > 0);
    scale_x /= picture.texture_size.width;
    scale_y /= picture.texture_size.height;
  }
  gles2_if_->Uniform2f(graphics_3d, shader_texcoord_scale_location_,
                       scale_x, scale_y);

  // Set viewport position & dimensions.
  gles2_if_->Viewport(graphics_3d, 0, 0, view_size_.width(),
                      view_size_.height());

  // Select the texture unit GL_TEXTURE0.
  gles2_if_->ActiveTexture(graphics_3d, GL_TEXTURE0);

  // Select the texture.
  gles2_if_->BindTexture(graphics_3d, picture.texture_target,
                         picture.texture_id);

  // Select linear filter in case the texture needs to be scaled.
  gles2_if_->TexParameteri(graphics_3d, picture.texture_target,
                           GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // When view dimensions are a multiple of the frame size then use
  // nearest-neighbor scaling to achieve crisper image. Linear filter is used in
  // all other cases.
  GLint mag_filter = GL_LINEAR;
  CHECK(picture.visible_rect.size.width > 0 &&
        picture.visible_rect.size.height > 0);
  if (view_size_.width() % picture.visible_rect.size.width == 0 &&
      view_size_.height() % picture.visible_rect.size.height == 0) {
    mag_filter = GL_NEAREST;
  }
  gles2_if_->TexParameteri(graphics_3d, picture.texture_target,
                           GL_TEXTURE_MAG_FILTER, mag_filter);

  // Render texture by drawing a triangle strip with 4 vertices.
  gles2_if_->DrawArrays(graphics_3d, GL_TRIANGLE_STRIP, 0, 4);

  CheckGLError();

  // Request PPAPI display the queued texture.
  int32_t result = graphics_.SwapBuffers(
      callback_factory_.NewCallback(&PepperVideoRenderer3D::OnPaintDone));
  CHECK_EQ(result, PP_OK_COMPLETIONPENDING);
  paint_pending_ = true;
}

void PepperVideoRenderer3D::OnPaintDone(int32_t result) {
  CHECK_EQ(result, PP_OK) << "Graphics3D::SwapBuffers() failed";

  paint_pending_ = false;

  for (const auto& tracker : current_picture_frames_) {
    tracker->OnRendered();
  }
  current_picture_frames_.clear();

  PaintIfNeeded();
}

void PepperVideoRenderer3D::EnsureProgramForTexture(uint32_t texture_target) {
  static const char kVertexShader[] =
      "varying vec2 v_texCoord;            \n"
      "attribute vec4 a_position;          \n"
      "attribute vec2 a_texCoord;          \n"
      "uniform vec2 v_scale;               \n"
      "void main()                         \n"
      "{                                   \n"
      "    v_texCoord = v_scale * a_texCoord; \n"
      "    gl_Position = a_position;       \n"
      "}";

  static const char kFragmentShader2D[] =
      "precision mediump float;            \n"
      "varying vec2 v_texCoord;            \n"
      "uniform sampler2D s_texture;        \n"
      "void main()                         \n"
      "{"
      "    gl_FragColor = texture2D(s_texture, v_texCoord); \n"
      "}";

  static const char kFragmentShaderRectangle[] =
      "#extension GL_ARB_texture_rectangle : require\n"
      "precision mediump float;            \n"
      "varying vec2 v_texCoord;            \n"
      "uniform sampler2DRect s_texture;    \n"
      "void main()                         \n"
      "{"
      "    gl_FragColor = texture2DRect(s_texture, v_texCoord).rgba; \n"
      "}";

  static const char kFragmentShaderExternal[] =
      "#extension GL_OES_EGL_image_external : require\n"
      "precision mediump float;            \n"
      "varying vec2 v_texCoord;            \n"
      "uniform samplerExternalOES s_texture; \n"
      "void main()                         \n"
      "{"
      "    gl_FragColor = texture2D(s_texture, v_texCoord); \n"
      "}";

  // Initialize shader program only if texture type has changed.
  if (current_shader_program_texture_target_ != texture_target) {
    current_shader_program_texture_target_ = texture_target;

    if (texture_target == GL_TEXTURE_2D) {
      CreateProgram(kVertexShader, kFragmentShader2D);
    } else if (texture_target == GL_TEXTURE_RECTANGLE_ARB) {
      CreateProgram(kVertexShader, kFragmentShaderRectangle);
    } else if (texture_target == GL_TEXTURE_EXTERNAL_OES) {
      CreateProgram(kVertexShader, kFragmentShaderExternal);
    } else {
      LOG(FATAL) << "Unknown texture target: " << texture_target;
    }
  }
}

void PepperVideoRenderer3D::CreateProgram(const char* vertex_shader,
                                          const char* fragment_shader) {
  PP_Resource graphics_3d = graphics_.pp_resource();
  if (shader_program_)
    gles2_if_->DeleteProgram(graphics_3d, shader_program_);

  // Create shader program.
  shader_program_ = gles2_if_->CreateProgram(graphics_3d);
  CreateShaderProgram(GL_VERTEX_SHADER, vertex_shader);
  CreateShaderProgram(GL_FRAGMENT_SHADER, fragment_shader);
  gles2_if_->LinkProgram(graphics_3d, shader_program_);
  gles2_if_->UseProgram(graphics_3d, shader_program_);
  gles2_if_->Uniform1i(
      graphics_3d,
      gles2_if_->GetUniformLocation(graphics_3d, shader_program_, "s_texture"),
      0);
  CheckGLError();

  shader_texcoord_scale_location_ = gles2_if_->GetUniformLocation(
      graphics_3d, shader_program_, "v_scale");

  GLint pos_location = gles2_if_->GetAttribLocation(
      graphics_3d, shader_program_, "a_position");
  GLint tc_location = gles2_if_->GetAttribLocation(
      graphics_3d, shader_program_, "a_texCoord");
  CheckGLError();

  // Construct the vertex array for DrawArrays(), using the buffer created in
  // Initialize().
  gles2_if_->EnableVertexAttribArray(graphics_3d, pos_location);
  gles2_if_->VertexAttribPointer(graphics_3d, pos_location, 2, GL_FLOAT,
                                 GL_FALSE, 0, 0);
  gles2_if_->EnableVertexAttribArray(graphics_3d, tc_location);
  gles2_if_->VertexAttribPointer(
      graphics_3d, tc_location, 2, GL_FLOAT, GL_FALSE, 0,
      static_cast<float*>(0) + 8);  // Skip position coordinates.

  gles2_if_->UseProgram(graphics_3d, 0);

  CheckGLError();
}

void PepperVideoRenderer3D::CreateShaderProgram(int type, const char* source) {
  int size = strlen(source);
  GLuint shader = gles2_if_->CreateShader(graphics_.pp_resource(), type);
  gles2_if_->ShaderSource(graphics_.pp_resource(), shader, 1, &source, &size);
  gles2_if_->CompileShader(graphics_.pp_resource(), shader);
  gles2_if_->AttachShader(graphics_.pp_resource(), shader_program_, shader);
  gles2_if_->DeleteShader(graphics_.pp_resource(), shader);
}

void PepperVideoRenderer3D::CheckGLError() {
  GLenum error = gles2_if_->GetError(graphics_.pp_resource());
  CHECK_EQ(error, static_cast<GLenum>(GL_NO_ERROR)) << "GL error: " << error;
}

}  // namespace remoting
