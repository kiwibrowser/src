// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "third_party/leveldatabase/leveldb_chrome.h"

#include <memory>

#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file.h"
#include "base/format_macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_provider.h"
#include "base/trace_event/process_memory_dump.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/helpers/memenv/memenv.h"
#include "util/mutexlock.h"

using MemoryPressureLevel = base::MemoryPressureListener::MemoryPressureLevel;
using base::trace_event::MemoryAllocatorDump;
using base::trace_event::MemoryDumpArgs;
using base::trace_event::MemoryDumpProvider;
using base::trace_event::ProcessMemoryDump;
using leveldb::Cache;
using leveldb::NewLRUCache;

namespace leveldb_chrome {

namespace {

size_t DefaultBlockCacheSize() {
  if (base::SysInfo::IsLowEndDevice())
    return 1 << 20;  // 1MB
  else
    return 8 << 20;  // 8MB
}

std::string GetDumpNameForMemEnv(const leveldb::Env* memenv) {
  return base::StringPrintf("leveldatabase/memenv_0x%" PRIXPTR,
                            reinterpret_cast<uintptr_t>(memenv));
}

// Singleton owning resources shared by Chrome's leveldb databases.
class Globals {
 public:
  static Globals* GetInstance() {
    static Globals* globals = new Globals();
    return globals;
  }

  Globals() : browser_block_cache_(NewLRUCache(DefaultBlockCacheSize())) {
    if (!base::SysInfo::IsLowEndDevice())
      web_block_cache_.reset(NewLRUCache(DefaultBlockCacheSize()));

    memory_pressure_listener_.reset(new base::MemoryPressureListener(
        base::Bind(&Globals::OnMemoryPressure, base::Unretained(this))));
  }

  Cache* web_block_cache() const {
    if (web_block_cache_)
      return web_block_cache_.get();
    return browser_block_cache();
  }

  Cache* browser_block_cache() const { return browser_block_cache_.get(); }

  // Called when the system is under memory pressure.
  void OnMemoryPressure(MemoryPressureLevel memory_pressure_level) {
    if (memory_pressure_level ==
        MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE)
      return;
    browser_block_cache()->Prune();
    if (browser_block_cache() == web_block_cache())
      return;
    web_block_cache()->Prune();
  }

  void DidCreateChromeMemEnv(leveldb::Env* env) {
    leveldb::MutexLock l(&env_mutex_);
    DCHECK(in_memory_envs_.find(env) == in_memory_envs_.end());
    in_memory_envs_.insert(env);
  }

  void WillDestroyChromeMemEnv(leveldb::Env* env) {
    leveldb::MutexLock l(&env_mutex_);
    DCHECK(in_memory_envs_.find(env) != in_memory_envs_.end());
    in_memory_envs_.erase(env);
  }

  bool IsInMemoryEnv(const leveldb::Env* env) const {
    leveldb::MutexLock l(&env_mutex_);
    return in_memory_envs_.find(env) != in_memory_envs_.end();
  }

  void DumpAllTrackedEnvs(const MemoryDumpArgs& dump_args,
                          base::trace_event::ProcessMemoryDump* pmd);

  void UpdateHistograms() {
    leveldb_env::DBTracker::GetInstance()->UpdateHistograms();

    // In-memory caches are hard-coded to be zero bytes so don't log
    // LevelDB.SharedCache.BytesUsed.InMemory.

    // leveldb limits the read cache size to 1GB, but its default value is 8MB,
    // and Chrome uses either 1MB or 8MB.
    if (GetSharedWebBlockCache() == GetSharedBrowserBlockCache()) {
      UMA_HISTOGRAM_COUNTS_100000("LevelDB.SharedCache.KBUsed.Unified",
                                  browser_block_cache_->TotalCharge() / 1024);
      return;
    }
    UMA_HISTOGRAM_COUNTS_100000("LevelDB.SharedCache.KBUsed.Web",
                                web_block_cache_->TotalCharge() / 1024);
    UMA_HISTOGRAM_COUNTS_100000("LevelDB.SharedCache.KBUsed.Browser",
                                browser_block_cache_->TotalCharge() / 1024);
  }

 private:
  ~Globals() {}

  std::unique_ptr<Cache> web_block_cache_;      // null on low end devices.
  std::unique_ptr<Cache> browser_block_cache_;  // Never null.
  // Listens for the system being under memory pressure.
  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;
  mutable leveldb::port::Mutex env_mutex_;
  base::flat_set<leveldb::Env*> in_memory_envs_;

  DISALLOW_COPY_AND_ASSIGN(Globals);
};

class ChromeMemEnv : public leveldb::EnvWrapper {
 public:
  ChromeMemEnv(leveldb::Env* base_env, const std::string& name)
      : EnvWrapper(leveldb::NewMemEnv(base_env)),
        base_env_(target()),
        name_(name) {
    Globals::GetInstance()->DidCreateChromeMemEnv(this);
  }

  ~ChromeMemEnv() override {
    Globals::GetInstance()->WillDestroyChromeMemEnv(this);
  }

  leveldb::Status NewWritableFile(const std::string& f,
                                  leveldb::WritableFile** r) override {
    leveldb::Status s = leveldb::EnvWrapper::NewWritableFile(f, r);
    if (s.ok()) {
      base::AutoLock lock(files_lock_);
      file_names_.insert(f);
    }
    return s;
  }

  leveldb::Status NewAppendableFile(const std::string& f,
                                    leveldb::WritableFile** r) override {
    leveldb::Status s = leveldb::EnvWrapper::NewAppendableFile(f, r);
    if (s.ok()) {
      base::AutoLock lock(files_lock_);
      file_names_.insert(f);
    }
    return s;
  }

