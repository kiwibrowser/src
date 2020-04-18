// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_PASSTHROUGH_PROGRAM_CACHE_H_
#define GPU_COMMAND_BUFFER_SERVICE_PASSTHROUGH_PROGRAM_CACHE_H_

#include "base/macros.h"
#include "gpu/command_buffer/service/decoder_context.h"
#include "gpu/command_buffer/service/program_cache.h"

namespace gpu {

namespace gles2 {

// Program cache that does not store binaries, but communicates with the
// underlying implementation via the cache control extension.
class GPU_GLES2_EXPORT PassthroughProgramCache : public ProgramCache {
 public:
  PassthroughProgramCache(size_t max_cache_size_bytes,
                          bool disable_gpu_shader_disk_cache);
  ~PassthroughProgramCache() override;

  ProgramLoadResult LoadLinkedProgram(
      GLuint program,
      Shader* shader_a,
      Shader* shader_b,
      const LocationMap* bind_attrib_location_map,
      const std::vector<std::string>& transform_feedback_varyings,
      GLenum transform_feedback_buffer_mode,
      DecoderClient* client) override;
  void SaveLinkedProgram(
      GLuint program,
      const Shader* shader_a,
      const Shader* shader_b,
      const LocationMap* bind_attrib_location_map,
      const std::vector<std::string>& transform_feedback_varyings,
      GLenum transform_feedback_buffer_mode,
      DecoderClient* client) override;

  void LoadProgram(const std::string& key, const std::string& program) override;

  size_t Trim(size_t limit) override;

 private:
  void ClearBackend() override;
  bool CacheEnabled() const;

  const bool disable_gpu_shader_disk_cache_;

  DISALLOW_COPY_AND_ASSIGN(PassthroughProgramCache);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_PASSTHROUGH_PROGRAM_CACHE_H_
