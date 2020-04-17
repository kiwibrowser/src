// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/common/aw_content_client.h"

#include "android_webview/common/aw_media_drm_bridge_client.h"
#include "android_webview/common/aw_resource.h"
#include "android_webview/common/crash_reporter/crash_keys.h"
#include "android_webview/common/url_constants.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/no_destructor.h"
#include "components/services/heap_profiling/public/cpp/client.h"
#include "components/version_info/version_info.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "content/public/common/user_agent.h"
#include "gpu/config/gpu_info.h"
#include "gpu/config/gpu_util.h"
#include "ipc/ipc_message.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace android_webview {

std::string GetProduct() {
  return version_info::GetProductNameAndVersionForUserAgent();
}

std::string GetUserAgent() {
  // "Version/4.0" had been hardcoded in the legacy WebView.
  std::string product = "Version/4.0 " + GetProduct();
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
        switches::kUseMobileUserAgent)) {
    product += " Mobile";
  }
  return content::BuildUserAgentFromProductAndExtraOSInfo(
      product, "; wv", true /* include_android_build_number */);
}

void AwContentClient::AddAdditionalSchemes(Schemes* schemes) {
  schemes->local_schemes.push_back(url::kContentScheme);
  schemes->secure_schemes.push_back(
      android_webview::kAndroidWebViewVideoPosterScheme);
}

std::string AwContentClient::GetProduct() const {
  return android_webview::GetProduct();
}

std::string AwContentClient::GetUserAgent() const {
  return android_webview::GetUserAgent();
}

base::string16 AwContentClient::GetLocalizedString(int message_id) const {
  // TODO(boliu): Used only by WebKit, so only bundle those resources for
  // Android WebView.
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece AwContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  // TODO(boliu): Used only by WebKit, so only bundle those resources for
  // Android WebView.
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

bool AwContentClient::CanSendWhileSwappedOut(const IPC::Message* message) {
  // For legacy API support we perform a few browser -> renderer synchronous IPC
  // messages that block the browser. However, the synchronous IPC replies might
  // be dropped by the renderer during a swap out, deadlocking the browser.
  // Because of this we should never drop any synchronous IPC replies.
  return message->type() == IPC_REPLY_ID;
}

void AwContentClient::SetGpuInfo(const gpu::GPUInfo& gpu_info) {
  gpu_fingerprint_ = gpu_info.gl_version + '|' + gpu_info.gl_vendor + '|' +
                     gpu_info.gl_renderer;
  std::replace_if(gpu_fingerprint_.begin(), gpu_fingerprint_.end(),
                  [](char c) { return !::isprint(c); }, '_');

  gpu::SetKeysForCrashLogging(gpu_info);
}

bool AwContentClient::UsingSynchronousCompositing() {
  return true;
}

media::MediaDrmBridgeClient* AwContentClient::GetMediaDrmBridgeClient() {
  return new AwMediaDrmBridgeClient(
      AwResource::GetConfigKeySystemUuidMapping());
}

void AwContentClient::OnServiceManagerConnected(
    content::ServiceManagerConnection* connection) {
  // This creates a process-wide HeapProfiling::Client that listens for requests
  // from the HeapProfilingService to start profiling the current process.
  static base::NoDestructor<heap_profiling::Client> profiling_client;

  std::unique_ptr<service_manager::BinderRegistry> registry(
      new service_manager::BinderRegistry);
  registry->AddInterface(
      base::BindRepeating(&heap_profiling::Client::BindToInterface,
                          base::Unretained(profiling_client.get())));
  connection->AddConnectionFilter(
      std::make_unique<content::SimpleConnectionFilter>(std::move(registry)));
}

}  // namespace android_webview
