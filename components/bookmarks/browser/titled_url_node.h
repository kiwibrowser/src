// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BOOKMARKS_BROWSER_TITLED_URL_NODE_H_
#define COMPONENTS_BOOKMARKS_BROWSER_TITLED_URL_NODE_H_

#include "url/gurl.h"

namespace bookmarks {

// TitledUrlNode is an interface for objects like bookmarks that expose a title
// and URL. TitledUrlNodes can be added to a BookmarkIndex to quickly retrieve
// all nodes that contain a particular word in their title or URL.
class TitledUrlNode {
 public:
  // Returns the title for the node.
  virtual const base::string16& GetTitledUrlNodeTitle() const = 0;

  // Returns the URL for the node.
  virtual const GURL& GetTitledUrlNodeUrl() const = 0;

 protected:
  virtual ~TitledUrlNode() {}
};

}  // namespace bookmarks

#endif  // COMPONENTS_BOOKMARKS_BROWSER_TITLED_URL_NODE_H_
