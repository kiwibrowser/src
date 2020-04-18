// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUPERVISED_USER_ERROR_PAGE_GIN_WRAPPER_H_
#define COMPONENTS_SUPERVISED_USER_ERROR_PAGE_GIN_WRAPPER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/web_restrictions/interfaces/web_restrictions.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/wrappable.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace content {
class RenderFrame;
}

namespace supervised_user_error_page {

class GinWrapper : public gin::Wrappable<GinWrapper> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static void InstallWhenFrameReady(
      content::RenderFrame* render_frame,
      const std::string& url,
      const web_restrictions::mojom::WebRestrictionsPtr&
          web_restrictions_service);

 private:
  class Loader : public content::RenderFrameObserver {
   public:
    Loader(content::RenderFrame* render_frame,
           const std::string& url,
           const web_restrictions::mojom::WebRestrictionsPtr&
               web_restrictions_service);
    ~Loader() override = default;

    void DidClearWindowObject() override;
    void OnDestruct() override;

   private:
    void InstallGinWrapper();

    std::string url_;
    const web_restrictions::mojom::WebRestrictionsPtr&
        web_restrictions_service_;
    DISALLOW_COPY_AND_ASSIGN(Loader);
  };

  GinWrapper(content::RenderFrame* render_frame,
             const std::string& url,
             const web_restrictions::mojom::WebRestrictionsPtr&
                 web_restrictions_service);
  ~GinWrapper() override;

  // Request permission to allow visiting the currently blocked site.
  bool RequestPermission(v8::Local<v8::Function> setRequestStatusCallback);

  void OnAccessRequestAdded(bool success);

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  const std::string url_;
  const web_restrictions::mojom::WebRestrictionsPtr& web_restrictions_service_;
  v8::Global<v8::Function> setRequestStatusCallback_;
  base::WeakPtrFactory<GinWrapper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GinWrapper);
};

}  // namespace supervised_user_error_page.

#endif  // COMPONENTS_SUPERVISED_USER_ERROR_PAGE_GIN_WRAPPER_H_
