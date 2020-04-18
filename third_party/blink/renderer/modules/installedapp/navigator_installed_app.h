// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_INSTALLEDAPP_NAVIGATOR_INSTALLED_APP_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_INSTALLEDAPP_NAVIGATOR_INSTALLED_APP_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class Document;
class Navigator;
class ScriptPromise;
class ScriptState;
class InstalledAppController;

class NavigatorInstalledApp final
    : public GarbageCollected<NavigatorInstalledApp>,
      public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorInstalledApp);

 public:
  static const char kSupplementName[];

  static NavigatorInstalledApp* From(Document&);
  static NavigatorInstalledApp& From(Navigator&);

  static ScriptPromise getInstalledRelatedApps(ScriptState*, Navigator&);
  ScriptPromise getInstalledRelatedApps(ScriptState*);

  InstalledAppController* Controller();

  void Trace(blink::Visitor*) override;

 private:
  explicit NavigatorInstalledApp(Navigator&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_INSTALLEDAPP_NAVIGATOR_INSTALLED_APP_H_
