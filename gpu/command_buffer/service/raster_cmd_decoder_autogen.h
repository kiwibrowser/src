// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_raster_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

// It is included by raster_cmd_decoder.cc
#ifndef GPU_COMMAND_BUFFER_SERVICE_RASTER_CMD_DECODER_AUTOGEN_H_
#define GPU_COMMAND_BUFFER_SERVICE_RASTER_CMD_DECODER_AUTOGEN_H_

error::Error RasterDecoderImpl::HandleActiveTexture(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::ActiveTexture& c =
      *static_cast<const volatile raster::cmds::ActiveTexture*>(cmd_data);
  GLenum texture = static_cast<GLenum>(c.texture);
  DoActiveTexture(texture);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleBindTexture(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::BindTexture& c =
      *static_cast<const volatile raster::cmds::BindTexture*>(cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLuint texture = c.texture;
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glBindTexture", target, "target");
    return error::kNoError;
  }
  DoBindTexture(target, texture);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleDeleteTexturesImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::DeleteTexturesImmediate& c =
      *static_cast<const volatile raster::cmds::DeleteTexturesImmediate*>(
          cmd_data);
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32_t data_size;
  if (!SafeMultiplyUint32(n, sizeof(GLuint), &data_size)) {
    return error::kOutOfBounds;
  }
  volatile const GLuint* textures = GetImmediateDataAs<volatile const GLuint*>(
      c, data_size, immediate_data_size);
  if (textures == NULL) {
    return error::kOutOfBounds;
  }
  DeleteTexturesHelper(n, textures);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleFinish(uint32_t immediate_data_size,
                                             const volatile void* cmd_data) {
  DoFinish();
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleFlush(uint32_t immediate_data_size,
                                            const volatile void* cmd_data) {
  DoFlush();
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleGenTexturesImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::GenTexturesImmediate& c =
      *static_cast<const volatile raster::cmds::GenTexturesImmediate*>(
          cmd_data);
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32_t data_size;
  if (!SafeMultiplyUint32(n, sizeof(GLuint), &data_size)) {
    return error::kOutOfBounds;
  }
  volatile GLuint* textures =
      GetImmediateDataAs<volatile GLuint*>(c, data_size, immediate_data_size);
  if (textures == NULL) {
    return error::kOutOfBounds;
  }
  auto textures_copy = std::make_unique<GLuint[]>(n);
  GLuint* textures_safe = textures_copy.get();
  std::copy(textures, textures + n, textures_safe);
  if (!CheckUniqueAndNonNullIds(n, textures_safe) ||
      !GenTexturesHelper(n, textures_safe)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleGetError(uint32_t immediate_data_size,
                                               const volatile void* cmd_data) {
  const volatile raster::cmds::GetError& c =
      *static_cast<const volatile raster::cmds::GetError*>(cmd_data);
  typedef cmds::GetError::Result Result;
  Result* result_dst = GetSharedMemoryAs<Result*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  if (!result_dst) {
    return error::kOutOfBounds;
  }
  *result_dst = GetErrorState()->GetGLError();
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleGetIntegerv(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::GetIntegerv& c =
      *static_cast<const volatile raster::cmds::GetIntegerv*>(cmd_data);
  GLenum pname = static_cast<GLenum>(c.pname);
  typedef cmds::GetIntegerv::Result Result;
  GLsizei num_values = 0;
  if (!GetNumValuesReturnedForGLGet(pname, &num_values)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM(":GetIntegerv", pname, "pname");
    return error::kNoError;
  }
  Result* result = GetSharedMemoryAs<Result*>(
      c.params_shm_id, c.params_shm_offset, Result::ComputeSize(num_values));
  GLint* params = result ? result->GetData() : NULL;
  if (!validators_->g_l_state.IsValid(pname)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glGetIntegerv", pname, "pname");
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  LOCAL_COPY_REAL_GL_ERRORS_TO_WRAPPER("GetIntegerv");
  // Check that the client initialized the result.
  if (result->size != 0) {
    return error::kInvalidArguments;
  }
  DoGetIntegerv(pname, params, num_values);
  GLenum error = LOCAL_PEEK_GL_ERROR("GetIntegerv");
  if (error == GL_NO_ERROR) {
    result->SetNumResults(num_values);
  }
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleTexParameteri(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::TexParameteri& c =
      *static_cast<const volatile raster::cmds::TexParameteri*>(cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint param = static_cast<GLint>(c.param);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexParameteri", target, "target");
    return error::kNoError;
  }
  if (!validators_->texture_parameter.IsValid(pname)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexParameteri", pname, "pname");
    return error::kNoError;
  }
  DoTexParameteri(target, pname, param);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleTexStorage2DEXT(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::TexStorage2DEXT& c =
      *static_cast<const volatile raster::cmds::TexStorage2DEXT*>(cmd_data);
  if (!features().ext_texture_storage) {
    return error::kUnknownCommand;
  }

  GLenum target = static_cast<GLenum>(c.target);
  GLsizei levels = static_cast<GLsizei>(c.levels);
  GLenum internalFormat = static_cast<GLenum>(c.internalFormat);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexStorage2DEXT", target, "target");
    return error::kNoError;
  }
  if (levels < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DEXT", "levels < 0");
    return error::kNoError;
  }
  if (!validators_->texture_internal_format_storage.IsValid(internalFormat)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexStorage2DEXT", internalFormat,
                                    "internalFormat");
    return error::kNoError;
  }
  if (width < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DEXT", "width < 0");
    return error::kNoError;
  }
  if (height < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DEXT", "height < 0");
    return error::kNoError;
  }
  DoTexStorage2DEXT(target, levels, internalFormat, width, height);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleGenQueriesEXTImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::GenQueriesEXTImmediate& c =
      *static_cast<const volatile raster::cmds::GenQueriesEXTImmediate*>(
          cmd_data);
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32_t data_size;
  if (!SafeMultiplyUint32(n, sizeof(GLuint), &data_size)) {
    return error::kOutOfBounds;
  }
  volatile GLuint* queries =
      GetImmediateDataAs<volatile GLuint*>(c, data_size, immediate_data_size);
  if (queries == NULL) {
    return error::kOutOfBounds;
  }
  auto queries_copy = std::make_unique<GLuint[]>(n);
  GLuint* queries_safe = queries_copy.get();
  std::copy(queries, queries + n, queries_safe);
  if (!CheckUniqueAndNonNullIds(n, queries_safe) ||
      !GenQueriesEXTHelper(n, queries_safe)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleDeleteQueriesEXTImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::DeleteQueriesEXTImmediate& c =
      *static_cast<const volatile raster::cmds::DeleteQueriesEXTImmediate*>(
          cmd_data);
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32_t data_size;
  if (!SafeMultiplyUint32(n, sizeof(GLuint), &data_size)) {
    return error::kOutOfBounds;
  }
  volatile const GLuint* queries = GetImmediateDataAs<volatile const GLuint*>(
      c, data_size, immediate_data_size);
  if (queries == NULL) {
    return error::kOutOfBounds;
  }
  DeleteQueriesEXTHelper(n, queries);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleCopySubTextureCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::CopySubTextureCHROMIUM& c =
      *static_cast<const volatile raster::cmds::CopySubTextureCHROMIUM*>(
          cmd_data);
  GLuint source_id = static_cast<GLuint>(c.source_id);
  GLint source_level = static_cast<GLint>(c.source_level);
  GLenum dest_target = static_cast<GLenum>(c.dest_target);
  GLuint dest_id = static_cast<GLuint>(c.dest_id);
  GLint dest_level = static_cast<GLint>(c.dest_level);
  GLint xoffset = static_cast<GLint>(c.xoffset);
  GLint yoffset = static_cast<GLint>(c.yoffset);
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLboolean unpack_flip_y = static_cast<GLboolean>(c.unpack_flip_y);
  GLboolean unpack_premultiply_alpha =
      static_cast<GLboolean>(c.unpack_premultiply_alpha);
  GLboolean unpack_unmultiply_alpha =
      static_cast<GLboolean>(c.unpack_unmultiply_alpha);
  if (!validators_->texture_target.IsValid(dest_target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glCopySubTextureCHROMIUM", dest_target,
                                    "dest_target");
    return error::kNoError;
  }
  if (width < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTextureCHROMIUM",
                       "width < 0");
    return error::kNoError;
  }
  if (height < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glCopySubTextureCHROMIUM",
                       "height < 0");
    return error::kNoError;
  }
  DoCopySubTextureCHROMIUM(source_id, source_level, dest_target, dest_id,
                           dest_level, xoffset, yoffset, x, y, width, height,
                           unpack_flip_y, unpack_premultiply_alpha,
                           unpack_unmultiply_alpha);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleCompressedCopyTextureCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::CompressedCopyTextureCHROMIUM& c =
      *static_cast<const volatile raster::cmds::CompressedCopyTextureCHROMIUM*>(
          cmd_data);
  GLuint source_id = static_cast<GLuint>(c.source_id);
  GLuint dest_id = static_cast<GLuint>(c.dest_id);
  DoCompressedCopyTextureCHROMIUM(source_id, dest_id);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleProduceTextureDirectCHROMIUMImmediate(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::ProduceTextureDirectCHROMIUMImmediate& c =
      *static_cast<
          const volatile raster::cmds::ProduceTextureDirectCHROMIUMImmediate*>(
          cmd_data);
  GLuint texture = c.texture;
  uint32_t data_size;
  if (!GLES2Util::ComputeDataSize<GLbyte, 16>(1, &data_size)) {
    return error::kOutOfBounds;
  }
  if (data_size > immediate_data_size) {
    return error::kOutOfBounds;
  }
  volatile const GLbyte* mailbox = GetImmediateDataAs<volatile const GLbyte*>(
      c, data_size, immediate_data_size);
  if (mailbox == NULL) {
    return error::kOutOfBounds;
  }
  DoProduceTextureDirectCHROMIUM(texture, mailbox);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleBindTexImage2DCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::BindTexImage2DCHROMIUM& c =
      *static_cast<const volatile raster::cmds::BindTexImage2DCHROMIUM*>(
          cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLint imageId = static_cast<GLint>(c.imageId);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glBindTexImage2DCHROMIUM", target,
                                    "target");
    return error::kNoError;
  }
  DoBindTexImage2DCHROMIUM(target, imageId);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleReleaseTexImage2DCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::ReleaseTexImage2DCHROMIUM& c =
      *static_cast<const volatile raster::cmds::ReleaseTexImage2DCHROMIUM*>(
          cmd_data);
  GLenum target = static_cast<GLenum>(c.target);
  GLint imageId = static_cast<GLint>(c.imageId);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glReleaseTexImage2DCHROMIUM", target,
                                    "target");
    return error::kNoError;
  }
  DoReleaseTexImage2DCHROMIUM(target, imageId);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleTraceEndCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  DoTraceEndCHROMIUM();
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleLoseContextCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::LoseContextCHROMIUM& c =
      *static_cast<const volatile raster::cmds::LoseContextCHROMIUM*>(cmd_data);
  GLenum current = static_cast<GLenum>(c.current);
  GLenum other = static_cast<GLenum>(c.other);
  if (!validators_->reset_status.IsValid(current)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glLoseContextCHROMIUM", current,
                                    "current");
    return error::kNoError;
  }
  if (!validators_->reset_status.IsValid(other)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glLoseContextCHROMIUM", other, "other");
    return error::kNoError;
  }
  DoLoseContextCHROMIUM(current, other);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleBeginRasterCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::BeginRasterCHROMIUM& c =
      *static_cast<const volatile raster::cmds::BeginRasterCHROMIUM*>(cmd_data);
  if (!features().chromium_raster_transport) {
    return error::kUnknownCommand;
  }

  GLuint texture_id = static_cast<GLuint>(c.texture_id);
  GLuint sk_color = static_cast<GLuint>(c.sk_color);
  GLuint msaa_sample_count = static_cast<GLuint>(c.msaa_sample_count);
  GLboolean can_use_lcd_text = static_cast<GLboolean>(c.can_use_lcd_text);
  GLboolean use_distance_field_text =
      static_cast<GLboolean>(c.use_distance_field_text);
  GLint color_type = static_cast<GLint>(c.color_type);
  DoBeginRasterCHROMIUM(texture_id, sk_color, msaa_sample_count,
                        can_use_lcd_text, use_distance_field_text, color_type);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleRasterCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::RasterCHROMIUM& c =
      *static_cast<const volatile raster::cmds::RasterCHROMIUM*>(cmd_data);
  if (!features().chromium_raster_transport) {
    return error::kUnknownCommand;
  }

  GLsizeiptr size = static_cast<GLsizeiptr>(c.size);
  uint32_t data_size = size;
  const void* list = GetSharedMemoryAs<const void*>(
      c.list_shm_id, c.list_shm_offset, data_size);
  if (size < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glRasterCHROMIUM", "size < 0");
    return error::kNoError;
  }
  if (list == NULL) {
    return error::kOutOfBounds;
  }
  DoRasterCHROMIUM(size, list);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleEndRasterCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  if (!features().chromium_raster_transport) {
    return error::kUnknownCommand;
  }

  DoEndRasterCHROMIUM();
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleCreateTransferCacheEntryINTERNAL(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::CreateTransferCacheEntryINTERNAL& c =
      *static_cast<
          const volatile raster::cmds::CreateTransferCacheEntryINTERNAL*>(
          cmd_data);
  GLuint entry_type = static_cast<GLuint>(c.entry_type);
  GLuint entry_id = static_cast<GLuint>(c.entry_id);
  GLuint handle_shm_id = static_cast<GLuint>(c.handle_shm_id);
  GLuint handle_shm_offset = static_cast<GLuint>(c.handle_shm_offset);
  GLuint data_shm_id = static_cast<GLuint>(c.data_shm_id);
  GLuint data_shm_offset = static_cast<GLuint>(c.data_shm_offset);
  GLuint data_size = static_cast<GLuint>(c.data_size);
  DoCreateTransferCacheEntryINTERNAL(entry_type, entry_id, handle_shm_id,
                                     handle_shm_offset, data_shm_id,
                                     data_shm_offset, data_size);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleDeleteTransferCacheEntryINTERNAL(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::DeleteTransferCacheEntryINTERNAL& c =
      *static_cast<
          const volatile raster::cmds::DeleteTransferCacheEntryINTERNAL*>(
          cmd_data);
  GLuint entry_type = static_cast<GLuint>(c.entry_type);
  GLuint entry_id = static_cast<GLuint>(c.entry_id);
  DoDeleteTransferCacheEntryINTERNAL(entry_type, entry_id);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleUnlockTransferCacheEntryINTERNAL(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::UnlockTransferCacheEntryINTERNAL& c =
      *static_cast<
          const volatile raster::cmds::UnlockTransferCacheEntryINTERNAL*>(
          cmd_data);
  GLuint entry_type = static_cast<GLuint>(c.entry_type);
  GLuint entry_id = static_cast<GLuint>(c.entry_id);
  DoUnlockTransferCacheEntryINTERNAL(entry_type, entry_id);
  return error::kNoError;
}

error::Error RasterDecoderImpl::HandleTexStorage2DImageCHROMIUM(
    uint32_t immediate_data_size,
    const volatile void* cmd_data) {
  const volatile raster::cmds::TexStorage2DImageCHROMIUM& c =
      *static_cast<const volatile raster::cmds::TexStorage2DImageCHROMIUM*>(
          cmd_data);
  if (!features().chromium_texture_storage_image) {
    return error::kUnknownCommand;
  }

  GLenum target = static_cast<GLenum>(c.target);
  GLenum internalFormat = static_cast<GLenum>(c.internalFormat);
  GLenum bufferUsage = static_cast<GLenum>(c.bufferUsage);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  if (!validators_->texture_bind_target.IsValid(target)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexStorage2DImageCHROMIUM", target,
                                    "target");
    return error::kNoError;
  }
  if (!validators_->texture_internal_format_storage.IsValid(internalFormat)) {
    LOCAL_SET_GL_ERROR_INVALID_ENUM("glTexStorage2DImageCHROMIUM",
                                    internalFormat, "internalFormat");
    return error::kNoError;
  }
  if (width < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DImageCHROMIUM",
                       "width < 0");
    return error::kNoError;
  }
  if (height < 0) {
    LOCAL_SET_GL_ERROR(GL_INVALID_VALUE, "glTexStorage2DImageCHROMIUM",
                       "height < 0");
    return error::kNoError;
  }
  DoTexStorage2DImageCHROMIUM(target, internalFormat, bufferUsage, width,
                              height);
  return error::kNoError;
}

#endif  // GPU_COMMAND_BUFFER_SERVICE_RASTER_CMD_DECODER_AUTOGEN_H_
