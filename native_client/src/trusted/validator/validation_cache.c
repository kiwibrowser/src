/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/validation_cache.h"

#include <string.h>
#include <sys/stat.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/trusted/desc/desc_metadata_types.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/validator/rich_file_info.h"
#include "native_client/src/trusted/validator/validation_cache_internal.h"
#include "native_client/src/trusted/validator/validation_metadata.h"

#if NACL_WINDOWS
#include <Windows.h>
#include <io.h>
#endif

#define ADD_LITERAL(cache, query, data) \
  ((cache)->AddData((query), (uint8_t*)&(data), sizeof(data)))

int NaClCachingIsInexpensive(struct NaClValidationCache *cache,
                             const struct NaClValidationMetadata *metadata) {
  if (cache->CachingIsInexpensive != NULL) {
    return cache->CachingIsInexpensive(metadata);
  } else {
    return NULL != metadata && metadata->identity_type == NaClCodeIdentityFile;
  }
}

void NaClMetadataFromFDCtor(struct NaClValidationMetadata *metadata,
                            int file_desc,
                            const char* file_name,
                            size_t file_name_length) {
  struct NaClHostDesc wrapper;
  nacl_host_stat_t stat;
#if NACL_WINDOWS
  BY_HANDLE_FILE_INFORMATION file_info;
#endif

  memset(metadata, 0, sizeof(*metadata));
  /* If we early out, identity_type will be 0 / NaClCodeIdentityData. */

  wrapper.d = file_desc;
  if(NaClHostDescFstat(&wrapper, &stat))
    return;

#if NACL_WINDOWS
  /*
   * This will not get us the complete file ID on ReFS, but doing the correct
   * thing (calling GetFileInformationByHandleEx) causes linkage issues on
   * Windows XP.  We aren't relying on the file ID for security, just collision
   * resistance, so we don't need all of it.
   * In many cases (including on NTFS) we're also getting the 32 least
   * significant bits of a 64-bit volume serial number - but again, since it's
   * random we can live with it.
   */
  if (!GetFileInformationByHandle((HANDLE) _get_osfhandle(file_desc),
                                  &file_info))
    return;
  metadata->device_id = file_info.dwVolumeSerialNumber;
  metadata->file_id = ((((uint64_t)file_info.nFileIndexHigh) << 32) |
                       file_info.nFileIndexLow);
#else
  /* st_dev is not actually a property of the device, so skip it. */
  metadata->file_id = stat.st_ino;
#endif

  metadata->file_size = stat.st_size;
  metadata->mtime = stat.st_mtime;
  metadata->ctime = stat.st_ctime;

  CHECK(0 < file_name_length);
  metadata->file_name = malloc(file_name_length);
  CHECK(NULL != metadata->file_name);
  memcpy(metadata->file_name, file_name, file_name_length);
  metadata->file_name_length = file_name_length;

  /* We have all the identity information we need. */
  metadata->identity_type = NaClCodeIdentityFile;
}

void NaClMetadataDtor(struct NaClValidationMetadata *metadata) {
  free(metadata->file_name);
  /* Prevent use after free. */
  memset(metadata, 0, sizeof(*metadata));
}

static void Serialize(uint8_t *buffer, const void *value, size_t size,
                      uint32_t *offset) {
  if (buffer != NULL)
    memcpy(&buffer[*offset], value, size);
  *offset += (uint32_t) size;
}

static void SerializeNaClDescMetadataInternal(
    const struct NaClRichFileInfo *info,
    uint8_t *buffer,
    uint32_t *offset) {
  *offset = 0;
  Serialize(buffer, &info->known_file, sizeof(info->known_file), offset);
  if (info->known_file) {
    Serialize(buffer, &info->file_path_length, sizeof(info->file_path_length),
              offset);
    Serialize(buffer, info->file_path, info->file_path_length, offset);
  }
}

