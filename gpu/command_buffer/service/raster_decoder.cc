// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/raster_decoder.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "cc/paint/color_space_transfer_cache_entry.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/transfer_cache_entry.h"
#include "components/viz/common/resources/resource_format_utils.h"
#include "gpu/command_buffer/common/capabilities.h"
#include "gpu/command_buffer/common/command_buffer_id.h"
#include "gpu/command_buffer/common/constants.h"
#include "gpu/command_buffer/common/context_result.h"
#include "gpu/command_buffer/common/debug_marker_manager.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/raster_cmd_format.h"
#include "gpu/command_buffer/common/raster_cmd_ids.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/command_buffer/service/buffer_manager.h"
#include "gpu/command_buffer/service/command_buffer_service.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/context_state.h"
#include "gpu/command_buffer/service/decoder_client.h"
#include "gpu/command_buffer/service/error_state.h"
#include "gpu/command_buffer/service/feature_info.h"
#include "gpu/command_buffer/service/framebuffer_manager.h"
#include "gpu/command_buffer/service/gl_stream_texture_image.h"
#include "gpu/command_buffer/service/gl_utils.h"
#include "gpu/command_buffer/service/gles2_cmd_copy_tex_image.h"
#include "gpu/command_buffer/service/gles2_cmd_copy_texture_chromium.h"
#include "gpu/command_buffer/service/gpu_tracer.h"
#include "gpu/command_buffer/service/image_factory.h"
#include "gpu/command_buffer/service/image_manager.h"
#include "gpu/command_buffer/service/indexed_buffer_binding_host.h"
#include "gpu/command_buffer/service/logger.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/query_manager.h"
#include "gpu/command_buffer/service/raster_cmd_validation.h"
#include "gpu/command_buffer/service/service_font_manager.h"
#include "gpu/command_buffer/service/service_transfer_cache.h"
#include "gpu/command_buffer/service/vertex_array_manager.h"
#include "gpu/command_buffer/service/vertex_attrib_manager.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpaceXformCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSurfaceProps.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/GrTypes.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gfx/ipc/color/gfx_param_traits.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_version_info.h"
#include "ui/gl/init/create_gr_gl_interface.h"

// Local versions of the SET_GL_ERROR macros
#define LOCAL_SET_GL_ERROR(error, function_name, msg) \
  ERRORSTATE_SET_GL_ERROR(state_.GetErrorState(), error, function_name, msg)
#define LOCAL_SET_GL_ERROR_INVALID_ENUM(function_name, value, label)          \
  ERRORSTATE_SET_GL_ERROR_INVALID_ENUM(state_.GetErrorState(), function_name, \
                                       static_cast<uint32_t>(value), label)
#define LOCAL_COPY_REAL_GL_ERRORS_TO_WRAPPER(function_name)         \
  ERRORSTATE_COPY_REAL_GL_ERRORS_TO_WRAPPER(state_.GetErrorState(), \
                                            function_name)
#define LOCAL_PEEK_GL_ERROR(function_name) \
  ERRORSTATE_PEEK_GL_ERROR(state_.GetErrorState(), function_name)
#define LOCAL_CLEAR_REAL_GL_ERRORS(function_name) \
  ERRORSTATE_CLEAR_REAL_GL_ERRORS(state_.GetErrorState(), function_name)
#define LOCAL_PERFORMANCE_WARNING(msg) \
  PerformanceWarning(__FILE__, __LINE__, msg)
#define LOCAL_RENDER_WARNING(msg) RenderWarning(__FILE__, __LINE__, msg)

using namespace gpu::gles2;

namespace gpu {
namespace raster {

namespace {

class TextureMetadata {
 public:
  TextureMetadata(bool use_buffer,
                  gfx::BufferUsage buffer_usage,
                  viz::ResourceFormat format,
                  const Capabilities& caps)
      : use_buffer_(use_buffer),
        buffer_usage_(buffer_usage),
        format_(format),
        target_(CalcTarget(use_buffer, buffer_usage, format, caps)) {}
  TextureMetadata(const TextureMetadata& tmd) = default;

  bool use_buffer() const { return use_buffer_; }
  gfx::BufferUsage buffer_usage() const { return buffer_usage_; }
  viz::ResourceFormat format() const { return format_; }
  GLenum target() const { return target_; }

 private:
  static GLenum CalcTarget(bool use_buffer,
                           gfx::BufferUsage buffer_usage,
                           viz::ResourceFormat format,
                           const Capabilities& caps) {
    if (use_buffer) {
      gfx::BufferFormat buffer_format = viz::BufferFormat(format);
      return GetBufferTextureTarget(buffer_usage, buffer_format, caps);
    } else {
      return GL_TEXTURE_2D;
    }
  }

  const bool use_buffer_;
  const gfx::BufferUsage buffer_usage_;
  const viz::ResourceFormat format_;
  const GLenum target_;
};

// This class prevents any GL errors that occur when it is in scope from
// being reported to the client.
class ScopedGLErrorSuppressor {
 public:
  ScopedGLErrorSuppressor(const char* function_name, ErrorState* error_state);
  ~ScopedGLErrorSuppressor();

 private:
  const char* function_name_;
  ErrorState* error_state_;
  DISALLOW_COPY_AND_ASSIGN(ScopedGLErrorSuppressor);
};

ScopedGLErrorSuppressor::ScopedGLErrorSuppressor(const char* function_name,
                                                 ErrorState* error_state)
    : function_name_(function_name), error_state_(error_state) {
  ERRORSTATE_COPY_REAL_GL_ERRORS_TO_WRAPPER(error_state_, function_name_);
}

ScopedGLErrorSuppressor::~ScopedGLErrorSuppressor() {
  ERRORSTATE_CLEAR_REAL_GL_ERRORS(error_state_, function_name_);
}

void RestoreCurrentTextureBindings(ContextState* state,
                                   GLenum target,
                                   GLuint texture_unit,
                                   GrContext* gr_context) {
  DCHECK(!state->texture_units.empty());
  DCHECK_LT(texture_unit, state->texture_units.size());
  TextureUnit& info = state->texture_units[texture_unit];
  GLuint last_id;
  TextureRef* texture_ref = info.GetInfoForTarget(target);
  if (texture_ref) {
    last_id = texture_ref->service_id();
  } else {
    last_id = 0;
  }

  state->api()->glBindTextureFn(target, last_id);

  if (gr_context) {
    gr_context->resetContext(kTextureBinding_GrGLBackendState);
  }
}

// Temporarily changes a decoder's bound texture and restore it when this
// object goes out of scope. Also temporarily switches to using active texture
// unit zero in case the client has changed that to something invalid.
class ScopedTextureBinder {
 public:
  ScopedTextureBinder(ContextState* state,
                      TextureManager* texture_manager,
                      TextureRef* texture_ref,
                      GLenum target,
                      GrContext* gr_context);
  ~ScopedTextureBinder();

 private:
  ContextState* state_;
  GLenum target_;
  TextureUnit old_unit_;
  GrContext* gr_context_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTextureBinder);
};

ScopedTextureBinder::ScopedTextureBinder(ContextState* state,
                                         TextureManager* texture_manager,
                                         TextureRef* texture_ref,
                                         GLenum target,
                                         GrContext* gr_context)
    : state_(state),
      target_(target),
      old_unit_(state->texture_units[0]),
      gr_context_(gr_context) {
  auto* api = state->api();
  api->glActiveTextureFn(GL_TEXTURE0);

  Texture* texture = texture_ref->texture();
  if (texture->target() == 0) {
    texture_manager->SetTarget(texture_ref, target);
  }
  DCHECK_EQ(texture->target(), target)
      << "Texture bound to more than 1 target.";

  api->glBindTextureFn(target, texture_ref->service_id());

  TextureUnit& unit = state_->texture_units[0];
  unit.bind_target = target;
  unit.SetInfoForTarget(target, texture_ref);
}

ScopedTextureBinder::~ScopedTextureBinder() {
  state_->texture_units[0] = old_unit_;

  RestoreCurrentTextureBindings(state_, target_, 0, gr_context_);
  state_->RestoreActiveTexture();
}

// Temporarily changes a decoder's PIXEL_UNPACK_BUFFER to 0 and set pixel unpack
// params to default, and restore them when this object goes out of scope.
class ScopedPixelUnpackState {
 public:
  explicit ScopedPixelUnpackState(ContextState* state);
  ~ScopedPixelUnpackState();

