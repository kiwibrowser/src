// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_CONTEXT_IMPL_H_
#define CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_CONTEXT_IMPL_H_

#include <stdint.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/atomicops.h"
#include "base/containers/circular_deque.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "base/trace_event/memory_dump_provider.h"
#include "content/browser/bad_message.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace base {
class FilePath;
class NullableString16;
}

namespace storage {
class SpecialStoragePolicy;
}

namespace content {

class DOMStorageArea;
class DOMStorageNamespace;
class DOMStorageSession;
class DOMStorageTaskRunner;
class SessionStorageDatabase;
struct SessionStorageUsageInfo;

// The Context is the root of an object containment hierarchy for
// Namespaces and Areas related to the owning profile.
// One instance is allocated in the main process for each profile,
// instance methods should be called serially in the background as
// determined by the task_runner. Specifically not on chrome's non-blocking
// IO thread since these methods can result in blocking file io.
//
// In general terms, the DOMStorage object relationships are...
//   Contexts (per-profile) own Namespaces which own Areas which share Maps.
//   Hosts(per-renderer) refer to Namespaces and Areas open in its renderer.
//   Sessions (per-tab) cause the creation and deletion of session Namespaces.
//
// Session Namespaces are cloned by initially making a shallow copy of
// all contained Areas, the shallow copies refer to the same refcounted Map,
// and does a deep copy-on-write if needed.
//
// Classes intended to be used by an embedder are DOMStorageContextImpl,
// DOMStorageHost, and DOMStorageSession. The other classes are for
// internal consumption.
class CONTENT_EXPORT DOMStorageContextImpl
    : public base::RefCountedThreadSafe<DOMStorageContextImpl>,
      public base::trace_event::MemoryDumpProvider {
 public:
  typedef std::map<std::string, scoped_refptr<DOMStorageNamespace>>
      StorageNamespaceMap;

  // An interface for observing Local and Session Storage events on the
  // background thread.
  class EventObserver {
   public:
    // |old_value| may be null on initial insert.
    virtual void OnDOMStorageItemSet(
        const DOMStorageArea* area,
        const base::string16& key,
        const base::string16& new_value,
        const base::NullableString16& old_value,
        const GURL& page_url) = 0;
    virtual void OnDOMStorageItemRemoved(
        const DOMStorageArea* area,
        const base::string16& key,
        const base::string16& old_value,
        const GURL& page_url) = 0;
    virtual void OnDOMStorageAreaCleared(
        const DOMStorageArea* area,
        const GURL& page_url) = 0;

   protected:
    virtual ~EventObserver() {}
  };

  // Option for PurgeMemory.
  enum PurgeOption {
    // Determines if purging is required based on the usage and the platform.
    PURGE_IF_NEEDED,

    // Purge unopened areas only.
    PURGE_UNOPENED,

    // Purge aggressively, i.e. discard cache even for areas that have
    // non-zero open count.
    PURGE_AGGRESSIVE,
  };

  // |sessionstorage_directory| may be empty for incognito browser contexts.
  DOMStorageContextImpl(const base::FilePath& sessionstorage_directory,
                        storage::SpecialStoragePolicy* special_storage_policy,
                        scoped_refptr<DOMStorageTaskRunner> task_runner);

  // Returns the directory path for sessionStorage, or an empty directory, if
  // there is no backing on disk.
  const base::FilePath& sessionstorage_directory() {
    return sessionstorage_directory_;
  }

  DOMStorageTaskRunner* task_runner() const { return task_runner_.get(); }
  DOMStorageNamespace* GetStorageNamespace(const std::string& namespace_id);

  void GetSessionStorageUsage(std::vector<SessionStorageUsageInfo>* infos);
  void DeleteSessionStorage(const SessionStorageUsageInfo& usage_info);

  // Used by content settings to alter the behavior around
  // what data to keep and what data to discard at shutdown.
  // The policy is not so straight forward to describe, see
  // the implementation for details.
  void SetForceKeepSessionState() {
    force_keep_session_state_ = true;
  }

  // Called when the owning BrowserContext is ending.
  // Schedules the commit of any unsaved changes and will delete
  // and keep data on disk per the content settings and special storage
  // policies. Contained areas and namespaces will stop functioning after
  // this method has been called.
  void Shutdown();

  // Initiate the process of flushing (writing - not sync'ing) any unwritten
  // data managed by this instance. Flushing will start "soon".
  void Flush();

  // Methods to add, remove, and notify EventObservers.
  void AddEventObserver(EventObserver* observer);
  void RemoveEventObserver(EventObserver* observer);
  void NotifyItemSet(
      const DOMStorageArea* area,
      const base::string16& key,
      const base::string16& new_value,
      const base::NullableString16& old_value,
      const GURL& page_url);
  void NotifyItemRemoved(
      const DOMStorageArea* area,
      const base::string16& key,
      const base::string16& old_value,
      const GURL& page_url);
  void NotifyAreaCleared(
      const DOMStorageArea* area,
      const GURL& page_url);

  // Must be called on the background thread.
  base::Optional<bad_message::BadMessageReason> DiagnoseSessionNamespaceId(
      const std::string& namespace_id);

  // Must be called on the background thread.
  void CreateSessionNamespace(const std::string& namespace_id);
  void DeleteSessionNamespace(const std::string& namespace_id,
                              bool should_persist_data);
  void CloneSessionNamespace(const std::string& existing_id,
                             const std::string& new_id);

  // Starts backing sessionStorage on disk. This function must be called right
  // after DOMStorageContextImpl is created, before it's used.
  void SetSaveSessionStorageOnDisk();

  // Deletes all namespaces which don't have an associated DOMStorageNamespace
  // alive. This function is used for deleting possible leftover data after an
  // unclean exit.
  void StartScavengingUnusedSessionStorage();

  // Frees up memory when possible. Purges caches and schedules commits
  // depending on the given |purge_option|.
  void PurgeMemory(PurgeOption purge_option);

  // base::trace_event::MemoryDumpProvider implementation.
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs& args,
                    base::trace_event::ProcessMemoryDump* pmd) override;

