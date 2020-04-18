// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_COMPONENT_ACTION_DELEGATE_H_
#define CHROME_BROWSER_UI_TOOLBAR_COMPONENT_ACTION_DELEGATE_H_

#include <string>

// Manages component actions in the toolbar model.
class ComponentActionDelegate {
 public:
  virtual ~ComponentActionDelegate() {}

  // Adds or removes the component action labeled by |action_id| from the
  // toolbar model. The caller must not add the same action twice.
  virtual void AddComponentAction(const std::string& action_id) = 0;
  virtual void RemoveComponentAction(const std::string& action_id) = 0;

  // Returns |true| if the toolbar model has an action for |action_id|.
  virtual bool HasComponentAction(const std::string& action_id) const = 0;
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_COMPONENT_ACTION_DELEGATE_H_
