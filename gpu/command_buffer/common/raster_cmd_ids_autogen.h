// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_raster_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

#ifndef GPU_COMMAND_BUFFER_COMMON_RASTER_CMD_IDS_AUTOGEN_H_
#define GPU_COMMAND_BUFFER_COMMON_RASTER_CMD_IDS_AUTOGEN_H_

#define RASTER_COMMAND_LIST(OP)                          \
  OP(DeleteTexturesImmediate)                  /* 256 */ \
  OP(Finish)                                   /* 257 */ \
  OP(Flush)                                    /* 258 */ \
  OP(GetError)                                 /* 259 */ \
  OP(GetIntegerv)                              /* 260 */ \
  OP(GenQueriesEXTImmediate)                   /* 261 */ \
  OP(DeleteQueriesEXTImmediate)                /* 262 */ \
  OP(BeginQueryEXT)                            /* 263 */ \
  OP(EndQueryEXT)                              /* 264 */ \
  OP(CompressedCopyTextureCHROMIUM)            /* 265 */ \
  OP(LoseContextCHROMIUM)                      /* 266 */ \
  OP(InsertFenceSyncCHROMIUM)                  /* 267 */ \
  OP(WaitSyncTokenCHROMIUM)                    /* 268 */ \
  OP(UnpremultiplyAndDitherCopyCHROMIUM)       /* 269 */ \
  OP(BeginRasterCHROMIUM)                      /* 270 */ \
  OP(RasterCHROMIUM)                           /* 271 */ \
  OP(EndRasterCHROMIUM)                        /* 272 */ \
  OP(CreateTransferCacheEntryINTERNAL)         /* 273 */ \
  OP(DeleteTransferCacheEntryINTERNAL)         /* 274 */ \
  OP(UnlockTransferCacheEntryINTERNAL)         /* 275 */ \
  OP(CreateTexture)                            /* 276 */ \
  OP(SetColorSpaceMetadata)                    /* 277 */ \
  OP(ProduceTextureDirectImmediate)            /* 278 */ \
  OP(CreateAndConsumeTextureINTERNALImmediate) /* 279 */ \
  OP(TexParameteri)                            /* 280 */ \
  OP(BindTexImage2DCHROMIUM)                   /* 281 */ \
  OP(ReleaseTexImage2DCHROMIUM)                /* 282 */ \
  OP(TexStorage2D)                             /* 283 */ \
  OP(CopySubTexture)                           /* 284 */ \
  OP(TraceBeginCHROMIUM)                       /* 285 */ \
  OP(TraceEndCHROMIUM)                         /* 286 */

enum CommandId {
  kOneBeforeStartPoint =
      cmd::kLastCommonId,  // All Raster commands start after this.
#define RASTER_CMD_OP(name) k##name,
  RASTER_COMMAND_LIST(RASTER_CMD_OP)
#undef RASTER_CMD_OP
      kNumCommands,
  kFirstRasterCommand = kOneBeforeStartPoint + 1
};

#endif  // GPU_COMMAND_BUFFER_COMMON_RASTER_CMD_IDS_AUTOGEN_H_
