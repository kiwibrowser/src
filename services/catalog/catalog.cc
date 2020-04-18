// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/catalog/catalog.h"

#include <memory>
#include <string>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "components/services/filesystem/directory_impl.h"
#include "components/services/filesystem/lock_table.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/catalog/constants.h"
#include "services/catalog/entry_cache.h"
#include "services/catalog/instance.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace catalog {

namespace {

const char kCatalogServicesKey[] = "services";
const char kCatalogServiceEmbeddedKey[] = "embedded";
const char kCatalogServiceExecutableKey[] = "executable";
const char kCatalogServiceManifestKey[] = "manifest";

base::LazyInstance<std::unique_ptr<base::Value>>::DestructorAtExit
    g_default_static_manifest = LAZY_INSTANCE_INITIALIZER;

void LoadCatalogManifestIntoCache(const base::Value* root, EntryCache* cache) {
  DCHECK(root);
  const base::DictionaryValue* catalog = nullptr;
  if (!root->GetAsDictionary(&catalog)) {
    LOG(ERROR) << "Catalog manifest is not a dictionary value.";
    return;
  }
  DCHECK(catalog);

  const base::DictionaryValue* services = nullptr;
  if (!catalog->GetDictionary(kCatalogServicesKey, &services)) {
    LOG(ERROR) << "Catalog manifest \"services\" is not a dictionary value.";
    return;
  }

  for (base::DictionaryValue::Iterator it(*services); !it.IsAtEnd();
       it.Advance()) {
    const base::DictionaryValue* service_entry = nullptr;
    if (!it.value().GetAsDictionary(&service_entry)) {
      LOG(ERROR) << "Catalog service entry for \"" << it.key()
                 << "\" is not a dictionary value.";
      continue;
    }

    bool is_embedded = false;
    service_entry->GetBoolean(kCatalogServiceEmbeddedKey, &is_embedded);

    base::FilePath executable_path;
    std::string executable_path_string;
    if (service_entry->GetString(kCatalogServiceExecutableKey,
                                 &executable_path_string)) {
      base::FilePath exe_dir;
      CHECK(base::PathService::Get(base::DIR_EXE, &exe_dir));
#if defined(OS_WIN)
      executable_path_string += ".exe";
      base::ReplaceFirstSubstringAfterOffset(
          &executable_path_string, 0, "@EXE_DIR",
          base::UTF16ToUTF8(exe_dir.value()));
      executable_path =
          base::FilePath(base::UTF8ToUTF16(executable_path_string));
#else
      base::ReplaceFirstSubstringAfterOffset(
          &executable_path_string, 0, "@EXE_DIR", exe_dir.value());
      executable_path = base::FilePath(executable_path_string);
#endif
    }

    const base::DictionaryValue* manifest = nullptr;
    if (!service_entry->GetDictionary(kCatalogServiceManifestKey, &manifest)) {
      LOG(ERROR) << "Catalog entry for \"" << it.key() << "\" has an invalid "
                 << "\"manifest\" value.";
      continue;
    }

    DCHECK(!(is_embedded && !executable_path.empty()));

    if (is_embedded)
      executable_path = base::CommandLine::ForCurrentProcess()->GetProgram();

    auto entry = Entry::Deserialize(*manifest);
    if (entry) {
      if (!executable_path.empty())
        entry->set_path(std::move(executable_path));
      bool added = cache->AddRootEntry(std::move(entry));
      DCHECK(added);
    } else {
      LOG(ERROR) << "Failed to read manifest entry for \"" << it.key() << "\".";
    }
  }
}

}  // namespace

// Wraps state needed for servicing directory requests on a separate thread.
// filesystem::LockTable is not thread safe, so it's wrapped in
// DirectoryThreadState.
class Catalog::DirectoryThreadState
    : public base::RefCountedDeleteOnSequence<DirectoryThreadState> {
 public:
  explicit DirectoryThreadState(
      scoped_refptr<base::SequencedTaskRunner> task_runner)
      : base::RefCountedDeleteOnSequence<DirectoryThreadState>(
            std::move(task_runner)) {}

  scoped_refptr<filesystem::LockTable> lock_table() {
    if (!lock_table_)
      lock_table_ = new filesystem::LockTable;
    return lock_table_;
  }

 private:
  friend class base::DeleteHelper<DirectoryThreadState>;
  friend class base::RefCountedDeleteOnSequence<DirectoryThreadState>;

  ~DirectoryThreadState() = default;

  scoped_refptr<filesystem::LockTable> lock_table_;

  DISALLOW_COPY_AND_ASSIGN(DirectoryThreadState);
};

