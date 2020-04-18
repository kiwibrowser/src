// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/dom_storage_area.h"

#include <inttypes.h>

#include <algorithm>
#include <cctype>  // for std::isalnum

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process_info.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "build/build_config.h"
#include "content/browser/dom_storage/dom_storage_namespace.h"
#include "content/browser/dom_storage/dom_storage_task_runner.h"
#include "content/browser/dom_storage/session_storage_database.h"
#include "content/browser/dom_storage/session_storage_database_adapter.h"
#include "content/browser/leveldb_wrapper_impl.h"
#include "content/common/dom_storage/dom_storage_map.h"
#include "content/common/dom_storage/dom_storage_types.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/database/database_util.h"
#include "storage/common/database/database_identifier.h"
#include "storage/common/fileapi/file_system_util.h"

using storage::DatabaseUtil;

namespace content {

namespace {

// To avoid excessive IO we apply limits to the amount of data being written
// and the frequency of writes. The specific values used are somewhat arbitrary.
constexpr int kMaxBytesPerHour = kPerStorageAreaQuota;
constexpr int kMaxCommitsPerHour = 60;

}  // namespace

bool DOMStorageArea::s_aggressive_flushing_enabled_ = false;

DOMStorageArea::RateLimiter::RateLimiter(size_t desired_rate,
                                         base::TimeDelta time_quantum)
    : rate_(desired_rate), samples_(0), time_quantum_(time_quantum) {
  DCHECK_GT(desired_rate, 0ul);
}

base::TimeDelta DOMStorageArea::RateLimiter::ComputeTimeNeeded() const {
  return time_quantum_ * (samples_ / rate_);
}

base::TimeDelta DOMStorageArea::RateLimiter::ComputeDelayNeeded(
    const base::TimeDelta elapsed_time) const {
  base::TimeDelta time_needed = ComputeTimeNeeded();
  if (time_needed > elapsed_time)
    return time_needed - elapsed_time;
  return base::TimeDelta();
}

DOMStorageArea::CommitBatch::CommitBatch() : clear_all_first(false) {}
DOMStorageArea::CommitBatch::~CommitBatch() {}

size_t DOMStorageArea::CommitBatch::GetDataSize() const {
  return DOMStorageMap::CountBytes(changed_values);
}

DOMStorageArea::CommitBatchHolder::CommitBatchHolder(
    Type type,
    scoped_refptr<CommitBatch> batch)
    : type(type), batch(batch) {}
DOMStorageArea::CommitBatchHolder::CommitBatchHolder(
    const DOMStorageArea::CommitBatchHolder& other) = default;
DOMStorageArea::CommitBatchHolder::~CommitBatchHolder() {}

// static
const base::FilePath::CharType DOMStorageArea::kDatabaseFileExtension[] =
    FILE_PATH_LITERAL(".localstorage");

// static
base::FilePath DOMStorageArea::DatabaseFileNameFromOrigin(
    const url::Origin& origin) {
  std::string filename = storage::GetIdentifierFromOrigin(origin);
  // There is no base::FilePath.AppendExtension() method, so start with just the
  // extension as the filename, and then InsertBeforeExtension the desired
  // name.
  return base::FilePath().Append(kDatabaseFileExtension).
      InsertBeforeExtensionASCII(filename);
}

// static
url::Origin DOMStorageArea::OriginFromDatabaseFileName(
    const base::FilePath& name) {
  DCHECK(name.MatchesExtension(kDatabaseFileExtension));
  std::string origin_id =
      name.BaseName().RemoveExtension().MaybeAsASCII();
  return storage::GetOriginFromIdentifier(origin_id);
}

void DOMStorageArea::EnableAggressiveCommitDelay() {
  s_aggressive_flushing_enabled_ = true;
  LevelDBWrapperImpl::EnableAggressiveCommitDelay();
}

DOMStorageArea::DOMStorageArea(const std::string& namespace_id,
                               std::vector<std::string> original_namespace_ids,
                               const url::Origin& origin,
                               SessionStorageDatabase* session_storage_backing,
                               DOMStorageTaskRunner* task_runner)
    : namespace_id_(namespace_id),
      original_namespace_ids_(std::move(original_namespace_ids)),
      origin_(origin),
      task_runner_(task_runner),
#if defined(OS_ANDROID)
      desired_load_state_(session_storage_backing ? LOAD_STATE_KEYS_ONLY
                                                  : LOAD_STATE_KEYS_AND_VALUES),
#else
      desired_load_state_(LOAD_STATE_KEYS_AND_VALUES),
#endif
      load_state_(session_storage_backing ? LOAD_STATE_UNLOADED
                                          : LOAD_STATE_KEYS_AND_VALUES),
      map_(new DOMStorageMap(
          kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance,
          desired_load_state_ == LOAD_STATE_KEYS_ONLY)),
      session_storage_backing_(session_storage_backing),
      is_shutdown_(false),
      start_time_(base::TimeTicks::Now()),
      data_rate_limiter_(kMaxBytesPerHour, base::TimeDelta::FromHours(1)),
      commit_rate_limiter_(kMaxCommitsPerHour, base::TimeDelta::FromHours(1)) {
  DCHECK(!namespace_id.empty());
  if (session_storage_backing) {
    backing_.reset(
        new SessionStorageDatabaseAdapter(session_storage_backing, namespace_id,
                                          original_namespace_ids_, origin));
  }
}

DOMStorageArea::~DOMStorageArea() {
}

void DOMStorageArea::ExtractValues(DOMStorageValuesMap* map) {
  if (is_shutdown_)
    return;

  if (load_state_ == LOAD_STATE_KEYS_AND_VALUES) {
    map_->ExtractValues(map);
    return;
  }
  LoadMapAndApplyUncommittedChangesIfNeeded(map);
}

unsigned DOMStorageArea::Length() {
  if (is_shutdown_)
    return 0;
  LoadMapAndApplyUncommittedChangesIfNeeded(nullptr);
  return map_->Length();
}

base::NullableString16 DOMStorageArea::Key(unsigned index) {
  if (is_shutdown_)
    return base::NullableString16();
  LoadMapAndApplyUncommittedChangesIfNeeded(nullptr);
  return map_->Key(index);
}

base::NullableString16 DOMStorageArea::GetItem(const base::string16& key) {
  if (is_shutdown_)
    return base::NullableString16();
  LoadMapAndApplyUncommittedChangesIfNeeded(nullptr);
  return map_->GetItem(key);
}

bool DOMStorageArea::SetItem(const base::string16& key,
                             const base::string16& value,
                             const base::NullableString16& client_old_value,
                             base::NullableString16* old_value) {
  if (is_shutdown_)
    return false;
  LoadMapAndApplyUncommittedChangesIfNeeded(nullptr);
  if (!map_->HasOneRef())
    map_ = map_->DeepCopy();
  bool success = map_->SetItem(key, value, old_value);
  if (map_->has_only_keys())
    *old_value = client_old_value;
  if (success && backing_ &&
      (old_value->is_null() || old_value->string() != value)) {
    CommitBatch* commit_batch = CreateCommitBatchIfNeeded();
    if (load_state_ == LOAD_STATE_KEYS_AND_VALUES) {
      // Values are populated later to avoid holding duplicate memory.
      commit_batch->changed_values[key] = base::NullableString16();
    } else {
      commit_batch->changed_values[key] = base::NullableString16(value, false);
    }
  }
  return success;
}

bool DOMStorageArea::RemoveItem(const base::string16& key,
                                const base::NullableString16& client_old_value,
                                base::string16* old_value) {
  if (is_shutdown_)
    return false;
  LoadMapAndApplyUncommittedChangesIfNeeded(nullptr);
  if (!map_->HasOneRef())
    map_ = map_->DeepCopy();
  bool success = map_->RemoveItem(key, old_value);
  if (map_->has_only_keys()) {
    DCHECK(!client_old_value.is_null());
    *old_value = client_old_value.string();
  }
  if (success && backing_) {
    CommitBatch* commit_batch = CreateCommitBatchIfNeeded();
    commit_batch->changed_values[key] = base::NullableString16();
  }
  return success;
}

bool DOMStorageArea::Clear() {
  if (is_shutdown_)
    return false;
  LoadMapAndApplyUncommittedChangesIfNeeded(nullptr);
  if (map_->Length() == 0)
    return false;

  map_ = new DOMStorageMap(
      kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance,
      desired_load_state_ == LOAD_STATE_KEYS_ONLY);

  if (backing_) {
    CommitBatch* commit_batch = CreateCommitBatchIfNeeded();
    commit_batch->clear_all_first = true;
    commit_batch->changed_values.clear();
  }

  return true;
}

void DOMStorageArea::FastClear() {
  if (is_shutdown_)
    return;

  map_ = new DOMStorageMap(
      kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance,
      desired_load_state_ == LOAD_STATE_KEYS_ONLY);
  // This ensures no load will happen while we're waiting to clear the data
  // from the database.
  load_state_ = desired_load_state_;

  if (backing_) {
    CommitBatch* commit_batch = CreateCommitBatchIfNeeded();
    commit_batch->clear_all_first = true;
    commit_batch->changed_values.clear();
  }
}

DOMStorageArea* DOMStorageArea::ShallowCopy(
    const std::string& destination_namespace_id) {
  DCHECK(!namespace_id_.empty());
  DCHECK(!destination_namespace_id.empty());

  std::vector<std::string> original_namespace_ids;
  original_namespace_ids.push_back(namespace_id_);
  original_namespace_ids.insert(original_namespace_ids.end(),
                                original_namespace_ids_.begin(),
                                original_namespace_ids_.end());
  DOMStorageArea* copy = new DOMStorageArea(
      destination_namespace_id, std::move(original_namespace_ids), origin_,
      session_storage_backing_.get(), task_runner_.get());
  copy->desired_load_state_ = desired_load_state_;
  copy->load_state_ = load_state_;
  copy->map_ = map_;
  copy->is_shutdown_ = is_shutdown_;

  // All the uncommitted changes to this area need to happen before the actual
  // shallow copy is made (scheduled by the upper layer sometime after return).
  if (GetCurrentCommitBatch())
    ScheduleImmediateCommit();
  if (load_state_ != LOAD_STATE_KEYS_AND_VALUES) {
    copy->commit_batches_ = commit_batches_;
    for (auto& it : copy->commit_batches_)
      it.type = CommitBatchHolder::TYPE_CLONE;
  }
  return copy;
}

bool DOMStorageArea::HasUncommittedChanges() const {
  return !commit_batches_.empty();
}

void DOMStorageArea::ScheduleImmediateCommit() {
  DCHECK(HasUncommittedChanges());
  PostCommitTask();
}

void DOMStorageArea::ClearShallowCopiedCommitBatches() {
  if (is_shutdown_)
    return;
  while (!commit_batches_.empty() &&
         commit_batches_.back().type == CommitBatchHolder::TYPE_CLONE) {
    commit_batches_.pop_back();
  }
  original_namespace_ids_.clear();
}

void DOMStorageArea::SetCacheOnlyKeys(bool only_keys) {
  LoadState new_desired_state =
      only_keys ? LOAD_STATE_KEYS_ONLY : LOAD_STATE_KEYS_AND_VALUES;
  if (is_shutdown_ || !backing_ || desired_load_state_ == new_desired_state)
    return;

  desired_load_state_ = new_desired_state;
  // Do not clear values immediately when desired state is set to keys only.
  // Either commit timer or a purge call will clear the map, in case new process
  // tries to open again. When values are desired it is ok to clear the map
  // immediately. The reload only happens when required.
  if (!map_->Length() || desired_load_state_ == LOAD_STATE_KEYS_AND_VALUES)
    UnloadMapIfDesired();
}

void DOMStorageArea::PurgeMemory() {
  DCHECK(!is_shutdown_);

  if (load_state_ == LOAD_STATE_UNLOADED ||  // We're not using any memory.
      !backing_.get() ||                     // We can't purge anything.
      HasUncommittedChanges())  // We leave things alone with changes pending.
    return;

  // Recreate the database object, this frees up the open sqlite connection
  // and its page cache.
  backing_->Reset();

  // Do not set load_state_ to |LOAD_STATE_UNLOADED| if map is empty since
  // FastClear is efficient with no reloads while waiting for clearing database.
  if (!map_ || !map_->Length())
    return;

  // Drop the in memory cache, we'll reload when needed.
  load_state_ = LOAD_STATE_UNLOADED;
  map_ = new DOMStorageMap(
      kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance,
      desired_load_state_ == LOAD_STATE_KEYS_ONLY);
}

void DOMStorageArea::UnloadMapIfDesired() {
  if (load_state_ == LOAD_STATE_UNLOADED || load_state_ == desired_load_state_)
    return;

  // Do not clear the map if there are uncommitted changes since the commit
  // batch might not have the values populated.
  if (!backing_ || HasUncommittedChanges())
    return;

  if (load_state_ == LOAD_STATE_KEYS_AND_VALUES) {
    scoped_refptr<DOMStorageMap> keys_values = map_;
    map_ = new DOMStorageMap(
        kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance,
        desired_load_state_ == LOAD_STATE_KEYS_ONLY);
    map_->TakeKeysFrom(keys_values->keys_values());
    load_state_ = LOAD_STATE_KEYS_ONLY;
    return;
  }

  map_ = new DOMStorageMap(
      kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance,
      desired_load_state_ == LOAD_STATE_KEYS_ONLY);
  load_state_ = LOAD_STATE_UNLOADED;
}

void DOMStorageArea::Shutdown() {
  if (is_shutdown_)
    return;
  is_shutdown_ = true;

  if (GetCurrentCommitBatch()) {
    DCHECK(backing_);
    PopulateCommitBatchValues();
  }

  map_ = nullptr;
  if (!backing_)
    return;

  bool success = task_runner_->PostShutdownBlockingTask(
      FROM_HERE, DOMStorageTaskRunner::COMMIT_SEQUENCE,
      base::BindOnce(&DOMStorageArea::ShutdownInCommitSequence, this));
  DCHECK(success);
}

bool DOMStorageArea::IsMapReloadNeeded() {
  return load_state_ < desired_load_state_;
}

void DOMStorageArea::OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd) {
  task_runner_->AssertIsRunningOnPrimarySequence();
  if (is_shutdown_ || load_state_ == LOAD_STATE_UNLOADED)
    return;

  // Limit the url length to 50 and strip special characters.
  std::string url = origin_.GetURL().spec().substr(0, 50);
  for (size_t index = 0; index < url.size(); ++index) {
    if (!std::isalnum(url[index]))
      url[index] = '_';
  }
  std::string name =
      base::StringPrintf("site_storage/%s/0x%" PRIXPTR, url.c_str(),
                         reinterpret_cast<uintptr_t>(this));

  const char* system_allocator_name =
      base::trace_event::MemoryDumpManager::GetInstance()
          ->system_allocator_pool_name();
  if (!commit_batches_.empty()) {
    size_t commit_batches_size = 0;
    for (const auto& it : commit_batches_)
      commit_batches_size += it.batch->GetDataSize();
    auto* commit_batch_mad = pmd->CreateAllocatorDump(name + "/commit_batch");
    commit_batch_mad->AddScalar(
        base::trace_event::MemoryAllocatorDump::kNameSize,
        base::trace_event::MemoryAllocatorDump::kUnitsBytes,
        commit_batches_size);
    if (system_allocator_name)
      pmd->AddSuballocation(commit_batch_mad->guid(), system_allocator_name);
  }

  // Do not add storage map usage if less than 1KB.
  if (map_->memory_used() < 1024)
    return;

  auto* map_mad = pmd->CreateAllocatorDump(name + "/storage_map");
  map_mad->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                     base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                     map_->memory_used());
  if (system_allocator_name)
    pmd->AddSuballocation(map_mad->guid(), system_allocator_name);
}

