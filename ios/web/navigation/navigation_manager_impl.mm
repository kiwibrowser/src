// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/navigation_manager_impl.h"

#import "ios/web/navigation/navigation_manager_delegate.h"
#import "ios/web/navigation/wk_navigation_util.h"
#import "ios/web/public/web_client.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

NavigationManager::WebLoadParams::WebLoadParams(const GURL& url)
    : url(url),
      transition_type(ui::PAGE_TRANSITION_LINK),
      user_agent_override_option(UserAgentOverrideOption::INHERIT),
      is_renderer_initiated(false),
      post_data(nil) {}

NavigationManager::WebLoadParams::~WebLoadParams() {}

NavigationManager::WebLoadParams::WebLoadParams(const WebLoadParams& other)
    : url(other.url),
      referrer(other.referrer),
      transition_type(other.transition_type),
      user_agent_override_option(other.user_agent_override_option),
      is_renderer_initiated(other.is_renderer_initiated),
      extra_headers([other.extra_headers copy]),
      post_data([other.post_data copy]) {}

NavigationManager::WebLoadParams& NavigationManager::WebLoadParams::operator=(
    const WebLoadParams& other) {
  url = other.url;
  referrer = other.referrer;
  is_renderer_initiated = other.is_renderer_initiated;
  transition_type = other.transition_type;
  user_agent_override_option = other.user_agent_override_option;
  extra_headers = [other.extra_headers copy];
  post_data = [other.post_data copy];

  return *this;
}

/* static */
NavigationItem* NavigationManagerImpl::GetLastCommittedNonRedirectedItem(
    const NavigationManager* nav_manager) {
  if (!nav_manager || !nav_manager->GetItemCount())
    return nullptr;

  int index = nav_manager->GetLastCommittedItemIndex();
  while (index >= 0) {
    NavigationItem* item = nav_manager->GetItemAtIndex(index);
    // Returns the first non-Redirect item found.
    if (!ui::PageTransitionIsRedirect(item->GetTransitionType()))
      return item;
    --index;
  }

  return nullptr;
}

/* static */
bool NavigationManagerImpl::IsFragmentChangeNavigationBetweenUrls(
    const GURL& existing_url,
    const GURL& new_url) {
  // TODO(crbug.com/749542): Current implementation incorrectly returns false
  // if URL changes from http://google.com#foo to http://google.com.
  if (existing_url == new_url || !new_url.has_ref())
    return false;

  return existing_url.EqualsIgnoringRef(new_url);
}

/* static */
void NavigationManagerImpl::UpdatePendingItemUserAgentType(
    UserAgentOverrideOption user_agent_override_option,
    const NavigationItem* inherit_from_item,
    NavigationItem* pending_item) {
  DCHECK(pending_item);

  // |user_agent_override_option| must be INHERIT if |pending_item|'s
  // UserAgentType is NONE, as requesting a desktop or mobile user agent should
  // be disabled for app-specific URLs.
  DCHECK(pending_item->GetUserAgentType() != UserAgentType::NONE ||
         user_agent_override_option == UserAgentOverrideOption::INHERIT);

  // Newly created pending items are created with UserAgentType::NONE for native
  // pages or UserAgentType::MOBILE for non-native pages.  If the pending item's
  // URL is non-native, check which user agent type it should be created with
  // based on |user_agent_override_option|.
  DCHECK_NE(UserAgentType::DESKTOP, pending_item->GetUserAgentType());
  if (pending_item->GetUserAgentType() == UserAgentType::NONE)
    return;

  switch (user_agent_override_option) {
    case UserAgentOverrideOption::DESKTOP:
      pending_item->SetUserAgentType(UserAgentType::DESKTOP);
      break;
    case UserAgentOverrideOption::MOBILE:
      pending_item->SetUserAgentType(UserAgentType::MOBILE);
      break;
    case UserAgentOverrideOption::INHERIT: {
      // Propagate the last committed non-native item's UserAgentType if there
      // is one, otherwise keep the default value, which is mobile.
      DCHECK(!inherit_from_item ||
             inherit_from_item->GetUserAgentType() != UserAgentType::NONE);
      if (inherit_from_item) {
        pending_item->SetUserAgentType(inherit_from_item->GetUserAgentType());
      }
      break;
    }
  }
}

NavigationManagerImpl::NavigationManagerImpl()
    : delegate_(nullptr), browser_state_(nullptr) {}

NavigationManagerImpl::~NavigationManagerImpl() = default;

