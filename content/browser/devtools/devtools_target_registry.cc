// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_target_registry.h"

#include "base/task_runner.h"
#include "base/threading/thread_checker.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

namespace {

std::unique_ptr<const DevToolsTargetRegistry::TargetInfo> BuildTargetInfo(
    RenderFrameHost* rfh) {
  std::unique_ptr<DevToolsTargetRegistry::TargetInfo> info(
      new DevToolsTargetRegistry::TargetInfo());
  info->child_id = rfh->GetProcess()->GetID();
  info->routing_id = rfh->GetRoutingID();

  const FrameTreeNode* ftn =
      static_cast<RenderFrameHostImpl*>(rfh)->frame_tree_node();
  info->devtools_token = ftn->devtools_frame_token();
  info->frame_tree_node_id = ftn->frame_tree_node_id();

  // TODO(crbug.com/777516): this actually needs to return the nearest local
  // root (or self). However, for now we keep the existing semantics of request
  // matching in interceptor, which is intercepting all requests for given
  // web contents, that tests (and potentially some clients) depend on.
  // When those are fixed, the condition in the loop below should include
  // !rfh->IsCrossProcessSubframe().
  // Watch out for current (leaf) frame host being null
  // when it's being destroyed.
  while (!ftn->IsMainFrame()) {
    ftn = ftn->parent();
    rfh = ftn->current_frame_host();
  }
  info->devtools_target_id = ftn->devtools_frame_token();

  return std::move(info);
}

}  // namespace

class DevToolsTargetRegistry::Impl : public DevToolsTargetRegistry::Resolver {
 public:
  using TargetInfo = DevToolsTargetRegistry::TargetInfo;

  const DevToolsTargetRegistry::TargetInfo* GetInfoByFrameTreeNodeId(
      int frame_node_id) override {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    auto it = target_info_by_ftn_id_.find(frame_node_id);
    return it != target_info_by_ftn_id_.end() ? it->second : nullptr;
  }

  const DevToolsTargetRegistry::TargetInfo* GetInfoByRenderFramePair(
      int child_id,
      int routing_id) override {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    auto it = target_info_by_render_frame_pair_.find(
        std::make_pair(child_id, routing_id));
    return it != target_info_by_render_frame_pair_.end() ? it->second.get()
                                                         : nullptr;
  }

  void Add(std::unique_ptr<const TargetInfo> info) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    if (info->frame_tree_node_id != -1) {
      target_info_by_ftn_id_.insert(
          std::make_pair(info->frame_tree_node_id, info.get()));
    }
    auto key = std::make_pair(info->child_id, info->routing_id);
    target_info_by_render_frame_pair_.insert(
        std::make_pair(key, std::move(info)));
  }

  void Remove(const TargetInfo& info) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    if (info.frame_tree_node_id != -1)
      target_info_by_ftn_id_.erase(info.frame_tree_node_id);
    target_info_by_render_frame_pair_.erase(
        std::make_pair(info.child_id, info.routing_id));
  }

  void AddAll(std::vector<std::unique_ptr<const TargetInfo>> infos) {
    for (auto& info : infos)
      Add(std::move(info));
  }

  void RemoveAll(std::vector<std::unique_ptr<const TargetInfo>> infos) {
    for (auto& info : infos)
      Remove(*info);
  }

  void Update(std::unique_ptr<const TargetInfo> old_info,
              std::unique_ptr<const TargetInfo> new_info) {
    if (old_info)
      Remove(*old_info);
    if (new_info)
      Add(std::move(new_info));
  }

  Impl() : weak_factory_(this) { DETACH_FROM_THREAD(thread_checker_); }
  ~Impl() override = default;

 private:
  friend class DevToolsTargetRegistry;

  base::flat_map<std::pair<int, int>, std::unique_ptr<const TargetInfo>>
      target_info_by_render_frame_pair_;
  base::flat_map<int, const TargetInfo*> target_info_by_ftn_id_;
  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<DevToolsTargetRegistry::Impl> weak_factory_;
};

class DevToolsTargetRegistry::ContentsObserver : public ObserverBase,
                                                 public WebContentsObserver {
 public:
  ContentsObserver(WebContents* web_contents, DevToolsTargetRegistry* registry)
      : WebContentsObserver(web_contents), registry_(registry) {}

 private:
  void RenderFrameHostChanged(RenderFrameHost* old_host,
                              RenderFrameHost* new_host) override {
    std::unique_ptr<const TargetInfo> old_target;
    if (old_host)
      old_target = BuildTargetInfo(old_host);
    std::unique_ptr<const TargetInfo> new_target = BuildTargetInfo(new_host);
    registry_->impl_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&DevToolsTargetRegistry::Impl::Update, registry_->impl_,
                       std::move(old_target), std::move(new_target)));
  }

  void FrameDeleted(RenderFrameHost* render_frame_host) override {
    registry_->impl_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&DevToolsTargetRegistry::Impl::Update, registry_->impl_,
                       BuildTargetInfo(render_frame_host), nullptr));
  }

  void WebContentsDestroyed() override {
    NOTREACHED() << "DevToolsTarget Registry clients should be destroyed "
                    "before WebContents";
    registry_->UnregisterWebContents(web_contents());
  }

  ~ContentsObserver() override {
    if (web_contents())
      registry_->UnregisterWebContents(web_contents());
  }

  DevToolsTargetRegistry* registry_;
};

DevToolsTargetRegistry::RegistrationHandle
DevToolsTargetRegistry::RegisterWebContents(WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto it = observers_.find(web_contents);
  if (it != observers_.end())
    return it->second;

  scoped_refptr<ContentsObserver> observer =
      new DevToolsTargetRegistry::ContentsObserver(web_contents, this);
  observers_.insert(std::make_pair(web_contents, observer.get()));
  std::vector<std::unique_ptr<const TargetInfo>> infos;
  for (RenderFrameHost* render_frame_host : web_contents->GetAllFrames())
    infos.push_back(BuildTargetInfo(render_frame_host));

  impl_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DevToolsTargetRegistry::Impl::AddAll, impl_,
                                std::move(infos)));
  return observer;
}

std::unique_ptr<DevToolsTargetRegistry::Resolver>
DevToolsTargetRegistry::CreateResolver() {
  DCHECK(!impl_);

  auto impl = std::make_unique<Impl>();
  impl_ = impl->weak_factory_.GetWeakPtr();
  return std::move(impl);
}

void DevToolsTargetRegistry::UnregisterWebContents(WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  size_t count = observers_.erase(web_contents);
  DCHECK_NE(count, 0ul);

  std::vector<std::unique_ptr<const TargetInfo>> infos;
  for (RenderFrameHost* render_frame_host : web_contents->GetAllFrames())
    infos.push_back(BuildTargetInfo(render_frame_host));

  impl_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DevToolsTargetRegistry::Impl::RemoveAll, impl_,
                                std::move(infos)));
}

DevToolsTargetRegistry::DevToolsTargetRegistry(
    scoped_refptr<base::SequencedTaskRunner> impl_task_runner)
    : impl_task_runner_(impl_task_runner) {}

DevToolsTargetRegistry::~DevToolsTargetRegistry() = default;

}  // namespace content