  leveldb::Status DeleteFile(const std::string& fname) override {
    leveldb::Status s = leveldb::EnvWrapper::DeleteFile(fname);
    if (s.ok()) {
      base::AutoLock lock(files_lock_);
      DCHECK(base::ContainsKey(file_names_, fname));
      file_names_.erase(fname);
    }
    return s;
  }

  leveldb::Status RenameFile(const std::string& src,
                             const std::string& target) override {
    leveldb::Status s = leveldb::EnvWrapper::RenameFile(src, target);
    if (s.ok()) {
      base::AutoLock lock(files_lock_);
      file_names_.erase(src);
      file_names_.insert(target);
    }
    return s;
  }

  // Calculate the memory used by this in-memory database. leveldb's in-memory
  // databases store their data in a MemEnv which can be shared between
  // databases. Chrome rarely (if ever) shares MemEnv's. This calculation is
  // also an estimate as leveldb does not expose a way to retrieve or properly
  // calculate the amount of RAM used by a MemEnv.
  uint64_t size() {
    // This is copied from
    // //third_party/leveldatabase/src/helpers/memenv/memenv.cc which cannot be
    // included here. Duplicated for size calculations only.
    struct FileState {
      leveldb::port::Mutex refs_mutex_;
      int refs_;
      std::vector<char*> blocks_;
      uint64_t size_;
      enum { kBlockSize = 8 * 1024 };
    };

    uint64_t total_size = 0;
    base::AutoLock lock(files_lock_);
    for (const std::string& fname : file_names_) {
      uint64_t file_size;
      leveldb::Status s = GetFileSize(fname, &file_size);
      DCHECK(s.ok());
      if (s.ok())
        total_size += file_size + fname.size() + 1 + sizeof(FileState);
    }
    return total_size;
  }

  const std::string& name() const { return name_; }

  bool OnMemoryDump(const MemoryDumpArgs& dump_args, ProcessMemoryDump* pmd) {
    auto* env_dump = pmd->CreateAllocatorDump(GetDumpNameForMemEnv(this));

    env_dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                        base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                        size());

    if (dump_args.level_of_detail !=
        base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND) {
      env_dump->AddString("name", "", name());
    }

    const char* system_allocator_name =
        base::trace_event::MemoryDumpManager::GetInstance()
            ->system_allocator_pool_name();
    if (system_allocator_name) {
      pmd->AddSuballocation(env_dump->guid(), system_allocator_name);
    }

    return true;
  }

 private:
  std::unique_ptr<leveldb::Env> base_env_;
  const std::string name_;
  base::Lock files_lock_;
  std::set<std::string> file_names_;
  DISALLOW_COPY_AND_ASSIGN(ChromeMemEnv);
};

void Globals::DumpAllTrackedEnvs(const MemoryDumpArgs& dump_args,
                                 ProcessMemoryDump* pmd) {
  leveldb::MutexLock l(&env_mutex_);
  for (auto env : in_memory_envs_)
    static_cast<ChromeMemEnv*>(env)->OnMemoryDump(dump_args, pmd);
}

}  // namespace

// Returns a separate (from the default) block cache for use by web APIs.
// This must be used when opening the databases accessible to Web-exposed APIs,
// so rogue pages can't mount a denial of service attack by hammering the block
// cache. Without separate caches, such an attack might slow down Chrome's UI to
// the point where the user can't close the offending page's tabs.
Cache* GetSharedWebBlockCache() {
  return Globals::GetInstance()->web_block_cache();
}

Cache* GetSharedBrowserBlockCache() {
  return Globals::GetInstance()->browser_block_cache();
}

Cache* GetSharedInMemoryBlockCache() {
  // Zero size cache to prevent cache hits.
  static leveldb::Cache* s_empty_cache = leveldb::NewLRUCache(0);
  return s_empty_cache;
}

bool IsMemEnv(const leveldb::Env* env) {
  DCHECK(env);
  return Globals::GetInstance()->IsInMemoryEnv(env);
}

std::unique_ptr<leveldb::Env> NewMemEnv(const std::string& name,
                                        leveldb::Env* base_env) {
  if (!base_env)
    base_env = leveldb::Env::Default();
  return std::make_unique<ChromeMemEnv>(base_env, name);
}

void UpdateHistograms() {
  return Globals::GetInstance()->UpdateHistograms();
}

bool ParseFileName(const std::string& filename,
                   uint64_t* number,
                   leveldb::FileType* type) {
  return leveldb::ParseFileName(filename, number, type);
}

bool CorruptClosedDBForTesting(const base::FilePath& db_path) {
  base::File current(db_path.Append(FILE_PATH_LITERAL("CURRENT")),
                     base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!current.IsValid()) {
    return false;
  }
  const char kString[] = "StringWithoutEOL";
  if (current.Write(0, kString, sizeof(kString)) != sizeof(kString))
    return false;
  current.Close();
  return true;
}

MemoryAllocatorDump* GetEnvAllocatorDump(ProcessMemoryDump* pmd,
                                         leveldb::Env* tracked_memenv) {
  DCHECK(Globals::GetInstance()->IsInMemoryEnv(tracked_memenv))
      << std::hex << tracked_memenv << " is not tracked";
  return pmd->GetAllocatorDump(GetDumpNameForMemEnv(tracked_memenv));
}

void DumpAllTrackedEnvs(ProcessMemoryDump* pmd) {
  Globals::GetInstance()->DumpAllTrackedEnvs(pmd->dump_args(), pmd);
}

}  // namespace leveldb_chrome