void DOMStorageArea::LoadMapAndApplyUncommittedChangesIfNeeded(
    DOMStorageValuesMap* map) {
  if (!backing_ || (!IsMapReloadNeeded() && !map))
    return;

  DOMStorageValuesMap read_values;
  auto most_recent_clear_all_iter = commit_batches_.begin();
  while (most_recent_clear_all_iter != commit_batches_.end()) {
    if (most_recent_clear_all_iter->batch->clear_all_first)
      break;
    ++most_recent_clear_all_iter;
  }

  if (most_recent_clear_all_iter == commit_batches_.end()) {
    base::TimeTicks before = base::TimeTicks::Now();
    backing_->ReadAllValues(&read_values);

    base::TimeDelta time_to_prime = base::TimeTicks::Now() - before;
    UMA_HISTOGRAM_TIMES("LocalStorage.BrowserTimeToPrimeLocalStorage",
                        time_to_prime);

    size_t local_storage_size_kb =
        DOMStorageMap::CountBytes(read_values) / 1024;
    // Track localStorage size, from 0-6MB. Note that the maximum size should be
    // 5MB, but we add some slop since we want to make sure the max size is
    // always above what we see in practice, since histograms can't change.
    UMA_HISTOGRAM_CUSTOM_COUNTS("LocalStorage.BrowserLocalStorageSizeInKB",
                                local_storage_size_kb, 1, 6 * 1024, 50);
    if (local_storage_size_kb < 100) {
      UMA_HISTOGRAM_TIMES(
          "LocalStorage.BrowserTimeToPrimeLocalStorageUnder100KB",
          time_to_prime);
    } else if (local_storage_size_kb < 1000) {
      UMA_HISTOGRAM_TIMES(
          "LocalStorage.BrowserTimeToPrimeLocalStorage100KBTo1MB",
          time_to_prime);
    } else {
      UMA_HISTOGRAM_TIMES("LocalStorage.BrowserTimeToPrimeLocalStorage1MBTo5MB",
                          time_to_prime);
    }
  }

  // Apply changes in reverse order of commit batches starting from the most
  // recent commit batch with clear all flag. It is possible that the changes in
  // one or more of the commit batches have already been written to the database
  // and reflected in the map returned by ReadAllValues(). It is okay to
  // re-apply these changes.
  auto it = most_recent_clear_all_iter == commit_batches_.end()
                ? commit_batches_.end()
                : ++most_recent_clear_all_iter;
  while (it != commit_batches_.begin()) {
    --it;
    for (const auto& item : it->batch->changed_values) {
      if (item.second.is_null())
        read_values.erase(item.first);
      else
        read_values[item.first] = item.second;
    }
  }

  if (!IsMapReloadNeeded()) {
    map->swap(read_values);
    return;
  }

  map_ = new DOMStorageMap(
      kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance,
      desired_load_state_ == LOAD_STATE_KEYS_ONLY);
  if (desired_load_state_ == LOAD_STATE_KEYS_ONLY) {
    map_->TakeKeysFrom(read_values);
    if (map)
      map->swap(read_values);
  } else {
    map_->SwapValues(&read_values);
    if (map)
      map_->ExtractValues(map);
  }
  load_state_ = desired_load_state_;
}

