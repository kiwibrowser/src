// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CORE_DOM_DISTILLER_OBSERVER_H_
#define COMPONENTS_DOM_DISTILLER_CORE_DOM_DISTILLER_OBSERVER_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "components/dom_distiller/core/article_entry.h"

namespace dom_distiller {

// Provides notifications for any mutations to entries of the reading list.
class DomDistillerObserver {
 public:
  // An update to an article entry.
  struct ArticleUpdate {
    enum UpdateType {
      ADD,
      UPDATE,
      REMOVE
    };
    std::string entry_id;
    UpdateType update_type;
  };

  virtual void ArticleEntriesUpdated(
      const std::vector<ArticleUpdate>& updates) = 0;

 protected:
  DomDistillerObserver() {}
  virtual ~DomDistillerObserver() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DomDistillerObserver);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CORE_DOM_DISTILLER_OBSERVER_H_