 private:
  friend class DOMStorageContextImplTest;
  FRIEND_TEST_ALL_PREFIXES(DOMStorageContextImplTest, Basics);
  friend class base::RefCountedThreadSafe<DOMStorageContextImpl>;

  ~DOMStorageContextImpl() override;

  void ClearSessionOnlyOrigins();

  // For scavenging unused sessionStorages.
  void FindUnusedNamespaces();
  void FindUnusedNamespacesInCommitSequence(
      const std::set<std::string>& namespace_ids_in_use,
      const std::set<std::string>& protected_persistent_session_ids);
  void DeleteNextUnusedNamespace();
  void DeleteNextUnusedNamespaceInCommitSequence();

  // Collection of namespaces keyed by id.
  StorageNamespaceMap namespaces_;

  // Where sessionstorage data is stored, maybe empty for the incognito use
  // case. Always empty until the file-backed session storage feature is
  // implemented.
  base::FilePath sessionstorage_directory_;

  // Used to schedule sequenced background tasks.
  scoped_refptr<DOMStorageTaskRunner> task_runner_;

  // List of objects observing local storage events.
  base::ObserverList<EventObserver> event_observers_;

  // For diagnostic purposes.
  base::circular_deque<std::string> recently_deleted_session_ids_;

  bool is_shutdown_;
  bool force_keep_session_state_;
  scoped_refptr<storage::SpecialStoragePolicy> special_storage_policy_;
  scoped_refptr<SessionStorageDatabase> session_storage_database_;

  // For cleaning up unused namespaces gradually.
  bool scavenging_started_;
  std::vector<std::string> deletable_namespace_ids_;

  // Persistent namespace IDs to protect from gradual deletion (they will
  // be needed for session restore).
  std::set<std::string> protected_session_ids_;

  // For cleaning up unused databases more aggressively.
  bool is_low_end_device_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_CONTEXT_IMPL_H_
