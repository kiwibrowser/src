// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/find_request_manager.h"

#include <algorithm>

#include "base/containers/queue.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/associated_interface_provider_impl.h"
#include "content/common/frame_messages.h"
#include "content/public/browser/guest_mode.h"
#include "third_party/blink/public/mojom/frame/find_in_page.mojom.h"

namespace content {

namespace {

// The following functions allow traversal over all frames, including those
// across WebContentses.
//
// Note that there are currently two different ways in which an inner
// WebContents may be embedded in an outer WebContents:
//
// 1) As a guest of the outer WebContents's BrowserPluginEmbedder.
// 2) Within an inner WebContentsTreeNode of the outer WebContents's
//    WebContentsTreeNode.

// Returns all child frames of |node|.
std::vector<FrameTreeNode*> GetChildren(FrameTreeNode* node) {
  std::vector<FrameTreeNode*> children(node->child_count());
  for (size_t i = 0; i != node->child_count(); ++i)
    children[i] = node->child_at(i);

  if (auto* contents = WebContentsImpl::FromOuterFrameTreeNode(node)) {
    children.push_back(contents->GetMainFrame()->frame_tree_node());
  } else {
    contents = WebContentsImpl::FromFrameTreeNode(node);
    if (node->IsMainFrame() && contents->GetBrowserPluginEmbedder()) {
      for (auto* inner_contents : contents->GetInnerWebContents()) {
        children.push_back(inner_contents->GetMainFrame()->frame_tree_node());
      }
    }
  }

  return children;
}

// Returns the first child FrameTreeNode under |node|, if |node| has a child, or
// nullptr otherwise.
FrameTreeNode* GetFirstChild(FrameTreeNode* node) {
  auto children = GetChildren(node);
  if (!children.empty())
    return children.front();
  return nullptr;
}

// Returns the last child FrameTreeNode under |node|, if |node| has a child, or
// nullptr otherwise.
FrameTreeNode* GetLastChild(FrameTreeNode* node) {
  auto children = GetChildren(node);
  if (!children.empty())
    return children.back();
  return nullptr;
}

// Returns the deepest last child frame under |node|/|rfh| in the frame tree.
FrameTreeNode* GetDeepestLastChild(FrameTreeNode* node) {
  while (FrameTreeNode* last_child = GetLastChild(node))
    node = last_child;
  return node;
}
RenderFrameHost* GetDeepestLastChild(RenderFrameHost* rfh) {
  FrameTreeNode* node =
      static_cast<RenderFrameHostImpl*>(rfh)->frame_tree_node();
  return GetDeepestLastChild(node)->current_frame_host();
}

// Returns the parent FrameTreeNode of |node|, if |node| has a parent, or
// nullptr otherwise.
FrameTreeNode* GetParent(FrameTreeNode* node) {
  if (node->parent())
    return node->parent();

  // The parent frame may be in another WebContents.
  if (node->IsMainFrame()) {
    auto* contents = WebContentsImpl::FromFrameTreeNode(node);
    if (GuestMode::IsCrossProcessFrameGuest(contents)) {
      int node_id = contents->GetOuterDelegateFrameTreeNodeId();
      if (node_id != FrameTreeNode::kFrameTreeNodeInvalidId)
        return FrameTreeNode::GloballyFindByID(node_id);
    } else if (auto* outer_contents = contents->GetOuterWebContents()) {
      return outer_contents->GetMainFrame()->frame_tree_node();
    }
  }

  return nullptr;
}

// Returns the previous sibling FrameTreeNode of |node|, if one exists, or
// nullptr otherwise.
FrameTreeNode* GetPreviousSibling(FrameTreeNode* node) {
  if (FrameTreeNode* previous_sibling = node->PreviousSibling())
    return previous_sibling;

  // The previous sibling may be in another WebContents.
  if (FrameTreeNode* parent = GetParent(node)) {
    auto children = GetChildren(parent);
    auto it = std::find(children.begin(), children.end(), node);
    // It is odd that this node may not be a child of its parent, but this is
    // actually possible during teardown, hence the need for the check for
    // "it != children.end()".
    if (it != children.end() && it != children.begin())
      return *--it;
  }

  return nullptr;
}

// Returns the next sibling FrameTreeNode of |node|, if one exists, or nullptr
// otherwise.
FrameTreeNode* GetNextSibling(FrameTreeNode* node) {
  if (FrameTreeNode* next_sibling = node->NextSibling())
    return next_sibling;

  // The next sibling may be in another WebContents.
  if (FrameTreeNode* parent = GetParent(node)) {
    auto children = GetChildren(parent);
    auto it = std::find(children.begin(), children.end(), node);
    // It is odd that this node may not be a child of its parent, but this is
    // actually possible during teardown, hence the need for the check for
    // "it != children.end()".
    if (it != children.end() && ++it != children.end())
      return *it;
  }

  return nullptr;
}

// Returns the FrameTreeNode directly after |node| in the frame tree in search
// order, or nullptr if one does not exist. If |wrap| is set, then wrapping
// between the first and last frames is permitted. Note that this traversal
// follows the same ordering as in blink::FrameTree::traverseNextWithWrap().
FrameTreeNode* TraverseNext(FrameTreeNode* node, bool wrap) {
  if (FrameTreeNode* first_child = GetFirstChild(node))
    return first_child;

  FrameTreeNode* sibling = GetNextSibling(node);
  while (!sibling) {
    FrameTreeNode* parent = GetParent(node);
    if (!parent)
      return wrap ? node : nullptr;
    node = parent;
    sibling = GetNextSibling(node);
  }
  return sibling;
}

// Returns the FrameTreeNode directly before |node| in the frame tree in search
// order, or nullptr if one does not exist. If |wrap| is set, then wrapping
// between the first and last frames is permitted. Note that this traversal
// follows the same ordering as in blink::FrameTree::traversePreviousWithWrap().
FrameTreeNode* TraversePrevious(FrameTreeNode* node, bool wrap) {
  if (FrameTreeNode* previous_sibling = GetPreviousSibling(node))
    return GetDeepestLastChild(previous_sibling);
  if (FrameTreeNode* parent = GetParent(node))
    return parent;
  return wrap ? GetDeepestLastChild(node) : nullptr;
}

// The same as either TraverseNext() or TraversePrevious(), depending on
// |forward|.
FrameTreeNode* TraverseNode(FrameTreeNode* node, bool forward, bool wrap) {
  return forward ? TraverseNext(node, wrap) : TraversePrevious(node, wrap);
}

}  // namespace

// Observes searched WebContentses for frame changes, including deletion,
// creation, and navigation.
class FindRequestManager::FrameObserver : public WebContentsObserver {
 public:
  FrameObserver(WebContentsImpl* web_contents, FindRequestManager* manager)
      : WebContentsObserver(web_contents), manager_(manager) {}

