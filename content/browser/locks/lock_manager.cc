// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/locks/lock_manager.h"

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using blink::mojom::LockMode;

namespace content {

namespace {

// Guaranteed to be smaller than any result of LockManager::NextLockId().
constexpr int64_t kPreemptiveLockId = 0;

// A LockHandle is passed to the client when a lock is granted. As long as the
// handle is held, the lock is held. Dropping the handle - either explicitly
// by script or by process termination - causes the lock to be released. The
// connection can also be closed here when a lock is stolen.
class LockHandleImpl final : public blink::mojom::LockHandle {
 public:
  static mojo::StrongBindingPtr<blink::mojom::LockHandle> Create(
      base::WeakPtr<LockManager> context,
      const url::Origin& origin,
      int64_t lock_id,
      blink::mojom::LockHandlePtr* ptr) {
    return mojo::MakeStrongBinding(
        std::make_unique<LockHandleImpl>(std::move(context), origin, lock_id),
        mojo::MakeRequest(ptr));
  }

  LockHandleImpl(base::WeakPtr<LockManager> context,
                 const url::Origin& origin,
                 int64_t lock_id)
      : context_(context), origin_(origin), lock_id_(lock_id) {}

  ~LockHandleImpl() override {
    if (context_)
      context_->ReleaseLock(origin_, lock_id_);
  }

  // Called when the handle will be released from this end of the pipe. It
  // nulls out the context so that the lock will not be double-released.
  void Close() { context_.reset(); }

 private:
  base::WeakPtr<LockManager> context_;
  const url::Origin origin_;
  const int64_t lock_id_;

  DISALLOW_COPY_AND_ASSIGN(LockHandleImpl);
};

}  // namespace

// A requested or held lock. When granted, a LockHandle will be minted
// and passed to the held callback. Eventually the client will drop the
// handle, which will notify the context and remove this.
class LockManager::Lock {
 public:
  Lock(const std::string& name,
       LockMode mode,
       int64_t lock_id,
       const std::string& client_id,
       blink::mojom::LockRequestPtr request)
      : name_(name),
        mode_(mode),
        client_id_(client_id),
        lock_id_(lock_id),
        request_(std::move(request)) {}

  ~Lock() = default;

  // Abort a lock request.
  void Abort(const std::string& message) {
    DCHECK(request_);
    DCHECK(!handle_);

    request_->Abort(message);
    request_ = nullptr;
  }

  // Grant a lock request. This mints a LockHandle and returns it over the
  // request pipe.
  void Grant(base::WeakPtr<LockManager> context, const url::Origin& origin) {
    DCHECK(context);
    DCHECK(request_);
    DCHECK(!handle_);

    // Get a new ID when granted, to maintain map in grant order.
    lock_id_ = context->NextLockId();

    blink::mojom::LockHandlePtr ptr;
    handle_ =
        LockHandleImpl::Create(std::move(context), origin, lock_id_, &ptr);
    request_->Granted(std::move(ptr));
    request_ = nullptr;
  }

  // Break a granted lock. This terminates the connection, signaling an error
  // on the other end of the pipe.
  void Break() {
    DCHECK(!request_);
    DCHECK(handle_);

    LockHandleImpl* impl = static_cast<LockHandleImpl*>(handle_->impl());
    // Explicitly close the LockHandle first; this ensures that when the
    // connection is subsequently closed it will not re-entrantly try to drop
    // the lock.
    impl->Close();
    handle_->Close();
  }

  const std::string& name() const { return name_; }
  LockMode mode() const { return mode_; }
  int64_t lock_id() const { return lock_id_; }
  const std::string& client_id() const { return client_id_; }

 private:
  const std::string name_;
  const LockMode mode_;
  const std::string client_id_;

  // |lock_id_| is assigned when the request is arrives, and serves as the key
  // in an ordered request map. If it is a PREEMPT request, it is given the
  // special kPreemptiveLockId value which precedes all others, and is
  // processed immediately. When the lock is granted, a new id is generated to
  // be the key in the ordered map of held locks.
  int64_t lock_id_;