 private:
  ContextState* state_;
  DISALLOW_COPY_AND_ASSIGN(ScopedPixelUnpackState);
};

ScopedPixelUnpackState::ScopedPixelUnpackState(ContextState* state)
    : state_(state) {
  DCHECK(state_);
  state_->PushTextureUnpackState();
}

ScopedPixelUnpackState::~ScopedPixelUnpackState() {
  state_->RestoreUnpackState();
}

bool AllowedBetweenBeginEndRaster(CommandId command) {
  switch (command) {
    case kCreateTransferCacheEntryINTERNAL:
    case kDeleteTransferCacheEntryINTERNAL:
    case kEndRasterCHROMIUM:
    case kFinish:
    case kFlush:
    case kGetError:
    case kRasterCHROMIUM:
    case kUnlockTransferCacheEntryINTERNAL:
      return true;
    default:
      return false;
  }
}

}  // namespace

class RasterDecoderImpl final : public RasterDecoder,
                                public gles2::ErrorStateClient,
                                public ServiceFontManager::Client {
 public:
  RasterDecoderImpl(DecoderClient* client,
                    CommandBufferServiceBase* command_buffer_service,
                    gles2::Outputter* outputter,
                    gles2::ContextGroup* group);
  ~RasterDecoderImpl() override;

  GLES2Util* GetGLES2Util() override { return &util_; }

  // DecoderContext implementation.
  base::WeakPtr<DecoderContext> AsWeakPtr() override;
  ContextResult Initialize(
      const scoped_refptr<gl::GLSurface>& surface,
      const scoped_refptr<gl::GLContext>& context,
      bool offscreen,
      const gles2::DisallowedFeatures& disallowed_features,
      const ContextCreationAttribs& attrib_helper) override;
  const ContextState* GetContextState() override;
  void Destroy(bool have_context) override;
  bool MakeCurrent() override;
  gl::GLContext* GetGLContext() override;
  const FeatureInfo* GetFeatureInfo() const override {
    return feature_info_.get();
  }
  Capabilities GetCapabilities() override;
  void RestoreGlobalState() const override;
  void ClearAllAttributes() const override;
  void RestoreAllAttributes() const override;
  void RestoreState(const gles2::ContextState* prev_state) override;
  void RestoreActiveTexture() const override;
  void RestoreAllTextureUnitAndSamplerBindings(
      const gles2::ContextState* prev_state) const override;
  void RestoreActiveTextureUnitBinding(unsigned int target) const override;
  void RestoreBufferBinding(unsigned int target) override;
  void RestoreBufferBindings() const override;
  void RestoreFramebufferBindings() const override;
  void RestoreRenderbufferBindings() override;
  void RestoreProgramBindings() const override;
  void RestoreTextureState(unsigned service_id) const override;
  void RestoreTextureUnitBindings(unsigned unit) const override;
  void RestoreVertexAttribArray(unsigned index) override;
  void RestoreAllExternalTextureBindingsIfNeeded() override;
  QueryManager* GetQueryManager() override;
  void SetQueryCallback(unsigned int query_client_id,
                        base::OnceClosure callback) override;
  gles2::GpuFenceManager* GetGpuFenceManager() override;
  bool HasPendingQueries() const override;
  void ProcessPendingQueries(bool did_finish) override;
  bool HasMoreIdleWork() const override;
  void PerformIdleWork() override;
  bool HasPollingWork() const override;
  void PerformPollingWork() override;
  TextureBase* GetTextureBase(uint32_t client_id) override;
  void SetLevelInfo(uint32_t client_id,
                    int level,
                    unsigned internal_format,
                    unsigned width,
                    unsigned height,
                    unsigned depth,
                    unsigned format,
                    unsigned type,
                    const gfx::Rect& cleared_rect) override;
  bool WasContextLost() const override;
  bool WasContextLostByRobustnessExtension() const override;
  void MarkContextLost(error::ContextLostReason reason) override;
  bool CheckResetStatus() override;
  void BeginDecoding() override;
  void EndDecoding() override;
  const char* GetCommandName(unsigned int command_id) const;
  error::Error DoCommands(unsigned int num_commands,
                          const volatile void* buffer,
                          int num_entries,
                          int* entries_processed) override;
  base::StringPiece GetLogPrefix() override;
  void BindImage(uint32_t client_texture_id,
                 uint32_t texture_target,
                 gl::GLImage* image,
                 bool can_bind_to_sampler) override;
  gles2::ContextGroup* GetContextGroup() override;
  gles2::ErrorState* GetErrorState() override;
  bool IsCompressedTextureFormat(unsigned format) override;
  bool ClearLevel(gles2::Texture* texture,
                  unsigned target,
                  int level,
                  unsigned format,
                  unsigned type,
                  int xoffset,
                  int yoffset,
                  int width,
                  int height) override;
  bool ClearCompressedTextureLevel(gles2::Texture* texture,
                                   unsigned target,
                                   int level,
                                   unsigned format,
                                   int width,
                                   int height) override;
  bool ClearLevel3D(gles2::Texture* texture,
                    unsigned target,
                    int level,
                    unsigned format,
                    unsigned type,
                    int width,
                    int height,
                    int depth) override {
    NOTIMPLEMENTED();
    return false;
  }
  ServiceTransferCache* GetTransferCacheForTest() override;

  // ErrorClientState implementation.
  void OnContextLostError() override;
  void OnOutOfMemoryError() override;

  Logger* GetLogger() override;

  void SetIgnoreCachedStateForTest(bool ignore) override;
  ImageManager* GetImageManagerForTest() override;

  void SetCopyTextureResourceManagerForTest(
      CopyTextureCHROMIUMResourceManager* copy_texture_resource_manager)
      override;

  // ServiceFontManager::Client implementation.
  scoped_refptr<Buffer> GetShmBuffer(uint32_t shm_id) override;

 private:
  std::unordered_map<GLuint, TextureMetadata> texture_metadata_;
  TextureMetadata* GetTextureMetadata(GLuint client_id) {
    auto it = texture_metadata_.find(client_id);
    DCHECK(it != texture_metadata_.end()) << "Undefined texture id";
    return &it->second;
  }

  gl::GLApi* api() const { return state_.api(); }

  const FeatureInfo::FeatureFlags& features() const {
    return feature_info_->feature_flags();
  }

  const GpuDriverBugWorkarounds& workarounds() const {
    return feature_info_->workarounds();
  }

  bool IsRobustnessSupported() {
    return has_robustness_extension_ &&
           context_->WasAllocatedUsingRobustnessExtension();
  }

  const gl::GLVersionInfo& gl_version_info() {
    return feature_info_->gl_version_info();
  }

  MemoryTracker* memory_tracker() { return group_->memory_tracker(); }

  VertexArrayManager* vertex_array_manager() {
    return vertex_array_manager_.get();
  }

  // Gets the vertex attrib manager for the given vertex array.
  VertexAttribManager* GetVertexAttribManager(GLuint client_id) {
    VertexAttribManager* info =
        vertex_array_manager()->GetVertexAttribManager(client_id);
    return info;
  }

  BufferManager* buffer_manager() { return group_->buffer_manager(); }

  bool EnsureGPUMemoryAvailable(size_t estimated_size) {
    MemoryTracker* tracker = memory_tracker();
    if (tracker) {
      return tracker->EnsureGPUMemoryAvailable(estimated_size);
    }
    return true;
  }

  const TextureManager* texture_manager() const {
    return group_->texture_manager();
  }

  TextureManager* texture_manager() { return group_->texture_manager(); }

  ImageManager* image_manager() { return group_->image_manager(); }

  // Creates a Texture for the given texture.
  TextureRef* CreateTexture(GLuint client_id, GLuint service_id) {
    return texture_manager()->CreateTexture(client_id, service_id);
  }

  // Gets the texture info for the given texture. Returns nullptr if none
  // exists.
  TextureRef* GetTexture(GLuint client_id) const {
    return texture_manager()->GetTexture(client_id);
  }

  // Deletes the texture info for the given texture.
  void RemoveTexture(GLuint client_id) {
    texture_manager()->RemoveTexture(client_id);

    auto texture_iter = texture_metadata_.find(client_id);
    DCHECK(texture_iter != texture_metadata_.end());

    texture_metadata_.erase(texture_iter);
  }

  void UnbindTexture(TextureRef* texture_ref) {
    if (gr_context_) {
      gr_context_->resetContext(kTextureBinding_GrGLBackendState);
    }

    // Unbind texture_ref from texture_ref units.
    state_.UnbindTexture(texture_ref);
  }

  // Creates a vertex attrib manager for the given vertex array.
  scoped_refptr<VertexAttribManager> CreateVertexAttribManager(
      GLuint client_id,
      GLuint service_id,
      bool client_visible) {
    return vertex_array_manager()->CreateVertexAttribManager(
        client_id, service_id, group_->max_vertex_attribs(), client_visible,
        feature_info_->IsWebGL2OrES3Context());
  }

  // Set remaining commands to process to 0 to force DoCommands to return
  // and allow context preemption and GPU watchdog checks in
  // CommandExecutor().
  void ExitCommandProcessingEarly() { commands_to_process_ = 0; }

  void PessimisticallyResetGrContext() const {
    // Calling GrContext::resetContext() is very cheap, so we do it
    // pessimistically. We could dirty less state if skia state setting
    // performance becomes an issue.
    if (gr_context_) {
      gr_context_->resetContext();
    }
  }

  template <bool DebugImpl>
  error::Error DoCommandsImpl(unsigned int num_commands,
                              const volatile void* buffer,
                              int num_entries,
                              int* entries_processed);

  // Helper for glGetIntegerv.  Returns false if pname is unhandled.
  bool GetHelper(GLenum pname, GLint* params, GLsizei* num_written);

  // Gets the number of values that will be returned by glGetXXX. Returns
  // false if pname is unknown.
  bool GetNumValuesReturnedForGLGet(GLenum pname, GLsizei* num_values);

  GLuint DoCreateTexture(bool use_buffer,
                         gfx::BufferUsage /* buffer_usage */,
                         viz::ResourceFormat /* resource_format */);
  void CreateTexture(GLuint client_id,
                     GLuint service_id,
                     bool use_buffer,
                     gfx::BufferUsage buffer_usage,
                     viz::ResourceFormat resource_format);
  void DoCreateAndConsumeTextureINTERNAL(GLuint client_id,
                                         bool use_buffer,
                                         gfx::BufferUsage buffer_usage,
                                         viz::ResourceFormat resource_format,
                                         const volatile GLbyte* key);
  void DeleteTexturesHelper(GLsizei n, const volatile GLuint* client_ids);
  bool GenQueriesEXTHelper(GLsizei n, const GLuint* client_ids);
  void DeleteQueriesEXTHelper(GLsizei n, const volatile GLuint* client_ids);
  void DoFinish();
  void DoFlush();
  void DoGetIntegerv(GLenum pname, GLint* params, GLsizei params_size);
  void DoTexParameteri(GLuint texture_id, GLenum pname, GLint param);
  void DoBindTexImage2DCHROMIUM(GLuint texture_id, GLint image_id);
  void DoTraceEndCHROMIUM();
  void DoProduceTextureDirect(GLuint texture, const volatile GLbyte* key);
  void DoReleaseTexImage2DCHROMIUM(GLuint texture_id, GLint image_id);
  void TexStorage2DImage(TextureRef* texture_ref,
                         const TextureMetadata& texture_metadata,
                         GLsizei width,
                         GLsizei height);
  void TexStorage2D(TextureRef* texture_ref,
                    const TextureMetadata& texture_metadata,
                    GLint levels,
                    GLsizei width,
                    GLsizei height);
  void TexImage2D(TextureRef* texture_ref,
                  const TextureMetadata& texture_metadata,
                  GLsizei width,
                  GLsizei height);
  void DoTexStorage2D(GLuint texture_id,
                      GLint levels,
                      GLsizei width,
                      GLsizei height);
  bool InitializeCopyTexImageBlitter();
  bool InitializeCopyTextureCHROMIUM();
  void DoCopySubTexture(GLuint source_id,
                        GLuint dest_id,
                        GLint xoffset,
                        GLint yoffset,
                        GLint x,
                        GLint y,
                        GLsizei width,
                        GLsizei height);
  // If the texture has an image but that image is not bound or copied to the
  // texture, this will first attempt to bind it, and if that fails
  // CopyTexImage on it. texture_unit is the texture unit it should be bound
  // to, or 0 if it doesn't matter - setting it to 0 will cause the previous
  // binding to be restored after the operation. This returns true if a copy
  // or bind happened and the caller needs to restore the previous texture
  // binding.
  bool DoBindOrCopyTexImageIfNeeded(Texture* texture,
                                    GLenum textarget,
                                    GLuint texture_unit);
  void DoCompressedCopyTextureCHROMIUM(GLuint source_id, GLuint dest_id) {
    NOTIMPLEMENTED();
  }
  void DoLoseContextCHROMIUM(GLenum current, GLenum other) { NOTIMPLEMENTED(); }
  void DoBeginRasterCHROMIUM(GLuint texture_id,
                             GLuint sk_color,
                             GLuint msaa_sample_count,
                             GLboolean can_use_lcd_text,
                             GLint color_type,
                             GLuint color_space_transfer_cache_id);
  void DoRasterCHROMIUM(GLuint raster_shm_id,
                        GLuint raster_shm_offset,
                        GLsizeiptr raster_shm_size,
                        GLuint font_shm_id,
                        GLuint font_shm_offset,
                        GLsizeiptr font_shm_size);
  void DoEndRasterCHROMIUM();
  void DoCreateTransferCacheEntryINTERNAL(GLuint entry_type,
                                          GLuint entry_id,
                                          GLuint handle_shm_id,
                                          GLuint handle_shm_offset,
                                          GLuint data_shm_id,
                                          GLuint data_shm_offset,
                                          GLuint data_size);
  void DoUnlockTransferCacheEntryINTERNAL(GLuint entry_type, GLuint entry_id);
  void DoDeleteTransferCacheEntryINTERNAL(GLuint entry_type, GLuint entry_id);
  void DoUnpremultiplyAndDitherCopyCHROMIUM(GLuint source_id,
                                            GLuint dest_id,
                                            GLint x,
                                            GLint y,
                                            GLsizei width,
                                            GLsizei height) {
    NOTIMPLEMENTED();
  }
  void DoBindVertexArrayOES(GLuint array);
  void EmulateVertexArrayState();
  void RestoreStateForAttrib(GLuint attrib, bool restore_array_binding);

#if defined(NDEBUG)
  void LogClientServiceMapping(const char* /* function_name */,
                               GLuint /* client_id */,
                               GLuint /* service_id */) {}
  template <typename T>
  void LogClientServiceForInfo(T* /* info */,
                               GLuint /* client_id */,
                               const char* /* function_name */) {}
#else
  void LogClientServiceMapping(const char* function_name,
                               GLuint client_id,
                               GLuint service_id) {
    if (service_logging_) {
      VLOG(1) << "[" << logger_.GetLogPrefix() << "] " << function_name
              << ": client_id = " << client_id
              << ", service_id = " << service_id;
    }
  }
  template <typename T>
  void LogClientServiceForInfo(T* info,
                               GLuint client_id,
                               const char* function_name) {
    if (info) {
      LogClientServiceMapping(function_name, client_id, info->service_id());
    }
  }
#endif

// Generate a member function prototype for each command in an automated and
// typesafe way.
#define RASTER_CMD_OP(name) \
  Error Handle##name(uint32_t immediate_data_size, const volatile void* data);

  RASTER_COMMAND_LIST(RASTER_CMD_OP)
#undef RASTER_CMD_OP

  typedef error::Error (RasterDecoderImpl::*CmdHandler)(
      uint32_t immediate_data_size,
      const volatile void* data);

  // A struct to hold info about each command.
  struct CommandInfo {
    CmdHandler cmd_handler;
    uint8_t arg_flags;   // How to handle the arguments for this command
    uint8_t cmd_flags;   // How to handle this command
    uint16_t arg_count;  // How many arguments are expected for this command.
  };

  // A table of CommandInfo for all the commands.
  static const CommandInfo command_info[kNumCommands - kFirstRasterCommand];

  // Most recent generation of the TextureManager.  If this no longer matches
  // the current generation when our context becomes current, then we'll rebind
  // all the textures to stay up to date with Texture::service_id() changes.
  uint32_t texture_manager_service_id_generation_ = 0;

  // Number of commands remaining to be processed in DoCommands().
  int commands_to_process_ = 0;

  bool supports_oop_raster_ = false;

  bool has_robustness_extension_ = false;
  bool context_was_lost_ = false;
  bool reset_by_robustness_extension_ = false;

  // The current decoder error communicates the decoder error through command
  // processing functions that do not return the error value. Should be set
  // only if not returning an error.
  error::Error current_decoder_error_ = error::kNoError;

  scoped_refptr<gl::GLSurface> surface_;
  scoped_refptr<gl::GLContext> context_;

  DecoderClient* client_;

  gles2::DebugMarkerManager debug_marker_manager_;
  gles2::Logger logger_;

  // The ContextGroup for this decoder uses to track resources.
  scoped_refptr<gles2::ContextGroup> group_;
  std::unique_ptr<Validators> validators_;
  scoped_refptr<gles2::FeatureInfo> feature_info_;

  std::unique_ptr<QueryManager> query_manager_;

  std::unique_ptr<VertexArrayManager> vertex_array_manager_;

  // ServiceTransferCache uses Ids based on transfer buffer shm_id+offset, which
  // are guaranteed to be unique within the scope of the TransferBufferManager
  // which generates them. Because of this, |transfer_cache_| must have a
  // narrower scope than |transfer_buffer_manager_|.
  // In the future, we could add necessary scoping Id(s) to allow a single
  // ServiceTransferCache to be shared among multiple contexts / channels.
  std::unique_ptr<ServiceTransferCache> transfer_cache_;

  // All the state for this context.
  gles2::ContextState state_;

  GLES2Util util_;

  // States related to each manager.
  DecoderTextureState texture_state_;
  DecoderFramebufferState framebuffer_state_;

  // An optional behaviour to lose the context and group when OOM.
  bool lose_context_when_out_of_memory_ = false;

  // Log extra info.
  bool service_logging_;

  std::unique_ptr<CopyTexImageResourceManager> copy_tex_image_blit_;
  std::unique_ptr<CopyTextureCHROMIUMResourceManager> copy_texture_chromium_;

  std::unique_ptr<GPUTracer> gpu_tracer_;
  const unsigned char* gpu_decoder_category_;
  static constexpr int gpu_trace_level_ = 2;
  bool gpu_trace_commands_ = false;
  bool gpu_debug_commands_ = false;

  // Raster helpers.
  ServiceFontManager font_manager_;
  sk_sp<GrContext> gr_context_;
  sk_sp<SkSurface> sk_surface_;
  std::unique_ptr<SkCanvas> raster_canvas_;
  uint32_t raster_color_space_id_;
  std::vector<SkDiscardableHandleId> locked_handles_;

  bool need_context_state_reset_ = false;

  base::WeakPtrFactory<DecoderContext> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RasterDecoderImpl);
};

constexpr RasterDecoderImpl::CommandInfo RasterDecoderImpl::command_info[] = {
#define RASTER_CMD_OP(name)                                    \
  {                                                            \
      &RasterDecoderImpl::Handle##name, cmds::name::kArgFlags, \
      cmds::name::cmd_flags,                                   \
      sizeof(cmds::name) / sizeof(CommandBufferEntry) - 1,     \
  }, /* NOLINT */
    RASTER_COMMAND_LIST(RASTER_CMD_OP)
#undef RASTER_CMD_OP
};

// static
RasterDecoder* RasterDecoder::Create(
    DecoderClient* client,
    CommandBufferServiceBase* command_buffer_service,
    Outputter* outputter,
    ContextGroup* group) {
  return new RasterDecoderImpl(client, command_buffer_service, outputter,
                               group);
}

RasterDecoder::RasterDecoder(CommandBufferServiceBase* command_buffer_service,
                             gles2::Outputter* outputter)
    : CommonDecoder(command_buffer_service), outputter_(outputter) {}

RasterDecoder::~RasterDecoder() {}

bool RasterDecoder::initialized() const {
  return initialized_;
}

TextureBase* RasterDecoder::GetTextureBase(uint32_t client_id) {
  return nullptr;
}

void RasterDecoder::SetLevelInfo(uint32_t client_id,
                                 int level,
                                 unsigned internal_format,
                                 unsigned width,
                                 unsigned height,
                                 unsigned depth,
                                 unsigned format,
                                 unsigned type,
                                 const gfx::Rect& cleared_rect) {}

void RasterDecoder::BeginDecoding() {}

void RasterDecoder::EndDecoding() {}

void RasterDecoder::SetLogCommands(bool log_commands) {
  log_commands_ = log_commands;
}

gles2::Outputter* RasterDecoder::outputter() const {
  return outputter_;
}

base::StringPiece RasterDecoder::GetLogPrefix() {
  return GetLogger()->GetLogPrefix();
}

