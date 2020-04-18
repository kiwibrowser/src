// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/ruleset_manager.h"

#include <algorithm>
#include <tuple>
#include <utility>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "components/web_cache/browser/web_cache_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/declarative_net_request/ruleset_matcher.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "extensions/browser/api/web_request/web_request_permissions.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/api/declarative_net_request/utils.h"
#include "extensions/common/constants.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace extensions {
namespace declarative_net_request {
namespace {

namespace flat_rule = url_pattern_index::flat;

// Maps content::ResourceType to flat_rule::ElementType.
flat_rule::ElementType GetElementType(content::ResourceType type) {
  switch (type) {
    case content::RESOURCE_TYPE_LAST_TYPE:
    case content::RESOURCE_TYPE_PREFETCH:
    case content::RESOURCE_TYPE_SUB_RESOURCE:
      return flat_rule::ElementType_OTHER;
    case content::RESOURCE_TYPE_MAIN_FRAME:
      return flat_rule::ElementType_MAIN_FRAME;
    case content::RESOURCE_TYPE_CSP_REPORT:
      return flat_rule::ElementType_CSP_REPORT;
    case content::RESOURCE_TYPE_SCRIPT:
    case content::RESOURCE_TYPE_WORKER:
    case content::RESOURCE_TYPE_SHARED_WORKER:
    case content::RESOURCE_TYPE_SERVICE_WORKER:
      return flat_rule::ElementType_SCRIPT;
    case content::RESOURCE_TYPE_IMAGE:
    case content::RESOURCE_TYPE_FAVICON:
      return flat_rule::ElementType_IMAGE;
    case content::RESOURCE_TYPE_STYLESHEET:
      return flat_rule::ElementType_STYLESHEET;
    case content::RESOURCE_TYPE_OBJECT:
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:
      return flat_rule::ElementType_OBJECT;
    case content::RESOURCE_TYPE_XHR:
      return flat_rule::ElementType_XMLHTTPREQUEST;
    case content::RESOURCE_TYPE_SUB_FRAME:
      return flat_rule::ElementType_SUBDOCUMENT;
    case content::RESOURCE_TYPE_PING:
      return flat_rule::ElementType_PING;
    case content::RESOURCE_TYPE_MEDIA:
      return flat_rule::ElementType_MEDIA;
    case content::RESOURCE_TYPE_FONT_RESOURCE:
      return flat_rule::ElementType_FONT;
  }
  NOTREACHED();
  return flat_rule::ElementType_OTHER;
}

// Returns the flat_rule::ElementType for the given |request|.
flat_rule::ElementType GetElementType(const WebRequestInfo& request) {
  if (request.url.SchemeIsWSOrWSS())
    return flat_rule::ElementType_WEBSOCKET;

  return request.type.has_value() ? GetElementType(request.type.value())
                                  : flat_rule::ElementType_OTHER;
}

// Returns whether the request to |url| is third party to its |document_origin|.
// TODO(crbug.com/696822): Look into caching this.
bool IsThirdPartyRequest(const GURL& url, const url::Origin& document_origin) {
  if (document_origin.unique())
    return true;

  return !net::registry_controlled_domains::SameDomainOrHost(
      url, document_origin,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

void ClearRendererCacheOnUI() {
  web_cache::WebCacheManager::GetInstance()->ClearCacheOnNavigation();
}

// Helper to clear each renderer's in-memory cache the next time it navigates.
void ClearRendererCacheOnNavigation() {
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    ClearRendererCacheOnUI();
  } else {
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     base::BindOnce(&ClearRendererCacheOnUI));
  }
}

}  // namespace

RulesetManager::RulesetManager(const InfoMap* info_map) : info_map_(info_map) {
  DCHECK(info_map_);

  // RulesetManager can be created on any sequence.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

RulesetManager::~RulesetManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void RulesetManager::AddRuleset(
    const ExtensionId& extension_id,
    std::unique_ptr<RulesetMatcher> ruleset_matcher) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsAPIAvailable());

  bool inserted;
  std::tie(std::ignore, inserted) =
      rulesets_.emplace(extension_id, info_map_->GetInstallTime(extension_id),
                        std::move(ruleset_matcher));
  DCHECK(inserted) << "AddRuleset called twice in succession for "
                   << extension_id;

  // Clear the renderers' cache so that they take the new rules into account.
  ClearRendererCacheOnNavigation();
}

void RulesetManager::RemoveRuleset(const ExtensionId& extension_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsAPIAvailable());

  auto compare_by_id =
      [&extension_id](const ExtensionRulesetData& ruleset_data) {
        return ruleset_data.extension_id == extension_id;
      };

  DCHECK(std::find_if(rulesets_.begin(), rulesets_.end(), compare_by_id) !=
         rulesets_.end())
      << "RemoveRuleset called without a corresponding AddRuleset for "
      << extension_id;

  base::EraseIf(rulesets_, compare_by_id);

  // Clear the renderers' cache so that they take the removed rules into
  // account.
  ClearRendererCacheOnNavigation();
}

