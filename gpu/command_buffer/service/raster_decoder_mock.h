// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the mock RasterDecoder class.

#ifndef GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_MOCK_H_
#define GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_MOCK_H_

#include <stdint.h>

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "gpu/command_buffer/common/context_creation_attribs.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/service/raster_decoder.h"
#include "gpu/command_buffer/service/shader_translator.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/gfx/geometry/size.h"

namespace gl {
class GLContext;
class GLSurface;
}  // namespace gl

namespace gpu {
class QueryManager;

namespace gles2 {
class ContextGroup;
class ErrorState;
class FeatureInfo;
class GpuFenceManager;
class GLES2Util;
class ImageManager;
class Logger;
class Texture;
struct ContextState;
}  // namespace gles2

namespace raster {

class MockRasterDecoder : public RasterDecoder {
 public:
  explicit MockRasterDecoder(CommandBufferServiceBase* command_buffer_service);
  ~MockRasterDecoder() override;

  base::WeakPtr<DecoderContext> AsWeakPtr() override;

  MOCK_METHOD5(
      Initialize,
      gpu::ContextResult(const scoped_refptr<gl::GLSurface>& surface,
                         const scoped_refptr<gl::GLContext>& context,
                         bool offscreen,
                         const gles2::DisallowedFeatures& disallowed_features,
                         const ContextCreationAttribs& attrib_helper));
  MOCK_METHOD1(Destroy, void(bool have_context));
  MOCK_METHOD0(MakeCurrent, bool());
  MOCK_METHOD1(GetServiceIdForTesting, uint32_t(uint32_t client_id));
  MOCK_METHOD0(GetGLES2Util, gles2::GLES2Util*());
  MOCK_METHOD0(GetGLSurface, gl::GLSurface*());
  MOCK_METHOD0(GetGLContext, gl::GLContext*());
  MOCK_METHOD0(GetContextGroup, gles2::ContextGroup*());
  MOCK_CONST_METHOD0(GetFeatureInfo, const gles2::FeatureInfo*());
  MOCK_METHOD0(GetContextState, const gles2::ContextState*());
  MOCK_METHOD0(GetCapabilities, Capabilities());
  MOCK_CONST_METHOD0(HasPendingQueries, bool());
  MOCK_METHOD1(ProcessPendingQueries, void(bool));
  MOCK_CONST_METHOD0(HasMoreIdleWork, bool());
  MOCK_METHOD0(PerformIdleWork, void());
  MOCK_CONST_METHOD0(HasPollingWork, bool());
  MOCK_METHOD0(PerformPollingWork, void());
  MOCK_CONST_METHOD0(RestoreGlobalState, void());
  MOCK_CONST_METHOD0(ClearAllAttributes, void());
  MOCK_CONST_METHOD0(RestoreAllAttributes, void());
  MOCK_METHOD1(RestoreState, void(const gles2::ContextState* prev_state));
  MOCK_CONST_METHOD0(RestoreActiveTexture, void());
  MOCK_CONST_METHOD1(RestoreAllTextureUnitAndSamplerBindings,
                     void(const gles2::ContextState* state));
  MOCK_CONST_METHOD1(RestoreActiveTextureUnitBinding,
                     void(unsigned int target));
  MOCK_METHOD0(RestoreAllExternalTextureBindingsIfNeeded, void());
  MOCK_METHOD1(RestoreBufferBinding, void(unsigned int target));
  MOCK_CONST_METHOD0(RestoreBufferBindings, void());
  MOCK_CONST_METHOD0(RestoreFramebufferBindings, void());
  MOCK_CONST_METHOD0(RestoreProgramBindings, void());
  MOCK_METHOD0(RestoreRenderbufferBindings, void());
  MOCK_CONST_METHOD1(RestoreTextureState, void(unsigned service_id));
  MOCK_CONST_METHOD1(RestoreTextureUnitBindings, void(unsigned unit));
  MOCK_METHOD1(RestoreVertexAttribArray, void(unsigned index));

  MOCK_METHOD0(GetQueryManager, QueryManager*());
  MOCK_METHOD2(SetQueryCallback, void(unsigned int, base::OnceClosure));
  MOCK_METHOD0(GetGpuFenceManager, gpu::gles2::GpuFenceManager*());
  MOCK_METHOD1(SetIgnoreCachedStateForTest, void(bool ignore));
  MOCK_METHOD0(GetImageManagerForTest, gles2::ImageManager*());
  MOCK_METHOD0(GetTransferCacheForTest, ServiceTransferCache*());
  MOCK_METHOD4(DoCommands,
               error::Error(unsigned int num_commands,
                            const volatile void* buffer,
                            int num_entries,
                            int* entries_processed));
  MOCK_METHOD2(GetServiceTextureId,
               bool(uint32_t client_texture_id, uint32_t* service_texture_id));
  MOCK_METHOD0(GetErrorState, gles2::ErrorState*());

  MOCK_METHOD0(GetLogger, gles2::Logger*());
  MOCK_CONST_METHOD0(WasContextLost, bool());
  MOCK_CONST_METHOD0(WasContextLostByRobustnessExtension, bool());
  MOCK_METHOD1(MarkContextLost, void(gpu::error::ContextLostReason reason));
  MOCK_METHOD0(CheckResetStatus, bool());
  MOCK_METHOD4(BindImage,
               void(uint32_t client_texture_id,
                    uint32_t texture_target,
                    gl::GLImage* image,
                    bool can_bind_to_sampler));
  MOCK_METHOD1(IsCompressedTextureFormat, bool(unsigned format));
  MOCK_METHOD9(ClearLevel,
               bool(gles2::Texture* texture,
                    unsigned target,
                    int level,
                    unsigned format,
                    unsigned type,
                    int xoffset,
                    int yoffset,
                    int width,
                    int height));
  MOCK_METHOD6(ClearCompressedTextureLevel,
               bool(gles2::Texture* texture,
                    unsigned target,
                    int level,
                    unsigned format,
                    int width,
                    int height));
  MOCK_METHOD8(ClearLevel3D,
               bool(gles2::Texture* texture,
                    unsigned target,
                    int level,
                    unsigned format,
                    unsigned type,
                    int width,
                    int height,
                    int depth));
  MOCK_METHOD1(SetCopyTextureResourceManagerForTest,
               void(gles2::CopyTextureCHROMIUMResourceManager*
                        copy_texture_resource_manager));

 private:
  base::WeakPtrFactory<MockRasterDecoder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MockRasterDecoder);
};

}  // namespace raster
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_RASTER_DECODER_MOCK_H_