RasterDecoderImpl::RasterDecoderImpl(
    DecoderClient* client,
    CommandBufferServiceBase* command_buffer_service,
    Outputter* outputter,
    ContextGroup* group)
    : RasterDecoder(command_buffer_service, outputter),
      client_(client),
      logger_(&debug_marker_manager_, client),
      group_(group),
      validators_(new Validators),
      feature_info_(group_->feature_info()),
      state_(group_->feature_info(), this, &logger_),
      texture_state_(group_->feature_info()->workarounds()),
      service_logging_(
          group_->gpu_preferences().enable_gpu_service_logging_gpu),
      gpu_decoder_category_(TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(
          TRACE_DISABLED_BY_DEFAULT("gpu_decoder"))),
      font_manager_(this),
      weak_ptr_factory_(this) {}

RasterDecoderImpl::~RasterDecoderImpl() {}

base::WeakPtr<DecoderContext> RasterDecoderImpl::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

ContextResult RasterDecoderImpl::Initialize(
    const scoped_refptr<gl::GLSurface>& surface,
    const scoped_refptr<gl::GLContext>& context,
    bool offscreen,
    const DisallowedFeatures& disallowed_features,
    const ContextCreationAttribs& attrib_helper) {
  TRACE_EVENT0("gpu", "RasterDecoderImpl::Initialize");
  DCHECK(context->IsCurrent(surface.get()));
  DCHECK(!context_.get());

  state_.set_api(gl::g_current_gl_context);

  set_initialized();

  if (!offscreen) {
    return ContextResult::kFatalFailure;
  }

  if (group_->gpu_preferences().enable_gpu_debugging)
    set_debug(true);

  if (group_->gpu_preferences().enable_gpu_command_logging)
    SetLogCommands(true);

  surface_ = surface;
  context_ = context;

  // Create GPU Tracer for timing values.
  gpu_tracer_.reset(new GPUTracer(this));

  // Save the loseContextWhenOutOfMemory context creation attribute.
  lose_context_when_out_of_memory_ =
      attrib_helper.lose_context_when_out_of_memory;

  auto result =
      group_->Initialize(this, attrib_helper.context_type, disallowed_features);
  if (result != ContextResult::kSuccess) {
    group_ =
        nullptr;  // Must not destroy ContextGroup if it is not initialized.
    Destroy(true);
    return result;
  }
  CHECK_GL_ERROR();

  // In theory |needs_emulation| needs to be true on Desktop GL 4.1 or lower.
  // However, we set it to true everywhere, not to trust drivers to handle
  // out-of-bounds buffer accesses.
  bool needs_emulation = true;
  state_.indexed_uniform_buffer_bindings =
      new gles2::IndexedBufferBindingHost(group_->max_uniform_buffer_bindings(),
                                          GL_UNIFORM_BUFFER, needs_emulation);
  state_.indexed_uniform_buffer_bindings->SetIsBound(true);

  state_.InitGenericAttribs(group_->max_vertex_attribs());
  vertex_array_manager_.reset(new VertexArrayManager());

  GLuint default_vertex_attrib_service_id = 0;
  if (features().native_vertex_array_object) {
    api()->glGenVertexArraysOESFn(1, &default_vertex_attrib_service_id);
    api()->glBindVertexArrayOESFn(default_vertex_attrib_service_id);
  }

  state_.default_vertex_attrib_manager =
      CreateVertexAttribManager(0, default_vertex_attrib_service_id, false);

  state_.default_vertex_attrib_manager->Initialize(
      group_->max_vertex_attribs(), workarounds().init_vertex_attributes);

  // vertex_attrib_manager is set to default_vertex_attrib_manager by this call
  DoBindVertexArrayOES(0);

  query_manager_.reset(new QueryManager());

  state_.texture_units.resize(group_->max_texture_units());
  state_.sampler_units.resize(group_->max_texture_units());
  for (uint32_t tt = 0; tt < state_.texture_units.size(); ++tt) {
    api()->glActiveTextureFn(GL_TEXTURE0 + tt);
    TextureRef* ref;
    ref = texture_manager()->GetDefaultTextureInfo(GL_TEXTURE_2D);
    state_.texture_units[tt].bound_texture_2d = ref;
    api()->glBindTextureFn(GL_TEXTURE_2D, ref ? ref->service_id() : 0);
  }
  api()->glActiveTextureFn(GL_TEXTURE0);
  CHECK_GL_ERROR();

  has_robustness_extension_ = features().arb_robustness ||
                              features().khr_robustness ||
                              features().ext_robustness;

  // Set all the default state because some GL drivers get it wrong.
  // TODO(backer): Not all of this state needs to be initialized. Reduce the set
  // if perf becomes a problem.
  state_.InitCapabilities(nullptr);
  state_.InitState(nullptr);

  if (attrib_helper.enable_oop_rasterization) {
    if (!features().chromium_raster_transport) {
      LOG(ERROR) << "ContextResult::kFatalFailure: "
                    "chromium_raster_transport not present";
      return ContextResult::kFatalFailure;
    }

    // Assume that we can create a GrContext. This will be set to false if
    // there's a failure.
    supports_oop_raster_ = true;
    sk_sp<const GrGLInterface> interface(
        gl::init::CreateGrGLInterface(gl_version_info()));
    if (!interface) {
      DLOG(ERROR) << "OOP raster support disabled: GrGLInterface creation "
                     "failed.";
      supports_oop_raster_ = false;
    }

    if (supports_oop_raster_) {
      GrContextOptions options;
      options.fDriverBugWorkarounds =
          GrDriverBugWorkarounds(workarounds().ToIntSet());
      gr_context_ = GrContext::MakeGL(std::move(interface), options);
      if (gr_context_) {
        // TODO(enne): This cache is for this decoder only and each decoder has
        // its own cache.  This is pretty unfortunate.  This really needs to be
        // rethought before shipping.  Most likely a different command buffer
        // context for raster-in-gpu, with a shared gl context / gr context
        // that different decoders can use.
        static constexpr int kMaxGaneshResourceCacheCount = 8196;
        static constexpr size_t kMaxGaneshResourceCacheBytes = 96 * 1024 * 1024;
        gr_context_->setResourceCacheLimits(kMaxGaneshResourceCacheCount,
                                            kMaxGaneshResourceCacheBytes);
        transfer_cache_ = std::make_unique<ServiceTransferCache>();
      } else {
        bool was_lost = CheckResetStatus();
        if (was_lost) {
          DLOG(ERROR)
              << "ContextResult::kTransientFailure: Could not create GrContext";
          return ContextResult::kTransientFailure;
        }
        DLOG(ERROR) << "OOP raster support disabled: GrContext creation "
                       "failed.";
        supports_oop_raster_ = false;
      }
    }
  }

  return ContextResult::kSuccess;
}

const ContextState* RasterDecoderImpl::GetContextState() {
  if (need_context_state_reset_) {
    need_context_state_reset_ = false;
    // Returning nullptr to force full state restoration by the caller.  We do
    // this because GrContext changes to GL state are untracked in our state_.
    return nullptr;
  }

  return &state_;
}

void RasterDecoderImpl::Destroy(bool have_context) {
  if (!initialized())
    return;

  DCHECK(!have_context || context_->IsCurrent(nullptr));

  if (have_context) {
    if (copy_tex_image_blit_.get()) {
      copy_tex_image_blit_->Destroy();
    }

    if (copy_texture_chromium_.get()) {
      copy_texture_chromium_->Destroy();
    }

    if (group_ && group_->texture_manager()) {
      group_->texture_manager()->MarkContextLost();
    }
    if (gr_context_) {
      gr_context_->abandonContext();
    }
    state_.MarkContextLost();
  }

  // Unbind everything.
  state_.vertex_attrib_manager = nullptr;
  state_.default_vertex_attrib_manager = nullptr;
  state_.texture_units.clear();
  state_.sampler_units.clear();
  state_.bound_pixel_pack_buffer = nullptr;
  state_.bound_pixel_unpack_buffer = nullptr;
  state_.indexed_uniform_buffer_bindings = nullptr;

  copy_tex_image_blit_.reset();
  copy_texture_chromium_.reset();

  if (query_manager_.get()) {
    query_manager_->Destroy(have_context);
    query_manager_.reset();
  }

  if (vertex_array_manager_.get()) {
    vertex_array_manager_->Destroy(have_context);
    vertex_array_manager_.reset();
  }

  if (group_.get()) {
    group_->Destroy(this, have_context);
    group_ = NULL;
  }

  // Destroy the surface before the context, some surface destructors make GL
  // calls.
  surface_ = nullptr;

  if (context_.get()) {
    context_->ReleaseCurrent(NULL);
    context_ = NULL;
  }
}

// Make this decoder's GL context current.
bool RasterDecoderImpl::MakeCurrent() {
  DCHECK(surface_);
  if (!context_.get())
    return false;

  if (WasContextLost()) {
    LOG(ERROR) << "  RasterDecoderImpl: Trying to make lost context current.";
    return false;
  }

  if (!context_->MakeCurrent(surface_.get())) {
    LOG(ERROR) << "  RasterDecoderImpl: Context lost during MakeCurrent.";
    MarkContextLost(error::kMakeCurrentFailed);
    group_->LoseContexts(error::kUnknown);
    return false;
  }
  DCHECK_EQ(api(), gl::g_current_gl_context);

  if (CheckResetStatus()) {
    LOG(ERROR)
        << "  RasterDecoderImpl: Context reset detected after MakeCurrent.";
    group_->LoseContexts(error::kUnknown);
    return false;
  }

  // Rebind textures if the service ids may have changed.
  RestoreAllExternalTextureBindingsIfNeeded();

  return true;
}

gl::GLContext* RasterDecoderImpl::GetGLContext() {
  return context_.get();
}

Capabilities RasterDecoderImpl::GetCapabilities() {
  Capabilities caps;
  caps.gpu_rasterization =
      group_->gpu_feature_info()
          .status_values[GPU_FEATURE_TYPE_GPU_RASTERIZATION] ==
      kGpuFeatureStatusEnabled;
  caps.supports_oop_raster = supports_oop_raster_;
  caps.texture_target_exception_list =
      group_->gpu_preferences().texture_target_exception_list;
  caps.texture_format_bgra8888 =
      feature_info_->feature_flags().ext_texture_format_bgra8888;
  caps.texture_storage_image =
      feature_info_->feature_flags().chromium_texture_storage_image;
  caps.texture_storage = feature_info_->feature_flags().ext_texture_storage;
  DoGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.max_texture_size, 1);

  // TODO(backer): If this feature is not turned on, CPU raster gives us random
  // junk, which is a bug (https://crbug.com/828578).
  caps.sync_query = feature_info_->feature_flags().chromium_sync_query;
  return caps;
}

void RasterDecoderImpl::RestoreGlobalState() const {
  PessimisticallyResetGrContext();
  state_.RestoreGlobalState(nullptr);
}

void RasterDecoderImpl::ClearAllAttributes() const {
  // Must use native VAO 0, as RestoreAllAttributes can't fully restore
  // other VAOs.
  if (feature_info_->feature_flags().native_vertex_array_object)
    api()->glBindVertexArrayOESFn(0);

  for (uint32_t i = 0; i < group_->max_vertex_attribs(); ++i) {
    if (i != 0)  // Never disable attribute 0
      state_.vertex_attrib_manager->SetDriverVertexAttribEnabled(i, false);
    if (features().angle_instanced_arrays)
      api()->glVertexAttribDivisorANGLEFn(i, 0);
  }
}

void RasterDecoderImpl::RestoreAllAttributes() const {
  PessimisticallyResetGrContext();
  state_.RestoreVertexAttribs(nullptr);
}

void RasterDecoderImpl::RestoreState(const ContextState* prev_state) {
  TRACE_EVENT1("gpu", "RasterDecoderImpl::RestoreState", "context",
               logger_.GetLogPrefix());
  PessimisticallyResetGrContext();
  state_.RestoreState(prev_state);
}

void RasterDecoderImpl::RestoreActiveTexture() const {
  PessimisticallyResetGrContext();
  state_.RestoreActiveTexture();
}

void RasterDecoderImpl::RestoreAllTextureUnitAndSamplerBindings(
    const ContextState* prev_state) const {
  PessimisticallyResetGrContext();
  state_.RestoreAllTextureUnitAndSamplerBindings(prev_state);
}

void RasterDecoderImpl::RestoreActiveTextureUnitBinding(
    unsigned int target) const {
  PessimisticallyResetGrContext();
  state_.RestoreActiveTextureUnitBinding(target);
}

void RasterDecoderImpl::RestoreBufferBinding(unsigned int target) {
  PessimisticallyResetGrContext();
  if (target == GL_PIXEL_PACK_BUFFER) {
    state_.UpdatePackParameters();
  } else if (target == GL_PIXEL_UNPACK_BUFFER) {
    state_.UpdateUnpackParameters();
  }
  gles2::Buffer* bound_buffer =
      buffer_manager()->GetBufferInfoForTarget(&state_, target);
  api()->glBindBufferFn(target, bound_buffer ? bound_buffer->service_id() : 0);
}

void RasterDecoderImpl::RestoreBufferBindings() const {
  PessimisticallyResetGrContext();
  state_.RestoreBufferBindings();
}

void RasterDecoderImpl::RestoreFramebufferBindings() const {
  NOTIMPLEMENTED();
}

void RasterDecoderImpl::RestoreRenderbufferBindings() {
  PessimisticallyResetGrContext();
  state_.RestoreRenderbufferBindings();
}

void RasterDecoderImpl::RestoreProgramBindings() const {
  PessimisticallyResetGrContext();
  state_.RestoreProgramSettings(nullptr, false);
}

void RasterDecoderImpl::RestoreTextureState(unsigned service_id) const {
  PessimisticallyResetGrContext();
  Texture* texture = texture_manager()->GetTextureForServiceId(service_id);
  if (texture) {
    GLenum target = texture->target();
    api()->glBindTextureFn(target, service_id);
    api()->glTexParameteriFn(target, GL_TEXTURE_WRAP_S, texture->wrap_s());
    api()->glTexParameteriFn(target, GL_TEXTURE_WRAP_T, texture->wrap_t());
    api()->glTexParameteriFn(target, GL_TEXTURE_MIN_FILTER,
                             texture->min_filter());
    api()->glTexParameteriFn(target, GL_TEXTURE_MAG_FILTER,
                             texture->mag_filter());
    if (feature_info_->IsWebGL2OrES3Context()) {
      api()->glTexParameteriFn(target, GL_TEXTURE_BASE_LEVEL,
                               texture->base_level());
    }
    RestoreTextureUnitBindings(state_.active_texture_unit);
  }
}

void RasterDecoderImpl::RestoreTextureUnitBindings(unsigned unit) const {
  PessimisticallyResetGrContext();
  state_.RestoreTextureUnitBindings(unit, nullptr);
}

