// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DESKTOP_MEDIA_ID_H_
#define CONTENT_PUBLIC_BROWSER_DESKTOP_MEDIA_ID_H_

#include <string>
#include <tuple>

#include "content/common/content_export.h"
#include "content/public/browser/web_contents_media_capture_id.h"

#if defined(USE_AURA)
namespace aura {
class Window;
}  // namespace aura
#endif  // defined(USE_AURA)

namespace content {

// Type used to identify desktop media sources. It's converted to string and
// stored in MediaStreamRequest::requested_video_device_id.
struct CONTENT_EXPORT DesktopMediaID {
 public:
  enum Type { TYPE_NONE, TYPE_SCREEN, TYPE_WINDOW, TYPE_WEB_CONTENTS };

  typedef intptr_t Id;

  // Represents an "unset" value for either |id| or |aura_id|.
  static const Id kNullId = 0;
  // Represents a fake id to create a dummy capturer for autotests.
  static const Id kFakeId = -3;

#if defined(USE_AURA)
  // Assigns integer identifier to the |window| and returns its DesktopMediaID.
  static DesktopMediaID RegisterAuraWindow(Type type, aura::Window* window);

  // Returns the Window that was previously registered using
  // RegisterAuraWindow(), else nullptr.
  static aura::Window* GetAuraWindowById(const DesktopMediaID& id);
#endif  // defined(USE_AURA)

  DesktopMediaID() = default;

  DesktopMediaID(Type type, Id id) : type(type), id(id) {}

  DesktopMediaID(Type type, Id id, WebContentsMediaCaptureId web_contents_id)
      : type(type), id(id), web_contents_id(web_contents_id) {}

  DesktopMediaID(Type type, Id id, bool audio_share)
      : type(type), id(id), audio_share(audio_share) {}

  // Operators so that DesktopMediaID can be used with STL containers.
  bool operator<(const DesktopMediaID& other) const;
  bool operator==(const DesktopMediaID& other) const;

  bool is_null() const { return type == TYPE_NONE; }
  std::string ToString() const;
  static DesktopMediaID Parse(const std::string& str);

  Type type = TYPE_NONE;

  // The IDs referring to the target native screen or window.  |id| will be
  // non-null if and only if it refers to a native screen/window.  |aura_id|
  // will be non-null if and only if it refers to an Aura window.  Note that is
  // it possible for both of these to be non-null, which means both IDs are
  // referring to the same logical window.
  Id id = kNullId;
#if defined(USE_AURA)
  // TODO(miu): Make this an int, after clean-up for http://crbug.com/513490.
  Id aura_id = kNullId;
#endif

  // This records whether the desktop share has sound or not.
  bool audio_share = false;

  // This id contains information for WebContents capture.
  WebContentsMediaCaptureId web_contents_id;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DESKTOP_MEDIA_ID_H_