DOMStorageArea::CommitBatch* DOMStorageArea::CreateCommitBatchIfNeeded() {
  DCHECK(!is_shutdown_);
  DCHECK(backing_);
  if (!GetCurrentCommitBatch()) {
    commit_batches_.emplace_front(CommitBatchHolder(
        CommitBatchHolder::TYPE_CURRENT_BATCH, new CommitBatch()));
    BrowserThread::PostAfterStartupTask(
        FROM_HERE, task_runner_,
        base::BindOnce(&DOMStorageArea::StartCommitTimer, this));
  }
  return GetCurrentCommitBatch()->batch.get();
}

const DOMStorageArea::CommitBatchHolder* DOMStorageArea::GetCurrentCommitBatch()
    const {
  return (!commit_batches_.empty() &&
          commit_batches_.front().type == CommitBatchHolder::TYPE_CURRENT_BATCH)
             ? &commit_batches_.front()
             : nullptr;
}

bool DOMStorageArea::HasCommitBatchInFlight() const {
  for (const auto& batch : commit_batches_) {
    if (batch.type == CommitBatchHolder::TYPE_IN_FLIGHT)
      return true;
  }
  return false;
}

void DOMStorageArea::PopulateCommitBatchValues() {
  task_runner_->AssertIsRunningOnPrimarySequence();
  if (load_state_ != LOAD_STATE_KEYS_AND_VALUES)
    return;
  CommitBatch* current_batch = GetCurrentCommitBatch()->batch.get();
  for (auto& key_value : current_batch->changed_values)
    key_value.second = map_->GetItem(key_value.first);
}

