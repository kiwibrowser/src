// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_TAB_MODEL_TAB_MODEL_LIST_H_
#define CHROME_BROWSER_UI_ANDROID_TAB_MODEL_TAB_MODEL_LIST_H_

#include <stddef.h>

#include <vector>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "components/sessions/core/session_id.h"

class TabModel;
class TabModelListObserver;

struct NavigateParams;

namespace content {
class WebContents;
}

// Stores a list of all TabModel objects.
class TabModelList {
 public:
  typedef std::vector<TabModel*> TabModelVector;
  typedef TabModelVector::iterator iterator;
  typedef TabModelVector::const_iterator const_iterator;

  static void HandlePopupNavigation(NavigateParams* params);
  static void AddTabModel(TabModel* tab_model);
  static void RemoveTabModel(TabModel* tab_model);

  static void AddObserver(TabModelListObserver* observer);
  static void RemoveObserver(TabModelListObserver* observer);

  static TabModel* GetTabModelForWebContents(
      content::WebContents* web_contents);
  static TabModel* FindTabModelWithId(SessionID desired_id);
  static bool IsOffTheRecordSessionActive();

  static const_iterator begin();
  static const_iterator end();
  static bool empty();
  static size_t size();

  static TabModel* get(size_t index);

  // A list of observers which will be notified of every TabModel addition and
  // removal across all TabModelLists.
  static base::LazyInstance<base::ObserverList<TabModelListObserver>>::Leaky
      observers_;

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TabModelList);
};

#endif  // CHROME_BROWSER_UI_ANDROID_TAB_MODEL_TAB_MODEL_LIST_H_
