// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_FONT_CONFIG_IPC_LINUX_H_
#define CONTENT_COMMON_FONT_CONFIG_IPC_LINUX_H_

#include "base/compiler_specific.h"
#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "third_party/skia/include/ports/SkFontConfigInterface.h"

#include <stddef.h>

#include <string>

class SkString;

namespace content {

struct SkFontConfigInterfaceFontIdentityHash {
  std::size_t operator()(const SkFontConfigInterface::FontIdentity& sp) const;
};

// FontConfig implementation for Skia that proxies out of process to get out
// of the sandbox. See https://chromium.googlesource.com/chromium/src/+/master/docs/linux_sandbox_ipc.md
class FontConfigIPC : public SkFontConfigInterface {
 public:
  explicit FontConfigIPC(int fd);
  ~FontConfigIPC() override;

  bool matchFamilyName(const char familyName[],
                       SkFontStyle requested,
                       FontIdentity* outFontIdentifier,
                       SkString* outFamilyName,
                       SkFontStyle* outStyle) override;

  sk_sp<SkTypeface> makeTypeface(const FontIdentity& identity) override
      WARN_UNUSED_RESULT;

  enum Method {
    METHOD_MATCH = 0,
    METHOD_OPEN = 1,
  };

  enum {
    kMaxFontFamilyLength = 2048
  };

 private:
  // Marking this private in Blink's implementation of SkFontConfigInterface
  // since our caching implementation's efficacy is impaired if both
  // createTypeface and openStream are used in parallel.
  SkStreamAsset* openStream(const FontIdentity&) override;

  SkMemoryStream* mapFileDescriptorToStream(int fd);

  const int fd_;
  // Lock preventing multiple threads from creating a typeface and removing
  // an element from |mapped_typefaces_| map at the same time.
  base::Lock lock_;
  // Practically, this hash_map definition means that we re-map the same font
  // file multiple times if we receive createTypeface requests for multiple
  // ttc-indices or styles but the same fontconfig interface id. Since the usage
  // frequency of ttc indices is very low, and style is not used by clients of
  // this API, this seems okay.
  base::HashingMRUCache<FontIdentity,
                        sk_sp<SkTypeface>,
                        SkFontConfigInterfaceFontIdentityHash>
      mapped_typefaces_;

  DISALLOW_COPY_AND_ASSIGN(FontConfigIPC);
};

}  // namespace content

#endif  // CONTENT_COMMON_FONT_CONFIG_IPC_LINUX_H_
