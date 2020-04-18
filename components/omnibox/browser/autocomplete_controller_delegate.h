// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_CONTROLLER_DELEGATE_H_
#define COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_CONTROLLER_DELEGATE_H_

class AutocompleteControllerDelegate {
 public:
  // Invoked when the result set of the AutocompleteController changes. If
  // |default_match_changed| is true, the default match of the result set has
  // changed.
  virtual void OnResultChanged(bool default_match_changed) = 0;

 protected:
  virtual ~AutocompleteControllerDelegate() {}
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_AUTOCOMPLETE_CONTROLLER_DELEGATE_H_