void DOMStorageArea::StartCommitTimer() {
  if (is_shutdown_ || !GetCurrentCommitBatch())
    return;

  // Start a timer to commit any changes that accrue in the batch, but only if
  // no commits are currently in flight. In that case the timer will be
  // started after the commits have happened.
  if (HasCommitBatchInFlight())
    return;

  task_runner_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&DOMStorageArea::OnCommitTimer, this),
      ComputeCommitDelay());
}

base::TimeDelta DOMStorageArea::ComputeCommitDelay() const {
  if (s_aggressive_flushing_enabled_)
    return base::TimeDelta::FromSeconds(1);

  // Delay for a moment after a value is set in anticipation
  // of other values being set, so changes are batched.
  static constexpr base::TimeDelta kCommitDefaultDelaySecs =
      base::TimeDelta::FromSeconds(5);

  base::TimeDelta elapsed_time = base::TimeTicks::Now() - start_time_;
  base::TimeDelta delay =
      std::max(kCommitDefaultDelaySecs,
               std::max(commit_rate_limiter_.ComputeDelayNeeded(elapsed_time),
                        data_rate_limiter_.ComputeDelayNeeded(elapsed_time)));
  UMA_HISTOGRAM_LONG_TIMES("LocalStorage.CommitDelay", delay);
  return delay;
}

