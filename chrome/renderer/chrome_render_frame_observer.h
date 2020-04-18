// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_CHROME_RENDER_FRAME_OBSERVER_H_
#define CHROME_RENDERER_CHROME_RENDER_FRAME_OBSERVER_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "chrome/common/chrome_render_frame.mojom.h"
#include "chrome/common/prerender_types.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_binding_set.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace gfx {
class Size;
}

namespace safe_browsing {
class PhishingClassifierDelegate;
}

namespace translate {
class TranslateHelper;
}

// This class holds the Chrome specific parts of RenderFrame, and has the same
// lifetime.
class ChromeRenderFrameObserver : public content::RenderFrameObserver,
                                  public chrome::mojom::ChromeRenderFrame {
 public:
  explicit ChromeRenderFrameObserver(content::RenderFrame* render_frame);
  ~ChromeRenderFrameObserver() override;

  service_manager::BinderRegistry* registry() { return &registry_; }

 private:
  enum TextCaptureType { PRELIMINARY_CAPTURE, FINAL_CAPTURE };

  // RenderFrameObserver implementation.
  void OnInterfaceRequestForFrame(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void DidStartProvisionalLoad(blink::WebDocumentLoader* loader) override;
  void DidFinishLoad() override;
  void DidCreateNewDocument() override;
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_document_navigation) override;
  void DidClearWindowObject() override;
  void DidMeaningfulLayout(blink::WebMeaningfulLayout layout_type) override;
  void OnDestruct() override;

  // IPC handlers
  void OnSetIsPrerendering(prerender::PrerenderMode mode,
                           const std::string& histogram_prefix);
  void OnRequestThumbnailForContextNode(
      int thumbnail_min_area_pixels,
      const gfx::Size& thumbnail_max_size_pixels,
      int callback_id);
  void OnPrintNodeUnderContextMenu();
  void OnSetClientSidePhishingDetection(bool enable_phishing_detection);

  // chrome::mojom::ChromeRenderFrame:
  void SetWindowFeatures(
      blink::mojom::WindowFeaturesPtr window_features) override;
  void ExecuteWebUIJavaScript(const base::string16& javascript) override;
  void RequestThumbnailForContextNode(
      int32_t thumbnail_min_area_pixels,
      const gfx::Size& thumbnail_max_size_pixels,
      chrome::mojom::ImageFormat image_format,
      const RequestThumbnailForContextNodeCallback& callback) override;
  void RequestReloadImageForContextNode() override;
  void SetClientSidePhishingDetection(bool enable_phishing_detection) override;
  void GetWebApplicationInfo(
      const GetWebApplicationInfoCallback& callback) override;
#if defined(OS_ANDROID)
  void UpdateBrowserControlsState(content::BrowserControlsState constraints,
                                  content::BrowserControlsState current,
                                  bool animate) override;
#endif

  void OnRenderFrameObserverRequest(
      chrome::mojom::ChromeRenderFrameAssociatedRequest request);

  // Captures page information using the top (main) frame of a frame tree.
  // Currently, this page information is just the text content of the all
  // frames, collected and concatenated until a certain limit (kMaxIndexChars)
  // is reached.
  // TODO(dglazkov): This is incompatible with OOPIF and needs to be updated.
  void CapturePageText(TextCaptureType capture_type);

  void CapturePageTextLater(TextCaptureType capture_type,
                            base::TimeDelta delay);

  // Have the same lifetime as us.
  translate::TranslateHelper* translate_helper_;
  safe_browsing::PhishingClassifierDelegate* phishing_classifier_;


#if !defined(OS_ANDROID)
  // Save the JavaScript to preload if ExecuteWebUIJavaScript is invoked.
  std::vector<base::string16> webui_javascript_;
#endif

  mojo::AssociatedBindingSet<chrome::mojom::ChromeRenderFrame> bindings_;

  service_manager::BinderRegistry registry_;

  DISALLOW_COPY_AND_ASSIGN(ChromeRenderFrameObserver);
};

#endif  // CHROME_RENDERER_CHROME_RENDER_FRAME_OBSERVER_H_
