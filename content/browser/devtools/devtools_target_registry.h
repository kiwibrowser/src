// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TARGET_REGISTRY_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TARGET_REGISTRY_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread_checker.h"
#include "base/unguessable_token.h"

namespace content {

class WebContents;

class DevToolsTargetRegistry {
 public:
  struct TargetInfo {
    int child_id;
    int routing_id;
    int frame_tree_node_id;
    base::UnguessableToken devtools_token;
    base::UnguessableToken devtools_target_id;
  };

  class ObserverBase : public base::RefCounted<ObserverBase> {
   protected:
    friend class base::RefCounted<ObserverBase>;
    virtual ~ObserverBase() {}
  };
  using RegistrationHandle = scoped_refptr<ObserverBase>;

  // Impl thread methods
  class Resolver {
   public:
    virtual const TargetInfo* GetInfoByFrameTreeNodeId(int frame_node_id) = 0;
    virtual const TargetInfo* GetInfoByRenderFramePair(int child_id,
                                                       int routing_id) = 0;
    virtual ~Resolver() {}
  };

  // UI thread method
  RegistrationHandle RegisterWebContents(WebContents* web_contents);
  std::unique_ptr<Resolver> CreateResolver();

  explicit DevToolsTargetRegistry(
      scoped_refptr<base::SequencedTaskRunner> impl_task_runner);
  ~DevToolsTargetRegistry();

 private:
  void UnregisterWebContents(WebContents* web_contents);

  class ContentsObserver;
  class Impl;

  scoped_refptr<base::SequencedTaskRunner> impl_task_runner_;
  // Observers are owned by the clients and are cleaned up from this
  // map when destroyed.
  base::flat_map<WebContents*, ContentsObserver*> observers_;

  base::WeakPtr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsTargetRegistry);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TARGET_REGISTRY_H_