void DOMStorageArea::OnCommitTimer() {
  if (is_shutdown_)
    return;

  // It's possible that there is nothing to commit if an immediate
  // commit occured after the timer was scheduled but before it fired.
  if (!GetCurrentCommitBatch())
    return;

  PostCommitTask();
}

void DOMStorageArea::PostCommitTask() {
  if (is_shutdown_ || !GetCurrentCommitBatch())
    return;

  DCHECK(backing_.get());
  CommitBatchHolder& current_batch = commit_batches_.front();
  PopulateCommitBatchValues();
  current_batch.type = CommitBatchHolder::TYPE_IN_FLIGHT;

  commit_rate_limiter_.add_samples(1);
  data_rate_limiter_.add_samples(current_batch.batch->GetDataSize());

  // This method executes on the primary sequence, we schedule
  // a task for immediate execution on the commit sequence.
  task_runner_->AssertIsRunningOnPrimarySequence();
  bool success = task_runner_->PostShutdownBlockingTask(
      FROM_HERE, DOMStorageTaskRunner::COMMIT_SEQUENCE,
      base::BindOnce(&DOMStorageArea::CommitChanges, this,
                     base::RetainedRef(current_batch.batch)));
  DCHECK(success);
}

void DOMStorageArea::CommitChanges(const CommitBatch* commit_batch) {
  // This method executes on the commit sequence.
  task_runner_->AssertIsRunningOnCommitSequence();
  backing_->CommitChanges(commit_batch->clear_all_first,
                          commit_batch->changed_values);
  // TODO(michaeln): what if CommitChanges returns false (e.g., we're trying to
  // commit to a DB which is in an inconsistent state?)
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DOMStorageArea::OnCommitComplete, this));
}