int NaClSerializeNaClDescMetadata(
    const struct NaClRichFileInfo *info,
    uint8_t **buffer,
    uint32_t *buffer_length) {

  *buffer = NULL;

  /* Calculate the buffer size. */
  SerializeNaClDescMetadataInternal(info, NULL, buffer_length);

  /* Allocate the buffer. */
  *buffer = malloc(*buffer_length);
  if (NULL == *buffer)
    return 1;

  /* Fill the buffer. */
  SerializeNaClDescMetadataInternal(info, *buffer, buffer_length);
  return 0;
}

int NaClSetFileOriginInfo(struct NaClDesc *desc,
                          struct NaClRichFileInfo *info) {
  uint8_t *buffer = NULL;
  uint32_t buffer_length = 0;
  int status;
  if (NaClSerializeNaClDescMetadata(info, &buffer, &buffer_length)) {
    return 1;
  }
  status = NACL_VTBL(NaClDesc, desc)->SetMetadata(
      desc,
      NACL_DESC_METADATA_ORIGIN_INFO_TYPE,
      buffer_length,
      (uint8_t *) buffer);
  free(buffer);
  return status;
}

static int Deserialize(const uint8_t *buffer, uint32_t buffer_length,
                       void *value, size_t size, uint32_t *offset) {
  if (*offset + size > buffer_length)
    return 1;
  memcpy(value, &buffer[*offset], size);
  *offset += (uint32_t) size;
  return 0;
}

int NaClDeserializeNaClDescMetadata(
    const uint8_t *buffer,
    uint32_t buffer_length,
    struct NaClRichFileInfo *info) {
  /* Work around const issues. */
  char *file_path = NULL;
  uint32_t offset = 0;
  NaClRichFileInfoCtor(info);

  if (Deserialize(buffer, buffer_length, &info->known_file,
                  sizeof(info->known_file), &offset))
    goto on_error;

  if (info->known_file) {
    if (Deserialize(buffer, buffer_length, &info->file_path_length,
                    sizeof(info->file_path_length), &offset))
      goto on_error;
    file_path = malloc(info->file_path_length);
    if (NULL == file_path)
      goto on_error;
    if (Deserialize(buffer, buffer_length, file_path, info->file_path_length,
                    &offset))
      goto on_error;
    info->file_path = file_path;
    file_path = NULL;
  }

  /* Entire buffer consumed? */
  if (offset != buffer_length)
    goto on_error;
  return 0;

 on_error:
  free(file_path);
  NaClRichFileInfoDtor(info);
  return 1;
}

void NaClRichFileInfoCtor(struct NaClRichFileInfo *info) {
  memset(info, 0, sizeof(*info));
}

void NaClRichFileInfoDtor(struct NaClRichFileInfo *info) {
  /*
   * file_path is "const" to express intent, we need to cast away the const to
   * dallocate it.
   */
  free((void *) info->file_path);
  /* Prevent use after Dtor. */
  memset(info, 0, sizeof(*info));
}

int NaClGetFileOriginInfo(struct NaClDesc *desc,
                          struct NaClRichFileInfo *info) {
  int32_t metadata_type;
  uint8_t *buffer = NULL;
  uint32_t buffer_length = 0;
  int status;

  /* Get the buffer length. */
  metadata_type = NACL_VTBL(NaClDesc, desc)->GetMetadata(
      desc,
      &buffer_length,
      NULL);
  if (metadata_type != NACL_DESC_METADATA_ORIGIN_INFO_TYPE)
    return 1;

  buffer = (uint8_t *) malloc(buffer_length);
  if (NULL == buffer)
    return 1;

  metadata_type = NACL_VTBL(NaClDesc, desc)->GetMetadata(
      desc,
      &buffer_length,
      buffer);
  if (metadata_type != NACL_DESC_METADATA_ORIGIN_INFO_TYPE)
    return 1;

  status = NaClDeserializeNaClDescMetadata(buffer, buffer_length, info);
  free(buffer);
  return status;
}

