// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/content/browser/content_password_manager_driver_factory.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/content/browser/content_password_manager_driver.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "net/cert/cert_status_flags.h"

namespace password_manager {

namespace {

const char kContentPasswordManagerDriverFactoryWebContentsUserDataKey[] =
    "web_contents_password_manager_driver_factory";

}  // namespace

void ContentPasswordManagerDriverFactory::CreateForWebContents(
    content::WebContents* web_contents,
    PasswordManagerClient* password_client,
    autofill::AutofillClient* autofill_client) {
  if (FromWebContents(web_contents))
    return;

  // NOTE: Can't use |std::make_unique| due to private constructor.
  auto new_factory = base::WrapUnique(new ContentPasswordManagerDriverFactory(
      web_contents, password_client, autofill_client));
  const std::vector<content::RenderFrameHost*> frames =
      web_contents->GetAllFrames();
  for (content::RenderFrameHost* frame : frames) {
    if (frame->IsRenderFrameLive())
      new_factory->RenderFrameCreated(frame);
  }

  web_contents->SetUserData(
      kContentPasswordManagerDriverFactoryWebContentsUserDataKey,
      std::move(new_factory));
}

ContentPasswordManagerDriverFactory::ContentPasswordManagerDriverFactory(
    content::WebContents* web_contents,
    PasswordManagerClient* password_client,
    autofill::AutofillClient* autofill_client)
    : content::WebContentsObserver(web_contents),
      password_client_(password_client),
      autofill_client_(autofill_client) {}

ContentPasswordManagerDriverFactory::~ContentPasswordManagerDriverFactory() {}

// static
ContentPasswordManagerDriverFactory*
ContentPasswordManagerDriverFactory::FromWebContents(
    content::WebContents* contents) {
  return static_cast<ContentPasswordManagerDriverFactory*>(
      contents->GetUserData(
          kContentPasswordManagerDriverFactoryWebContentsUserDataKey));
}

// static
void ContentPasswordManagerDriverFactory::BindPasswordManagerDriver(
    autofill::mojom::PasswordManagerDriverRequest request,
    content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return;

  ContentPasswordManagerDriverFactory* factory =
      ContentPasswordManagerDriverFactory::FromWebContents(web_contents);
  // We try to bind to the driver, but if driver is not ready for now or totally
  // not available for this render frame host, the request will be just dropped.
  // This would cause the message pipe to be closed, which will raise a
  // connection error on the peer side.
  if (!factory)
    return;

  ContentPasswordManagerDriver* driver =
      factory->GetDriverForFrame(render_frame_host);
  if (driver)
    driver->BindRequest(std::move(request));
}

ContentPasswordManagerDriver*
ContentPasswordManagerDriverFactory::GetDriverForFrame(
    content::RenderFrameHost* render_frame_host) {
  auto mapping = frame_driver_map_.find(render_frame_host);
  return mapping == frame_driver_map_.end() ? nullptr : mapping->second.get();
}

void ContentPasswordManagerDriverFactory::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  auto insertion_result =
      frame_driver_map_.insert(std::make_pair(render_frame_host, nullptr));
  // This is called twice for the main frame.
  if (insertion_result.second) {  // This was the first time.
    insertion_result.first->second =
        std::make_unique<ContentPasswordManagerDriver>(
            render_frame_host, password_client_, autofill_client_);
  }
}

void ContentPasswordManagerDriverFactory::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  frame_driver_map_.erase(render_frame_host);
}

void ContentPasswordManagerDriverFactory::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted())
    return;

  frame_driver_map_.find(navigation_handle->GetRenderFrameHost())
      ->second->DidNavigateFrame(navigation_handle);
}

void ContentPasswordManagerDriverFactory::RequestSendLoggingAvailability() {
  for (const auto& key_val_iterator : frame_driver_map_) {
    key_val_iterator.second->SendLoggingAvailability();
  }
}

}  // namespace password_manager