void NavigationManagerImpl::SetDelegate(NavigationManagerDelegate* delegate) {
  delegate_ = delegate;
}

void NavigationManagerImpl::SetBrowserState(BrowserState* browser_state) {
  browser_state_ = browser_state;
}

void NavigationManagerImpl::DetachFromWebView() {}

void NavigationManagerImpl::RemoveTransientURLRewriters() {
  transient_url_rewriters_.clear();
}

std::unique_ptr<NavigationItemImpl> NavigationManagerImpl::CreateNavigationItem(
    const GURL& url,
    const Referrer& referrer,
    ui::PageTransition transition,
    NavigationInitiationType initiation_type) {
  NavigationItem* last_committed_item = GetLastCommittedItem();
  auto item = CreateNavigationItemWithRewriters(
      url, referrer, transition, initiation_type,
      last_committed_item ? last_committed_item->GetURL() : GURL::EmptyGURL(),
      &transient_url_rewriters_);
  RemoveTransientURLRewriters();
  return item;
}

void NavigationManagerImpl::UpdatePendingItemUrl(const GURL& url) const {
  // If there is no pending item, navigation is probably happening within the
  // back forward history. Don't modify the item list.
  NavigationItemImpl* pending_item = GetPendingItemImpl();
  if (!pending_item || url == pending_item->GetURL())
    return;

  // UpdatePendingItemUrl is used to handle redirects after loading starts for
  // the currenting pending item. No transient item should exists at this point.
  DCHECK(!GetTransientItem());
  pending_item->SetURL(url);
  pending_item->SetVirtualURL(url);
  // Redirects (3xx response code), or client side navigation must change POST
  // requests to GETs.
  pending_item->SetPostData(nil);
  pending_item->ResetHttpRequestHeaders();
}

NavigationItemImpl* NavigationManagerImpl::GetCurrentItemImpl() const {
  NavigationItemImpl* transient_item = GetTransientItemImpl();
  if (transient_item)
    return transient_item;

  NavigationItemImpl* pending_item = GetPendingItemImpl();
  if (pending_item)
    return pending_item;

  return GetLastCommittedItemImpl();
}

void NavigationManagerImpl::UpdateCurrentItemForReplaceState(
    const GURL& url,
    NSString* state_object) {
  DCHECK(!GetTransientItem());
  NavigationItemImpl* current_item = GetCurrentItemImpl();
  current_item->SetURL(url);
  current_item->SetSerializedStateObject(state_object);
  current_item->SetHasStateBeenReplaced(true);
  current_item->SetPostData(nil);
  // If the change is to a committed item, notify interested parties.
  if (current_item != GetPendingItem()) {
    OnNavigationItemChanged();
  }
}

void NavigationManagerImpl::GoToIndex(int index,
                                      NavigationInitiationType initiation_type,
                                      bool has_user_gesture) {
  if (index < 0 || index >= GetItemCount()) {
    NOTREACHED();
    return;
  }

  if (!GetTransientItem()) {
    delegate_->RecordPageStateInNavigationItem();
  }
  delegate_->ClearTransientContent();

  // Notify delegate if the new navigation will use a different user agent.
  UserAgentType to_item_user_agent_type =
      GetItemAtIndex(index)->GetUserAgentType();
  NavigationItem* pending_item = GetPendingItem();
  NavigationItem* previous_item =
      pending_item ? pending_item : GetLastCommittedItem();
  UserAgentType previous_item_user_agent_type =
      previous_item ? previous_item->GetUserAgentType() : UserAgentType::NONE;

  if (to_item_user_agent_type != UserAgentType::NONE &&
      to_item_user_agent_type != previous_item_user_agent_type) {
    delegate_->WillChangeUserAgentType();
  }

  FinishGoToIndex(index, initiation_type, has_user_gesture);
}

void NavigationManagerImpl::GoToIndex(int index) {
  GoToIndex(index, NavigationInitiationType::BROWSER_INITIATED,
            /*has_user_gesture=*/true);
}

NavigationItem* NavigationManagerImpl::GetLastCommittedItem() const {
  return GetLastCommittedItemImpl();
}

NavigationItem* NavigationManagerImpl::GetPendingItem() const {
  return GetPendingItemImpl();
}

NavigationItem* NavigationManagerImpl::GetTransientItem() const {
  return GetTransientItemImpl();
}