class Catalog::ServiceImpl : public service_manager::Service {
 public:
  explicit ServiceImpl(Catalog* catalog) : catalog_(catalog) {
    registry_.AddInterface<mojom::Catalog>(
        base::Bind(&Catalog::BindCatalogRequest, base::Unretained(catalog_)));
    registry_.AddInterface<filesystem::mojom::Directory>(
        base::Bind(&Catalog::BindDirectoryRequest, base::Unretained(catalog_)));
  }
  ~ServiceImpl() override {}

  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe),
                            source_info);
  }

 private:
  Catalog* const catalog_;
  service_manager::BinderRegistryWithArgs<
      const service_manager::BindSourceInfo&>
      registry_;

  DISALLOW_COPY_AND_ASSIGN(ServiceImpl);
};

Catalog::Catalog(std::unique_ptr<base::Value> static_manifest,
                 ManifestProvider* service_manifest_provider)
    : service_context_(new service_manager::ServiceContext(
          std::make_unique<ServiceImpl>(this),
          mojo::MakeRequest(&service_))),
      service_manifest_provider_(service_manifest_provider),
      weak_factory_(this) {
  if (static_manifest) {
    LoadCatalogManifestIntoCache(static_manifest.get(), &system_cache_);
  } else if (g_default_static_manifest.Get()) {
    LoadCatalogManifestIntoCache(
        g_default_static_manifest.Get().get(), &system_cache_);
  }
}

Catalog::~Catalog() {}

service_manager::mojom::ServicePtr Catalog::TakeService() {
  return std::move(service_);
}

// static
void Catalog::SetDefaultCatalogManifest(
    std::unique_ptr<base::Value> static_manifest) {
  g_default_static_manifest.Get() = std::move(static_manifest);
}

// static
void Catalog::LoadDefaultCatalogManifest(const base::FilePath& path) {
  std::string catalog_contents;
  base::FilePath exe_path;
  base::PathService::Get(base::DIR_EXE, &exe_path);
  base::FilePath catalog_path = exe_path.Append(path);
  bool result = base::ReadFileToString(catalog_path, &catalog_contents);
  DCHECK(result);
  std::unique_ptr<base::Value> manifest_value =
      base::JSONReader::Read(catalog_contents);
  DCHECK(manifest_value);
  catalog::Catalog::SetDefaultCatalogManifest(std::move(manifest_value));
}

Instance* Catalog::GetInstanceForUserId(const std::string& user_id) {
  auto it = instances_.find(user_id);
  if (it != instances_.end())
    return it->second.get();

  auto result = instances_.insert(std::make_pair(
      user_id,
      std::make_unique<Instance>(&system_cache_, service_manifest_provider_)));
  return result.first->second.get();
}

void Catalog::BindCatalogRequest(
    mojom::CatalogRequest request,
    const service_manager::BindSourceInfo& source_info) {
  Instance* instance = GetInstanceForUserId(source_info.identity.user_id());
  instance->BindCatalog(std::move(request));
}

void Catalog::BindDirectoryRequest(
    filesystem::mojom::DirectoryRequest request,
    const service_manager::BindSourceInfo& source_info) {
  if (!directory_task_runner_) {
    directory_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::MayBlock(),
         // Use USER_BLOCKING as this gates showing UI during startup.
         base::TaskPriority::USER_BLOCKING,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
    directory_thread_state_ = new DirectoryThreadState(directory_task_runner_);
  }
  directory_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Catalog::BindDirectoryRequestOnBackgroundThread,
                     directory_thread_state_, std::move(request), source_info));
}

// static
void Catalog::BindDirectoryRequestOnBackgroundThread(
    scoped_refptr<DirectoryThreadState> thread_state,
    filesystem::mojom::DirectoryRequest request,
    const service_manager::BindSourceInfo& source_info) {
  base::FilePath resources_path;
  base::PathService::Get(base::DIR_MODULE, &resources_path);
  mojo::MakeStrongBinding(
      std::make_unique<filesystem::DirectoryImpl>(
          resources_path, scoped_refptr<filesystem::SharedTempDir>(),
          thread_state->lock_table()),
      std::move(request));
}

}  // namespace catalog
