// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_model_list.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/supports_user_data.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_list_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Wrapper around a NSMutableArray<TabModel> allowing to bind it to a
// base::SupportsUserData. Any base::SupportsUserData that owns this
// wrapper will owns the reference to the TabModels.
class TabModelListUserData : public base::SupportsUserData::Data {
 public:
  TabModelListUserData();
  ~TabModelListUserData() override;

  static TabModelListUserData* GetForBrowserState(
      ios::ChromeBrowserState* browser_state,
      bool create);

  NSMutableSet<TabModel*>* tab_models() const { return tab_models_; }

 private:
  NSMutableSet<TabModel*>* tab_models_;

  DISALLOW_COPY_AND_ASSIGN(TabModelListUserData);
};

const char kTabModelListKey = 0;

TabModelListUserData::TabModelListUserData()
    : tab_models_([[NSMutableSet alloc] init]) {}

TabModelListUserData::~TabModelListUserData() {
  // TabModelList is destroyed during base::SupportsUserData destruction. At
  // that point, all TabModels must have been unregistered already.
  DCHECK_EQ([tab_models_ count], 0u);
}

// static
TabModelListUserData* TabModelListUserData::GetForBrowserState(
    ios::ChromeBrowserState* browser_state,
    bool create) {
  TabModelListUserData* tab_model_list_user_data =
      static_cast<TabModelListUserData*>(
          browser_state->GetUserData(&kTabModelListKey));
  if (!tab_model_list_user_data && create) {
    tab_model_list_user_data = new TabModelListUserData;
    browser_state->SetUserData(&kTabModelListKey,
                               base::WrapUnique(tab_model_list_user_data));
  }
  return tab_model_list_user_data;
}

base::LazyInstance<base::ObserverList<TabModelListObserver>>::Leaky
    g_observer_list;

}  // namespace

// static
void TabModelList::AddObserver(TabModelListObserver* observer) {
  g_observer_list.Get().AddObserver(observer);
}

// static
void TabModelList::RemoveObserver(TabModelListObserver* observer) {
  g_observer_list.Get().RemoveObserver(observer);
}

// static
void TabModelList::RegisterTabModelWithChromeBrowserState(
    ios::ChromeBrowserState* browser_state,
    TabModel* tab_model) {
  NSMutableSet<TabModel*>* tab_models =
      TabModelListUserData::GetForBrowserState(browser_state, true)
          ->tab_models();
  DCHECK(![tab_models containsObject:tab_model]);
  [tab_models addObject:tab_model];

  for (auto& observer : g_observer_list.Get())
    observer.TabModelRegisteredWithBrowserState(tab_model, browser_state);
}

// static
void TabModelList::UnregisterTabModelFromChromeBrowserState(
    ios::ChromeBrowserState* browser_state,
    TabModel* tab_model) {
  NSMutableSet<TabModel*>* tab_models =
      TabModelListUserData::GetForBrowserState(browser_state, false)
          ->tab_models();
  DCHECK([tab_models containsObject:tab_model]);
  [tab_models removeObject:tab_model];

  for (auto& observer : g_observer_list.Get())
    observer.TabModelUnregisteredFromBrowserState(tab_model, browser_state);
}

// static
NSArray<TabModel*>* TabModelList::GetTabModelsForChromeBrowserState(
    ios::ChromeBrowserState* browser_state) {
  TabModelListUserData* tab_model_list_user_data =
      TabModelListUserData::GetForBrowserState(browser_state, false);
  return tab_model_list_user_data
             ? [tab_model_list_user_data->tab_models() allObjects]
             : nil;
}

// static
TabModel* TabModelList::GetLastActiveTabModelForChromeBrowserState(
    ios::ChromeBrowserState* browser_state) {
  TabModelListUserData* tab_model_list_user_data =
      TabModelListUserData::GetForBrowserState(browser_state, false);
  if (!tab_model_list_user_data ||
      [tab_model_list_user_data->tab_models() count] == 0u)
    return nil;

  // There is currently no way to mark a TabModel as active. Assert that there
  // is only one TabModel associated with |browser_state| until it is possible
  // to mark a TabModel as active.
  DCHECK_EQ([tab_model_list_user_data->tab_models() count], 1u);
  return [tab_model_list_user_data->tab_models() anyObject];
}

// static
bool TabModelList::IsOffTheRecordSessionActive() {
  std::vector<ios::ChromeBrowserState*> browser_states =
      GetApplicationContext()
          ->GetChromeBrowserStateManager()
          ->GetLoadedBrowserStates();

  for (ios::ChromeBrowserState* browser_state : browser_states) {
    DCHECK(!browser_state->IsOffTheRecord());
    if (!browser_state->HasOffTheRecordChromeBrowserState())
      continue;

    ios::ChromeBrowserState* otr_browser_state =
        browser_state->GetOffTheRecordChromeBrowserState();

    TabModelListUserData* tab_model_list_user_data =
        TabModelListUserData::GetForBrowserState(otr_browser_state, false);
    if (!tab_model_list_user_data)
      continue;

    for (TabModel* tab_model in tab_model_list_user_data->tab_models()) {
      if (![tab_model isEmpty])
        return true;
    }
  }

  return false;
}