void NavigationManagerImpl::LoadURLWithParams(
    const NavigationManager::WebLoadParams& params) {
  DCHECK(!(params.transition_type & ui::PAGE_TRANSITION_FORWARD_BACK));
  delegate_->ClearTransientContent();
  delegate_->RecordPageStateInNavigationItem();

  NavigationInitiationType initiation_type =
      params.is_renderer_initiated
          ? NavigationInitiationType::RENDERER_INITIATED
          : NavigationInitiationType::BROWSER_INITIATED;
  AddPendingItem(params.url, params.referrer, params.transition_type,
                 initiation_type, params.user_agent_override_option);

  // Mark pending item as created from hash change if necessary. This is needed
  // because window.hashchange message may not arrive on time.
  NavigationItemImpl* pending_item = GetPendingItemImpl();
  if (pending_item) {
    NavigationItem* last_committed_item = GetLastCommittedItem();
    GURL last_committed_url = last_committed_item
                                  ? last_committed_item->GetVirtualURL()
                                  : GURL::EmptyGURL();
    GURL pending_url = pending_item->GetURL();
    if (last_committed_url != pending_url &&
        last_committed_url.EqualsIgnoringRef(pending_url)) {
      pending_item->SetIsCreatedFromHashChange(true);
    }

    if (params.virtual_url.is_valid())
      pending_item->SetVirtualURL(params.virtual_url);
  }

  // Add additional headers to the NavigationItem before loading it in the web
  // view. This implementation must match CRWWebController's |currentNavItem|.
  // However, to avoid introducing a GetCurrentItem() that is only used here,
  // the logic in |currentNavItem| is inlined here with the small simplification
  // since AddPendingItem() implies that any transient item would have been
  // cleared.
  DCHECK(!GetTransientItem());
  NavigationItemImpl* added_item =
      pending_item ? pending_item : GetLastCommittedItemImpl();
  DCHECK(added_item);
  if (params.extra_headers)
    added_item->AddHttpRequestHeaders(params.extra_headers);
  if (params.post_data) {
    DCHECK([added_item->GetHttpRequestHeaders() objectForKey:@"Content-Type"])
        << "Post data should have an associated content type";
    added_item->SetPostData(params.post_data);
    added_item->SetShouldSkipRepostFormConfirmation(true);
  }

  FinishLoadURLWithParams();
}

void NavigationManagerImpl::AddTransientURLRewriter(
    BrowserURLRewriter::URLRewriter rewriter) {
  DCHECK(rewriter);
  transient_url_rewriters_.push_back(rewriter);
}

void NavigationManagerImpl::Reload(ReloadType reload_type,
                                   bool check_for_reposts) {
  if (!GetTransientItem() && !GetPendingItem() && !GetLastCommittedItem())
    return;

  // Reload with ORIGINAL_REQUEST_URL type should reload with the original
  // request url of the transient item, or pending item if transient doesn't
  // exist, or last committed item if both of them don't exist. The reason is
  // that a server side redirect may change the item's url.
  // For example, the user visits www.chromium.org and is then redirected
  // to m.chromium.org, when the user wants to refresh the page with a different
  // configuration (e.g. user agent), the user would be expecting to visit
  // www.chromium.org instead of m.chromium.org.
  if (reload_type == web::ReloadType::ORIGINAL_REQUEST_URL) {
    NavigationItem* reload_item = nullptr;
    if (GetTransientItem())
      reload_item = GetTransientItem();
    else if (GetPendingItem())
      reload_item = GetPendingItem();
    else
      reload_item = GetLastCommittedItem();
    DCHECK(reload_item);

    reload_item->SetURL(reload_item->GetOriginalRequestURL());
  }

  FinishReload();
}

