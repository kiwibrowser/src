// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/font_config_ipc_linux.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/memory_mapped_file.h"
#include "base/memory/ref_counted.h"
#include "base/pickle.h"
#include "base/posix/unix_domain_socket.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_event.h"
#include "skia/ext/skia_utils_base.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace content {

std::size_t SkFontConfigInterfaceFontIdentityHash::operator()(
    const SkFontConfigInterface::FontIdentity& sp) const {
  std::hash<std::string> stringhash;
  std::hash<int> inthash;
  size_t r = inthash(sp.fID);
  r = r * 41 + inthash(sp.fTTCIndex);
  r = r * 41 + stringhash(sp.fString.c_str());
  r = r * 41 + inthash(sp.fStyle.weight());
  r = r * 41 + inthash(sp.fStyle.slant());
  r = r * 41 + inthash(sp.fStyle.width());
  return r;
}

// Wikpedia's main country selection page activates 21 fallback fonts,
// doubling this we should be on the generous side as an upper bound,
// but nevertheless not have the mapped typefaces cache grow excessively.
const size_t kMaxMappedTypefaces = 42;

void CloseFD(int fd) {
  int err = IGNORE_EINTR(close(fd));
  DCHECK(!err);
}

FontConfigIPC::FontConfigIPC(int fd)
    : fd_(fd)
    , mapped_typefaces_(kMaxMappedTypefaces) {
}

FontConfigIPC::~FontConfigIPC() {
  CloseFD(fd_);
}

bool FontConfigIPC::matchFamilyName(const char familyName[],
                                    SkFontStyle requestedStyle,
                                    FontIdentity* outFontIdentity,
                                    SkString* outFamilyName,
                                    SkFontStyle* outStyle) {
  TRACE_EVENT0("sandbox_ipc", "FontConfigIPC::matchFamilyName");
  size_t familyNameLen = familyName ? strlen(familyName) : 0;
  if (familyNameLen > kMaxFontFamilyLength)
    return false;

  base::Pickle request;
  request.WriteInt(METHOD_MATCH);
  request.WriteData(familyName, familyNameLen);
  skia::WriteSkFontStyle(&request, requestedStyle);

  uint8_t reply_buf[2048];
  const ssize_t r = base::UnixDomainSocket::SendRecvMsg(
      fd_, reply_buf, sizeof(reply_buf), nullptr, request);
  if (r == -1)
    return false;

  base::Pickle reply(reinterpret_cast<char*>(reply_buf), r);
  base::PickleIterator iter(reply);
  bool result;
  if (!iter.ReadBool(&result))
    return false;
  if (!result)
    return false;

  SkString     reply_family;
  FontIdentity reply_identity;
  SkFontStyle  reply_style;
  if (!skia::ReadSkString(&iter, &reply_family) ||
      !skia::ReadSkFontIdentity(&iter, &reply_identity) ||
      !skia::ReadSkFontStyle(&iter, &reply_style)) {
    return false;
  }

  if (outFontIdentity)
    *outFontIdentity = reply_identity;
  if (outFamilyName)
    *outFamilyName = reply_family;
  if (outStyle)
    *outStyle = reply_style;

  return true;
}

static void DestroyMemoryMappedFile(const void*, void* context) {
  base::ThreadRestrictions::ScopedAllowIO allow_munmap;
  delete static_cast<base::MemoryMappedFile*>(context);
}

SkMemoryStream* FontConfigIPC::mapFileDescriptorToStream(int fd) {
  std::unique_ptr<base::MemoryMappedFile> mapped_font_file(
      new base::MemoryMappedFile);
  base::ThreadRestrictions::ScopedAllowIO allow_mmap;
  mapped_font_file->Initialize(base::File(fd));
  DCHECK(mapped_font_file->IsValid());

  sk_sp<SkData> data =
      SkData::MakeWithProc(mapped_font_file->data(), mapped_font_file->length(),
                           &DestroyMemoryMappedFile, mapped_font_file.get());
  if (!data)
    return nullptr;
  ignore_result(mapped_font_file.release()); // Ownership transferred to SkDataB
  return new SkMemoryStream(std::move(data));
}

SkStreamAsset* FontConfigIPC::openStream(const FontIdentity& identity) {
  TRACE_EVENT0("sandbox_ipc", "FontConfigIPC::openStream");

  base::Pickle request;
  request.WriteInt(METHOD_OPEN);
  request.WriteUInt32(identity.fID);

  int result_fd = -1;
  uint8_t reply_buf[256];
  const ssize_t r = base::UnixDomainSocket::SendRecvMsg(
      fd_, reply_buf, sizeof(reply_buf), &result_fd, request);
  if (r == -1)
    return nullptr;

  base::Pickle reply(reinterpret_cast<char*>(reply_buf), r);
  bool result;
  base::PickleIterator iter(reply);
  if (!iter.ReadBool(&result) || !result) {
    if (result_fd)
      CloseFD(result_fd);
    return nullptr;
  }

  return mapFileDescriptorToStream(result_fd);
}

sk_sp<SkTypeface> FontConfigIPC::makeTypeface(
    const SkFontConfigInterface::FontIdentity& identity) {
  base::AutoLock lock(lock_);
  auto mapped_typefaces_it = mapped_typefaces_.Get(identity);
  if (mapped_typefaces_it != mapped_typefaces_.end())
    return mapped_typefaces_it->second;

  SkStreamAsset* typeface_stream = openStream(identity);
  if (!typeface_stream)
    return nullptr;
  sk_sp<SkTypeface> typeface_from_stream(
      SkTypeface::MakeFromStream(typeface_stream, identity.fTTCIndex));
  auto mapped_typefaces_insert_it =
      mapped_typefaces_.Put(identity, std::move(typeface_from_stream));
  return mapped_typefaces_insert_it->second;
}

}  // namespace content
