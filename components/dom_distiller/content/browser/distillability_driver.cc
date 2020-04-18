// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/dom_distiller/content/browser/distillability_driver.h"

#include <memory>

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    dom_distiller::DistillabilityDriver);

namespace dom_distiller {

// Implementation of the Mojo DistillabilityService. This is called by the
// renderer to notify the browser that a page is distillable.
class DistillabilityServiceImpl : public mojom::DistillabilityService {
 public:
  explicit DistillabilityServiceImpl(
      base::WeakPtr<DistillabilityDriver> distillability_driver)
      : distillability_driver_(distillability_driver) {}

  ~DistillabilityServiceImpl() override {
  }

  void NotifyIsDistillable(bool is_distillable,
                           bool is_last_update,
                           bool is_mobile_friendly) override {
    if (!distillability_driver_) return;
    distillability_driver_->OnDistillability(is_distillable, is_last_update,
                                             is_mobile_friendly);
  }

 private:
  base::WeakPtr<DistillabilityDriver> distillability_driver_;
};

DistillabilityDriver::DistillabilityDriver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      weak_factory_(this) {
  if (!web_contents) return;
  frame_interfaces_.AddInterface(
      base::BindRepeating(&DistillabilityDriver::CreateDistillabilityService,
                          base::Unretained(this)));
}

DistillabilityDriver::~DistillabilityDriver() {
  content::WebContentsObserver::Observe(nullptr);
}

void DistillabilityDriver::CreateDistillabilityService(
    mojom::DistillabilityServiceRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<DistillabilityServiceImpl>(weak_factory_.GetWeakPtr()),
      std::move(request));
}

void DistillabilityDriver::SetDelegate(
    const base::RepeatingCallback<void(bool, bool, bool)>& delegate) {
  m_delegate_ = delegate;
}

void DistillabilityDriver::OnDistillability(bool distillable,
                                            bool is_last,
                                            bool is_mobile_friendly) {
  if (m_delegate_.is_null()) return;

  m_delegate_.Run(distillable, is_last, is_mobile_friendly);
}

void DistillabilityDriver::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  frame_interfaces_.TryBindInterface(interface_name, interface_pipe);
}

}  // namespace dom_distiller