void RasterDecoderImpl::RestoreVertexAttribArray(unsigned index) {
  NOTIMPLEMENTED();
}

void RasterDecoderImpl::RestoreAllExternalTextureBindingsIfNeeded() {
  PessimisticallyResetGrContext();
  if (texture_manager()->GetServiceIdGeneration() ==
      texture_manager_service_id_generation_)
    return;

  // Texture manager's version has changed, so rebind all external textures
  // in case their service ids have changed.
  for (unsigned texture_unit_index = 0;
       texture_unit_index < state_.texture_units.size(); texture_unit_index++) {
    const TextureUnit& texture_unit = state_.texture_units[texture_unit_index];
    if (texture_unit.bind_target != GL_TEXTURE_EXTERNAL_OES)
      continue;

    if (TextureRef* texture_ref =
            texture_unit.bound_texture_external_oes.get()) {
      api()->glActiveTextureFn(GL_TEXTURE0 + texture_unit_index);
      api()->glBindTextureFn(GL_TEXTURE_EXTERNAL_OES,
                             texture_ref->service_id());
    }
  }

  api()->glActiveTextureFn(GL_TEXTURE0 + state_.active_texture_unit);

  texture_manager_service_id_generation_ =
      texture_manager()->GetServiceIdGeneration();
}

QueryManager* RasterDecoderImpl::GetQueryManager() {
  return query_manager_.get();
}

void RasterDecoderImpl::SetQueryCallback(unsigned int query_client_id,
                                         base::OnceClosure callback) {
  QueryManager::Query* query = query_manager_->GetQuery(query_client_id);
  if (query) {
    query->AddCallback(std::move(callback));
  } else {
    VLOG(1) << "RasterDecoderImpl::SetQueryCallback: No query with ID "
            << query_client_id << ". Running the callback immediately.";
    std::move(callback).Run();
  }
}

GpuFenceManager* RasterDecoderImpl::GetGpuFenceManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

bool RasterDecoderImpl::HasPendingQueries() const {
  return query_manager_.get() && query_manager_->HavePendingQueries();
}

void RasterDecoderImpl::ProcessPendingQueries(bool did_finish) {
  if (!query_manager_.get())
    return;
  query_manager_->ProcessPendingQueries(did_finish);
}

bool RasterDecoderImpl::HasMoreIdleWork() const {
  return gpu_tracer_->HasTracesToProcess();
}

void RasterDecoderImpl::PerformIdleWork() {
  gpu_tracer_->ProcessTraces();
}

bool RasterDecoderImpl::HasPollingWork() const {
  return false;
}

void RasterDecoderImpl::PerformPollingWork() {}

TextureBase* RasterDecoderImpl::GetTextureBase(uint32_t client_id) {
  NOTIMPLEMENTED();
  return nullptr;
}

void RasterDecoderImpl::SetLevelInfo(uint32_t client_id,
                                     int level,
                                     unsigned internal_format,
                                     unsigned width,
                                     unsigned height,
                                     unsigned depth,
                                     unsigned format,
                                     unsigned type,
                                     const gfx::Rect& cleared_rect) {
  NOTIMPLEMENTED();
}

bool RasterDecoderImpl::WasContextLost() const {
  return context_was_lost_;
}

bool RasterDecoderImpl::WasContextLostByRobustnessExtension() const {
  return WasContextLost() && reset_by_robustness_extension_;
}

void RasterDecoderImpl::MarkContextLost(error::ContextLostReason reason) {
  // Only lose the context once.
  if (WasContextLost())
    return;

  // Don't make GL calls in here, the context might not be current.
  command_buffer_service()->SetContextLostReason(reason);
  current_decoder_error_ = error::kLostContext;
  context_was_lost_ = true;

  if (vertex_array_manager_.get()) {
    vertex_array_manager_->MarkContextLost();
  }
  state_.MarkContextLost();
}

bool RasterDecoderImpl::CheckResetStatus() {
  DCHECK(!WasContextLost());
  DCHECK(context_->IsCurrent(nullptr));

  if (IsRobustnessSupported()) {
    // If the reason for the call was a GL error, we can try to determine the
    // reset status more accurately.
    GLenum driver_status = api()->glGetGraphicsResetStatusARBFn();
    if (driver_status == GL_NO_ERROR)
      return false;

    LOG(ERROR)
        << "RasterDecoder context lost via ARB/EXT_robustness. Reset status = "
        << GLES2Util::GetStringEnum(driver_status);

    // Don't pretend we know which client was responsible.
    if (workarounds().use_virtualized_gl_contexts)
      driver_status = GL_UNKNOWN_CONTEXT_RESET_ARB;

    switch (driver_status) {
      case GL_GUILTY_CONTEXT_RESET_ARB:
        MarkContextLost(error::kGuilty);
        break;
      case GL_INNOCENT_CONTEXT_RESET_ARB:
        MarkContextLost(error::kInnocent);
        break;
      case GL_UNKNOWN_CONTEXT_RESET_ARB:
        MarkContextLost(error::kUnknown);
        break;
      default:
        NOTREACHED();
        return false;
    }
    reset_by_robustness_extension_ = true;
    return true;
  }
  return false;
}

Logger* RasterDecoderImpl::GetLogger() {
  return &logger_;
}

void RasterDecoderImpl::SetIgnoreCachedStateForTest(bool ignore) {
  state_.SetIgnoreCachedStateForTest(ignore);
}

ImageManager* RasterDecoderImpl::GetImageManagerForTest() {
  return group_->image_manager();
}

void RasterDecoderImpl::SetCopyTextureResourceManagerForTest(
    CopyTextureCHROMIUMResourceManager* copy_texture_resource_manager) {
  copy_texture_chromium_.reset(copy_texture_resource_manager);
}

void RasterDecoderImpl::BeginDecoding() {
  gpu_tracer_->BeginDecoding();
  gpu_trace_commands_ = gpu_tracer_->IsTracing() && *gpu_decoder_category_;
  gpu_debug_commands_ = log_commands() || debug() || gpu_trace_commands_;
}

void RasterDecoderImpl::EndDecoding() {
  gpu_tracer_->EndDecoding();
}

const char* RasterDecoderImpl::GetCommandName(unsigned int command_id) const {
  if (command_id >= kFirstRasterCommand && command_id < kNumCommands) {
    return raster::GetCommandName(static_cast<CommandId>(command_id));
  }
  return GetCommonCommandName(static_cast<cmd::CommandId>(command_id));
}

template <bool DebugImpl>
error::Error RasterDecoderImpl::DoCommandsImpl(unsigned int num_commands,
                                               const volatile void* buffer,
                                               int num_entries,
                                               int* entries_processed) {
  DCHECK(entries_processed);
  commands_to_process_ = num_commands;
  error::Error result = error::kNoError;
  const volatile CommandBufferEntry* cmd_data =
      static_cast<const volatile CommandBufferEntry*>(buffer);
  int process_pos = 0;
  CommandId command = static_cast<CommandId>(0);

  while (process_pos < num_entries && result == error::kNoError &&
         commands_to_process_--) {
    const unsigned int size = cmd_data->value_header.size;
    command = static_cast<CommandId>(cmd_data->value_header.command);

    if (size == 0) {
      result = error::kInvalidSize;
      break;
    }

    if (static_cast<int>(size) + process_pos > num_entries) {
      result = error::kOutOfBounds;
      break;
    }

    if (DebugImpl && log_commands()) {
      LOG(ERROR) << "[" << logger_.GetLogPrefix() << "]"
                 << "cmd: " << GetCommandName(command);
    }

    const unsigned int arg_count = size - 1;
    unsigned int command_index = command - kFirstRasterCommand;
    if (command_index < arraysize(command_info)) {
      const CommandInfo& info = command_info[command_index];
      if (sk_surface_) {
        if (!AllowedBetweenBeginEndRaster(command)) {
          LOCAL_SET_GL_ERROR(
              GL_INVALID_OPERATION, GetCommandName(command),
              "Unexpected command between BeginRasterCHROMIUM and "
              "EndRasterCHROMIUM");
          process_pos += size;
          cmd_data += size;
          continue;
        }
      }
      unsigned int info_arg_count = static_cast<unsigned int>(info.arg_count);
      if ((info.arg_flags == cmd::kFixed && arg_count == info_arg_count) ||
          (info.arg_flags == cmd::kAtLeastN && arg_count >= info_arg_count)) {
        bool doing_gpu_trace = false;
        if (DebugImpl && gpu_trace_commands_) {
          if (CMD_FLAG_GET_TRACE_LEVEL(info.cmd_flags) <= gpu_trace_level_) {
            doing_gpu_trace = true;
            gpu_tracer_->Begin(TRACE_DISABLED_BY_DEFAULT("gpu_decoder"),
                               GetCommandName(command), kTraceDecoder);
          }
        }

        uint32_t immediate_data_size = (arg_count - info_arg_count) *
                                       sizeof(CommandBufferEntry);  // NOLINT
        result = (this->*info.cmd_handler)(immediate_data_size, cmd_data);

        if (DebugImpl && doing_gpu_trace)
          gpu_tracer_->End(kTraceDecoder);

        if (DebugImpl && debug() && !WasContextLost()) {
          GLenum error;
          while ((error = api()->glGetErrorFn()) != GL_NO_ERROR) {
            LOG(ERROR) << "[" << logger_.GetLogPrefix() << "] "
                       << "GL ERROR: " << GLES2Util::GetStringEnum(error)
                       << " : " << GetCommandName(command);
            LOCAL_SET_GL_ERROR(error, "DoCommand", "GL error from driver");
          }
        }
      } else {
        result = error::kInvalidArguments;
      }
    } else {
      result = DoCommonCommand(command, arg_count, cmd_data);
    }

    if (result == error::kNoError &&
        current_decoder_error_ != error::kNoError) {
      result = current_decoder_error_;
      current_decoder_error_ = error::kNoError;
    }

    if (result != error::kDeferCommandUntilLater) {
      process_pos += size;
      cmd_data += size;
    }
  }

  *entries_processed = process_pos;

  if (error::IsError(result)) {
    LOG(ERROR) << "Error: " << result << " for Command "
               << GetCommandName(command);
  }

  return result;
}

error::Error RasterDecoderImpl::DoCommands(unsigned int num_commands,
                                           const volatile void* buffer,
                                           int num_entries,
                                           int* entries_processed) {
  if (gpu_debug_commands_) {
    return DoCommandsImpl<true>(num_commands, buffer, num_entries,
                                entries_processed);
  } else {
    return DoCommandsImpl<false>(num_commands, buffer, num_entries,
                                 entries_processed);
  }
}

bool RasterDecoderImpl::GetHelper(GLenum pname,
                                  GLint* params,
                                  GLsizei* num_written) {
  DCHECK(num_written);
  switch (pname) {
    case GL_MAX_TEXTURE_SIZE:
      *num_written = 1;
      if (params) {
        params[0] = texture_manager()->MaxSizeForTarget(GL_TEXTURE_2D);
      }
      return true;
    default:
      *num_written = util_.GLGetNumValuesReturned(pname);
      if (*num_written)
        break;

      return false;
  }

  // TODO(backer): Only GL_ACTIVE_TEXTURE supported?
  if (pname != GL_ACTIVE_TEXTURE) {
    return false;
  }

  if (params) {
    api()->glGetIntegervFn(pname, params);
  }
  return true;
}

bool RasterDecoderImpl::GetNumValuesReturnedForGLGet(GLenum pname,
                                                     GLsizei* num_values) {
  *num_values = 0;
  if (state_.GetStateAsGLint(pname, NULL, num_values)) {
    return true;
  }
  return GetHelper(pname, NULL, num_values);
}

base::StringPiece RasterDecoderImpl::GetLogPrefix() {
  return logger_.GetLogPrefix();
}

void RasterDecoderImpl::BindImage(uint32_t client_texture_id,
                                  uint32_t texture_target,
                                  gl::GLImage* image,
                                  bool can_bind_to_sampler) {
  NOTIMPLEMENTED();
}

gles2::ContextGroup* RasterDecoderImpl::GetContextGroup() {
  return group_.get();
}

gles2::ErrorState* RasterDecoderImpl::GetErrorState() {
  return state_.GetErrorState();
}

bool RasterDecoderImpl::IsCompressedTextureFormat(unsigned format) {
  return feature_info_->validators()->compressed_texture_format.IsValid(format);
}

bool RasterDecoderImpl::ClearLevel(gles2::Texture* texture,
                                   unsigned target,
                                   int level,
                                   unsigned format,
                                   unsigned type,
                                   int xoffset,
                                   int yoffset,
                                   int width,
                                   int height) {
  DCHECK(target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY &&
         target != GL_TEXTURE_EXTERNAL_OES);
  uint32_t channels = GLES2Util::GetChannelsForFormat(format);
  if (channels & GLES2Util::kDepth) {
    DCHECK(false) << "depth not supported";
    return false;
  }

  static constexpr uint32_t kMaxZeroSize = 1024 * 1024 * 4;

  uint32_t size;
  uint32_t padded_row_size;
  if (!GLES2Util::ComputeImageDataSizes(width, height, 1, format, type,
                                        state_.unpack_alignment, &size, nullptr,
                                        &padded_row_size)) {
    return false;
  }

  TRACE_EVENT1("gpu", "RasterDecoderImpl::ClearLevel", "size", size);

  int tile_height;

  if (size > kMaxZeroSize) {
    if (kMaxZeroSize < padded_row_size) {
      // That'd be an awfully large texture.
      return false;
    }
    // We should never have a large total size with a zero row size.
    DCHECK_GT(padded_row_size, 0U);
    tile_height = kMaxZeroSize / padded_row_size;
    if (!GLES2Util::ComputeImageDataSizes(width, tile_height, 1, format, type,
                                          state_.unpack_alignment, &size,
                                          nullptr, nullptr)) {
      return false;
    }
  } else {
    tile_height = height;
  }

  api()->glBindTextureFn(texture->target(), texture->service_id());
  {
    // Add extra scope to destroy zero and the object it owns right
    // after its usage.
    // Assumes the size has already been checked.
    std::unique_ptr<char[]> zero(new char[size]);
    memset(zero.get(), 0, size);

    ScopedPixelUnpackState reset_restore(&state_);
    GLint y = 0;
    while (y < height) {
      GLint h = y + tile_height > height ? height - y : tile_height;
      api()->glTexSubImage2DFn(
          target, level, xoffset, yoffset + y, width, h,
          TextureManager::AdjustTexFormat(feature_info_.get(), format), type,
          zero.get());
      y += tile_height;
    }
  }

  TextureRef* bound_texture =
      texture_manager()->GetTextureInfoForTarget(&state_, texture->target());
  api()->glBindTextureFn(texture->target(),
                         bound_texture ? bound_texture->service_id() : 0);
  DCHECK(glGetError() == GL_NO_ERROR);

  if (gr_context_) {
    gr_context_->resetContext(kPixelStore_GrGLBackendState |
                              kTextureBinding_GrGLBackendState);
  }

  return true;
}