void NavigationManagerImpl::ReloadWithUserAgentType(
    UserAgentType user_agent_type) {
  DCHECK_NE(user_agent_type, UserAgentType::NONE);

  // This removes the web view, which will be recreated at the end of this.
  delegate_->WillChangeUserAgentType();

  NavigationItem* last_non_redirect_item = GetTransientItem();
  if (!last_non_redirect_item ||
      ui::PageTransitionIsRedirect(last_non_redirect_item->GetTransitionType()))
    last_non_redirect_item = GetVisibleItem();
  if (!last_non_redirect_item ||
      ui::PageTransitionIsRedirect(last_non_redirect_item->GetTransitionType()))
    last_non_redirect_item = GetLastCommittedNonRedirectedItem(this);

  if (!last_non_redirect_item)
    return;

  // |reloadURL| will be empty if a page was open by DOM.
  GURL reload_url(last_non_redirect_item->GetOriginalRequestURL());
  if (reload_url.is_empty()) {
    reload_url = last_non_redirect_item->GetVirtualURL();
  }

  // Reload using a client-side redirect URL to create a new entry in
  // WKBackForwardList for the new user agent type. This hack is not needed for
  // LegacyNavigationManagerImpl which manages its own history entries.
  if (web::GetWebClient()->IsSlimNavigationManagerEnabled()) {
    GURL target_url;
    // If current entry is a redirect URL, reload the original target URL. This
    // can happen on a slow connection when user taps on Request Desktop Site
    // before the previous redirect has finished (https://crbug.com/833958).
    if (wk_navigation_util::IsRestoreSessionUrl(reload_url) &&
        wk_navigation_util::ExtractTargetURL(reload_url, &target_url)) {
      reload_url = target_url;
    }
    reload_url = wk_navigation_util::CreateRedirectUrl(reload_url);
  }

  WebLoadParams params(reload_url);
  if (last_non_redirect_item->GetVirtualURL() != reload_url)
    params.virtual_url = last_non_redirect_item->GetVirtualURL();
  params.referrer = last_non_redirect_item->GetReferrer();
  params.transition_type = ui::PAGE_TRANSITION_RELOAD;

  switch (user_agent_type) {
    case UserAgentType::DESKTOP:
      params.user_agent_override_option = UserAgentOverrideOption::DESKTOP;
      break;
    case UserAgentType::MOBILE:
      params.user_agent_override_option = UserAgentOverrideOption::MOBILE;
      break;
    case UserAgentType::NONE:
      NOTREACHED();
  }

  LoadURLWithParams(params);
}

void NavigationManagerImpl::LoadIfNecessary() {
  delegate_->LoadIfNecessary();
}

std::unique_ptr<NavigationItemImpl>
NavigationManagerImpl::CreateNavigationItemWithRewriters(
    const GURL& url,
    const Referrer& referrer,
    ui::PageTransition transition,
    NavigationInitiationType initiation_type,
    const GURL& previous_url,
    const std::vector<BrowserURLRewriter::URLRewriter>* additional_rewriters)
    const {
  GURL loaded_url(url);

  bool url_was_rewritten = false;
  if (additional_rewriters && !additional_rewriters->empty()) {
    url_was_rewritten = web::BrowserURLRewriter::RewriteURLWithWriters(
        &loaded_url, browser_state_, *additional_rewriters);
  }

  if (!url_was_rewritten) {
    web::BrowserURLRewriter::GetInstance()->RewriteURLIfNecessary(
        &loaded_url, browser_state_);
  }

  // The URL should not be changed to app-specific URL in two cases:
  // 1) The load is renderer-initiated requested by non-app-specific URL. Pages
  //    with app-specific urls have elevated previledges and should not be
  //    allowed to open app-specific URLs.
  // 2) The load is a placeholder URL. Navigation code relies on this special
  //    URL to implement native view and WebUI.
  bool is_renderer_initiated_app_specific_url_from_non_app_specific_url =
      initiation_type == web::NavigationInitiationType::RENDERER_INITIATED &&
      loaded_url != url && web::GetWebClient()->IsAppSpecificURL(loaded_url) &&
      !web::GetWebClient()->IsAppSpecificURL(previous_url);
  if (is_renderer_initiated_app_specific_url_from_non_app_specific_url ||
      IsPlaceholderUrl(url)) {
    loaded_url = url;
  }

  auto item = std::make_unique<NavigationItemImpl>();
  item->SetOriginalRequestURL(loaded_url);
  item->SetURL(loaded_url);
  item->SetReferrer(referrer);
  item->SetTransitionType(transition);
  item->SetNavigationInitiationType(initiation_type);
  if (web::GetWebClient()->IsAppSpecificURL(loaded_url)) {
    item->SetUserAgentType(web::UserAgentType::NONE);
  }

  return item;
}

NavigationItem* NavigationManagerImpl::GetLastCommittedNonAppSpecificItem()
    const {
  WebClient* client = GetWebClient();
  for (int index = GetLastCommittedItemIndex(); index >= 0; index--) {
    NavigationItem* item = GetItemAtIndex(index);
    if (!client->IsAppSpecificURL(item->GetURL()))
      return item;
  }
  return nullptr;
}

void NavigationManagerImpl::FinishReload() {
  delegate_->Reload();
}

void NavigationManagerImpl::FinishLoadURLWithParams() {
  delegate_->LoadCurrentItem();
}

bool NavigationManagerImpl::IsPlaceholderUrl(const GURL& url) const {
  return false;  // Default implementation doesn't use placeholder URLs
}

}  // namespace web
