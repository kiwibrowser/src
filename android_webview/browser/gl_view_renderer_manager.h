// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_GL_VIEW_RENDERER_MANAGER_H_
#define ANDROID_WEBVIEW_BROWSER_GL_VIEW_RENDERER_MANAGER_H_

#include <list>

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"

namespace android_webview {

class RenderThreadManager;

class GLViewRendererManager {
 public:
  typedef RenderThreadManager* RendererType;

 private:
  typedef std::list<RendererType> ListType;

 public:
  typedef ListType::iterator Key;

  static GLViewRendererManager* GetInstance();

  Key NullKey();

  Key PushBack(RendererType view);

  // |key| must be already in manager. Move renderer corresponding to |key| to
  // most recent.
  void DidDrawGL(Key key);

  void Remove(Key key);

  RendererType GetMostRecentlyDrawn() const;

 private:
  friend struct base::LazyInstanceTraitsBase<GLViewRendererManager>;

  GLViewRendererManager();
  ~GLViewRendererManager();

  mutable base::Lock lock_;
  ListType mru_list_;

  DISALLOW_COPY_AND_ASSIGN(GLViewRendererManager);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_GL_VIEW_RENDERER_MANAGER_H_
