// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/child_process_sandbox_support_impl_linux.h"

#include <stddef.h>
#include <sys/stat.h>

#include <limits>
#include <memory>

#include "base/pickle.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/unix_domain_socket.h"
#include "base/sys_byteorder.h"
#include "base/trace_event/trace_event.h"
#include "services/service_manager/sandbox/linux/sandbox_linux.h"
#include "services/service_manager/zygote/common/common_sandbox_support_linux.h"
#include "third_party/blink/public/platform/linux/web_fallback_font.h"
#include "third_party/blink/public/platform/web_font_render_style.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace content {

void GetFallbackFontForCharacter(int32_t character,
                                 const char* preferred_locale,
                                 blink::WebFallbackFont* fallbackFont) {
  TRACE_EVENT0("sandbox_ipc", "GetFontFamilyForCharacter");

  base::Pickle request;
  request.WriteInt(
      service_manager::SandboxLinux::METHOD_GET_FALLBACK_FONT_FOR_CHAR);
  request.WriteInt(character);
  request.WriteString(preferred_locale);

  uint8_t buf[512];
  const ssize_t n = base::UnixDomainSocket::SendRecvMsg(
      service_manager::GetSandboxFD(), buf, sizeof(buf), nullptr, request);

  std::string family_name;
  std::string filename;
  int fontconfigInterfaceId = 0;
  int ttcIndex = 0;
  bool isBold = false;
  bool isItalic = false;
  if (n != -1) {
    base::Pickle reply(reinterpret_cast<char*>(buf), n);
    base::PickleIterator pickle_iter(reply);
    if (pickle_iter.ReadString(&family_name) &&
        pickle_iter.ReadString(&filename) &&
        pickle_iter.ReadInt(&fontconfigInterfaceId) &&
        pickle_iter.ReadInt(&ttcIndex) && pickle_iter.ReadBool(&isBold) &&
        pickle_iter.ReadBool(&isItalic)) {
      fallbackFont->name = blink::WebString::FromUTF8(family_name);
      fallbackFont->filename = blink::WebVector<char>(filename);
      fallbackFont->fontconfig_interface_id = fontconfigInterfaceId;
      fallbackFont->ttc_index = ttcIndex;
      fallbackFont->is_bold = isBold;
      fallbackFont->is_italic = isItalic;
    }
  }
}

void GetRenderStyleForStrike(const char* family,
                             int size_and_style,
                             blink::WebFontRenderStyle* out) {
  TRACE_EVENT0("sandbox_ipc", "GetRenderStyleForStrike");

  *out = blink::WebFontRenderStyle();

  if (size_and_style < 0)
    return;

  const bool bold = size_and_style & 1;
  const bool italic = size_and_style & 2;
  const int pixel_size = size_and_style >> 2;
  if (pixel_size > std::numeric_limits<uint16_t>::max())
    return;

  base::Pickle request;
  request.WriteInt(service_manager::SandboxLinux::METHOD_GET_STYLE_FOR_STRIKE);
  request.WriteString(family);
  request.WriteBool(bold);
  request.WriteBool(italic);
  request.WriteUInt16(pixel_size);

  uint8_t buf[512];
  const ssize_t n = base::UnixDomainSocket::SendRecvMsg(
      service_manager::GetSandboxFD(), buf, sizeof(buf), nullptr, request);
  if (n == -1)
    return;

  base::Pickle reply(reinterpret_cast<char*>(buf), n);
  base::PickleIterator pickle_iter(reply);
  int use_bitmaps, use_autohint, use_hinting, hint_style, use_antialias;
  int use_subpixel_rendering, use_subpixel_positioning;
  if (pickle_iter.ReadInt(&use_bitmaps) && pickle_iter.ReadInt(&use_autohint) &&
      pickle_iter.ReadInt(&use_hinting) && pickle_iter.ReadInt(&hint_style) &&
      pickle_iter.ReadInt(&use_antialias) &&
      pickle_iter.ReadInt(&use_subpixel_rendering) &&
      pickle_iter.ReadInt(&use_subpixel_positioning)) {
    out->use_bitmaps = use_bitmaps;
    out->use_auto_hint = use_autohint;
    out->use_hinting = use_hinting;
    out->hint_style = hint_style;
    out->use_anti_alias = use_antialias;
    out->use_subpixel_rendering = use_subpixel_rendering;
    out->use_subpixel_positioning = use_subpixel_positioning;
  }
}

int MatchFontWithFallback(const std::string& face,
                          bool bold,
                          bool italic,
                          int charset,
                          PP_BrowserFont_Trusted_Family fallback_family) {
  TRACE_EVENT0("sandbox_ipc", "MatchFontWithFallback");

  base::Pickle request;
  request.WriteInt(service_manager::SandboxLinux::METHOD_MATCH_WITH_FALLBACK);
  request.WriteString(face);
  request.WriteBool(bold);
  request.WriteBool(italic);
  request.WriteUInt32(charset);
  request.WriteUInt32(fallback_family);
  uint8_t reply_buf[64];
  int fd = -1;
  base::UnixDomainSocket::SendRecvMsg(service_manager::GetSandboxFD(),
                                      reply_buf, sizeof(reply_buf), &fd,
                                      request);
  return fd;
}

}  // namespace content