RulesetManager::Action RulesetManager::EvaluateRequest(
    const WebRequestInfo& request,
    bool is_incognito_context,
    GURL* redirect_url) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(redirect_url);

  if (!ShouldEvaluateRequest(request))
    return Action::NONE;

  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.DeclarativeNetRequest.EvaluateRequestTime.AllExtensions");

  if (test_observer_)
    test_observer_->OnEvaluateRequest(request, is_incognito_context);

  const GURL& url = request.url;
  const url::Origin first_party_origin =
      request.initiator.value_or(url::Origin());
  const flat_rule::ElementType element_type = GetElementType(request);
  const bool is_third_party = IsThirdPartyRequest(url, first_party_origin);

  std::vector<bool> should_evaluate_rulesets_for_request(rulesets_.size());

  // We first check if any extension wants the request to be blocked.
  {
    size_t i = 0;
    auto ruleset_data = rulesets_.begin();
    for (; ruleset_data != rulesets_.end(); ++ruleset_data, ++i) {
      // As a minor optimization, cache the value of
      // |ShouldEvaluateRulesetForRequest|.
      should_evaluate_rulesets_for_request[i] = ShouldEvaluateRulesetForRequest(
          *ruleset_data, request, is_incognito_context);

      if (!should_evaluate_rulesets_for_request[i])
        continue;

      if (ruleset_data->matcher->ShouldBlockRequest(
              url, first_party_origin, element_type, is_third_party)) {
        return Action::BLOCK;
      }
    }
  }

  // The request shouldn't be blocked. Now check if any extension wants to
  // redirect the request.

  // Redirecting WebSocket handshake request is prohibited.
  if (element_type == flat_rule::ElementType_WEBSOCKET)
    return Action::NONE;

  // This iterates in decreasing order of extension installation time. Hence
  // more recently installed extensions get higher priority in choosing the
  // redirect url.
  {
    size_t i = 0;
    auto ruleset_data = rulesets_.begin();
    for (; ruleset_data != rulesets_.end(); ++ruleset_data, ++i) {
      if (!should_evaluate_rulesets_for_request[i])
        continue;

      if (ruleset_data->matcher->ShouldRedirectRequest(
              url, first_party_origin, element_type, is_third_party,
              redirect_url)) {
        return Action::REDIRECT;
      }
    }
  }

  return Action::NONE;
}

void RulesetManager::SetObserverForTest(TestObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  test_observer_ = observer;
}

RulesetManager::ExtensionRulesetData::ExtensionRulesetData(
    const ExtensionId& extension_id,
    const base::Time& extension_install_time,
    std::unique_ptr<RulesetMatcher> matcher)
    : extension_id(extension_id),
      extension_install_time(extension_install_time),
      matcher(std::move(matcher)) {}
RulesetManager::ExtensionRulesetData::~ExtensionRulesetData() = default;
RulesetManager::ExtensionRulesetData::ExtensionRulesetData(
    ExtensionRulesetData&& other) = default;
RulesetManager::ExtensionRulesetData& RulesetManager::ExtensionRulesetData::
operator=(ExtensionRulesetData&& other) = default;

bool RulesetManager::ExtensionRulesetData::operator<(
    const ExtensionRulesetData& other) const {
  // Sort based on descending installation time, using extension id to break
  // ties.
  return (extension_install_time != other.extension_install_time)
             ? (extension_install_time > other.extension_install_time)
             : (extension_id < other.extension_id);
}

bool RulesetManager::ShouldEvaluateRequest(
    const WebRequestInfo& request) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Ensure clients filter out sensitive requests.
  DCHECK(!WebRequestPermissions::HideRequest(info_map_, request));

  if (!IsAPIAvailable()) {
    DCHECK(rulesets_.empty());
    return false;
  }

  // Prevent extensions from modifying any resources on the chrome-extension
  // scheme. Practically, this has the effect of not allowing an extension to
  // modify its own resources (The extension wouldn't have the permission to
  // other extension origins anyway).
  if (request.url.SchemeIs(kExtensionScheme))
    return false;

  return true;
}

bool RulesetManager::ShouldEvaluateRulesetForRequest(
    const ExtensionRulesetData& ruleset,
    const WebRequestInfo& request,
    bool is_incognito_context) const {
  // Only extensions enabled in incognito should have access to requests in an
  // incognito context.
  if (is_incognito_context &&
      !info_map_->IsIncognitoEnabled(ruleset.extension_id)) {
    return false;
  }

  const int tab_id = request.frame_data ? request.frame_data->tab_id
                                        : extension_misc::kUnknownTabId;

  // We have already checked that the extension has access to the request as far
  // as the browser context is concerned. Since there is nothing special that we
  // have to do for split mode incognito extensions, pass false for
  // |crosses_incognito|.
  const bool crosses_incognito = false;
  PermissionsData::PageAccess result =
      WebRequestPermissions::CanExtensionAccessURL(
          info_map_, ruleset.extension_id, request.url, tab_id,
          crosses_incognito,
          WebRequestPermissions::REQUIRE_HOST_PERMISSION_FOR_URL_AND_INITIATOR,
          request.initiator);

  // TODO(crbug.com/809680): Handle ACCESS_WITHHELD.
  return result == PermissionsData::PageAccess::kAllowed;
}

}  // namespace declarative_net_request
}  // namespace extensions