  // Exactly one of the following is non-null at any given time.

  // |request_| is valid until the lock is granted (or failure).
  blink::mojom::LockRequestPtr request_;

  // Once granted, |handle_| holds this end of the pipe that lets us monitor
  // for the other end going away.
  mojo::StrongBindingPtr<blink::mojom::LockHandle> handle_;
};

LockManager::LockManager() : weak_ptr_factory_(this) {}

LockManager::~LockManager() = default;

class LockManager::OriginState {
 public:
  OriginState() = default;
  ~OriginState() = default;

  const Lock* AddRequest(int64_t lock_id,
                         const std::string& name,
                         LockMode mode,
                         const std::string& client_id,
                         blink::mojom::LockRequestPtr request) {
    auto it = requested_.emplace(
        lock_id, std::make_unique<Lock>(name, mode, lock_id, client_id,
                                        std::move(request)));

    DCHECK(it.second) << "Insertion should have taken place";
    return it.first->second.get();
  }

  bool EraseLock(int64_t lock_id) {
    return requested_.erase(lock_id) || held_.erase(lock_id);
  }

  bool IsEmpty() const { return requested_.empty() && held_.empty(); }

#if DCHECK_IS_ON()
  bool IsHeld(int64_t lock_id) { return held_.find(lock_id) != held_.end(); }
#endif

  bool IsGrantable(const std::string& name, LockMode mode) const {
    if (mode == LockMode::EXCLUSIVE) {
      return !shared_.count(name) && !exclusive_.count(name);
    } else {
      return !exclusive_.count(name);
    }
  }

  // Break any held locks by that name.
  void Break(const std::string& name) {
    for (auto it = held_.begin(); it != held_.end();) {
      if (it->second->name() == name) {
        std::unique_ptr<Lock> lock = std::move(it->second);
        it = held_.erase(it);

        // Deleting the LockHandleImpl will signal an error on the other end
        // of the pipe, which will notify script that the lock was broken.
        lock->Break();
      } else {
        ++it;
      }
    }
  }

  void ProcessRequests(LockManager* lock_manager, const url::Origin& origin) {
    shared_.clear();
    exclusive_.clear();
    for (const auto& id_lock_pair : held_) {
      const auto& lock = id_lock_pair.second;
      MergeLockState(lock->name(), lock->mode());
    }

    for (auto it = requested_.begin(); it != requested_.end();) {
      auto& lock = it->second;

      bool granted = IsGrantable(lock->name(), lock->mode());

      MergeLockState(lock->name(), lock->mode());

      if (granted) {
        std::unique_ptr<Lock> grantee = std::move(lock);
        it = requested_.erase(it);
        grantee->Grant(lock_manager->weak_ptr_factory_.GetWeakPtr(), origin);
        held_.insert(std::make_pair(grantee->lock_id(), std::move(grantee)));
      } else {
        ++it;
      }
    }
  }

  std::vector<blink::mojom::LockInfoPtr> SnapshotRequested() const {
    std::vector<blink::mojom::LockInfoPtr> out;
    out.reserve(requested_.size());
    for (const auto& id_lock_pair : requested_) {
      const auto& lock = id_lock_pair.second;
      out.emplace_back(base::in_place, lock->name(), lock->mode(),
                       lock->client_id());
    }
    return out;
  }

  std::vector<blink::mojom::LockInfoPtr> SnapshotHeld() const {
    std::vector<blink::mojom::LockInfoPtr> out;
    out.reserve(held_.size());
    for (const auto& id_lock_pair : held_) {
      const auto& lock = id_lock_pair.second;
      out.emplace_back(base::in_place, lock->name(), lock->mode(),
                       lock->client_id());
    }
    return out;
  }

 private:
  void MergeLockState(const std::string& name, LockMode mode) {
    (mode == LockMode::SHARED ? shared_ : exclusive_).insert(name);
  }