bool RasterDecoderImpl::ClearCompressedTextureLevel(gles2::Texture* texture,
                                                    unsigned target,
                                                    int level,
                                                    unsigned format,
                                                    int width,
                                                    int height) {
  DCHECK(target != GL_TEXTURE_3D && target != GL_TEXTURE_2D_ARRAY);
  // This code path can only be called if the texture was originally
  // allocated via TexStorage2D. Note that TexStorage2D is exposed
  // internally for ES 2.0 contexts, but compressed texture support is
  // not part of that exposure.
  DCHECK(feature_info_->IsWebGL2OrES3Context());

  GLsizei bytes_required = 0;
  std::string error_str;
  if (!GetCompressedTexSizeInBytes("ClearCompressedTextureLevel", width, height,
                                   1, format, &bytes_required,
                                   state_.GetErrorState())) {
    return false;
  }

  TRACE_EVENT1("gpu", "RasterDecoderImpl::ClearCompressedTextureLevel",
               "bytes_required", bytes_required);

  api()->glBindBufferFn(GL_PIXEL_UNPACK_BUFFER, 0);
  {
    // Add extra scope to destroy zero and the object it owns right
    // after its usage.
    std::unique_ptr<char[]> zero(new char[bytes_required]);
    memset(zero.get(), 0, bytes_required);
    api()->glBindTextureFn(texture->target(), texture->service_id());
    api()->glCompressedTexSubImage2DFn(target, level, 0, 0, width, height,
                                       format, bytes_required, zero.get());
  }
  TextureRef* bound_texture =
      texture_manager()->GetTextureInfoForTarget(&state_, texture->target());
  api()->glBindTextureFn(texture->target(),
                         bound_texture ? bound_texture->service_id() : 0);
  gles2::Buffer* bound_buffer =
      buffer_manager()->GetBufferInfoForTarget(&state_, GL_PIXEL_UNPACK_BUFFER);
  if (bound_buffer) {
    api()->glBindBufferFn(GL_PIXEL_UNPACK_BUFFER, bound_buffer->service_id());
  }

  if (gr_context_) {
    gr_context_->resetContext(kTextureBinding_GrGLBackendState);
  }
  return true;
}

ServiceTransferCache* RasterDecoderImpl::GetTransferCacheForTest() {
  return transfer_cache_.get();
}

void RasterDecoderImpl::OnContextLostError() {
  if (!WasContextLost()) {
    // Need to lose current context before broadcasting!
    CheckResetStatus();
    group_->LoseContexts(error::kUnknown);
    reset_by_robustness_extension_ = true;
  }
}

void RasterDecoderImpl::OnOutOfMemoryError() {
  if (lose_context_when_out_of_memory_ && !WasContextLost()) {
    error::ContextLostReason other = error::kOutOfMemory;
    if (CheckResetStatus()) {
      other = error::kUnknown;
    } else {
      // Need to lose current context before broadcasting!
      MarkContextLost(error::kOutOfMemory);
    }
    group_->LoseContexts(other);
  }
}

error::Error RasterDecoderImpl::HandleWaitSyncTokenCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::WaitSyncTokenCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::WaitSyncTokenCHROMIUM*>(
          cmd_data);

  static constexpr CommandBufferNamespace kMinNamespaceId =
      CommandBufferNamespace::INVALID;
  static constexpr CommandBufferNamespace kMaxNamespaceId =
      CommandBufferNamespace::NUM_COMMAND_BUFFER_NAMESPACES;

  CommandBufferNamespace namespace_id =
      static_cast<CommandBufferNamespace>(c.namespace_id);
  if ((namespace_id < static_cast<int32_t>(kMinNamespaceId)) ||
      (namespace_id >= static_cast<int32_t>(kMaxNamespaceId))) {
    namespace_id = CommandBufferNamespace::INVALID;
  }
  const CommandBufferId command_buffer_id =
      CommandBufferId::FromUnsafeValue(c.command_buffer_id());
  const uint64_t release = c.release_count();

  SyncToken sync_token;
  sync_token.Set(namespace_id, command_buffer_id, release);
  return client_->OnWaitSyncToken(sync_token) ? error::kDeferCommandUntilLater
                                              : error::kNoError;
}

error::Error RasterDecoderImpl::HandleSetColorSpaceMetadata(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::SetColorSpaceMetadata& c =
      *static_cast<const volatile raster::cmds::SetColorSpaceMetadata*>(
          cmd_data);

  GLuint texture_id = c.texture_id;
  GLsizei color_space_size = c.color_space_size;
  const char* data = static_cast<const char*>(
      GetAddressAndCheckSize(c.shm_id, c.shm_offset, color_space_size));
  if (!data)
    return error::kOutOfBounds;

  // Make a copy to reduce the risk of a time of check to time of use attack.
  std::vector<char> color_space_data(data, data + color_space_size);
  base::Pickle color_space_pickle(color_space_data.data(), color_space_size);
  base::PickleIterator iterator(color_space_pickle);
  gfx::ColorSpace color_space;
  if (!IPC::ParamTraits<gfx::ColorSpace>::Read(&color_space_pickle, &iterator,
                                               &color_space))
    return error::kOutOfBounds;

  TextureRef* ref = texture_manager()->GetTexture(texture_id);
  if (!ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "SetColorSpaceMetadata",
                       "unknown texture");
    return error::kNoError;
  }

  scoped_refptr<gl::GLImage> image =
      ref->texture()->GetLevelImage(ref->texture()->target(), 0);
  if (!image) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "SetColorSpaceMetadata",
                       "no image associated with texture");
    return error::kNoError;
  }

  image->SetColorSpace(color_space);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleBeginQueryEXT(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::BeginQueryEXT& c =
      *static_cast<const volatile raster::cmds::BeginQueryEXT*>(cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLuint client_id = static_cast<GLuint>(c.id);
  int32_t sync_shm_id = static_cast<int32_t>(c.sync_data_shm_id);
  uint32_t sync_shm_offset = static_cast<uint32_t>(c.sync_data_shm_offset);

  switch (target) {
    case GL_COMMANDS_ISSUED_CHROMIUM:
    case GL_COMMANDS_COMPLETED_CHROMIUM:
      break;
    default:
      LOCAL_SET_GL_ERROR(GL_INVALID_ENUM, "glBeginQueryEXT",
                         "unknown query target");
      return error::kNoError;
  }

  if (query_manager_->GetActiveQuery(target)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginQueryEXT",
                       "query already in progress");
    return error::kNoError;
  }

  if (client_id == 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginQueryEXT", "id is 0");
    return error::kNoError;
  }

  scoped_refptr<Buffer> buffer = GetSharedMemoryBuffer(sync_shm_id);
  if (!buffer)
    return error::kInvalidArguments;
  QuerySync* sync = static_cast<QuerySync*>(
      buffer->GetDataAddress(sync_shm_offset, sizeof(QuerySync)));
  if (!sync)
    return error::kOutOfBounds;

  QueryManager::Query* query = query_manager_->GetQuery(client_id);
  if (!query) {
    if (!query_manager_->IsValidQuery(client_id)) {
      LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginQueryEXT",
                         "id not made by glGenQueriesEXT");
      return error::kNoError;
    }

    query =
        query_manager_->CreateQuery(target, client_id, std::move(buffer), sync);
  } else {
    if (query->target() != target) {
      LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginQueryEXT",
                         "target does not match");
      return error::kNoError;
    } else if (query->sync() != sync) {
      DLOG(ERROR) << "Shared memory used by query not the same as before";
      return error::kInvalidArguments;
    }
  }

  query_manager_->BeginQuery(query);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleEndQueryEXT(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::EndQueryEXT& c =
      *static_cast<const volatile raster::cmds::EndQueryEXT*>(cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  uint32_t submit_count = static_cast<GLuint>(c.submit_count);

  QueryManager::Query* query = query_manager_->GetActiveQuery(target);
  if (!query) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glEndQueryEXT",
                       "No active query");
    return error::kNoError;
  }

  query_manager_->EndQuery(query, submit_count);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleInsertFenceSyncCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::InsertFenceSyncCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::InsertFenceSyncCHROMIUM*>(
          cmd_data);

  const uint64_t release_count = c.release_count();
  client_->OnFenceSyncRelease(release_count);
  // Exit inner command processing loop so that we check the scheduling state
  // and yield if necessary as we may have unblocked a higher priority context.
  ExitCommandProcessingEarly();
  return error::kNoError;
}

void RasterDecoderImpl::DoFinish() {
  api()->glFinishFn();
  ProcessPendingQueries(true);
}

void RasterDecoderImpl::DoFlush() {
  api()->glFlushFn();
  ProcessPendingQueries(false);
}

void RasterDecoderImpl::DoGetIntegerv(GLenum pname,
                                      GLint* params,
                                      GLsizei params_size) {
  DCHECK(params);
  GLsizei num_written = 0;
  if (state_.GetStateAsGLint(pname, params, &num_written) ||
      GetHelper(pname, params, &num_written)) {
    DCHECK_EQ(num_written, params_size);
    return;
  }
  NOTREACHED() << "Unhandled enum " << pname;
}

GLuint RasterDecoderImpl::DoCreateTexture(
    bool use_buffer,
    gfx::BufferUsage /* buffer_usage */,
    viz::ResourceFormat /* resource_format */) {
  GLuint service_id = 0;
  api()->glGenTexturesFn(1, &service_id);
  DCHECK(service_id);
  return service_id;
}

void RasterDecoderImpl::CreateTexture(GLuint client_id,
                                      GLuint service_id,
                                      bool use_buffer,
                                      gfx::BufferUsage buffer_usage,
                                      viz::ResourceFormat resource_format) {
  texture_metadata_.emplace(std::make_pair(
      client_id, TextureMetadata(use_buffer, buffer_usage, resource_format,
                                 GetCapabilities())));
  texture_manager()->CreateTexture(client_id, service_id);
}

void RasterDecoderImpl::DoCreateAndConsumeTextureINTERNAL(
    GLuint client_id,
    bool use_buffer,
    gfx::BufferUsage buffer_usage,
    viz::ResourceFormat resource_format,
    const volatile GLbyte* key) {
  TRACE_EVENT2("gpu", "RasterDecoderImpl::DoCreateAndConsumeTextureINTERNAL",
               "context", logger_.GetLogPrefix(), "key[0]",
               static_cast<unsigned char>(key[0]));
  Mailbox mailbox =
      Mailbox::FromVolatile(*reinterpret_cast<const volatile Mailbox*>(key));
  DLOG_IF(ERROR, !mailbox.Verify()) << "CreateAndConsumeTexture was "
                                       "passed a mailbox that was not "
                                       "generated by GenMailboxCHROMIUM.";
  if (!client_id) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION,
                       "glCreateAndConsumeTextureCHROMIUM",
                       "invalid client id");
    return;
  }

  TextureRef* texture_ref = GetTexture(client_id);
  if (texture_ref) {
    // No need to create texture here, the client_id already has an associated
    // texture.
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION,
                       "glCreateAndConsumeTextureCHROMIUM",
                       "client id already in use");
    return;
  }

  texture_metadata_.emplace(std::make_pair(
      client_id, TextureMetadata(use_buffer, buffer_usage, resource_format,
                                 GetCapabilities())));

  Texture* texture =
      static_cast<Texture*>(group_->mailbox_manager()->ConsumeTexture(mailbox));
  if (!texture) {
    // Create texture to handle invalid mailbox (see http://crbug.com/472465).
    GLuint service_id = 0;
    api()->glGenTexturesFn(1, &service_id);
    DCHECK(service_id);
    texture_manager()->CreateTexture(client_id, service_id);

    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION,
                       "glCreateAndConsumeTextureCHROMIUM",
                       "invalid mailbox name");
    return;
  }

  texture_ref = texture_manager()->Consume(client_id, texture);

  // TODO(backer): Validate that the consumed texture is consistent with
  // TextureMetadata.
}

void RasterDecoderImpl::DeleteTexturesHelper(
    GLsizei n,
    const volatile GLuint* client_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    GLuint client_id = client_ids[ii];
    TextureRef* texture_ref = GetTexture(client_id);
    if (texture_ref) {
      UnbindTexture(texture_ref);
      RemoveTexture(client_id);
    }
  }
}

bool RasterDecoderImpl::GenQueriesEXTHelper(GLsizei n,
                                            const GLuint* client_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    if (query_manager_->IsValidQuery(client_ids[ii])) {
      return false;
    }
  }
  query_manager_->GenQueries(n, client_ids);
  return true;
}

void RasterDecoderImpl::DeleteQueriesEXTHelper(
    GLsizei n,
    const volatile GLuint* client_ids) {
  for (GLsizei ii = 0; ii < n; ++ii) {
    GLuint client_id = client_ids[ii];
    query_manager_->RemoveQuery(client_id);
  }
}

void RasterDecoderImpl::DoTexParameteri(GLuint client_id,
                                        GLenum pname,
                                        GLint param) {
  TextureRef* texture = texture_manager()->GetTexture(client_id);
  if (!texture) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexParameteri", "unknown texture");
    return;
  }

  TextureMetadata* texture_metadata = GetTextureMetadata(client_id);
  if (!texture_metadata) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexParameteri", "unknown texture");
    return;
  }

  // TextureManager uses validators from the share group, which may include
  // GLES2. Perform stronger validation here.
  bool valid_param = true;
  bool valid_pname = true;
  switch (pname) {
    case GL_TEXTURE_MIN_FILTER:
      valid_param = validators_->texture_min_filter_mode.IsValid(param);
      break;
    case GL_TEXTURE_MAG_FILTER:
      valid_param = validators_->texture_mag_filter_mode.IsValid(param);
      break;
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
      valid_param = validators_->texture_wrap_mode.IsValid(param);
      break;
    default:
      valid_pname = false;
  }
  if (!valid_pname) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexParameteri", pname, "pname");
    return;
  }
  if (!valid_param) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexParameteri", param, "pname");
    return;
  }

  ScopedTextureBinder binder(&state_, texture_manager(), texture,
                             texture_metadata->target(), gr_context_.get());

  texture_manager()->SetParameteri("glTexParameteri", GetErrorState(), texture,
                                   pname, param);
}