  ~FrameObserver() override {}

  void DidFinishLoad(RenderFrameHost* rfh, const GURL& validated_url) override {
    if (manager_->current_session_id_ == kInvalidId)
      return;

    manager_->RemoveFrame(rfh);
    manager_->AddFrame(rfh, true /* force */);
  }

  void RenderFrameDeleted(RenderFrameHost* rfh) override {
    manager_->RemoveFrame(rfh);
  }

  void RenderFrameHostChanged(RenderFrameHost* old_host,
                              RenderFrameHost* new_host) override {
    manager_->RemoveFrame(old_host);
  }

  void FrameDeleted(RenderFrameHost* rfh) override {
    manager_->RemoveFrame(rfh);
  }

 private:
  // The FindRequestManager that owns this FrameObserver.
  FindRequestManager* const manager_;

  DISALLOW_COPY_AND_ASSIGN(FrameObserver);
};

#if defined(OS_ANDROID)
FindRequestManager::ActivateNearestFindResultState::
ActivateNearestFindResultState() = default;
FindRequestManager::ActivateNearestFindResultState::
    ActivateNearestFindResultState(float x, float y)
    : current_request_id(GetNextID()), point(x, y) {}
FindRequestManager::ActivateNearestFindResultState::
~ActivateNearestFindResultState() {}

FindRequestManager::FrameRects::FrameRects() = default;
FindRequestManager::FrameRects::FrameRects(const std::vector<gfx::RectF>& rects,
                                           int version)
    : rects(rects), version(version) {}
FindRequestManager::FrameRects::~FrameRects() {}

FindRequestManager::FindMatchRectsState::FindMatchRectsState() = default;
FindRequestManager::FindMatchRectsState::~FindMatchRectsState() {}
#endif

// static
const int FindRequestManager::kInvalidId = -1;

FindRequestManager::FindRequestManager(WebContentsImpl* web_contents)
    : contents_(web_contents),
      current_session_id_(kInvalidId),
      pending_find_next_reply_(nullptr),
      pending_active_match_ordinal_(false),
      number_of_matches_(0),
      active_frame_(nullptr),
      relative_active_match_ordinal_(0),
      active_match_ordinal_(0),
      last_reported_id_(kInvalidId) {}

FindRequestManager::~FindRequestManager() {}

void FindRequestManager::Find(int request_id,
                              const base::string16& search_text,
                              const blink::WebFindOptions& options) {
  // Every find request must have a unique ID, and these IDs must strictly
  // increase so that newer requests always have greater IDs than older
  // requests.
  DCHECK_GT(request_id, current_request_.id);
  DCHECK_GT(request_id, current_session_id_);

  // If this is a new find session, clear any queued requests from last session.
  if (!options.find_next)
    find_request_queue_ = base::queue<FindRequest>();

  find_request_queue_.emplace(request_id, search_text, options);
  if (find_request_queue_.size() == 1)
    FindInternal(find_request_queue_.front());
}

void FindRequestManager::StopFinding(StopFindAction action) {
  for (WebContentsImpl* contents : contents_->GetWebContentsAndAllInner()) {
    for (FrameTreeNode* node : contents->GetFrameTree()->Nodes()) {
      RenderFrameHostImpl* rfh = node->current_frame_host();
      if (!CheckFrame(rfh) || !rfh->IsRenderFrameLive())
        continue;
      rfh->GetFindInPage()->StopFinding(
          static_cast<blink::mojom::StopFindAction>(action));
    }
  }

  current_session_id_ = kInvalidId;
#if defined(OS_ANDROID)
  // It is important that these pending replies are cleared whenever a find
  // session ends, so that subsequent replies for the old session are ignored.
  activate_.pending_replies.clear();
  match_rects_.pending_replies.clear();
#endif
}

void FindRequestManager::OnFindReply(RenderFrameHostImpl* rfh,
                                     int request_id,
                                     int number_of_matches,
                                     const gfx::Rect& selection_rect,
                                     int active_match_ordinal,
                                     bool final_update) {
  // Ignore stale replies from abandoned find sessions or dead frames.
  if (current_session_id_ == kInvalidId ||
      request_id < current_session_id_ ||
      !CheckFrame(rfh)) {
    return;
  }

  // Update the stored find results.

  DCHECK_GE(number_of_matches, -1);
  DCHECK_GE(active_match_ordinal, -1);

  // Check for an update to the number of matches.
  if (number_of_matches != -1) {
    DCHECK_GE(number_of_matches, 0);
    auto matches_per_frame_it = matches_per_frame_.find(rfh);
    if (int matches_delta = number_of_matches - matches_per_frame_it->second) {
      // Increment the global number of matches by the number of additional
      // matches found for this frame.
      number_of_matches_ += matches_delta;
      matches_per_frame_it->second = number_of_matches;

      // All matches may have been removed since the last find reply.
      if (rfh == active_frame_ && !number_of_matches)
        relative_active_match_ordinal_ = 0;

      // The active match ordinal may need updating since the number of matches
      // before the active match may have changed.
      if (rfh != active_frame_ || !number_of_matches)
        UpdateActiveMatchOrdinal();
    }
  }

  // Check for an update to the selection rect.
  if (!selection_rect.IsEmpty())
    selection_rect_ = selection_rect;

  // Check for an update to the active match ordinal.
  if (active_match_ordinal > 0) {
    if (rfh == active_frame_) {
      active_match_ordinal_ +=
          active_match_ordinal - relative_active_match_ordinal_;
      relative_active_match_ordinal_ = active_match_ordinal;
    } else {
      if (active_frame_) {
        // The new active match is in a different frame than the previous, so
        // the previous active frame needs to be informed (to clear its active
        // match highlighting).
        ClearActiveFindMatch();
      }
      active_frame_ = rfh;
      relative_active_match_ordinal_ = active_match_ordinal;
      UpdateActiveMatchOrdinal();
    }
    if (pending_active_match_ordinal_ && request_id == current_request_.id)
      pending_active_match_ordinal_ = false;
    AdvanceQueue(request_id);
  }

  if (!final_update) {
    NotifyFindReply(request_id, false /* final_update */);
    return;
  }

  // This is the final update for this frame for the current find operation.

  pending_initial_replies_.erase(rfh);
  if (request_id == current_session_id_ && !pending_initial_replies_.empty()) {
    NotifyFindReply(request_id, false /* final_update */);
    return;
  }

  // This is the final update for the current find operation.

  if (request_id == current_request_.id && request_id != current_session_id_) {
    DCHECK(current_request_.options.find_next);
    DCHECK_EQ(pending_find_next_reply_, rfh);
    pending_find_next_reply_ = nullptr;
  }

  FinalUpdateReceived(request_id, rfh);
}

void FindRequestManager::OnActivateNearestFindResultReply(
    RenderFrameHostImpl* rfh,
    int request_id,
    const gfx::Rect& active_match_rect,
    int number_of_matches,
    int active_match_ordinal,
    bool final_update) {
  if (active_match_ordinal > 0)
    contents_->SetFocusedFrame(rfh->frame_tree_node(), rfh->GetSiteInstance());
  OnFindReply(rfh, request_id, number_of_matches, active_match_rect,
              active_match_ordinal, final_update);
}

void FindRequestManager::RemoveFrame(RenderFrameHost* rfh) {
  if (current_session_id_ == kInvalidId || !CheckFrame(rfh))
    return;

  // If matches are counted for the frame that is being removed, decrement the
  // match total before erasing that entry.
  auto it = matches_per_frame_.find(rfh);
  if (it != matches_per_frame_.end()) {
    number_of_matches_ -= it->second;
    matches_per_frame_.erase(it);
  }

  // Update the active match ordinal, since it may have changed.
  if (active_frame_ == rfh) {
    active_frame_ = nullptr;
    relative_active_match_ordinal_ = 0;
    selection_rect_ = gfx::Rect();
  }
  UpdateActiveMatchOrdinal();

#if defined(OS_ANDROID)
  // The removed frame may contain the nearest find result known so far. Note
  // that once all queried frames have responded, if this result was the overall
  // nearest, then no activation will occur.
  if (rfh == activate_.nearest_frame)
    activate_.nearest_frame = nullptr;

  // Match rects in the removed frame are no longer relevant.
  if (match_rects_.frame_rects.count(rfh)) {
    match_rects_.frame_rects.erase(rfh);
    ++match_rects_.known_version;
  }

  // A reply should not be expected from the removed frame.
  RemoveNearestFindResultPendingReply(rfh);
  RemoveFindMatchRectsPendingReply(rfh);
#endif

  // If no pending find replies are expected for the removed frame, then just
  // report the updated results.
  if (!pending_initial_replies_.count(rfh) && pending_find_next_reply_ != rfh) {
    bool final_update =
        pending_initial_replies_.empty() && !pending_find_next_reply_;
    NotifyFindReply(current_session_id_, final_update);
    return;
  }

  if (pending_initial_replies_.count(rfh)) {
    // A reply should not be expected from the removed frame.
    pending_initial_replies_.erase(rfh);
    if (pending_initial_replies_.empty()) {
      FinalUpdateReceived(current_session_id_, rfh);
    }
  }

  if (pending_find_next_reply_ == rfh) {
    // A reply should not be expected from the removed frame.
    pending_find_next_reply_ = nullptr;
    FinalUpdateReceived(current_request_.id, rfh);
  }
}

void FindRequestManager::ClearActiveFindMatch() {
  active_frame_->GetFindInPage()->ClearActiveFindMatch();
}

#if defined(OS_ANDROID)
void FindRequestManager::ActivateNearestFindResult(float x, float y) {
  if (current_session_id_ == kInvalidId)
    return;

  activate_ = ActivateNearestFindResultState(x, y);

  // Request from each frame the distance to the nearest find result (in that
  // frame) from the point (x, y), defined in find-in-page coordinates.
  for (WebContentsImpl* contents : contents_->GetWebContentsAndAllInner()) {
    for (FrameTreeNode* node : contents->GetFrameTree()->Nodes()) {
      RenderFrameHostImpl* rfh = node->current_frame_host();

      if (!CheckFrame(rfh) || !rfh->IsRenderFrameLive())
        continue;

      activate_.pending_replies.insert(rfh);
      // Lifetime of FindRequestManager > RenderFrameHost > Mojo connection,
      // so it's safe to bind |this| and |rfh|.
      rfh->GetFindInPage()->GetNearestFindResult(
          activate_.point,
          base::BindOnce(&FindRequestManager::OnGetNearestFindResultReply,
                         base::Unretained(this), rfh,
                         activate_.current_request_id));
    }
  }
}

void FindRequestManager::OnGetNearestFindResultReply(RenderFrameHostImpl* rfh,
                                                     int request_id,
                                                     float distance) {
  if (request_id != activate_.current_request_id ||
      !activate_.pending_replies.count(rfh)) {
    return;
  }

  // Check if this frame has a nearer find result than the current nearest.
  if (distance < activate_.nearest_distance) {
    activate_.nearest_frame = rfh;
    activate_.nearest_distance = distance;
  }

  RemoveNearestFindResultPendingReply(rfh);
}

void FindRequestManager::RequestFindMatchRects(int current_version) {
  match_rects_.pending_replies.clear();
  match_rects_.request_version = current_version;

  // Request the latest find match rects from each frame.
  for (WebContentsImpl* contents : contents_->GetWebContentsAndAllInner()) {
    for (FrameTreeNode* node : contents->GetFrameTree()->Nodes()) {
      RenderFrameHostImpl* rfh = node->current_frame_host();

      if (!CheckFrame(rfh) || !rfh->IsRenderFrameLive())
        continue;

      match_rects_.pending_replies.insert(rfh);
      auto it = match_rects_.frame_rects.find(rfh);
      int version = (it != match_rects_.frame_rects.end()) ? it->second.version
                                                           : kInvalidId;
      rfh->GetFindInPage()->FindMatchRects(
          version, base::BindOnce(&FindRequestManager::OnFindMatchRectsReply,
                                  base::Unretained(this), rfh));
    }
  }
}

void FindRequestManager::OnFindMatchRectsReply(
    RenderFrameHost* rfh,
    int version,
    const std::vector<gfx::RectF>& rects,
    const gfx::RectF& active_rect) {
  auto it = match_rects_.frame_rects.find(rfh);
  if (it == match_rects_.frame_rects.end() || it->second.version != version) {
    // New version of rects has been received, so update the data.
    match_rects_.frame_rects[rfh] = FrameRects(rects, version);
    ++match_rects_.known_version;
  }
  if (!active_rect.IsEmpty())
    match_rects_.active_rect = active_rect;
  RemoveFindMatchRectsPendingReply(rfh);
}
#endif

void FindRequestManager::Reset(const FindRequest& initial_request) {
  current_session_id_ = initial_request.id;
  current_request_ = initial_request;
  pending_initial_replies_.clear();
  pending_find_next_reply_ = nullptr;
  pending_active_match_ordinal_ = true;
  matches_per_frame_.clear();
  number_of_matches_ = 0;
  active_frame_ = nullptr;
  relative_active_match_ordinal_ = 0;
  active_match_ordinal_ = 0;
  selection_rect_ = gfx::Rect();
  last_reported_id_ = kInvalidId;
  frame_observers_.clear();
#if defined(OS_ANDROID)
  activate_ = ActivateNearestFindResultState();
  match_rects_.pending_replies.clear();
#endif
}

void FindRequestManager::FindInternal(const FindRequest& request) {
  DCHECK_GT(request.id, current_request_.id);
  DCHECK_GT(request.id, current_session_id_);

  if (request.options.find_next) {
    // This is a find next operation.

    // This implies that there is an ongoing find session with the same search
    // text.
    DCHECK_GE(current_session_id_, 0);
    DCHECK_EQ(request.search_text, current_request_.search_text);

    // The find next request will be directed at the focused frame if there is
    // one, or the first frame with matches otherwise.
    RenderFrameHost* target_rfh =
        contents_->GetFocusedWebContents()->GetFocusedFrame();
    if (!target_rfh || !CheckFrame(target_rfh))
      target_rfh = GetInitialFrame(request.options.forward);

    SendFindIPC(request, target_rfh);
    current_request_ = request;
    pending_active_match_ordinal_ = true;
    return;
  }

  // This is an initial find operation.
  Reset(request);
  for (WebContentsImpl* contents : contents_->GetWebContentsAndAllInner()) {
    frame_observers_.push_back(std::make_unique<FrameObserver>(contents, this));
    for (FrameTreeNode* node : contents->GetFrameTree()->Nodes()) {
      AddFrame(node->current_frame_host(), false /* force */);
    }
  }
}

void FindRequestManager::AdvanceQueue(int request_id) {
  if (find_request_queue_.empty() ||
      request_id != find_request_queue_.front().id) {
    return;
  }

  find_request_queue_.pop();
  if (!find_request_queue_.empty())
    FindInternal(find_request_queue_.front());
}

void FindRequestManager::SendFindIPC(const FindRequest& request,
                                     RenderFrameHost* rfh) {
  DCHECK(CheckFrame(rfh));
  DCHECK(rfh->IsRenderFrameLive());

  if (request.options.find_next)
    pending_find_next_reply_ = rfh;
  else
    pending_initial_replies_.insert(rfh);

  rfh->Send(new FrameMsg_Find(rfh->GetRoutingID(), request.id,
                              request.search_text, request.options));
}

void FindRequestManager::NotifyFindReply(int request_id, bool final_update) {
  if (request_id == kInvalidId) {
    NOTREACHED();
    return;
  }

  // Ensure that replies are not reported with IDs lower than the ID of the
  // latest request we have results from.
  if (request_id < last_reported_id_)
    request_id = last_reported_id_;
  else
    last_reported_id_ = request_id;

  contents_->NotifyFindReply(request_id, number_of_matches_, selection_rect_,
                             active_match_ordinal_, final_update);
}

RenderFrameHost* FindRequestManager::GetInitialFrame(bool forward) const {
  RenderFrameHost* rfh = contents_->GetMainFrame();

  if (!forward)
    rfh = GetDeepestLastChild(rfh);

  return rfh;
}

RenderFrameHost* FindRequestManager::Traverse(RenderFrameHost* from_rfh,
                                              bool forward,
                                              bool matches_only,
                                              bool wrap) const {
  DCHECK(from_rfh);
  FrameTreeNode* node =
      static_cast<RenderFrameHostImpl*>(from_rfh)->frame_tree_node();

  while ((node = TraverseNode(node, forward, wrap)) != nullptr) {
    if (!CheckFrame(node->current_frame_host()))
      continue;
    RenderFrameHost* current_rfh = node->current_frame_host();
    if (!matches_only || matches_per_frame_.find(current_rfh)->second ||
        pending_initial_replies_.count(current_rfh)) {
      // Note that if there is still a pending reply expected for this frame,
      // then it may have unaccounted matches and will not be skipped via
      // |matches_only|.
      return node->current_frame_host();
    }
    if (wrap && node->current_frame_host() == from_rfh)
      return nullptr;
  }

  return nullptr;
}

void FindRequestManager::AddFrame(RenderFrameHost* rfh, bool force) {
  if (!rfh || !rfh->IsRenderFrameLive())
    return;

  // A frame that is already being searched should not normally be added again.
  DCHECK(force || !CheckFrame(rfh));

  matches_per_frame_[rfh] = 0;

  FindRequest request = current_request_;
  request.id = current_session_id_;
  request.options.find_next = false;
  request.options.force = force;
  SendFindIPC(request, rfh);
}

bool FindRequestManager::CheckFrame(RenderFrameHost* rfh) const {
  return rfh && matches_per_frame_.count(rfh);
}

void FindRequestManager::UpdateActiveMatchOrdinal() {
  active_match_ordinal_ = 0;

  if (!active_frame_ || !relative_active_match_ordinal_) {
    DCHECK(!active_frame_ && !relative_active_match_ordinal_);
    return;
  }

  // Traverse the frame tree backwards (in search order) and count all of the
  // matches in frames before the frame with the active match, in order to
  // determine the overall active match ordinal.
  RenderFrameHost* frame = active_frame_;
  while ((frame = Traverse(frame,
                           false /* forward */,
                           true /* matches_only */,
                           false /* wrap */)) != nullptr) {
    active_match_ordinal_ += matches_per_frame_[frame];
  }
  active_match_ordinal_ += relative_active_match_ordinal_;
}

void FindRequestManager::FinalUpdateReceived(int request_id,
                                             RenderFrameHost* rfh) {
  if (!number_of_matches_ ||
      (active_match_ordinal_ && !pending_active_match_ordinal_) ||
      pending_find_next_reply_) {
    // All the find results for |request_id| are in and ready to report. Note
    // that |final_update| will be set to false if there are still pending
    // replies expected from the initial find request.
    NotifyFindReply(request_id,
                    pending_initial_replies_.empty() /* final_update */);
    AdvanceQueue(request_id);
    return;
  }

  // There are matches, but no active match was returned, so another find next
  // request must be sent.

  RenderFrameHost* target_rfh;
  if (request_id == current_request_.id && current_request_.options.find_next) {
    // If this was a find next operation, then the active match will be in the
    // next frame with matches after this one.
    target_rfh = Traverse(rfh,
                          current_request_.options.forward,
                          true /* matches_only */,
                          true /* wrap */);
  } else if ((target_rfh =
                  contents_->GetFocusedWebContents()->GetFocusedFrame()) !=
             nullptr) {
    // Otherwise, if there is a focused frame, then the active match will be in
    // the next frame with matches after that one.
    target_rfh = Traverse(target_rfh,
                          current_request_.options.forward,
                          true /* matches_only */,
                          true /* wrap */);
  } else {
    // Otherwise, the first frame with matches will have the active match.
    target_rfh = GetInitialFrame(current_request_.options.forward);
    if (!CheckFrame(target_rfh) || !matches_per_frame_[target_rfh]) {
      target_rfh = Traverse(target_rfh,
                            current_request_.options.forward,
                            true /* matches_only */,
                            false /* wrap */);
    }
  }
  DCHECK(target_rfh);

  // Forward the find reply without |final_update| set because the active match
  // has not yet been found.
  NotifyFindReply(request_id, false /* final_update */);

  current_request_.options.find_next = true;
  SendFindIPC(current_request_, target_rfh);
}

#if defined(OS_ANDROID)
void FindRequestManager::RemoveNearestFindResultPendingReply(
    RenderFrameHost* rfh) {
  auto it = activate_.pending_replies.find(rfh);
  if (it == activate_.pending_replies.end())
    return;

  activate_.pending_replies.erase(it);
  if (activate_.pending_replies.empty() &&
      CheckFrame(activate_.nearest_frame)) {
    // Lifetime of FindRequestManager > activate_.nearest_frame  > Mojo
    // connection, so it's safe to bind |this| and |activate_.nearest_frame|
    activate_.nearest_frame->GetFindInPage()->ActivateNearestFindResult(
        activate_.point,
        base::BindOnce(&FindRequestManager::OnActivateNearestFindResultReply,
                       base::Unretained(this),
                       base::Unretained(activate_.nearest_frame),
                       current_session_id_));
  }
}

void FindRequestManager::RemoveFindMatchRectsPendingReply(
    RenderFrameHost* rfh) {
  auto it = match_rects_.pending_replies.find(rfh);
  if (it == match_rects_.pending_replies.end())
    return;

  match_rects_.pending_replies.erase(it);
  if (match_rects_.pending_replies.empty()) {
    // All replies are in.
    std::vector<gfx::RectF> aggregate_rects;
    if (match_rects_.request_version != match_rects_.known_version) {
      // Request version is stale, so aggregate and report the newer find
      // match rects. The rects should be aggregated in search order.
      for (RenderFrameHost* frame = GetInitialFrame(true /* forward */); frame;
           frame = Traverse(frame,
                            true /* forward */,
                            true /* matches_only */,
                            false /* wrap */)) {
        auto frame_it = match_rects_.frame_rects.find(frame);
        if (frame_it == match_rects_.frame_rects.end())
          continue;

        std::vector<gfx::RectF>& frame_rects = frame_it->second.rects;
        aggregate_rects.insert(
            aggregate_rects.end(), frame_rects.begin(), frame_rects.end());
      }
    }
    contents_->NotifyFindMatchRectsReply(
        match_rects_.known_version, aggregate_rects, match_rects_.active_rect);
  }
}
#endif

}  // namespace content
