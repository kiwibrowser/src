// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_web_contents_manager.h"

#include <algorithm>
#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/stl_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "chromecast/browser/cast_web_view_default.h"
#include "chromecast/browser/cast_web_view_factory.h"
#include "chromecast/chromecast_buildflags.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_CHROMECAST_EXTENSIONS)
#include "chromecast/browser/cast_web_view_extension.h"
#endif

namespace chromecast {

CastWebContentsManager::CastWebContentsManager(
    content::BrowserContext* browser_context,
    CastWebViewFactory* web_view_factory)
    : browser_context_(browser_context),
      web_view_factory_(web_view_factory),
      task_runner_(base::SequencedTaskRunnerHandle::Get()),
      weak_factory_(this) {
  DCHECK(browser_context_);
  DCHECK(web_view_factory_);
  DCHECK(task_runner_);
}

CastWebContentsManager::~CastWebContentsManager() = default;

std::unique_ptr<CastWebView> CastWebContentsManager::CreateWebView(
    const CastWebView::CreateParams& params,
    scoped_refptr<content::SiteInstance> site_instance,
    const extensions::Extension* extension,
    const GURL& initial_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return web_view_factory_->CreateWebView(
      params, this, std::move(site_instance), extension, initial_url);
}

void CastWebContentsManager::DelayWebContentsDeletion(
    std::unique_ptr<content::WebContents> web_contents,
    base::TimeDelta time_delta) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(web_contents);
  if (time_delta <= base::TimeDelta()) {
    LOG(INFO) << "Deleting WebContents for " << web_contents->GetVisibleURL();
    web_contents.reset();
    return;
  }
  auto* web_contents_ptr = web_contents.get();
  // Suspend the MediaSession to free up media resources for the next content
  // window.
  content::MediaSession::Get(web_contents_ptr)
      ->Suspend(content::MediaSession::SuspendType::SYSTEM);
  LOG(INFO) << "WebContents for " << web_contents->GetVisibleURL()
            << " will be deleted in " << time_delta.InMilliseconds()
            << " milliseconds.";
  expiring_web_contents_.insert(std::move(web_contents));
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CastWebContentsManager::DeleteWebContents,
                     weak_factory_.GetWeakPtr(), web_contents_ptr),
      time_delta);
}

void CastWebContentsManager::DeleteWebContents(
    content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(web_contents);
  LOG(INFO) << "Deleting WebContents for " << web_contents->GetVisibleURL();
  base::EraseIf(
      expiring_web_contents_,
      [web_contents](const std::unique_ptr<content::WebContents>& ptr) {
        return ptr.get() == web_contents;
      });
}

}  // namespace chromecast
