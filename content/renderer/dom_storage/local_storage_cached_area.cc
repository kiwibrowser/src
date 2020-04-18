// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/local_storage_cached_area.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/services/leveldb/public/cpp/util.h"
#include "content/common/dom_storage/dom_storage_map.h"
#include "content/common/storage_partition_service.mojom.h"
#include "content/renderer/dom_storage/local_storage_area.h"
#include "content/renderer/dom_storage/local_storage_cached_areas.h"
#include "content/renderer/dom_storage/session_web_storage_namespace_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_storage_event_dispatcher.h"

namespace content {

namespace {

// Don't change or reorder any of the values in this enum, as these values
// are serialized on disk.
enum class StorageFormat : uint8_t { UTF16 = 0, Latin1 = 1 };

class GetAllCallback : public mojom::LevelDBWrapperGetAllCallback {
 public:
  static mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo CreateAndBind(
      base::OnceCallback<void(bool)> callback) {
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo ptr_info;
    auto request = mojo::MakeRequest(&ptr_info);
    mojo::MakeStrongAssociatedBinding(
        base::WrapUnique(new GetAllCallback(std::move(callback))),
        std::move(request));
    return ptr_info;
  }

 private:
  explicit GetAllCallback(base::OnceCallback<void(bool)> callback)
      : m_callback(std::move(callback)) {}
  void Complete(bool success) override { std::move(m_callback).Run(success); }

