// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_GLOBAL_SCOPE_CREATION_PARAMS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_GLOBAL_SCOPE_CREATION_PARAMS_H_

#include <memory>
#include "base/macros.h"
#include "base/optional.h"
#include "base/unguessable_token.h"
#include "services/service_manager/public/mojom/interface_provider.mojom-blink.h"
#include "third_party/blink/public/mojom/net/ip_address_space.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_cache_options.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/script/script.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"
#include "third_party/blink/renderer/core/workers/worker_settings.h"
#include "third_party/blink/renderer/core/workers/worklet_module_responses_map.h"
#include "third_party/blink/renderer/platform/graphics/begin_frame_provider.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_parsers.h"
#include "third_party/blink/renderer/platform/network/content_security_policy_response_headers.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/referrer_policy.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class WorkerClients;

// GlobalScopeCreationParams contains parameters for initializing
// WorkerGlobalScope or WorkletGlobalScope.
struct CORE_EXPORT GlobalScopeCreationParams final {
  USING_FAST_MALLOC(GlobalScopeCreationParams);

 public:
  GlobalScopeCreationParams(
      const KURL& script_url,
      ScriptType script_type,
      const String& user_agent,
      const Vector<CSPHeaderAndType>* content_security_policy_parsed_headers,
      ReferrerPolicy referrer_policy,
      const SecurityOrigin*,
      bool starter_secure_context,
      WorkerClients*,
      mojom::IPAddressSpace,
      const Vector<String>* origin_trial_tokens,
      const base::UnguessableToken& parent_devtools_token,
      std::unique_ptr<WorkerSettings>,
      V8CacheOptions,
      WorkletModuleResponsesMap*,
      service_manager::mojom::blink::InterfaceProviderPtrInfo = {},
      BeginFrameProviderParams begin_frame_provider_params = {});

  ~GlobalScopeCreationParams() = default;

  KURL script_url;
  ScriptType script_type;
  String user_agent;

  // |content_security_policy_parsed_headers| and
  // |content_security_policy_raw_headers| are mutually exclusive.
  // |content_security_policy_parsed_headers| is an empty vector
  // when |content_security_policy_raw_headers| is set.
  std::unique_ptr<Vector<CSPHeaderAndType>>
      content_security_policy_parsed_headers;
  base::Optional<ContentSecurityPolicyResponseHeaders>
      content_security_policy_raw_headers;

  ReferrerPolicy referrer_policy;
  std::unique_ptr<Vector<String>> origin_trial_tokens;

  // The SecurityOrigin of the Document creating a Worker/Worklet.
  //
  // For Workers, the origin may have been configured with extra policy
  // privileges when it was created (e.g., enforce path-based file:// origins.)
  // To ensure that these are transferred to the origin of a new worker global
  // scope, supply the Document's SecurityOrigin as the 'starter origin'. See
  // SecurityOrigin::TransferPrivilegesFrom() for details on what privileges are
  // transferred.
  //
  // For Worklets, the origin is used for fetching module scripts. Worklet
  // scripts need to be fetched as sub-resources of the Document, and a module
  // script loader uses Document's SecurityOrigin for security checks.
  scoped_refptr<const SecurityOrigin> starter_origin;

  // Indicates if the Document creating a Worker/Worklet is a secure context.
  //
  // Worklets are defined to have a unique, opaque origin, so are not secure:
  // https://drafts.css-houdini.org/worklets/#script-settings-for-worklets
  // Origin trials are only enabled in secure contexts, and the trial tokens are
  // inherited from the document, so also consider the context of the document.
  // The value should be supplied as the result of Document.IsSecureContext().
  bool starter_secure_context;

  // This object is created and initialized on the thread creating
  // a new worker context, but ownership of it and this
  // GlobalScopeCreationParams structure is passed along to the new worker
  // thread, where it is finalized.
  //
  // Hence, CrossThreadPersistent<> is required to allow finalization
  // to happen on a thread different than the thread creating the
  // persistent reference. If the worker thread creation context
  // supplies no extra 'clients', m_workerClients can be left as empty/null.
  CrossThreadPersistent<WorkerClients> worker_clients;

  mojom::IPAddressSpace address_space;

  base::UnguessableToken parent_devtools_token;

  std::unique_ptr<WorkerSettings> worker_settings;

  V8CacheOptions v8_cache_options;

  CrossThreadPersistent<WorkletModuleResponsesMap> module_responses_map;

  service_manager::mojom::blink::InterfaceProviderPtrInfo interface_provider;

  BeginFrameProviderParams begin_frame_provider_params;

  DISALLOW_COPY_AND_ASSIGN(GlobalScopeCreationParams);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_GLOBAL_SCOPE_CREATION_PARAMS_H_
