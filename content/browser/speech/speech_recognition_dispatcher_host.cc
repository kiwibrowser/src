// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/speech/speech_recognition_dispatcher_host.h"

#include <memory>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "content/browser/browser_plugin/browser_plugin_guest.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_manager.h"
#include "content/browser/speech/speech_recognition_manager_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/speech_recognition_manager_delegate.h"
#include "content/public/browser/speech_recognition_session_config.h"
#include "content/public/browser/speech_recognition_session_context.h"
#include "content/public/common/content_switches.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

SpeechRecognitionDispatcherHost::SpeechRecognitionDispatcherHost(
    int render_process_id,
    int render_frame_id,
    scoped_refptr<net::URLRequestContextGetter> context_getter)
    : render_process_id_(render_process_id),
      render_frame_id_(render_frame_id),
      context_getter_(std::move(context_getter)),
      weak_factory_(this) {
  // Do not add any non-trivial initialization here, instead do it lazily when
  // required (e.g. see the method |SpeechRecognitionManager::GetInstance()|) or
  // add an Init() method.
}

// static
void SpeechRecognitionDispatcherHost::Create(
    int render_process_id,
    int render_frame_id,
    scoped_refptr<net::URLRequestContextGetter> context_getter,
    mojom::SpeechRecognizerRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<SpeechRecognitionDispatcherHost>(
          render_process_id, render_frame_id, std::move(context_getter)),
      std::move(request));
}

SpeechRecognitionDispatcherHost::~SpeechRecognitionDispatcherHost() {}

base::WeakPtr<SpeechRecognitionDispatcherHost>
SpeechRecognitionDispatcherHost::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

// -------- mojom::SpeechRecognizer interface implementation ------------------

void SpeechRecognitionDispatcherHost::Start(
    mojom::StartSpeechRecognitionRequestParamsPtr params) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Check that the origin specified by the renderer process is one
  // that it is allowed to access.
  if (!params->origin.unique() &&
      !ChildProcessSecurityPolicyImpl::GetInstance()->CanRequestURL(
          render_process_id_, params->origin.GetURL())) {
    LOG(ERROR) << "SRDH::OnStartRequest, disallowed origin: "
               << params->origin.Serialize();
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&SpeechRecognitionDispatcherHost::StartRequestOnUI,
                     AsWeakPtr(), render_process_id_, render_frame_id_,
                     std::move(params)));
}

// static
void SpeechRecognitionDispatcherHost::StartRequestOnUI(
    base::WeakPtr<SpeechRecognitionDispatcherHost>
        speech_recognition_dispatcher_host,
    int render_process_id,
    int render_frame_id,
    mojom::StartSpeechRecognitionRequestParamsPtr params) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  int embedder_render_process_id = 0;
  int embedder_render_frame_id = MSG_ROUTING_NONE;

  WebContentsImpl* web_contents =
      static_cast<WebContentsImpl*>(WebContentsImpl::FromRenderFrameHostID(
          render_process_id, render_frame_id));
  if (!web_contents) {
    // The render frame id is renderer-provided. If it's invalid, don't crash.
    DLOG(ERROR) << "SRDH::OnStartRequest, invalid frame";
    return;
  }

  // If the speech API request was from an inner WebContents or a guest, save
  // the context of the outer WebContents or the embedder since we will use it
  // to decide permission.
  WebContents* outer_web_contents = web_contents->GetOuterWebContents();
  if (outer_web_contents) {
    RenderFrameHost* embedder_frame = nullptr;

    FrameTreeNode* embedder_frame_node = web_contents->GetMainFrame()
                                             ->frame_tree_node()
                                             ->render_manager()
                                             ->GetOuterDelegateNode();
    if (embedder_frame_node) {
      embedder_frame = embedder_frame_node->current_frame_host();
    } else {
      // The outer web contents is embedded using the browser plugin. Fall back
      // to a simple lookup of the main frame. TODO(avi): When the browser
      // plugin is retired, remove this code.
      embedder_frame = outer_web_contents->GetMainFrame();
    }

    embedder_render_process_id = embedder_frame->GetProcess()->GetID();
    DCHECK_NE(embedder_render_process_id, 0);
    embedder_render_frame_id = embedder_frame->GetRoutingID();
    DCHECK_NE(embedder_render_frame_id, MSG_ROUTING_NONE);
  }

  bool filter_profanities =
      SpeechRecognitionManagerImpl::GetInstance() &&
      SpeechRecognitionManagerImpl::GetInstance()->delegate() &&
      SpeechRecognitionManagerImpl::GetInstance()
          ->delegate()
          ->FilterProfanities(embedder_render_process_id);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&SpeechRecognitionDispatcherHost::StartSessionOnIO,
                     speech_recognition_dispatcher_host, std::move(params),
                     embedder_render_process_id, embedder_render_frame_id,
                     filter_profanities));
}

