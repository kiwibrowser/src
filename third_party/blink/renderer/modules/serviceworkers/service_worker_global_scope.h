/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_GLOBAL_SCOPE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_GLOBAL_SCOPE_H_

#include <memory>
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_registration.h"
#include "third_party/blink/renderer/bindings/core/v8/request_or_usv_string.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class Dictionary;
class RespondWithObserver;
class ScriptPromise;
class ScriptState;
class ServiceWorkerClients;
class ServiceWorkerRegistration;
class ServiceWorkerThread;
class WaitUntilObserver;
struct GlobalScopeCreationParams;

typedef RequestOrUSVString RequestInfo;

class MODULES_EXPORT ServiceWorkerGlobalScope final : public WorkerGlobalScope {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static ServiceWorkerGlobalScope* Create(
      ServiceWorkerThread*,
      std::unique_ptr<GlobalScopeCreationParams>,
      double time_origin);

  ~ServiceWorkerGlobalScope() override;
  bool IsServiceWorkerGlobalScope() const override { return true; }

  // Implements WorkerGlobalScope.
  void EvaluateClassicScript(
      const KURL& script_url,
      String source_code,
      std::unique_ptr<Vector<char>> cached_meta_data) override;
  void ImportModuleScript(const KURL& module_url_record,
                          network::mojom::FetchCredentialsMode) override;

  // Counts an evaluated script and its size. Called for the main worker script.
  void CountWorkerScript(size_t script_size, size_t cached_metadata_size);

  // Counts an evaluated script and its size. Called for each of imported
  // scripts.
  void CountImportedScript(size_t script_size, size_t cached_metadata_size);

  // Called when the main worker script is evaluated.
  void DidEvaluateClassicScript();

  // ServiceWorkerGlobalScope.idl
  ServiceWorkerClients* clients();
  ServiceWorkerRegistration* registration();

  ScriptPromise fetch(ScriptState*,
                      const RequestInfo&,
                      const Dictionary&,
                      ExceptionState&);

  ScriptPromise skipWaiting(ScriptState*);

  void SetRegistration(std::unique_ptr<WebServiceWorkerRegistration::Handle>);

  // EventTarget
  const AtomicString& InterfaceName() const override;

  void DispatchExtendableEvent(Event*, WaitUntilObserver*);

  // For ExtendableEvents that also have a respondWith() function.
  void DispatchExtendableEventWithRespondWith(Event*,
                                              WaitUntilObserver*,
                                              RespondWithObserver*);

  bool IsInstalling() const { return is_installing_; }
  void SetIsInstalling(bool is_installing);

  // Script evaluation does not start until this function is called.
  void ReadyToEvaluateScript();

  void CountCacheStorageInstalledScript(uint64_t script_size,
                                        uint64_t script_metadata_size);

  DEFINE_ATTRIBUTE_EVENT_LISTENER(install);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(activate);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(fetch);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(message);

  void Trace(blink::Visitor*) override;

 protected:
  // EventTarget
  bool AddEventListenerInternal(
      const AtomicString& event_type,
      EventListener*,
      const AddEventListenerOptionsResolved&) override;

 private:
  ServiceWorkerGlobalScope(std::unique_ptr<GlobalScopeCreationParams>,
                           ServiceWorkerThread*,
                           double time_origin);
  void importScripts(const Vector<String>& urls, ExceptionState&) override;
  SingleCachedMetadataHandler* CreateWorkerScriptCachedMetadataHandler(
      const KURL& script_url,
      const Vector<char>* meta_data) override;
  void ExceptionThrown(ErrorEvent*) override;

  // Records the |script_size| and |cached_metadata_size| for UMA to measure the
  // number of scripts and the total bytes of scripts.
  void RecordScriptSize(size_t script_size, size_t cached_metadata_size);

  Member<ServiceWorkerClients> clients_;
  Member<ServiceWorkerRegistration> registration_;
  bool did_evaluate_script_ = false;
  size_t script_count_ = 0;
  size_t script_total_size_ = 0;
  size_t script_cached_metadata_total_size_ = 0;
  bool is_installing_ = false;
  size_t cache_storage_installed_script_count_ = 0;
  uint64_t cache_storage_installed_script_total_size_ = 0;
  uint64_t cache_storage_installed_script_metadata_total_size_ = 0;

  bool evaluate_script_ready_ = false;
  base::OnceClosure evaluate_script_;
};

DEFINE_TYPE_CASTS(ServiceWorkerGlobalScope,
                  ExecutionContext,
                  context,
                  context->IsServiceWorkerGlobalScope(),
                  context.IsServiceWorkerGlobalScope());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_GLOBAL_SCOPE_H_
