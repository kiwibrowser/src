// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CONTENT_VIEW_DELEGATE_H_
#define SERVICES_CONTENT_VIEW_DELEGATE_H_

class GURL;

namespace content {

// A virtual interface which must be implemented as a backing for ViewImpl
// instances.
//
// This is the primary interface by which the Content Service delegates ViewImpl
// behavior out to WebContentsImpl in src/content. As such it is a transitional
// API which will be removed as soon as WebContentsImpl itself can be fully
// migrated into Content Service.
//
// Each instance of this interface is constructed by the ContentServiceDelegate
// implementation and owned by a ViewImpl.
class ViewDelegate {
 public:
  virtual ~ViewDelegate() {}

  // Navigates the content object to a new URL.
  virtual void Navigate(const GURL& url) = 0;
};

}  // namespace content

#endif  // SERVICES_CONTENT_VIEW_DELEGATE_H_