void NaClMetadataFromNaClDescCtor(struct NaClValidationMetadata *metadata,
                                  struct NaClDesc *desc) {
  struct NaClRichFileInfo info;
  int32_t fd = -1;

  NaClRichFileInfoCtor(&info);
  memset(metadata, 0, sizeof(*metadata));

  if (NACL_VTBL(NaClDesc, desc)->typeTag != NACL_DESC_HOST_IO)
    goto done;
  fd = ((struct NaClDescIoDesc *) desc)->hd->d;
  if (NaClGetFileOriginInfo(desc, &info))
    goto done;
  if (!info.known_file || info.file_path == NULL || info.file_path_length <= 0)
    goto done;
  NaClMetadataFromFDCtor(metadata, fd, info.file_path, info.file_path_length);
 done:
  NaClRichFileInfoDtor(&info);
}

void NaClAddCodeIdentity(uint8_t *data,
                         size_t size,
                         const struct NaClValidationMetadata *metadata,
                         struct NaClValidationCache *cache,
                         void *query) {
  NaClCodeIdentityType identity_type;
  if (NULL != metadata) {
    identity_type = metadata->identity_type;
  } else {
    /* Fallback: identity unknown, treat it as anonymous data. */
    identity_type = NaClCodeIdentityData;
  }
  CHECK(identity_type < NaClCodeIdentityMax);

  /*
   * Explicitly record the type of identity being used to prevent attacks
   * that confuse the payload of different identity types.
   */
  ADD_LITERAL(cache, query, identity_type);

  if (identity_type == NaClCodeIdentityFile) {
    /* Sanity checks. */
    CHECK(metadata->file_name);
    CHECK(metadata->file_name_length);
    CHECK(metadata->code_offset + (int64_t) size <= metadata->file_size);

    /* The location of the code in the file. */
    ADD_LITERAL(cache, query, metadata->code_offset);
    ADD_LITERAL(cache, query, size);

    /* The identity of the file. */
    ADD_LITERAL(cache, query, metadata->file_name_length);
    cache->AddData(query, (uint8_t *) metadata->file_name,
                   metadata->file_name_length);
    ADD_LITERAL(cache, query, metadata->device_id);
    ADD_LITERAL(cache, query, metadata->file_id);
    ADD_LITERAL(cache, query, metadata->file_size);
    ADD_LITERAL(cache, query, metadata->mtime);
    ADD_LITERAL(cache, query, metadata->ctime);
  } else {
    /* Hash all the code. */
    cache->AddData(query, data, size);
  }
}

struct NaClDesc *NaClDescCreateWithFilePathMetadata(NaClHandle handle,
                                                    const char *file_path) {
  struct NaClDesc *desc = NaClDescIoMakeFromHandle(handle, NACL_ABI_O_RDONLY);
  char *alloc_file_path;
  size_t file_path_length = strlen(file_path);

  struct NaClRichFileInfo info;

  if (desc == NULL)
    return NULL;

  /*
   * If there is no file path metadata, just return the created NaClDesc
   * without adding rich file info.
   */
  if (file_path_length == 0)
    return desc;

  /* Mark the desc as OK for mmapping. */
  NaClDescMarkSafeForMmap(desc);

  alloc_file_path = (char *) malloc(file_path_length + 1);
  if (alloc_file_path == NULL) {
    NaClDescUnref(desc);
    return NULL;
  }
  memcpy(alloc_file_path, file_path, file_path_length + 1);

  /* Provide metadata for validation. */
  NaClRichFileInfoCtor(&info);
  info.known_file = 1;
  info.file_path = alloc_file_path;  /* Takes ownership. */
  info.file_path_length = (uint32_t) file_path_length;
  NaClSetFileOriginInfo(desc, &info);
  NaClRichFileInfoDtor(&info);
  return desc;
}
