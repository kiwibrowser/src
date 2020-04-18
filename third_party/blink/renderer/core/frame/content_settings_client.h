// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_CONTENT_SETTINGS_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_CONTENT_SETTINGS_CLIENT_H_

#include <memory>

#include "third_party/blink/public/platform/web_client_hints_type.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class ContentSettingCallbacks;
class KURL;
class SecurityOrigin;
class WebContentSettingsClient;
struct WebEnabledClientHints;

// This class provides the content settings information which tells
// whether each feature is allowed. Most of the methods return the
// given default values if embedder doesn't provide their own
// content settings client implementation (via m_client).
class CORE_EXPORT ContentSettingsClient {
 public:
  ContentSettingsClient();

  void SetClient(WebContentSettingsClient* client) { client_ = client; }

  // Controls whether access to Web Databases is allowed.
  bool AllowDatabase(const String& name,
                     const String& display_name,
                     unsigned estimated_size);

  // Controls whether access to File System is allowed for this frame.
  bool RequestFileSystemAccessSync();

  // Controls whether access to File System is allowed for this frame.
  void RequestFileSystemAccessAsync(std::unique_ptr<ContentSettingCallbacks>);

  // Controls whether access to File System is allowed.
  bool AllowIndexedDB(const String& name, const SecurityOrigin*);

  // Controls whether scripts are allowed to execute.
  bool AllowScript(bool enabled_per_settings);

  // Controls whether scripts loaded from the given URL are allowed to execute.
  bool AllowScriptFromSource(bool enabled_per_settings, const KURL&);

  // Retrieves the client hints that should be attached to the request for the
  // given URL.
  void GetAllowedClientHintsFromSource(const KURL&, WebEnabledClientHints*);
  // Controls whether images are allowed.
  bool AllowImage(bool enabled_per_settings, const KURL&);

  // Controls whether insecure scripts are allowed to execute for this frame.
  bool AllowRunningInsecureContent(bool enabled_per_settings,
                                   const SecurityOrigin*,
                                   const KURL&);

  // Controls whether HTML5 Web Storage is allowed for this frame.
  enum class StorageType { kLocal, kSession };
  bool AllowStorage(StorageType);

  // Controls whether access to read the clipboard is allowed for this frame.
  bool AllowReadFromClipboard(bool default_value);

  // Controls whether access to write the clipboard is allowed for this frame.
  bool AllowWriteToClipboard(bool default_value);

  // Controls whether to enable MutationEvents for this frame.
  // The common use case of this method is actually to selectively disable
  // MutationEvents, but it's been named for consistency with the rest of the
  // interface.
  bool AllowMutationEvents(bool default_value);

  // Controls whether autoplay is allowed for this frame.
  bool AllowAutoplay(bool default_value);

  // Controls whether popups and abusive redirects (e.g. framebusting) are
  // allowed for this frame.
  bool AllowPopupsAndRedirects(bool default_value);

  // Reports that passive mixed content was found at the provided URL. It may or
  // may not be actually displayed later, what would be flagged by
  // didDisplayInsecureContent.
  void PassiveInsecureContentFound(const KURL&);

  // This callback notifies the client that the frame was about to run
  // JavaScript but did not because allowScript returned false. We have a
  // separate callback here because there are a number of places that need to
  // know if JavaScript is enabled but are not necessarily preparing to execute
  // script.
  void DidNotAllowScript();

  // This callback is similar, but for plugins.
  void DidNotAllowPlugins();

  // Called to persist the client hint preferences received when |url| was
  // fetched. The preferences should be persisted for |duration|.
  void PersistClientHints(const WebEnabledClientHints&,
                          TimeDelta duration,
                          const KURL&);

 private:
  WebContentSettingsClient* client_ = nullptr;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_CONTENT_SETTINGS_CLIENT_H_