void DOMStorageArea::OnCommitComplete() {
  // We're back on the primary sequence in this method.
  task_runner_->AssertIsRunningOnPrimarySequence();
  if (is_shutdown_)
    return;

  DCHECK_EQ(CommitBatchHolder::TYPE_IN_FLIGHT, commit_batches_.back().type);
  commit_batches_.pop_back();
  if (GetCurrentCommitBatch() && !HasCommitBatchInFlight()) {
    // More changes have accrued, restart the timer.
    task_runner_->PostDelayedTask(
        FROM_HERE, base::BindOnce(&DOMStorageArea::OnCommitTimer, this),
        ComputeCommitDelay());
  } else {
    // When the desired load state is changed, the unload of map is deferred
    // when there are uncommitted changes. So, try again after committing.
    UnloadMapIfDesired();
  }
}

void DOMStorageArea::ShutdownInCommitSequence() {
  // This method executes on the commit sequence.
  task_runner_->AssertIsRunningOnCommitSequence();
  DCHECK(backing_.get());
  if (GetCurrentCommitBatch()) {
    CommitBatch* batch = GetCurrentCommitBatch()->batch.get();
    // Commit any changes that accrued prior to the timer firing.
    bool success =
        backing_->CommitChanges(batch->clear_all_first, batch->changed_values);
    DCHECK(success);
  }
  commit_batches_.clear();
  backing_.reset();
  session_storage_backing_ = nullptr;
}

}  // namespace content
