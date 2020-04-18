// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/browser/content_autofill_driver_factory.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace autofill {

namespace {

std::unique_ptr<AutofillDriver> CreateDriver(
    content::RenderFrameHost* render_frame_host,
    AutofillClient* client,
    const std::string& app_locale,
    AutofillManager::AutofillDownloadManagerState enable_download_manager,
    AutofillProvider* provider) {
  return std::make_unique<ContentAutofillDriver>(
      render_frame_host, client, app_locale, enable_download_manager, provider);
}

}  // namespace

const char ContentAutofillDriverFactory::
    kContentAutofillDriverFactoryWebContentsUserDataKey[] =
        "web_contents_autofill_driver_factory";

ContentAutofillDriverFactory::~ContentAutofillDriverFactory() {}

// static
void ContentAutofillDriverFactory::CreateForWebContentsAndDelegate(
    content::WebContents* contents,
    AutofillClient* client,
    const std::string& app_locale,
    AutofillManager::AutofillDownloadManagerState enable_download_manager) {
  CreateForWebContentsAndDelegate(contents, client, app_locale,
                                  enable_download_manager, nullptr);
}

void ContentAutofillDriverFactory::CreateForWebContentsAndDelegate(
    content::WebContents* contents,
    AutofillClient* client,
    const std::string& app_locale,
    AutofillManager::AutofillDownloadManagerState enable_download_manager,
    AutofillProvider* provider) {
  if (FromWebContents(contents))
    return;

  auto new_factory = base::WrapUnique(new ContentAutofillDriverFactory(
      contents, client, app_locale, enable_download_manager, provider));
  const std::vector<content::RenderFrameHost*> frames =
      contents->GetAllFrames();
  for (content::RenderFrameHost* frame : frames) {
    if (frame->IsRenderFrameLive())
      new_factory->RenderFrameCreated(frame);
  }

  contents->SetUserData(kContentAutofillDriverFactoryWebContentsUserDataKey,
                        std::move(new_factory));
}

// static
ContentAutofillDriverFactory* ContentAutofillDriverFactory::FromWebContents(
    content::WebContents* contents) {
  return static_cast<ContentAutofillDriverFactory*>(contents->GetUserData(
      kContentAutofillDriverFactoryWebContentsUserDataKey));
}

// static
void ContentAutofillDriverFactory::BindAutofillDriver(
    mojom::AutofillDriverRequest request,
    content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  // We try to bind to the driver of this render frame host,
  // but if driver is not ready for this render frame host for now,
  // the request will be just dropped, this would cause closing the message pipe
  // which would raise connection error to peer side.
  // Peer side could reconnect later when needed.
  if (!web_contents)
    return;

  ContentAutofillDriverFactory* factory =
      ContentAutofillDriverFactory::FromWebContents(web_contents);
  if (!factory)
    return;

  ContentAutofillDriver* driver = factory->DriverForFrame(render_frame_host);
  if (driver)
    driver->BindRequest(std::move(request));
}

ContentAutofillDriverFactory::ContentAutofillDriverFactory(
    content::WebContents* web_contents,
    AutofillClient* client,
    const std::string& app_locale,
    AutofillManager::AutofillDownloadManagerState enable_download_manager,
    AutofillProvider* provider)
    : AutofillDriverFactory(client),
      content::WebContentsObserver(web_contents),
      app_locale_(app_locale),
      enable_download_manager_(enable_download_manager),
      provider_(provider) {}

ContentAutofillDriver* ContentAutofillDriverFactory::DriverForFrame(
    content::RenderFrameHost* render_frame_host) {
  // This cast is safe because AutofillDriverFactory::AddForKey is protected
  // and always called with ContentAutofillDriver instances within
  // ContentAutofillDriverFactory.
  return static_cast<ContentAutofillDriver*>(DriverForKey(render_frame_host));
}

void ContentAutofillDriverFactory::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  AddForKey(render_frame_host,
            base::Bind(CreateDriver, render_frame_host, client(), app_locale_,
                       enable_download_manager_, provider_));
}

void ContentAutofillDriverFactory::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  DeleteForKey(render_frame_host);
}

void ContentAutofillDriverFactory::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // For the purposes of this code, a navigation is not important if it has not
  // committed yet or if it's in a subframe.
  if (!navigation_handle->HasCommitted() ||
      !navigation_handle->IsInMainFrame()) {
    return;
  }

  // A main frame navigation has occured. We suppress the autofill popup and
  // tell the autofill driver.
  NavigationFinished();
  DriverForFrame(navigation_handle->GetRenderFrameHost())
      ->DidNavigateMainFrame(navigation_handle);
}

void ContentAutofillDriverFactory::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::HIDDEN)
    TabHidden();
}

}  // namespace autofill
