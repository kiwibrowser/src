// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/desktop_media_id.h"

#include <stdint.h>

#include <vector>

#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"  // nogncheck
#include "ui/aura/window_observer.h"  // nogncheck
#endif  // defined(USE_AURA)

namespace  {

#if defined(USE_AURA)

class AuraWindowRegistry : public aura::WindowObserver {
 public:
  static AuraWindowRegistry* GetInstance() {
    return base::Singleton<AuraWindowRegistry>::get();
  }

  int RegisterWindow(aura::Window* window) {
    base::IDMap<aura::Window*>::const_iterator it(&registered_windows_);
    for (; !it.IsAtEnd(); it.Advance()) {
      if (it.GetCurrentValue() == window)
        return it.GetCurrentKey();
    }

    window->AddObserver(this);
    return registered_windows_.Add(window);
  }

  aura::Window* GetWindowById(int id) {
    return registered_windows_.Lookup(id);
  }

 private:
  friend struct base::DefaultSingletonTraits<AuraWindowRegistry>;

  AuraWindowRegistry() {}
  ~AuraWindowRegistry() override {}

  // WindowObserver overrides.
  void OnWindowDestroying(aura::Window* window) override {
    base::IDMap<aura::Window*>::iterator it(&registered_windows_);
    for (; !it.IsAtEnd(); it.Advance()) {
      if (it.GetCurrentValue() == window) {
        registered_windows_.Remove(it.GetCurrentKey());
        return;
      }
    }
    NOTREACHED();
  }

  base::IDMap<aura::Window*> registered_windows_;

  DISALLOW_COPY_AND_ASSIGN(AuraWindowRegistry);
};

#endif  // defined(USE_AURA)

}  // namespace

namespace content {

const char kScreenPrefix[] = "screen";
const char kWindowPrefix[] = "window";

#if defined(USE_AURA)

// static
DesktopMediaID DesktopMediaID::RegisterAuraWindow(DesktopMediaID::Type type,
                                                  aura::Window* window) {
  DCHECK(type == TYPE_SCREEN || type == TYPE_WINDOW);
  DCHECK(window);
  DesktopMediaID media_id(type, kNullId);
  media_id.aura_id = AuraWindowRegistry::GetInstance()->RegisterWindow(window);
  return media_id;
}

// static
aura::Window* DesktopMediaID::GetAuraWindowById(const DesktopMediaID& id) {
  return AuraWindowRegistry::GetInstance()->GetWindowById(id.aura_id);
}

#endif  // defined(USE_AURA)

bool DesktopMediaID::operator<(const DesktopMediaID& other) const {
#if defined(USE_AURA)
  return std::tie(type, id, aura_id, web_contents_id, audio_share) <
         std::tie(other.type, other.id, other.aura_id, other.web_contents_id,
                  other.audio_share);
#else
  return std::tie(type, id, web_contents_id, audio_share) <
         std::tie(other.type, other.id, other.web_contents_id,
                  other.audio_share);
#endif
}

bool DesktopMediaID::operator==(const DesktopMediaID& other) const {
#if defined(USE_AURA)
  return type == other.type && id == other.id && aura_id == other.aura_id &&
         web_contents_id == other.web_contents_id &&
         audio_share == other.audio_share;
#else
  return type == other.type && id == other.id &&
         web_contents_id == other.web_contents_id &&
         audio_share == other.audio_share;
#endif
}

// static
// Input string should in format:
// for WebContents:
// web-contents-media-stream://"render_process_id":"render_process_id"
// for no aura screen and window: screen:"window_id" or window:"window_id"
// for aura screen and window: screen:"window_id:aura_id" or
//                         window:"window_id:aura_id".
DesktopMediaID DesktopMediaID::Parse(const std::string& str) {
  // For WebContents type.
  WebContentsMediaCaptureId web_id;
  if (WebContentsMediaCaptureId::Parse(str, &web_id))
    return DesktopMediaID(TYPE_WEB_CONTENTS, 0, web_id);

  // For screen and window types.
  std::vector<std::string> parts = base::SplitString(
      str, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

#if defined(USE_AURA)
  if (parts.size() != 3)
    return DesktopMediaID();
#else
  if (parts.size() != 2)
    return DesktopMediaID();
#endif

  Type type = TYPE_NONE;
  if (parts[0] == kScreenPrefix) {
    type = TYPE_SCREEN;
  } else if (parts[0] == kWindowPrefix) {
    type = TYPE_WINDOW;
  } else {
    return DesktopMediaID();
  }

  int64_t id;
  if (!base::StringToInt64(parts[1], &id))
    return DesktopMediaID();

  DesktopMediaID media_id(type, id);

#if defined(USE_AURA)
  int64_t aura_id;
  if (!base::StringToInt64(parts[2], &aura_id))
    return DesktopMediaID();
  media_id.aura_id = aura_id;
#endif  // defined(USE_AURA)

  return media_id;
}

std::string DesktopMediaID::ToString() const {
  std::string prefix;
  switch (type) {
    case TYPE_NONE:
      NOTREACHED();
      return std::string();
    case TYPE_SCREEN:
      prefix = kScreenPrefix;
      break;
    case TYPE_WINDOW:
      prefix = kWindowPrefix;
      break;
    case TYPE_WEB_CONTENTS:
      return web_contents_id.ToString();
      break;
  }
  DCHECK(!prefix.empty());

  // Screen and Window types.
  prefix.append(":");
  prefix.append(base::Int64ToString(id));

#if defined(USE_AURA)
  prefix.append(":");
  prefix.append(base::Int64ToString(aura_id));
#endif  // defined(USE_AURA)

  return prefix;
}

}  // namespace content
