// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLIBILITY_DRIVER_H_
#define COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLIBILITY_DRIVER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/dom_distiller/content/browser/distillable_page_utils.h"
#include "components/dom_distiller/content/common/distillability_service.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace dom_distiller {

// This is an IPC helper for determining whether a page should be distilled.
class DistillabilityDriver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<DistillabilityDriver> {
 public:
  ~DistillabilityDriver() override;
  void CreateDistillabilityService(
      mojom::DistillabilityServiceRequest request);

  void SetDelegate(const DistillabilityDelegate& delegate);

  // content::WebContentsObserver implementation.
  void OnInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;

 private:
  explicit DistillabilityDriver(content::WebContents* web_contents);
  friend class content::WebContentsUserData<DistillabilityDriver>;
  friend class DistillabilityServiceImpl;

  void OnDistillability(bool distillable,
                        bool is_last,
                        bool is_mobile_friendly);

  DistillabilityDelegate m_delegate_;

  service_manager::BinderRegistry frame_interfaces_;

  base::WeakPtrFactory<DistillabilityDriver> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DistillabilityDriver);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CONTENT_BROWSER_DISTILLIBILITY_DRIVER_H_