void RasterDecoderImpl::DoBindTexImage2DCHROMIUM(GLuint client_id,
                                                 GLint image_id) {
  TextureRef* texture_ref = GetTexture(client_id);
  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "BindTexImage2DCHROMIUM",
                       "unknown texture");
    return;
  }

  TextureMetadata* texture_metadata = GetTextureMetadata(client_id);
  if (!texture_metadata) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "BindTexImage2DCHROMIUM",
                       "unknown texture");
    return;
  }

  gl::GLImage* image = image_manager()->LookupImage(image_id);
  if (!image) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "BindTexImage2DCHROMIUM",
                       "no image found with the given ID");
    return;
  }

  Texture::ImageState image_state = Texture::UNBOUND;

  {
    ScopedTextureBinder binder(&state_, texture_manager(), texture_ref,
                               texture_metadata->target(), gr_context_.get());

    if (image->BindTexImage(texture_metadata->target()))
      image_state = Texture::BOUND;
  }

  gfx::Size size = image->GetSize();
  texture_manager()->SetLevelInfo(
      texture_ref, texture_metadata->target(), 0, image->GetInternalFormat(),
      size.width(), size.height(), 1, 0, image->GetInternalFormat(),
      GL_UNSIGNED_BYTE, gfx::Rect(size));
  texture_manager()->SetLevelImage(texture_ref, texture_metadata->target(), 0,
                                   image, image_state);
}

void RasterDecoderImpl::DoReleaseTexImage2DCHROMIUM(GLuint client_id,
                                                    GLint image_id) {
  TRACE_EVENT0("gpu", "RasterDecoderImpl::DoReleaseTexImage2DCHROMIUM");

  TextureRef* texture_ref = GetTexture(client_id);
  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "ReleaseTexImage2DCHROMIUM",
                       "unknown texture");
    return;
  }

  TextureMetadata* texture_metadata = GetTextureMetadata(client_id);
  if (!texture_metadata) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "ReleaseTexImage2DCHROMIUM",
                       "unknown texture");
    return;
  }

  gl::GLImage* image = image_manager()->LookupImage(image_id);
  if (!image) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "ReleaseTexImage2DCHROMIUM",
                       "no image found with the given ID");
    return;
  }

  Texture::ImageState image_state;

  // Do nothing when image is not currently bound.
  if (texture_ref->texture()->GetLevelImage(texture_metadata->target(), 0,
                                            &image_state) != image)
    return;

  if (image_state == Texture::BOUND) {
    ScopedTextureBinder binder(&state_, texture_manager(), texture_ref,
                               texture_metadata->target(), gr_context_.get());

    image->ReleaseTexImage(texture_metadata->target());
    texture_manager()->SetLevelInfo(texture_ref, texture_metadata->target(), 0,
                                    GL_RGBA, 0, 0, 1, 0, GL_RGBA,
                                    GL_UNSIGNED_BYTE, gfx::Rect());
  }

  texture_manager()->SetLevelImage(texture_ref, texture_metadata->target(), 0,
                                   nullptr, Texture::UNBOUND);
}

error::Error RasterDecoderImpl::HandleTraceBeginCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile gles2::cmds::TraceBeginCHROMIUM& c =
      *static_cast<const volatile gles2::cmds::TraceBeginCHROMIUM*>(cmd_data);
  Bucket* category_bucket = GetBucket(c.category_bucket_id);
  Bucket* name_bucket = GetBucket(c.name_bucket_id);
  if (!category_bucket || category_bucket->size() == 0 || !name_bucket ||
      name_bucket->size() == 0) {
    return error::kInvalidArguments;
  }

  std::string category_name;
  std::string trace_name;
  if (!category_bucket->GetAsString(&category_name) ||
      !name_bucket->GetAsString(&trace_name)) {
    return error::kInvalidArguments;
  }

  debug_marker_manager_.PushGroup(trace_name);
  if (!gpu_tracer_->Begin(category_name, trace_name, kTraceCHROMIUM)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glTraceBeginCHROMIUM",
                       "unable to create begin trace");
    return error::kNoError;
  }
  return error::kNoError;
}

void RasterDecoderImpl::DoTraceEndCHROMIUM() {
  debug_marker_manager_.PopGroup();
  if (!gpu_tracer_->End(kTraceCHROMIUM)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glTraceEndCHROMIUM",
                       "no trace begin found");
    return;
  }
}

void RasterDecoderImpl::DoProduceTextureDirect(GLuint client_id,
                                               const volatile GLbyte* key) {
  TRACE_EVENT2("gpu", "RasterDecoderImpl::DoProduceTextureDirect", "context",
               logger_.GetLogPrefix(), "key[0]",
               static_cast<unsigned char>(key[0]));

  Mailbox mailbox =
      Mailbox::FromVolatile(*reinterpret_cast<const volatile Mailbox*>(key));
  DLOG_IF(ERROR, !mailbox.Verify()) << "ProduceTextureDirect was passed a "
                                       "mailbox that was not generated by "
                                       "GenMailboxCHROMIUM.";

  TextureRef* texture_ref = GetTexture(client_id);

  bool clear = !client_id;
  if (clear) {
    DCHECK(!texture_ref);

    group_->mailbox_manager()->ProduceTexture(mailbox, nullptr);
    return;
  }

  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "ProduceTextureDirect",
                       "unknown texture");
    return;
  }

  Texture* produced = texture_manager()->Produce(texture_ref);
  if (!produced) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "ProduceTextureDirect",
                       "invalid texture");
    return;
  }

  group_->mailbox_manager()->ProduceTexture(mailbox, produced);
}

void RasterDecoderImpl::TexStorage2DImage(
    TextureRef* texture_ref,
    const TextureMetadata& texture_metadata,
    GLsizei width,
    GLsizei height) {
  TRACE_EVENT2("gpu", "RasterDecoderImpl::TexStorage2DImage", "width", width,
               "height", height);

  gpu::gles2::Texture* texture = texture_ref->texture();
  if (texture->IsImmutable()) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glTexStorage2DImage",
                       "texture is immutable");
    return;
  }

  gfx::BufferFormat buffer_format =
      viz::BufferFormat(texture_metadata.format());
  switch (buffer_format) {
    case gfx::BufferFormat::RGBA_8888:
    case gfx::BufferFormat::BGRA_8888:
    case gfx::BufferFormat::RGBA_F16:
    case gfx::BufferFormat::R_8:
      break;
    default:
      LOCAL_SET_GL_ERROR(GL_INVALID_ENUM, "glTexStorage2DImage",
                         "Invalid buffer format");
      return;
  }
  GLint untyped_format = viz::GLDataFormat(texture_metadata.format());

  if (!GetContextGroup()->image_factory()) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glTexStorage2DImage",
                       "Cannot create GL image");
    return;
  }

  bool is_cleared = false;
  scoped_refptr<gl::GLImage> image =
      GetContextGroup()->image_factory()->CreateAnonymousImage(
          gfx::Size(width, height), buffer_format, gfx::BufferUsage::SCANOUT,
          untyped_format, &is_cleared);

  ScopedTextureBinder binder(&state_, texture_manager(), texture_ref,
                             texture_metadata.target(), gr_context_.get());
  if (!texture_manager()->ValidForTarget(texture_metadata.target(), 0, width,
                                         height, 1)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DImage",
                       "dimensions out of range");
    return;
  }
  if (!image || !image->BindTexImage(texture_metadata.target())) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glTexStorage2DImage",
                       "Failed to create or bind GL Image");
    return;
  }

  gfx::Rect cleared_rect;
  if (is_cleared)
    cleared_rect = gfx::Rect(width, height);

  texture_manager()->SetLevelInfo(texture_ref, texture_metadata.target(), 0,
                                  image->GetInternalFormat(), width, height, 1,
                                  0, image->GetInternalFormat(),
                                  GL_UNSIGNED_BYTE, cleared_rect);
  texture_manager()->SetLevelImage(texture_ref, texture_metadata.target(), 0,
                                   image.get(), gpu::gles2::Texture::BOUND);
}

void RasterDecoderImpl::TexStorage2D(TextureRef* texture_ref,
                                     const TextureMetadata& texture_metadata,
                                     GLint levels,
                                     GLsizei width,
                                     GLsizei height) {
  TRACE_EVENT2("gpu", "RasterDecoderImpl::TexStorage2D", "width", width,
               "height", height);

  if (!texture_manager()->ValidForTarget(texture_metadata.target(), 0, width,
                                         height, 1 /* depth */) ||
      TextureManager::ComputeMipMapCount(texture_metadata.target(), width,
                                         height, 1 /* depth */) < levels) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2D",
                       "dimensions out of range");
    return;
  }

  ScopedTextureBinder binder(&state_, texture_manager(), texture_ref,
                             texture_metadata.target(), gr_context_.get());

  unsigned int internal_format =
      viz::TextureStorageFormat(texture_metadata.format());
  GLenum format =
      TextureManager::ExtractFormatFromStorageFormat(internal_format);
  GLenum type = TextureManager::ExtractTypeFromStorageFormat(internal_format);

  // First lookup compatibility format via texture manager for swizzling legacy
  // LUMINANCE/ALPHA formats.
  GLenum compatibility_internal_format =
      texture_manager()->AdjustTexStorageFormat(feature_info_.get(),
                                                internal_format);

  Texture* texture = texture_ref->texture();
  if (workarounds().reset_base_mipmap_level_before_texstorage &&
      texture->base_level() > 0)
    api()->glTexParameteriFn(texture_metadata.target(), GL_TEXTURE_BASE_LEVEL,
                             0);

  // TODO(zmo): We might need to emulate TexStorage using TexImage or
  // CompressedTexImage on Mac OSX where we expose ES3 APIs when the underlying
  // driver is lower than 4.2 and ARB_texture_storage extension doesn't exist.
  api()->glTexStorage2DEXTFn(texture_metadata.target(), levels,
                             compatibility_internal_format, width, height);
  if (workarounds().reset_base_mipmap_level_before_texstorage &&
      texture->base_level() > 0)
    api()->glTexParameteriFn(texture_metadata.target(), GL_TEXTURE_BASE_LEVEL,
                             texture->base_level());

  {
    GLsizei level_width = width;
    GLsizei level_height = height;

    GLenum adjusted_internal_format =
        feature_info_->IsWebGL1OrES2Context() ? format : internal_format;
    for (int ii = 0; ii < levels; ++ii) {
      texture_manager()->SetLevelInfo(texture_ref, texture_metadata.target(),
                                      ii, adjusted_internal_format, level_width,
                                      level_height, 1 /* level_depth */, 0,
                                      format, type, gfx::Rect());
      level_width = std::max(1, level_width >> 1);
      level_height = std::max(1, level_height >> 1);
    }
    texture->ApplyFormatWorkarounds(feature_info_.get());
  }
}

void RasterDecoderImpl::TexImage2D(TextureRef* texture_ref,
                                   const TextureMetadata& texture_metadata,
                                   GLsizei width,
                                   GLsizei height) {
  TRACE_EVENT2("gpu", "RasterDecoderImpl::TexImage2D", "width", width, "height",
               height);

  // Set as failed for now, but if it successed, this will be set to not failed.
  texture_state_.tex_image_failed = true;

  DCHECK(!state_.bound_pixel_unpack_buffer.get());

  ScopedTextureBinder binder(&state_, texture_manager(), texture_ref,
                             texture_metadata.target(), gr_context_.get());

  TextureManager::DoTexImageArguments args = {
      texture_metadata.target(),
      0 /* level */,
      viz::GLInternalFormat(texture_metadata.format()),
      width,
      height,
      1 /* depth */,
      0 /* border */,
      viz::GLDataFormat(texture_metadata.format()),
      viz::GLDataType(texture_metadata.format()),
      nullptr /* pixels */,
      0 /* pixels_size */,
      0 /* padding */,
      TextureManager::DoTexImageArguments::kTexImage2D};
  texture_manager()->ValidateAndDoTexImage(
      &texture_state_, &state_, &framebuffer_state_, "glTexStorage2D", args);

  // This may be a slow command.  Exit command processing to allow for
  // context preemption and GPU watchdog checks.
  ExitCommandProcessingEarly();
}

void RasterDecoderImpl::DoTexStorage2D(GLuint client_id,
                                       GLint levels,
                                       GLsizei width,
                                       GLsizei height) {
  TextureRef* texture_ref = GetTexture(client_id);
  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2D", "unknown texture");
    return;
  }

  TextureMetadata* texture_metadata = GetTextureMetadata(client_id);
  if (!texture_metadata) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2D", "unknown texture");
    return;
  }

  Texture* texture = texture_ref->texture();
  if (texture->IsImmutable()) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glTexStorage2D",
                       "texture is immutable");
    return;
  }

  if (levels == 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2D", "levels == 0");
    return;
  }

  if (width < 1 || height < 1) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2D", "dimensions < 1");
    return;
  }

  // For testing only. Allows us to stress the ability to respond to OOM errors.
  if (workarounds().simulate_out_of_memory_on_large_textures &&
      (width * height >= 4096 * 4096)) {
    LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, "glTexStorage2D",
                       "synthetic out of memory");
    return;
  }

  // Check if we have enough memory.
  unsigned int internal_format =
      viz::GLInternalFormat(texture_metadata->format());
  bool is_compressed_format =
      viz::IsResourceFormatCompressed(texture_metadata->format());
  GLenum format =
      TextureManager::ExtractFormatFromStorageFormat(internal_format);
  GLenum type = TextureManager::ExtractTypeFromStorageFormat(internal_format);
  GLsizei level_width = width;
  GLsizei level_height = height;
  base::CheckedNumeric<uint32_t> estimated_size(0);
  PixelStoreParams params;
  params.alignment = 1;
  for (int ii = 0; ii < levels; ++ii) {
    uint32_t size;
    if (is_compressed_format) {
      GLsizei level_size;
      if (!GetCompressedTexSizeInBytes("glTexStorage2D", level_width,
                                       level_height, 1, internal_format,
                                       &level_size, state_.GetErrorState())) {
        return;
      }
      size = static_cast<uint32_t>(level_size);
    } else {
      if (!GLES2Util::ComputeImageDataSizesES3(
              level_width, level_height, 1 /* level_depth */, format, type,
              params, &size, nullptr, nullptr, nullptr, nullptr)) {
        LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, "glTexStorage2D",
                           "dimensions too large");
        return;
      }
    }
    estimated_size += size;
    level_width = std::max(1, level_width >> 1);
    level_height = std::max(1, level_height >> 1);
  }

  if (!estimated_size.IsValid() ||
      !EnsureGPUMemoryAvailable(estimated_size.ValueOrDefault(0))) {
    LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, "glTexStorage2D", "out of memory");
    return;
  }

  if (texture_metadata->use_buffer()) {
    if (!GetCapabilities().texture_storage_image) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DImage", "use_buffer");
      return;
    }
    DCHECK_EQ(levels, 1);
    TexStorage2DImage(texture_ref, *texture_metadata, width, height);
  } else if (GetCapabilities().texture_storage) {
    TexStorage2D(texture_ref, *texture_metadata, levels, width, height);
  } else {
    // TODO(backer): Support more than one texture level.
    DCHECK_EQ(levels, 1);
    TexImage2D(texture_ref, *texture_metadata, width, height);
  }

  texture->SetImmutable(true);
}

