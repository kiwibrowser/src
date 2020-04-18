// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_FIND_IN_PAGE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_FIND_IN_PAGE_H_

#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/blink/public/mojom/frame/find_in_page.mojom-blink.h"
#include "third_party/blink/public/platform/interface_registry.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_plugin_container.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/editing/finder/text_finder.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace blink {

class WebLocalFrameImpl;
class WebString;
struct WebFindOptions;
struct WebFloatRect;

class CORE_EXPORT FindInPage final
    : public GarbageCollectedFinalized<FindInPage>,
      public mojom::blink::FindInPage {
  USING_PRE_FINALIZER(FindInPage, Dispose);

 public:
  static FindInPage* Create(WebLocalFrameImpl& frame,
                            InterfaceRegistry* interface_registry) {
    return new FindInPage(frame, interface_registry);
  }

  void RequestFind(int identifier,
                   const WebString& search_text,
                   const WebFindOptions&);

  bool Find(int identifier,
            const WebString& search_text,
            const WebFindOptions&,
            bool wrap_within_frame,
            bool* active_now = nullptr);

  void SetTickmarks(const WebVector<WebRect>&);

  int FindMatchMarkersVersion() const;

  // Returns the bounding box of the active find-in-page match marker or an
  // empty rect if no such marker exists. The rect is returned in find-in-page
  // coordinates.
  WebFloatRect ActiveFindMatchRect();

  // mojom::blink::FindInPage overrides

  void ActivateNearestFindResult(const WebFloatPoint&,
                                 ActivateNearestFindResultCallback) final;

  // Stops the current find-in-page, following the given |action|
  void StopFinding(mojom::StopFindAction action) final;

  // Returns the distance (squared) to the closest find-in-page match from the
  // provided point, in find-in-page coordinates.
  void GetNearestFindResult(const WebFloatPoint&,
                            GetNearestFindResultCallback) final;

  // Returns the bounding boxes of the find-in-page match markers in the frame,
  // in find-in-page coordinates.
  void FindMatchRects(int current_version, FindMatchRectsCallback) final;

  // Clears the active find match in the frame, if one exists.
  void ClearActiveFindMatch() final;

  TextFinder* GetTextFinder() const;

  // Returns the text finder object if it already exists.
  // Otherwise creates it and then returns.
  TextFinder& EnsureTextFinder();

  void SetPluginFindHandler(WebPluginContainer* plugin);

  WebPluginContainer* PluginFindHandler() const;

  WebPlugin* GetWebPluginForFind();

  void BindToRequest(mojom::blink::FindInPageAssociatedRequest request);

  void Dispose();

  void Trace(blink::Visitor* visitor) {
    visitor->Trace(text_finder_);
    visitor->Trace(frame_);
  }

 private:
  FindInPage(WebLocalFrameImpl& frame, InterfaceRegistry* interface_registry)
      : frame_(&frame), binding_(this) {
    if (!interface_registry)
      return;
    interface_registry->AddAssociatedInterface(WTF::BindRepeating(
        &FindInPage::BindToRequest, WrapWeakPersistent(this)));
  }

  // Will be initialized after first call to ensureTextFinder().
  Member<TextFinder> text_finder_;

  WebPluginContainer* plugin_find_handler_;

  const Member<WebLocalFrameImpl> frame_;

  mojo::AssociatedBinding<mojom::blink::FindInPage> binding_;

  DISALLOW_COPY_AND_ASSIGN(FindInPage);
};

}  // namespace blink

#endif