void SpeechRecognitionDispatcherHost::StartSessionOnIO(
    mojom::StartSpeechRecognitionRequestParamsPtr params,
    int embedder_render_process_id,
    int embedder_render_frame_id,
    bool filter_profanities) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  SpeechRecognitionSessionContext context;
  context.security_origin = params->origin;
  context.render_process_id = render_process_id_;
  context.render_frame_id = render_frame_id_;
  context.embedder_render_process_id = embedder_render_process_id;
  context.embedder_render_frame_id = embedder_render_frame_id;

  auto session =
      std::make_unique<SpeechRecognitionSession>(std::move(params->client));

  SpeechRecognitionSessionConfig config;
  config.language = params->language;
  config.max_hypotheses = params->max_hypotheses;
  config.origin = params->origin;
  config.initial_context = context;
  config.url_request_context_getter = context_getter_.get();
  config.filter_profanities = filter_profanities;
  config.continuous = params->continuous;
  config.interim_results = params->interim_results;
  config.event_listener = session->AsWeakPtr();

  for (mojom::SpeechRecognitionGrammarPtr& grammar_ptr : params->grammars) {
    config.grammars.push_back(*grammar_ptr);
  }

  int session_id =
      SpeechRecognitionManager::GetInstance()->CreateSession(config);
  DCHECK_NE(session_id, SpeechRecognitionManager::kSessionIDInvalid);
  session->SetSessionId(session_id);
  mojo::MakeStrongBinding(std::move(session),
                          std::move(params->session_request));

  SpeechRecognitionManager::GetInstance()->StartSession(session_id);
}

// ---------------------- SpeechRecognizerSession -----------------------------

SpeechRecognitionSession::SpeechRecognitionSession(
    mojom::SpeechRecognitionSessionClientPtrInfo client_ptr_info)
    : session_id_(SpeechRecognitionManager::kSessionIDInvalid),
      client_(std::move(client_ptr_info)),
      weak_factory_(this) {}

SpeechRecognitionSession::~SpeechRecognitionSession() = default;

base::WeakPtr<SpeechRecognitionSession> SpeechRecognitionSession::AsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void SpeechRecognitionSession::Abort() {
  SpeechRecognitionManager::GetInstance()->AbortSession(session_id_);
}

void SpeechRecognitionSession::StopCapture() {
  SpeechRecognitionManager::GetInstance()->StopAudioCaptureForSession(
      session_id_);
}

// -------- SpeechRecognitionEventListener interface implementation -----------

void SpeechRecognitionSession::OnRecognitionStart(int session_id) {
  client_->Started();
}

void SpeechRecognitionSession::OnAudioStart(int session_id) {
  client_->AudioStarted();
}

void SpeechRecognitionSession::OnSoundStart(int session_id) {
  client_->SoundStarted();
}

void SpeechRecognitionSession::OnSoundEnd(int session_id) {
  client_->SoundEnded();
}

void SpeechRecognitionSession::OnAudioEnd(int session_id) {
  client_->AudioEnded();
}

void SpeechRecognitionSession::OnRecognitionEnd(int session_id) {
  client_->Ended();
}

void SpeechRecognitionSession::OnRecognitionResults(
    int session_id,
    const SpeechRecognitionResults& results) {
  client_->ResultRetrieved(results);
}

void SpeechRecognitionSession::OnRecognitionError(
    int session_id,
    const SpeechRecognitionError& error) {
  client_->ErrorOccurred(error);
}

// The events below are currently not used by speech JS APIs implementation.
void SpeechRecognitionSession::OnAudioLevelsChange(int session_id,
                                                   float volume,
                                                   float noise_volume) {}

void SpeechRecognitionSession::OnEnvironmentEstimationComplete(int session_id) {
}

}  // namespace content