bool RasterDecoderImpl::InitializeCopyTexImageBlitter() {
  if (!copy_tex_image_blit_.get()) {
    LOCAL_COPY_REAL_GL_ERRORS_TO_WRAPPER("glCopySubTexture");
    copy_tex_image_blit_.reset(
        new CopyTexImageResourceManager(feature_info_.get()));
    copy_tex_image_blit_->Initialize(this);
    if (LOCAL_PEEK_GL_ERROR("glCopySubTexture") != GL_NO_ERROR)
      return false;
  }
  return true;
}

bool RasterDecoderImpl::InitializeCopyTextureCHROMIUM() {
  // Defer initializing the CopyTextureCHROMIUMResourceManager until it is
  // needed because it takes 10s of milliseconds to initialize.
  if (!copy_texture_chromium_.get()) {
    LOCAL_COPY_REAL_GL_ERRORS_TO_WRAPPER("glCopySubTexture");
    copy_texture_chromium_.reset(CopyTextureCHROMIUMResourceManager::Create());
    copy_texture_chromium_->Initialize(this, features());
    if (LOCAL_PEEK_GL_ERROR("glCopySubTexture") != GL_NO_ERROR)
      return false;

    // On the desktop core profile this also needs emulation of
    // CopyTex{Sub}Image2D for luminance, alpha, and luminance_alpha
    // textures.
    if (CopyTexImageResourceManager::CopyTexImageRequiresBlit(
            feature_info_.get(), GL_LUMINANCE)) {
      if (!InitializeCopyTexImageBlitter())
        return false;
    }
  }
  return true;
}

void RasterDecoderImpl::DoCopySubTexture(GLuint source_id,
                                         GLuint dest_id,
                                         GLint xoffset,
                                         GLint yoffset,
                                         GLint x,
                                         GLint y,
                                         GLsizei width,
                                         GLsizei height) {
  TextureRef* source_texture_ref = GetTexture(source_id);
  TextureRef* dest_texture_ref = GetTexture(dest_id);
  if (!source_texture_ref || !dest_texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture",
                       "unknown texture id");
    return;
  }

  Texture* source_texture = source_texture_ref->texture();
  Texture* dest_texture = dest_texture_ref->texture();
  if (source_texture == dest_texture) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glCopySubTexture",
                       "source and destination textures are the same");
    return;
  }

  TextureMetadata* source_texture_metadata = GetTextureMetadata(source_id);
  if (!source_texture_metadata) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture", "unknown texture");
    return;
  }

  TextureMetadata* dest_texture_metadata = GetTextureMetadata(dest_id);
  if (!dest_texture_metadata) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture", "unknown texture");
    return;
  }

  GLenum source_target = source_texture_metadata->target();
  GLint source_level = 0;
  GLenum dest_target = dest_texture_metadata->target();
  GLint dest_level = 0;

  ScopedTextureBinder binder(&state_, texture_manager(), dest_texture_ref,
                             dest_target, gr_context_.get());

  int source_width = 0;
  int source_height = 0;
  gl::GLImage* image =
      source_texture->GetLevelImage(source_target, 0 /* level */);
  if (image) {
    gfx::Size size = image->GetSize();
    source_width = size.width();
    source_height = size.height();
    if (source_width <= 0 || source_height <= 0) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture",
                         "invalid image size");
      return;
    }

    // Ideally we should not need to check that the sub-texture copy rectangle
    // is valid in two different ways, here and below. However currently there
    // is no guarantee that a texture backed by a GLImage will have sensible
    // level info. If this synchronization were to be enforced then this and
    // other functions in this file could be cleaned up.
    // See: https://crbug.com/586476
    int32_t max_x;
    int32_t max_y;
    if (!SafeAddInt32(x, width, &max_x) || !SafeAddInt32(y, height, &max_y) ||
        x < 0 || y < 0 || max_x > source_width || max_y > source_height) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture",
                         "source texture bad dimensions");
      return;
    }
  } else {
    if (!source_texture->GetLevelSize(source_target, 0 /* level */,
                                      &source_width, &source_height, nullptr)) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture",
                         "source texture has no data for level");
      return;
    }

    // Check that this type of texture is allowed.
    if (!texture_manager()->ValidForTarget(source_target, 0 /* level */,
                                           source_width, source_height, 1)) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture",
                         "source texture bad dimensions");
      return;
    }

    if (!source_texture->ValidForTexture(source_target, 0 /* level */, x, y, 0,
                                         width, height, 1)) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture",
                         "source texture bad dimensions.");
      return;
    }
  }

  GLenum source_type = 0;
  GLenum source_internal_format = 0;
  source_texture->GetLevelType(source_target, 0 /* level */, &source_type,
                               &source_internal_format);

  GLenum dest_type = 0;
  GLenum dest_internal_format = 0;
  bool dest_level_defined = dest_texture->GetLevelType(
      dest_target, 0 /* level */, &dest_type, &dest_internal_format);
  if (!dest_level_defined) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glCopySubTexture",
                       "destination texture is not defined");
    return;
  }
  if (!dest_texture->ValidForTexture(dest_target, 0 /* level */, xoffset,
                                     yoffset, 0, width, height, 1)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTexture",
                       "destination texture bad dimensions.");
    return;
  }

  if (feature_info_->feature_flags().desktop_srgb_support) {
    bool enable_framebuffer_srgb =
        GLES2Util::GetColorEncodingFromInternalFormat(source_internal_format) ==
            GL_SRGB ||
        GLES2Util::GetColorEncodingFromInternalFormat(dest_internal_format) ==
            GL_SRGB;
    state_.EnableDisableFramebufferSRGB(enable_framebuffer_srgb);
  }

  // Clear the source texture if necessary.
  if (!texture_manager()->ClearTextureLevel(this, source_texture_ref,
                                            source_target, 0 /* level */)) {
    LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, "glCopySubTexture",
                       "source texture dimensions too big");
    return;
  }

  int dest_width = 0;
  int dest_height = 0;
  bool ok = dest_texture->GetLevelSize(dest_target, dest_level, &dest_width,
                                       &dest_height, nullptr);
  DCHECK(ok);
  if (xoffset != 0 || yoffset != 0 || width != dest_width ||
      height != dest_height) {
    gfx::Rect cleared_rect;
    if (TextureManager::CombineAdjacentRects(
            dest_texture->GetLevelClearedRect(dest_target, dest_level),
            gfx::Rect(xoffset, yoffset, width, height), &cleared_rect)) {
      DCHECK_GE(cleared_rect.size().GetArea(),
                dest_texture->GetLevelClearedRect(dest_target, dest_level)
                    .size()
                    .GetArea());
      texture_manager()->SetLevelClearedRect(dest_texture_ref, dest_target,
                                             dest_level, cleared_rect);
    } else {
      // Otherwise clear part of texture level that is not already cleared.
      if (!texture_manager()->ClearTextureLevel(this, dest_texture_ref,
                                                dest_target, dest_level)) {
        LOCAL_SET_GL_ERROR(GL_OUT_OF_MEMORY, "glCopySubTexture",
                           "destination texture dimensions too big");
        return;
      }
    }
  } else {
    texture_manager()->SetLevelCleared(dest_texture_ref, dest_target,
                                       dest_level, true);
  }

  // TODO(qiankun.miao@intel.com): Support level > 0 for CopyTexSubImage.
  if (image && dest_internal_format == source_internal_format &&
      dest_level == 0) {
    if (image->CopyTexSubImage(dest_target, gfx::Point(xoffset, yoffset),
                               gfx::Rect(x, y, width, height))) {
      return;
    }
  }

  if (!InitializeCopyTextureCHROMIUM())
    return;

  DoBindOrCopyTexImageIfNeeded(source_texture, source_target, 0);

  // GL_TEXTURE_EXTERNAL_OES texture requires apply a transform matrix
  // before presenting.
  if (source_target == GL_TEXTURE_EXTERNAL_OES) {
    if (GLStreamTextureImage* image =
            source_texture->GetLevelStreamTextureImage(GL_TEXTURE_EXTERNAL_OES,
                                                       source_level)) {
      GLfloat transform_matrix[16];
      image->GetTextureMatrix(transform_matrix);

      copy_texture_chromium_->DoCopySubTextureWithTransform(
          this, source_target, source_texture->service_id(), source_level,
          source_internal_format, dest_target, dest_texture->service_id(),
          dest_level, dest_internal_format, xoffset, yoffset, x, y, width,
          height, dest_width, dest_height, source_width, source_height,
          false /* unpack_flip_y */, false /* unpack_premultiply_alpha */,
          false /* unpack_unmultiply_alpha */, false /* dither */,
          transform_matrix, copy_tex_image_blit_.get());
      return;
    }
  }

  CopyTextureMethod method = GetCopyTextureCHROMIUMMethod(
      GetFeatureInfo(), source_target, source_level, source_internal_format,
      source_type, dest_target, dest_level, dest_internal_format,
      false /* unpack_flip_y */, false /* unpack_premultiply_alpha */,
      false /* unpack_unmultiply_alpha */, false /* dither */);
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_X86_FAMILY)
  // glDrawArrays is faster than glCopyTexSubImage2D on IA Mesa driver,
  // although opposite in Android.
  // TODO(dshwang): After Mesa fixes this issue, remove this hack.
  // https://bugs.freedesktop.org/show_bug.cgi?id=98478,
  // https://crbug.com/535198.
  if (Texture::ColorRenderable(GetFeatureInfo(), dest_internal_format,
                               dest_texture->IsImmutable()) &&
      method == CopyTextureMethod::DIRECT_COPY) {
    method = CopyTextureMethod::DIRECT_DRAW;
  }
#endif

  copy_texture_chromium_->DoCopySubTexture(
      this, source_target, source_texture->service_id(), source_level,
      source_internal_format, dest_target, dest_texture->service_id(),
      dest_level, dest_internal_format, xoffset, yoffset, x, y, width, height,
      dest_width, dest_height, source_width, source_height,
      false /* unpack_flip_y */, false /* unpack_premultiply_alpha */,
      false /* unpack_unmultiply_alpha */, false /* dither */, method,
      copy_tex_image_blit_.get());
}

bool RasterDecoderImpl::DoBindOrCopyTexImageIfNeeded(Texture* texture,
                                                     GLenum textarget,
                                                     GLuint texture_unit) {
  // Image is already in use if texture is attached to a framebuffer.
  if (texture && !texture->IsAttachedToFramebuffer()) {
    Texture::ImageState image_state;
    gl::GLImage* image = texture->GetLevelImage(textarget, 0, &image_state);
    if (image && image_state == Texture::UNBOUND) {
      ScopedGLErrorSuppressor suppressor(
          "RasterDecoderImpl::DoBindOrCopyTexImageIfNeeded", GetErrorState());
      if (texture_unit)
        api()->glActiveTextureFn(texture_unit);
      api()->glBindTextureFn(textarget, texture->service_id());
      if (!image->BindTexImage(textarget)) {
        // Note: We update the state to COPIED prior to calling CopyTexImage()
        // as that allows the GLImage implemenatation to set it back to UNBOUND
        // and ensure that CopyTexImage() is called each time the texture is
        // used.
        texture->SetLevelImageState(textarget, 0, Texture::COPIED);
        bool rv = image->CopyTexImage(textarget);
        DCHECK(rv) << "CopyTexImage() failed";
      }
      if (!texture_unit) {
        RestoreCurrentTextureBindings(
            &state_, textarget, state_.active_texture_unit, gr_context_.get());
        return false;
      }
      return true;
    }
  }
  return false;
}

namespace {

// Helper to read client data from transfer cache.
class TransferCacheDeserializeHelperImpl final
    : public cc::TransferCacheDeserializeHelper {
 public:
  explicit TransferCacheDeserializeHelperImpl(
      ServiceTransferCache* transfer_cache)
      : transfer_cache_(transfer_cache) {
    DCHECK(transfer_cache_);
  }
  ~TransferCacheDeserializeHelperImpl() override = default;

  void CreateLocalEntry(
      uint32_t id,
      std::unique_ptr<cc::ServiceTransferCacheEntry> entry) override {
    transfer_cache_->CreateLocalEntry(id, std::move(entry));
  }

 private:
  cc::ServiceTransferCacheEntry* GetEntryInternal(
      cc::TransferCacheEntryType entry_type,
      uint32_t entry_id) override {
    return transfer_cache_->GetEntry(entry_type, entry_id);
  }
  ServiceTransferCache* const transfer_cache_;

  DISALLOW_COPY_AND_ASSIGN(TransferCacheDeserializeHelperImpl);
};

}  // namespace