  // These represent the actual state of locks in the origin.
  std::map<int64_t, std::unique_ptr<Lock>> requested_;
  std::map<int64_t, std::unique_ptr<Lock>> held_;

  // These sets represent what is held or requested, so that "IsGrantable"
  // tests are simple. These are cleared/rebuilt when the request queue
  // is processed.
  std::unordered_set<std::string> shared_;
  std::unordered_set<std::string> exclusive_;
};

void LockManager::CreateService(blink::mojom::LockManagerRequest request,
                                const url::Origin& origin) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // TODO(jsbell): This should reflect the 'environment id' from HTML,
  // and be the same opaque string seen in Service Worker client ids.
  const std::string client_id = base::GenerateGUID();

  bindings_.AddBinding(this, std::move(request), {origin, client_id});
}

void LockManager::RequestLock(const std::string& name,
                              LockMode mode,
                              WaitMode wait,
                              blink::mojom::LockRequestPtr request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (wait == WaitMode::PREEMPT && mode != LockMode::EXCLUSIVE) {
    mojo::ReportBadMessage("Invalid option combinaton");
    return;
  }

  if (name.length() > 0 && name[0] == '-') {
    mojo::ReportBadMessage("Reserved name");
    return;
  }

  const auto& context = bindings_.dispatch_context();
  int64_t lock_id =
      (wait == WaitMode::PREEMPT) ? kPreemptiveLockId : NextLockId();

  if (wait == WaitMode::PREEMPT) {
    Break(context.origin, name);
  } else if (wait == WaitMode::NO_WAIT &&
             !IsGrantable(context.origin, name, mode)) {
    request->Failed();
    return;
  }

  request.set_connection_error_handler(base::BindOnce(&LockManager::ReleaseLock,
                                                      base::Unretained(this),
                                                      context.origin, lock_id));
  const Lock* lock = origins_[context.origin].AddRequest(
      lock_id, name, mode, context.client_id, std::move(request));
  ProcessRequests(context.origin);

  DCHECK_GT(lock->lock_id(), kPreemptiveLockId)
      << "Preemptive lock should be assigned a new id";

#if DCHECK_IS_ON()
  DCHECK(wait != WaitMode::PREEMPT ||
         origins_[context.origin].IsHeld(lock->lock_id()))
      << "Preemptive lock should be granted immediately";
#endif
}

void LockManager::ReleaseLock(const url::Origin& origin, int64_t lock_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!base::ContainsKey(origins_, origin))
    return;
  OriginState& state = origins_[origin];

  bool dirty = state.EraseLock(lock_id);

  if (state.IsEmpty())
    origins_.erase(origin);
  else if (dirty)
    ProcessRequests(origin);
}

void LockManager::QueryState(QueryStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const url::Origin& origin = bindings_.dispatch_context().origin;
  if (!base::ContainsKey(origins_, origin)) {
    std::move(callback).Run(std::vector<blink::mojom::LockInfoPtr>(),
                            std::vector<blink::mojom::LockInfoPtr>());
    return;
  }

  OriginState& state = origins_[origin];
  std::move(callback).Run(state.SnapshotRequested(), state.SnapshotHeld());
}

bool LockManager::IsGrantable(const url::Origin& origin,
                              const std::string& name,
                              LockMode mode) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = origins_.find(origin);
  if (it == origins_.end())
    return true;
  return it->second.IsGrantable(name, mode);
}

int64_t LockManager::NextLockId() {
  int64_t lock_id = ++next_lock_id_;
  DCHECK_GT(lock_id, kPreemptiveLockId);
  return lock_id;
}

void LockManager::Break(const url::Origin& origin, const std::string& name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto it = origins_.find(origin);
  if (it != origins_.end())
    it->second.Break(name);
}

void LockManager::ProcessRequests(const url::Origin& origin) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::ContainsKey(origins_, origin))
    return;

  OriginState& state = origins_[origin];
  DCHECK(!state.IsEmpty());
  state.ProcessRequests(this, origin);
  DCHECK(!state.IsEmpty());
}

}  // namespace content