  base::OnceCallback<void(bool)> m_callback;
};

}  // namespace

// These methods are used to pack and unpack the page_url/storage_area_id into
// source strings to/from the browser.
std::string PackSource(const GURL& page_url,
                       const std::string& storage_area_id) {
  return page_url.spec() + "\n" + storage_area_id;
}

void UnpackSource(const std::string& source,
                  GURL* page_url,
                  std::string* storage_area_id) {
  std::vector<std::string> result = base::SplitString(
      source, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  DCHECK_EQ(result.size(), 2u);
  *page_url = GURL(result[0]);
  *storage_area_id = result[1];
}

LocalStorageCachedArea::LocalStorageCachedArea(
    const std::string& namespace_id,
    const url::Origin& origin,
    mojom::SessionStorageNamespace* session_namespace,
    LocalStorageCachedAreas* cached_areas,
    blink::scheduler::WebMainThreadScheduler* main_thread_scheduler)
    : namespace_id_(namespace_id),
      origin_(origin),
      binding_(this),
      cached_areas_(cached_areas),
      main_thread_scheduler_(main_thread_scheduler),
      weak_factory_(this) {
  DCHECK(!namespace_id_.empty());

  mojom::LevelDBWrapperAssociatedPtrInfo wrapper_ptr_info;
  session_namespace->OpenArea(origin_, mojo::MakeRequest(&wrapper_ptr_info));
  leveldb_.Bind(std::move(wrapper_ptr_info),
                main_thread_scheduler->IPCTaskRunner());
  mojom::LevelDBObserverAssociatedPtrInfo ptr_info;
  binding_.Bind(mojo::MakeRequest(&ptr_info),
                main_thread_scheduler->IPCTaskRunner());
}

LocalStorageCachedArea::LocalStorageCachedArea(
    const url::Origin& origin,
    mojom::StoragePartitionService* storage_partition_service,
    LocalStorageCachedAreas* cached_areas,
    blink::scheduler::WebMainThreadScheduler* main_thread_scheduler)
    : origin_(origin),
      binding_(this),
      cached_areas_(cached_areas),
      main_thread_scheduler_(main_thread_scheduler),
      weak_factory_(this) {
  DCHECK(namespace_id_.empty());
  mojom::LevelDBWrapperPtrInfo wrapper_ptr_info;
  storage_partition_service->OpenLocalStorage(
      origin_, mojo::MakeRequest(&wrapper_ptr_info));
  leveldb_.Bind(std::move(wrapper_ptr_info),
                main_thread_scheduler->IPCTaskRunner());
  mojom::LevelDBObserverAssociatedPtrInfo ptr_info;
  binding_.Bind(mojo::MakeRequest(&ptr_info),
                main_thread_scheduler->IPCTaskRunner());
  leveldb_->AddObserver(std::move(ptr_info));
}

LocalStorageCachedArea::~LocalStorageCachedArea() {}

unsigned LocalStorageCachedArea::GetLength() {
  EnsureLoaded();
  return map_->Length();
}

base::NullableString16 LocalStorageCachedArea::GetKey(unsigned index) {
  EnsureLoaded();
  return map_->Key(index);
}

base::NullableString16 LocalStorageCachedArea::GetItem(
    const base::string16& key) {
  EnsureLoaded();
  return map_->GetItem(key);
}

bool LocalStorageCachedArea::SetItem(const base::string16& key,
                                     const base::string16& value,
                                     const GURL& page_url,
                                     const std::string& storage_area_id) {
  // A quick check to reject obviously overbudget items to avoid priming the
  // cache.
  if ((key.length() + value.length()) * sizeof(base::char16) >
      kPerStorageAreaQuota)
    return false;

  EnsureLoaded();
  bool result = false;
  base::NullableString16 old_nullable_value;
  if (should_send_old_value_on_mutations_)
    result = map_->SetItem(key, value, &old_nullable_value);
  else
    result = map_->SetItem(key, value, nullptr);
  if (!result)
    return false;

  // Determine data formats.
  bool is_session_storage = IsSessionStorage();
  FormatOption key_format = is_session_storage
                                ? FormatOption::kSessionStorageForceUTF8
                                : FormatOption::kLocalStorageDetectFormat;
  FormatOption value_format = is_session_storage
                                  ? FormatOption::kSessionStorageForceUTF16
                                  : FormatOption::kLocalStorageDetectFormat;

  // Ignore mutations to |key| until OnSetItemComplete.
  ignore_key_mutations_[key]++;
  base::Optional<std::vector<uint8_t>> optional_old_value;
  if (!old_nullable_value.is_null())
    optional_old_value =
        String16ToUint8Vector(old_nullable_value.string(), value_format);

  blink::WebScopedVirtualTimePauser virtual_time_pauser =
      main_thread_scheduler_->CreateWebScopedVirtualTimePauser(
          "LocalStorageCachedArea");
  virtual_time_pauser.PauseVirtualTime();
  leveldb_->Put(String16ToUint8Vector(key, key_format),
                String16ToUint8Vector(value, value_format), optional_old_value,
                PackSource(page_url, storage_area_id),
                base::BindOnce(&LocalStorageCachedArea::OnSetItemComplete,
                               weak_factory_.GetWeakPtr(), key,
                               std::move(virtual_time_pauser)));
  if (IsSessionStorage() &&
      (old_nullable_value.is_null() || old_nullable_value.string() != value)) {
    blink::WebStorageArea* originating_area = areas_[storage_area_id];
    DCHECK_NE(nullptr, originating_area);
    SessionWebStorageNamespaceImpl session_namespace_for_event_dispatch(
        namespace_id_, nullptr);
    blink::WebStorageEventDispatcher::DispatchSessionStorageEvent(
        blink::WebString::FromUTF16(key),
        blink::WebString::FromUTF16(old_nullable_value),
        blink::WebString::FromUTF16(value), origin_.GetURL(), page_url,
        session_namespace_for_event_dispatch, originating_area);
  }
  return true;
}

void LocalStorageCachedArea::RemoveItem(const base::string16& key,
                                        const GURL& page_url,
                                        const std::string& storage_area_id) {
  EnsureLoaded();
  bool result = false;
  base::string16 old_value;
  if (should_send_old_value_on_mutations_)
    result = map_->RemoveItem(key, &old_value);
  else
    result = map_->RemoveItem(key, nullptr);
  if (!result)
    return;

  // Determine data formats.
  bool is_session_storage = IsSessionStorage();
  FormatOption key_format = is_session_storage
                                ? FormatOption::kSessionStorageForceUTF8
                                : FormatOption::kLocalStorageDetectFormat;
  FormatOption value_format = is_session_storage
                                  ? FormatOption::kSessionStorageForceUTF16
                                  : FormatOption::kLocalStorageDetectFormat;

  // Ignore mutations to |key| until OnRemoveItemComplete.
  ignore_key_mutations_[key]++;
  base::Optional<std::vector<uint8_t>> optional_old_value;
  if (should_send_old_value_on_mutations_)
    optional_old_value = String16ToUint8Vector(old_value, value_format);

  blink::WebScopedVirtualTimePauser virtual_time_pauser =
      main_thread_scheduler_->CreateWebScopedVirtualTimePauser(
          "LocalStorageCachedArea");
  virtual_time_pauser.PauseVirtualTime();
  leveldb_->Delete(String16ToUint8Vector(key, key_format), optional_old_value,
                   PackSource(page_url, storage_area_id),
                   base::BindOnce(&LocalStorageCachedArea::OnRemoveItemComplete,
                                  weak_factory_.GetWeakPtr(), key,
                                  std::move(virtual_time_pauser)));
  if (IsSessionStorage() && old_value != base::string16()) {
    blink::WebStorageArea* originating_area = areas_[storage_area_id];
    DCHECK_NE(nullptr, originating_area);
    SessionWebStorageNamespaceImpl session_namespace_for_event_dispatch(
        namespace_id_, nullptr);
    blink::WebStorageEventDispatcher::DispatchSessionStorageEvent(
        blink::WebString::FromUTF16(key),
        blink::WebString::FromUTF16(old_value), blink::WebString(),
        origin_.GetURL(), page_url, session_namespace_for_event_dispatch,
        originating_area);
  }
}

void LocalStorageCachedArea::Clear(const GURL& page_url,
                                   const std::string& storage_area_id) {
  bool already_empty = false;
  if (IsSessionStorage()) {
    EnsureLoaded();
    already_empty = map_->Length() == 0u;
  }
  // No need to prime the cache in this case.
  Reset();
  map_ = new DOMStorageMap(kPerStorageAreaQuota);
  ignore_all_mutations_ = true;

  blink::WebScopedVirtualTimePauser virtual_time_pauser =
      main_thread_scheduler_->CreateWebScopedVirtualTimePauser(
          "LocalStorageCachedArea");
  virtual_time_pauser.PauseVirtualTime();
  leveldb_->DeleteAll(PackSource(page_url, storage_area_id),
                      base::BindOnce(&LocalStorageCachedArea::OnClearComplete,
                                     weak_factory_.GetWeakPtr(),
                                     std::move(virtual_time_pauser)));
  if (IsSessionStorage() && !already_empty) {
    blink::WebStorageArea* originating_area = areas_[storage_area_id];
    DCHECK_NE(nullptr, originating_area);
    SessionWebStorageNamespaceImpl session_namespace_for_event_dispatch(
        namespace_id_, nullptr);
    blink::WebStorageEventDispatcher::DispatchSessionStorageEvent(
        blink::WebString(), blink::WebString(), blink::WebString(),
        origin_.GetURL(), page_url, session_namespace_for_event_dispatch,
        originating_area);
  }
}

void LocalStorageCachedArea::AreaCreated(LocalStorageArea* area) {
  areas_[area->id()] = area;
}

void LocalStorageCachedArea::AreaDestroyed(LocalStorageArea* area) {
  areas_.erase(area->id());
}

// static
base::string16 LocalStorageCachedArea::Uint8VectorToString16(
    const std::vector<uint8_t>& input,
    FormatOption format_option) {
  if (input.empty())
    return base::string16();
  size_t input_size = input.size();
  base::string16 result;
  switch (format_option) {
    case FormatOption::kSessionStorageForceUTF16:
      if (input_size % sizeof(base::char16) != 0) {
        // TODO(mek): Better error recovery when corrupt (or otherwise invalid)
        // data is detected.
        LOCAL_HISTOGRAM_BOOLEAN("LocalStorageCachedArea.CorruptData", true);
        LOG(ERROR) << "Corrupt data in domstorage";
        return base::string16();
      }
      result.resize(input_size / sizeof(base::char16));
      std::memcpy(&result[0], input.data(), input_size);
      return result;
    case FormatOption::kSessionStorageForceUTF8:
      // Encoding / codepoint errors are ignored on purpose.
      return base::UTF8ToUTF16(leveldb::Uint8VectorToStringPiece(input));
    case FormatOption::kLocalStorageDetectFormat:
      break;
  }
  StorageFormat format = static_cast<StorageFormat>(input[0]);
  const size_t payload_size = input_size - 1;
  bool corrupt = false;
  switch (format) {
    case StorageFormat::UTF16:
      if (payload_size % sizeof(base::char16) != 0) {
        corrupt = true;
        break;
      }
      result.resize(payload_size / sizeof(base::char16));
      std::memcpy(&result[0], input.data() + 1, payload_size);
      break;
    case StorageFormat::Latin1:
      result.resize(payload_size);
      std::copy(input.begin() + 1, input.end(), result.begin());
      break;
    default:
      corrupt = true;
  }
  if (corrupt) {
    // TODO(mek): Better error recovery when corrupt (or otherwise invalid) data
    // is detected.
    LOCAL_HISTOGRAM_BOOLEAN("LocalStorageCachedArea.CorruptData", true);
    LOG(ERROR) << "Corrupt data in localstorage";
    return base::string16();
  }
  return result;
}

// static
std::vector<uint8_t> LocalStorageCachedArea::String16ToUint8Vector(
    const base::string16& input,
    FormatOption format_option) {
  switch (format_option) {
    case FormatOption::kSessionStorageForceUTF16: {
      std::vector<uint8_t> result;
      result.reserve(input.size() * sizeof(base::char16));
      const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
      result.insert(result.begin(), data,
                    data + input.size() * sizeof(base::char16));
      return result;
    }
    case FormatOption::kSessionStorageForceUTF8: {
      // Encoding / codepoint errors are ignored on purpose.
      std::string utf8 = base::UTF16ToUTF8(base::StringPiece16(input));
      return leveldb::StdStringToUint8Vector(utf8);
    }
    case FormatOption::kLocalStorageDetectFormat:
      break;
  }
  bool is_8bit = true;
  for (const auto& c : input) {
    if (c & 0xff00) {
      is_8bit = false;
      break;
    }
  }
  if (is_8bit) {
    std::vector<uint8_t> result(input.size() + 1);
    result[0] = static_cast<uint8_t>(StorageFormat::Latin1);
    std::copy(input.begin(), input.end(), result.begin() + 1);
    return result;
  }
  const uint8_t* data = reinterpret_cast<const uint8_t*>(input.data());
  std::vector<uint8_t> result;
  result.reserve(input.size() * sizeof(base::char16) + 1);
  result.push_back(static_cast<uint8_t>(StorageFormat::UTF16));
  result.insert(result.end(), data, data + input.size() * sizeof(base::char16));
  return result;
}

void LocalStorageCachedArea::KeyAdded(const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& value,
                                      const std::string& source) {
  DCHECK(!IsSessionStorage());
  base::NullableString16 null_value;
  KeyAddedOrChanged(key, value, null_value, source);
}

void LocalStorageCachedArea::KeyChanged(const std::vector<uint8_t>& key,
                                        const std::vector<uint8_t>& new_value,
                                        const std::vector<uint8_t>& old_value,
                                        const std::string& source) {
  DCHECK(!IsSessionStorage());
  base::NullableString16 old_value_str(
      Uint8VectorToString16(old_value, FormatOption::kLocalStorageDetectFormat),
      false);
  KeyAddedOrChanged(key, new_value, old_value_str, source);
}

void LocalStorageCachedArea::KeyDeleted(const std::vector<uint8_t>& key,
                                        const std::vector<uint8_t>& old_value,
                                        const std::string& source) {
  DCHECK(!IsSessionStorage());
  GURL page_url;
  std::string storage_area_id;
  UnpackSource(source, &page_url, &storage_area_id);

  base::string16 key_string =
      Uint8VectorToString16(key, FormatOption::kLocalStorageDetectFormat);

  blink::WebStorageArea* originating_area = nullptr;
  if (areas_.find(storage_area_id) != areas_.end()) {
    // The source storage area is in this process.
    originating_area = areas_[storage_area_id];
  } else if (map_ && !ignore_all_mutations_) {
    // This was from another process or the storage area is gone. If the former,
    // remove it from our cache if we haven't already changed it and are waiting
    // for the confirmation callback. In the latter case, we won't do anything
    // because ignore_key_mutations_ won't be updated until the callback runs.
    if (ignore_key_mutations_.find(key_string) == ignore_key_mutations_.end())
      map_->RemoveItem(key_string, nullptr);
  }

  blink::WebStorageEventDispatcher::DispatchLocalStorageEvent(
      blink::WebString::FromUTF16(key_string),
      blink::WebString::FromUTF16(Uint8VectorToString16(
          old_value, FormatOption::kLocalStorageDetectFormat)),
      blink::WebString(), origin_.GetURL(), page_url, originating_area);
}

void LocalStorageCachedArea::AllDeleted(const std::string& source) {
  DCHECK(!IsSessionStorage());
  GURL page_url;
  std::string storage_area_id;
  UnpackSource(source, &page_url, &storage_area_id);

  blink::WebStorageArea* originating_area = nullptr;
  if (areas_.find(storage_area_id) != areas_.end()) {
    // The source storage area is in this process.
    originating_area = areas_[storage_area_id];
  } else if (map_ && !ignore_all_mutations_) {
    scoped_refptr<DOMStorageMap> old = map_;
    map_ = new DOMStorageMap(kPerStorageAreaQuota);

    // We have to retain local additions which happened after this clear
    // operation from another process.
    auto iter = ignore_key_mutations_.begin();
    while (iter != ignore_key_mutations_.end()) {
      base::NullableString16 value = old->GetItem(iter->first);
      if (!value.is_null())
        map_->SetItem(iter->first, value.string(), nullptr);
      ++iter;
    }
  }

  blink::WebStorageEventDispatcher::DispatchLocalStorageEvent(
      blink::WebString(), blink::WebString(), blink::WebString(),
      origin_.GetURL(), page_url, originating_area);
}

void LocalStorageCachedArea::ShouldSendOldValueOnMutations(bool value) {
  DCHECK(!IsSessionStorage());
  should_send_old_value_on_mutations_ = value;
}

void LocalStorageCachedArea::KeyAddedOrChanged(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& new_value,
    const base::NullableString16& old_value,
    const std::string& source) {
  DCHECK(!IsSessionStorage());
  GURL page_url;
  std::string storage_area_id;
  UnpackSource(source, &page_url, &storage_area_id);

  base::string16 key_string =
      Uint8VectorToString16(key, FormatOption::kLocalStorageDetectFormat);
  base::string16 new_value_string =
      Uint8VectorToString16(new_value, FormatOption::kLocalStorageDetectFormat);

  blink::WebStorageArea* originating_area = nullptr;
  if (areas_.find(storage_area_id) != areas_.end()) {
    // The source storage area is in this process.
    originating_area = areas_[storage_area_id];
  } else if (map_ && !ignore_all_mutations_) {
    // This was from another process or the storage area is gone. If the former,
    // apply it to our cache if we haven't already changed it and are waiting
    // for the confirmation callback. In the latter case, we won't do anything
    // because ignore_key_mutations_ won't be updated until the callback runs.
    if (ignore_key_mutations_.find(key_string) == ignore_key_mutations_.end()) {
      // We turn off quota checking here to accomodate the over budget allowance
      // that's provided in the browser process.
      map_->set_quota(std::numeric_limits<int32_t>::max());
      map_->SetItem(key_string, new_value_string, nullptr);
      map_->set_quota(kPerStorageAreaQuota);
    }
  }

  blink::WebStorageEventDispatcher::DispatchLocalStorageEvent(
      blink::WebString::FromUTF16(key_string),
      blink::WebString::FromUTF16(old_value),
      blink::WebString::FromUTF16(new_value_string), origin_.GetURL(), page_url,
      originating_area);
}

void LocalStorageCachedArea::EnsureLoaded() {
  if (map_)
    return;

  base::TimeTicks before = base::TimeTicks::Now();
  ignore_all_mutations_ = true;
  leveldb::mojom::DatabaseError status = leveldb::mojom::DatabaseError::OK;
  std::vector<content::mojom::KeyValuePtr> data;
  leveldb_->GetAll(GetAllCallback::CreateAndBind(
                       base::BindOnce(&LocalStorageCachedArea::OnGetAllComplete,
                                      weak_factory_.GetWeakPtr())),
                   &status, &data);

  DOMStorageValuesMap values;
  bool is_session_storage = IsSessionStorage();
  FormatOption key_format = is_session_storage
                                ? FormatOption::kSessionStorageForceUTF8
                                : FormatOption::kLocalStorageDetectFormat;
  FormatOption value_format = is_session_storage
                                  ? FormatOption::kSessionStorageForceUTF16
                                  : FormatOption::kLocalStorageDetectFormat;
  for (size_t i = 0; i < data.size(); ++i) {
    values[Uint8VectorToString16(data[i]->key, key_format)] =
        base::NullableString16(
            Uint8VectorToString16(data[i]->value, value_format), false);
  }

  map_ = new DOMStorageMap(kPerStorageAreaQuota);
  map_->SwapValues(&values);

  base::TimeDelta time_to_prime = base::TimeTicks::Now() - before;
  UMA_HISTOGRAM_TIMES("LocalStorage.MojoTimeToPrime", time_to_prime);

  size_t local_storage_size_kb = map_->storage_used() / 1024;
  // Track localStorage size, from 0-6MB. Note that the maximum size should be
  // 5MB, but we add some slop since we want to make sure the max size is always
  // above what we see in practice, since histograms can't change.
  UMA_HISTOGRAM_CUSTOM_COUNTS("LocalStorage.MojoSizeInKB",
                              local_storage_size_kb,
                              1, 6 * 1024, 50);
  if (local_storage_size_kb < 100) {
    UMA_HISTOGRAM_TIMES("LocalStorage.MojoTimeToPrimeForUnder100KB",
                        time_to_prime);
  } else if (local_storage_size_kb < 1000) {
    UMA_HISTOGRAM_TIMES("LocalStorage.MojoTimeToPrimeFor100KBTo1MB",
                        time_to_prime);
  } else {
    UMA_HISTOGRAM_TIMES("LocalStorage.MojoTimeToPrimeFor1MBTo5MB",
                        time_to_prime);
  }
}

void LocalStorageCachedArea::OnSetItemComplete(
    const base::string16& key,
    blink::WebScopedVirtualTimePauser,
    bool success) {
  if (!success) {
    Reset();
    return;
  }

  auto found = ignore_key_mutations_.find(key);
  DCHECK(found != ignore_key_mutations_.end());
  if (--found->second == 0)
    ignore_key_mutations_.erase(found);
}

void LocalStorageCachedArea::OnRemoveItemComplete(
    const base::string16& key,
    blink::WebScopedVirtualTimePauser,
    bool success) {
  DCHECK(success);
  auto found = ignore_key_mutations_.find(key);
  DCHECK(found != ignore_key_mutations_.end());
  if (--found->second == 0)
    ignore_key_mutations_.erase(found);
}

void LocalStorageCachedArea::OnClearComplete(blink::WebScopedVirtualTimePauser,
                                             bool success) {
  DCHECK(success);
  DCHECK(ignore_all_mutations_);
  ignore_all_mutations_ = false;
}

void LocalStorageCachedArea::OnGetAllComplete(bool success) {
  // Since the GetAll method is synchronous, we need this asynchronously
  // delivered notification to avoid applying changes to the returned array
  // that we already have.
  DCHECK(success);
  DCHECK(ignore_all_mutations_);
  ignore_all_mutations_ = false;
}

void LocalStorageCachedArea::Reset() {
  map_ = nullptr;
  ignore_key_mutations_.clear();
  ignore_all_mutations_ = false;
  weak_factory_.InvalidateWeakPtrs();
}

}  // namespace content