void RasterDecoderImpl::DoBeginRasterCHROMIUM(
    GLuint texture_id,
    GLuint sk_color,
    GLuint msaa_sample_count,
    GLboolean can_use_lcd_text,
    GLint color_type,
    GLuint color_space_transfer_cache_id) {
  if (!gr_context_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "chromium_raster_transport not enabled via attribs");
    return;
  }
  if (sk_surface_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "BeginRasterCHROMIUM without EndRasterCHROMIUM");
    return;
  }

  DCHECK(locked_handles_.empty());
  DCHECK(!raster_canvas_);
  need_context_state_reset_ = true;

  // This function should look identical to
  // ResourceProvider::ScopedSkSurfaceProvider.
  GrGLTextureInfo texture_info;
  auto* texture_ref = GetTexture(texture_id);
  if (!texture_ref) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "unknown texture id");
    return;
  }
  auto* texture = texture_ref->texture();
  int width;
  int height;
  int depth;
  if (!texture->GetLevelSize(texture->target(), 0, &width, &height, &depth)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "missing texture size info");
    return;
  }
  GLenum type;
  GLenum internal_format;
  if (!texture->GetLevelType(texture->target(), 0, &type, &internal_format)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "missing texture type info");
    return;
  }
  texture_info.fID = texture_ref->service_id();
  texture_info.fTarget = texture->target();

  if (texture->target() != GL_TEXTURE_2D &&
      texture->target() != GL_TEXTURE_RECTANGLE_ARB) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "invalid texture target");
    return;
  }

  // GetInternalFormat may return a base internal format but Skia requires a
  // sized internal format. So this may be adjusted below.
  texture_info.fFormat = GetInternalFormat(&gl_version_info(), internal_format);
  switch (color_type) {
    case kARGB_4444_SkColorType:
      if (internal_format != GL_RGBA4 && internal_format != GL_RGBA) {
        LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                           "color type mismatch");
        return;
      }
      if (texture_info.fFormat == GL_RGBA)
        texture_info.fFormat = GL_RGBA4;
      break;
    case kRGBA_8888_SkColorType:
      if (internal_format != GL_RGBA8_OES && internal_format != GL_RGBA) {
        LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                           "color type mismatch");
        return;
      }
      if (texture_info.fFormat == GL_RGBA)
        texture_info.fFormat = GL_RGBA8_OES;
      break;
    case kBGRA_8888_SkColorType:
      if (internal_format != GL_BGRA_EXT && internal_format != GL_BGRA8_EXT) {
        LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                           "color type mismatch");
        return;
      }
      if (texture_info.fFormat == GL_BGRA_EXT)
        texture_info.fFormat = GL_BGRA8_EXT;
      if (texture_info.fFormat == GL_RGBA)
        texture_info.fFormat = GL_RGBA8_OES;
      break;
    default:
      LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                         "unsupported color type");
      return;
  }

  GrBackendTexture gr_texture(width, height, GrMipMapped::kNo, texture_info);

  uint32_t flags = 0;

  // Use unknown pixel geometry to disable LCD text.
  SkSurfaceProps surface_props(flags, kUnknown_SkPixelGeometry);
  if (can_use_lcd_text) {
    // LegacyFontHost will get LCD text and skia figures out what type to use.
    surface_props =
        SkSurfaceProps(flags, SkSurfaceProps::kLegacyFontHost_InitType);
  }

  SkColorType sk_color_type = static_cast<SkColorType>(color_type);
  // If we can't match requested MSAA samples, don't use MSAA.
  int final_msaa_count = std::max(static_cast<int>(msaa_sample_count), 0);
  if (final_msaa_count >
      gr_context_->maxSurfaceSampleCountForColorType(sk_color_type))
    final_msaa_count = 0;
  sk_surface_ = SkSurface::MakeFromBackendTextureAsRenderTarget(
      gr_context_.get(), gr_texture, kTopLeft_GrSurfaceOrigin, final_msaa_count,
      sk_color_type, nullptr, &surface_props);

  if (!sk_surface_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "failed to create surface");
    return;
  }

  TransferCacheDeserializeHelperImpl transfer_cache_deserializer(
      transfer_cache_.get());
  auto* color_space_entry =
      transfer_cache_deserializer
          .GetEntryAs<cc::ServiceColorSpaceTransferCacheEntry>(
              color_space_transfer_cache_id);
  if (!color_space_entry || !color_space_entry->color_space().IsValid()) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "failed to find valid color space");
    return;
  }
  raster_canvas_ = SkCreateColorSpaceXformCanvas(
      sk_surface_->getCanvas(),
      color_space_entry->color_space().ToSkColorSpace());
  raster_color_space_id_ = color_space_transfer_cache_id;

  // All or nothing clearing, as no way to validate the client's input on what
  // is the "used" part of the texture.
  if (texture->IsLevelCleared(texture->target(), 0))
    return;

  // TODO(enne): This doesn't handle the case where the background color
  // changes and so any extra pixels outside the raster area that get
  // sampled may be incorrect.
  raster_canvas_->drawColor(sk_color);
  texture_manager()->SetLevelCleared(texture_ref, texture->target(), 0, true);
}

scoped_refptr<Buffer> RasterDecoderImpl::GetShmBuffer(uint32_t shm_id) {
  return GetSharedMemoryBuffer(shm_id);
}

void RasterDecoderImpl::DoRasterCHROMIUM(GLuint raster_shm_id,
                                         GLuint raster_shm_offset,
                                         GLsizeiptr raster_shm_size,
                                         GLuint font_shm_id,
                                         GLuint font_shm_offset,
                                         GLsizeiptr font_shm_size) {
  TRACE_EVENT0("gpu", "RasterDecoderImpl::DoRasterCHROMIUM");

  if (!sk_surface_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glRasterCHROMIUM",
                       "RasterCHROMIUM without BeginRasterCHROMIUM");
    return;
  }
  DCHECK(transfer_cache_);
  need_context_state_reset_ = true;

  if (font_shm_size > 0) {
    // Deserialize fonts before raster.
    volatile char* font_buffer_memory =
        GetSharedMemoryAs<char*>(font_shm_id, font_shm_offset, font_shm_size);
    if (!font_buffer_memory) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glRasterCHROMIUM",
                         "Can not read font buffer.");
      return;
    }

    if (!font_manager_.Deserialize(font_buffer_memory, font_shm_size,
                                   &locked_handles_)) {
      LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glRasterCHROMIUM",
                         "Invalid font buffer.");
      return;
    }
  }

  char* paint_buffer_memory = GetSharedMemoryAs<char*>(
      raster_shm_id, raster_shm_offset, raster_shm_size);
  if (!paint_buffer_memory) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glRasterCHROMIUM",
                       "Can not read paint buffer.");
    return;
  }

  alignas(
      cc::PaintOpBuffer::PaintOpAlign) char data[sizeof(cc::LargestPaintOp)];

  SkCanvas* canvas = raster_canvas_.get();
  SkMatrix original_ctm;
  cc::PlaybackParams playback_params(nullptr, original_ctm);
  TransferCacheDeserializeHelperImpl impl(transfer_cache_.get());
  cc::PaintOp::DeserializeOptions options(&impl, font_manager_.strike_client());

  int op_idx = 0;
  size_t paint_buffer_size = raster_shm_size;
  while (paint_buffer_size > 4) {
    size_t skip = 0;
    cc::PaintOp* deserialized_op = cc::PaintOp::Deserialize(
        paint_buffer_memory, paint_buffer_size, &data[0],
        sizeof(cc::LargestPaintOp), &skip, options);
    if (!deserialized_op) {
      std::string msg =
          base::StringPrintf("RasterCHROMIUM: bad op: %i", op_idx);
      LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glRasterCHROMIUM", msg.c_str());
      return;
    }

    deserialized_op->Raster(canvas, playback_params);
    deserialized_op->DestroyThis();

    paint_buffer_size -= skip;
    paint_buffer_memory += skip;
    op_idx++;
  }
}

void RasterDecoderImpl::DoEndRasterCHROMIUM() {
  if (!sk_surface_) {
    LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBeginRasterCHROMIUM",
                       "EndRasterCHROMIUM without BeginRasterCHROMIUM");
    return;
  }

  need_context_state_reset_ = true;

  raster_canvas_.reset();
  sk_surface_->prepareForExternalIO();
  sk_surface_.reset();

  // Unlock all font handles. This needs to be deferred until
  // SkSurface::prepareForExternalIO since that flushes batched Gr operations in
  // skia that access the glyph data.
  // TODO(khushalsagar): We just unlocked a bunch of handles, do we need to give
  // a call to skia to attempt to purge any unlocked handles?
  if (!font_manager_.Unlock(locked_handles_)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glRasterCHROMIUM",
                       "Invalid font discardable handle.");
  }
  locked_handles_.clear();
}

void RasterDecoderImpl::DoCreateTransferCacheEntryINTERNAL(
    GLuint raw_entry_type,
    GLuint entry_id,
    GLuint handle_shm_id,
    GLuint handle_shm_offset,
    GLuint data_shm_id,
    GLuint data_shm_offset,
    GLuint data_size) {
  if (!supports_oop_raster_) {
    LOCAL_SET_GL_ERROR(
        GL_INVALID_VALUE, "glCreateTransferCacheEntryINTERNAL",
        "Attempt to use OOP transfer cache on a context without OOP raster.");
    return;
  }
  DCHECK(gr_context_);
  DCHECK(transfer_cache_);

  // Validate the type we are about to create.
  cc::TransferCacheEntryType entry_type;
  if (!cc::ServiceTransferCacheEntry::SafeConvertToType(raw_entry_type,
                                                        &entry_type)) {
    LOCAL_SET_GL_ERROR(
        GL_INVALID_VALUE, "glCreateTransferCacheEntryINTERNAL",
        "Attempt to use OOP transfer cache with an invalid cache entry type.");
    return;
  }

  uint8_t* data_memory =
      GetSharedMemoryAs<uint8_t*>(data_shm_id, data_shm_offset, data_size);
  if (!data_memory) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCreateTransferCacheEntryINTERNAL",
                       "Can not read transfer cache entry data.");
    return;
  }

  scoped_refptr<Buffer> handle_buffer = GetSharedMemoryBuffer(handle_shm_id);
  if (!DiscardableHandleBase::ValidateParameters(handle_buffer.get(),
                                                 handle_shm_offset)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCreateTransferCacheEntryINTERNAL",
                       "Invalid shm for discardable handle.");
    return;
  }
  ServiceDiscardableHandle handle(std::move(handle_buffer), handle_shm_offset,
                                  handle_shm_id);

  if (!transfer_cache_->CreateLockedEntry(
          entry_type, entry_id, handle, gr_context_.get(),
          base::make_span(data_memory, data_size))) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCreateTransferCacheEntryINTERNAL",
                       "Failure to deserialize transfer cache entry.");
    return;
  }
}

void RasterDecoderImpl::DoUnlockTransferCacheEntryINTERNAL(
    GLuint raw_entry_type,
    GLuint entry_id) {
  if (!supports_oop_raster_) {
    LOCAL_SET_GL_ERROR(
        GL_INVALID_VALUE, "glUnlockTransferCacheEntryINTERNAL",
        "Attempt to use OOP transfer cache on a context without OOP raster.");
    return;
  }
  DCHECK(transfer_cache_);
  cc::TransferCacheEntryType entry_type;
  if (!cc::ServiceTransferCacheEntry::SafeConvertToType(raw_entry_type,
                                                        &entry_type)) {
    LOCAL_SET_GL_ERROR(
        GL_INVALID_VALUE, "glUnlockTransferCacheEntryINTERNAL",
        "Attempt to use OOP transfer cache with an invalid cache entry type.");
    return;
  }

  if (!transfer_cache_->UnlockEntry(entry_type, entry_id)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glUnlockTransferCacheEntryINTERNAL",
                       "Attempt to unlock an invalid ID");
  }
}

void RasterDecoderImpl::DoDeleteTransferCacheEntryINTERNAL(
    GLuint raw_entry_type,
    GLuint entry_id) {
  if (!supports_oop_raster_) {
    LOCAL_SET_GL_ERROR(
        GL_INVALID_VALUE, "glDeleteTransferCacheEntryINTERNAL",
        "Attempt to use OOP transfer cache on a context without OOP raster.");
    return;
  }
  DCHECK(transfer_cache_);
  cc::TransferCacheEntryType entry_type;
  if (!cc::ServiceTransferCacheEntry::SafeConvertToType(raw_entry_type,
                                                        &entry_type)) {
    LOCAL_SET_GL_ERROR(
        GL_INVALID_VALUE, "glDeleteTransferCacheEntryINTERNAL",
        "Attempt to use OOP transfer cache with an invalid cache entry type.");
    return;
  }

  if (!transfer_cache_->DeleteEntry(entry_type, entry_id)) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glDeleteTransferCacheEntryINTERNAL",
                       "Attempt to delete an invalid ID");
  }
}

void RasterDecoderImpl::DoBindVertexArrayOES(GLuint client_id) {
  PessimisticallyResetGrContext();
  VertexAttribManager* vao = nullptr;
  if (client_id != 0) {
    vao = GetVertexAttribManager(client_id);
    if (!vao) {
      // Unlike most Bind* methods, the spec explicitly states that VertexArray
      // only allows names that have been previously generated. As such, we do
      // not generate new names here.
      LOCAL_SET_GL_ERROR(GL_INVALID_OPERATION, "glBindVertexArrayOES",
                         "bad vertex array id.");
      current_decoder_error_ = error::kNoError;
      return;
    }
  } else {
    vao = state_.default_vertex_attrib_manager.get();
  }

  // Only set the VAO state if it's changed
  if (state_.vertex_attrib_manager.get() != vao) {
    if (state_.vertex_attrib_manager)
      state_.vertex_attrib_manager->SetIsBound(false);
    state_.vertex_attrib_manager = vao;
    if (vao)
      vao->SetIsBound(true);
    if (!features().native_vertex_array_object) {
      EmulateVertexArrayState();
    } else {
      GLuint service_id = vao->service_id();
      api()->glBindVertexArrayOESFn(service_id);
    }
  }
}

// Used when OES_vertex_array_object isn't natively supported
void RasterDecoderImpl::EmulateVertexArrayState() {
  // Setup the Vertex attribute state
  for (uint32_t vv = 0; vv < group_->max_vertex_attribs(); ++vv) {
    RestoreStateForAttrib(vv, true);
  }

  // Setup the element buffer
  gles2::Buffer* element_array_buffer =
      state_.vertex_attrib_manager->element_array_buffer();
  api()->glBindBufferFn(
      GL_ELEMENT_ARRAY_BUFFER,
      element_array_buffer ? element_array_buffer->service_id() : 0);
}

void RasterDecoderImpl::RestoreStateForAttrib(GLuint attrib_index,
                                              bool restore_array_binding) {
  PessimisticallyResetGrContext();
  const VertexAttrib* attrib =
      state_.vertex_attrib_manager->GetVertexAttrib(attrib_index);
  if (restore_array_binding) {
    const void* ptr = reinterpret_cast<const void*>(attrib->offset());
    gles2::Buffer* buffer = attrib->buffer();
    api()->glBindBufferFn(GL_ARRAY_BUFFER, buffer ? buffer->service_id() : 0);
    api()->glVertexAttribPointerFn(attrib_index, attrib->size(), attrib->type(),
                                   attrib->normalized(), attrib->gl_stride(),
                                   ptr);
  }

  // Attrib divisors should only be non-zero when the ANGLE_instanced_arrays
  // extension is available
  DCHECK(attrib->divisor() == 0 ||
         feature_info_->feature_flags().angle_instanced_arrays);

  if (feature_info_->feature_flags().angle_instanced_arrays)
    api()->glVertexAttribDivisorANGLEFn(attrib_index, attrib->divisor());
  api()->glBindBufferFn(GL_ARRAY_BUFFER,
                        state_.bound_array_buffer.get()
                            ? state_.bound_array_buffer->service_id()
                            : 0);

  // Never touch vertex attribute 0's state (in particular, never disable it)
  // when running on desktop GL with compatibility profile because it will
  // never be re-enabled.
  if (attrib_index != 0 || gl_version_info().BehavesLikeGLES()) {
    // Restore the vertex attrib array enable-state according to
    // the VertexAttrib enabled_in_driver value (which really represents the
    // state of the virtual context - not the driver - notably, above the
    // vertex array object emulation layer).
    if (attrib->enabled_in_driver()) {
      api()->glEnableVertexAttribArrayFn(attrib_index);
    } else {
      api()->glDisableVertexAttribArrayFn(attrib_index);
    }
  }
}

// Include the auto-generated part of this file. We split this because it means
// we can easily edit the non-auto generated parts right here in this file
// instead of having to edit some template or the code generator.
#include "base/macros.h"
#include "gpu/command_buffer/service/raster_decoder_autogen.h"

}  // namespace raster
}  // namespace gpu
