/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "third_party/blink/renderer/core/inspector/inspector_application_cache_agent.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/platform/network/network_state_notifier.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

using protocol::Response;

namespace ApplicationCacheAgentState {
static const char kApplicationCacheAgentEnabled[] =
    "applicationCacheAgentEnabled";
}

InspectorApplicationCacheAgent::InspectorApplicationCacheAgent(
    InspectedFrames* inspected_frames)
    : inspected_frames_(inspected_frames) {}

void InspectorApplicationCacheAgent::Restore() {
  if (state_->booleanProperty(
          ApplicationCacheAgentState::kApplicationCacheAgentEnabled, false)) {
    enable();
  }
}

Response InspectorApplicationCacheAgent::enable() {
  state_->setBoolean(ApplicationCacheAgentState::kApplicationCacheAgentEnabled,
                     true);
  instrumenting_agents_->addInspectorApplicationCacheAgent(this);
  GetFrontend()->networkStateUpdated(GetNetworkStateNotifier().OnLine());
  return Response::OK();
}

Response InspectorApplicationCacheAgent::disable() {
  state_->setBoolean(ApplicationCacheAgentState::kApplicationCacheAgentEnabled,
                     false);
  instrumenting_agents_->removeInspectorApplicationCacheAgent(this);
  return Response::OK();
}

void InspectorApplicationCacheAgent::UpdateApplicationCacheStatus(
    LocalFrame* frame) {
  DocumentLoader* document_loader = frame->Loader().GetDocumentLoader();
  if (!document_loader)
    return;

  ApplicationCacheHost* host = document_loader->GetApplicationCacheHost();
  ApplicationCacheHost::Status status = host->GetStatus();
  ApplicationCacheHost::CacheInfo info = host->ApplicationCacheInfo();

  String manifest_url = info.manifest_.GetString();
  String frame_id = IdentifiersFactory::FrameId(frame);
  GetFrontend()->applicationCacheStatusUpdated(frame_id, manifest_url,
                                               static_cast<int>(status));
}

void InspectorApplicationCacheAgent::NetworkStateChanged(LocalFrame* frame,
                                                         bool online) {
  if (frame == inspected_frames_->Root())
    GetFrontend()->networkStateUpdated(online);
}

Response InspectorApplicationCacheAgent::getFramesWithManifests(
    std::unique_ptr<
        protocol::Array<protocol::ApplicationCache::FrameWithManifest>>*
        result) {
  *result =
      protocol::Array<protocol::ApplicationCache::FrameWithManifest>::create();

  for (LocalFrame* frame : *inspected_frames_) {
    DocumentLoader* document_loader = frame->Loader().GetDocumentLoader();
    if (!document_loader)
      continue;

    ApplicationCacheHost* host = document_loader->GetApplicationCacheHost();
    ApplicationCacheHost::CacheInfo info = host->ApplicationCacheInfo();
    String manifest_url = info.manifest_.GetString();
    if (!manifest_url.IsEmpty()) {
      std::unique_ptr<protocol::ApplicationCache::FrameWithManifest> value =
          protocol::ApplicationCache::FrameWithManifest::create()
              .setFrameId(IdentifiersFactory::FrameId(frame))
              .setManifestURL(manifest_url)
              .setStatus(static_cast<int>(host->GetStatus()))
              .build();
      (*result)->addItem(std::move(value));
    }
  }
  return Response::OK();
}

Response InspectorApplicationCacheAgent::AssertFrameWithDocumentLoader(
    String frame_id,
    DocumentLoader*& result) {
  LocalFrame* frame =
      IdentifiersFactory::FrameById(inspected_frames_, frame_id);
  if (!frame)
    return Response::Error("No frame for given id found");

  result = frame->Loader().GetDocumentLoader();
  if (!result)
    return Response::Error("No documentLoader for given frame found");
  return Response::OK();
}

Response InspectorApplicationCacheAgent::getManifestForFrame(
    const String& frame_id,
    String* manifest_url) {
  DocumentLoader* document_loader = nullptr;
  Response response = AssertFrameWithDocumentLoader(frame_id, document_loader);
  if (!response.isSuccess())
    return response;

  ApplicationCacheHost::CacheInfo info =
      document_loader->GetApplicationCacheHost()->ApplicationCacheInfo();
  *manifest_url = info.manifest_.GetString();
  return Response::OK();
}

Response InspectorApplicationCacheAgent::getApplicationCacheForFrame(
    const String& frame_id,
    std::unique_ptr<protocol::ApplicationCache::ApplicationCache>*
        application_cache) {
  DocumentLoader* document_loader = nullptr;
  Response response = AssertFrameWithDocumentLoader(frame_id, document_loader);
  if (!response.isSuccess())
    return response;

  ApplicationCacheHost* host = document_loader->GetApplicationCacheHost();
  ApplicationCacheHost::CacheInfo info = host->ApplicationCacheInfo();

  ApplicationCacheHost::ResourceInfoList resources;
  host->FillResourceList(&resources);

  *application_cache = BuildObjectForApplicationCache(resources, info);
  return Response::OK();
}

std::unique_ptr<protocol::ApplicationCache::ApplicationCache>
InspectorApplicationCacheAgent::BuildObjectForApplicationCache(
    const ApplicationCacheHost::ResourceInfoList& application_cache_resources,
    const ApplicationCacheHost::CacheInfo& application_cache_info) {
  return protocol::ApplicationCache::ApplicationCache::create()
      .setManifestURL(application_cache_info.manifest_.GetString())
      .setSize(application_cache_info.size_)
      .setCreationTime(application_cache_info.creation_time_)
      .setUpdateTime(application_cache_info.update_time_)
      .setResources(
          BuildArrayForApplicationCacheResources(application_cache_resources))
      .build();
}

std::unique_ptr<
    protocol::Array<protocol::ApplicationCache::ApplicationCacheResource>>
InspectorApplicationCacheAgent::BuildArrayForApplicationCacheResources(
    const ApplicationCacheHost::ResourceInfoList& application_cache_resources) {
  std::unique_ptr<
      protocol::Array<protocol::ApplicationCache::ApplicationCacheResource>>
      resources = protocol::Array<
          protocol::ApplicationCache::ApplicationCacheResource>::create();

  ApplicationCacheHost::ResourceInfoList::const_iterator end =
      application_cache_resources.end();
  ApplicationCacheHost::ResourceInfoList::const_iterator it =
      application_cache_resources.begin();
  for (int i = 0; it != end; ++it, i++)
    resources->addItem(BuildObjectForApplicationCacheResource(*it));

  return resources;
}

std::unique_ptr<protocol::ApplicationCache::ApplicationCacheResource>
InspectorApplicationCacheAgent::BuildObjectForApplicationCacheResource(
    const ApplicationCacheHost::ResourceInfo& resource_info) {
  StringBuilder builder;
  if (resource_info.is_master_)
    builder.Append("Master ");

  if (resource_info.is_manifest_)
    builder.Append("Manifest ");

  if (resource_info.is_fallback_)
    builder.Append("Fallback ");

  if (resource_info.is_foreign_)
    builder.Append("Foreign ");

  if (resource_info.is_explicit_)
    builder.Append("Explicit ");

  std::unique_ptr<protocol::ApplicationCache::ApplicationCacheResource> value =
      protocol::ApplicationCache::ApplicationCacheResource::create()
          .setUrl(resource_info.resource_.GetString())
          .setSize(static_cast<int>(resource_info.size_))
          .setType(builder.ToString())
          .build();
  return value;
}

void InspectorApplicationCacheAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  InspectorBaseAgent::Trace(visitor);
}

}  // namespace blink
