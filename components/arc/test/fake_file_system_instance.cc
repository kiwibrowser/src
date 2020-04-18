// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/test/fake_file_system_instance.h"

#include <string.h>
#include <unistd.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/optional.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/edk/embedder/embedder.h"

namespace arc {

namespace {

base::ScopedFD CreateRegularFileDescriptor(const std::string& content,
                                           const base::FilePath& temp_dir) {
  base::FilePath path;
  bool create_success = base::CreateTemporaryFileInDir(temp_dir, &path);
  DCHECK(create_success);
  int written_size = base::WriteFile(path, content.data(), content.size());
  DCHECK_EQ(static_cast<int>(content.size()), written_size);
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  DCHECK(file.IsValid());
  return base::ScopedFD(file.TakePlatformFile());
}

base::ScopedFD CreateStreamFileDescriptor(const std::string& content) {
  int fds[2];
  int ret = pipe(fds);
  DCHECK_EQ(0, ret);
  base::ScopedFD fd_read(fds[0]);
  base::ScopedFD fd_write(fds[1]);
  bool write_success =
      base::WriteFileDescriptor(fd_write.get(), content.data(), content.size());
  DCHECK(write_success);
  return fd_read;
}

mojom::DocumentPtr MakeDocument(const FakeFileSystemInstance::Document& doc) {
  mojom::DocumentPtr document = mojom::Document::New();
  document->document_id = doc.document_id;
  document->display_name = doc.display_name;
  document->mime_type = doc.mime_type;
  document->size = doc.size;
  document->last_modified = doc.last_modified;
  return document;
}

}  // namespace

FakeFileSystemInstance::File::File(const File& that) = default;

FakeFileSystemInstance::File::File(const std::string& url,
                                   const std::string& content,
                                   const std::string& mime_type,
                                   Seekable seekable)
    : url(url), content(content), mime_type(mime_type), seekable(seekable) {}

FakeFileSystemInstance::File::~File() = default;

FakeFileSystemInstance::Document::Document(const Document& that) = default;

FakeFileSystemInstance::Document::Document(
    const std::string& authority,
    const std::string& document_id,
    const std::string& parent_document_id,
    const std::string& display_name,
    const std::string& mime_type,
    int64_t size,
    uint64_t last_modified)
    : authority(authority),
      document_id(document_id),
      parent_document_id(parent_document_id),
      display_name(display_name),
      mime_type(mime_type),
      size(size),
      last_modified(last_modified) {}

FakeFileSystemInstance::Document::~Document() = default;

FakeFileSystemInstance::FakeFileSystemInstance() {
  bool temp_dir_created = temp_dir_.CreateUniqueTempDir();
  DCHECK(temp_dir_created);
}

FakeFileSystemInstance::~FakeFileSystemInstance() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

bool FakeFileSystemInstance::InitCalled() {
  return host_.is_bound();
}

void FakeFileSystemInstance::AddFile(const File& file) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(0u, files_.count(std::string(file.url)));
  files_.insert(std::make_pair(std::string(file.url), file));
}

void FakeFileSystemInstance::AddDocument(const Document& document) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DocumentKey key(document.authority, document.document_id);
  DCHECK_EQ(0u, documents_.count(key));
  documents_.insert(std::make_pair(key, document));
  child_documents_[key];  // Allocate a vector.
  if (!document.parent_document_id.empty()) {
    DocumentKey parent_key(document.authority, document.parent_document_id);
    DCHECK_EQ(1u, documents_.count(parent_key));
    child_documents_[parent_key].push_back(key);
  }
}

void FakeFileSystemInstance::AddRecentDocument(const std::string& root_id,
                                               const Document& document) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  RootKey key(document.authority, root_id);
  recent_documents_[key].push_back(document);
}

void FakeFileSystemInstance::TriggerWatchers(
    const std::string& authority,
    const std::string& document_id,
    storage::WatcherManager::ChangeType type) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!host_) {
    LOG(ERROR) << "FileSystemHost is not available.";
    return;
  }
  auto iter = document_to_watchers_.find(DocumentKey(authority, document_id));
  if (iter == document_to_watchers_.end())
    return;
  for (int64_t watcher_id : iter->second) {
    host_->OnDocumentChanged(watcher_id, type);
  }
}

void FakeFileSystemInstance::AddWatcher(const std::string& authority,
                                        const std::string& document_id,
                                        AddWatcherCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DocumentKey key(authority, document_id);
  auto iter = documents_.find(key);
  if (iter == documents_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), -1));
    return;
  }
  int64_t watcher_id = next_watcher_id_++;
  document_to_watchers_[key].insert(watcher_id);
  watcher_to_document_.insert(std::make_pair(watcher_id, key));
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), watcher_id));
}

void FakeFileSystemInstance::GetFileSize(const std::string& url,
                                         GetFileSizeCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto iter = files_.find(url);
  if (iter == files_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), -1));
    return;
  }
  const File& file = iter->second;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), file.content.size()));
}

void FakeFileSystemInstance::GetMimeType(const std::string& url,
                                         GetMimeTypeCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto iter = files_.find(url);
  if (iter == files_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), base::nullopt));
    return;
  }
  const File& file = iter->second;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), file.mime_type));
}

void FakeFileSystemInstance::OpenFileToRead(const std::string& url,
                                            OpenFileToReadCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto iter = files_.find(url);
  if (iter == files_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), mojo::ScopedHandle()));
    return;
  }
  const File& file = iter->second;
  base::ScopedFD fd =
      file.seekable == File::Seekable::YES
          ? CreateRegularFileDescriptor(file.content, temp_dir_.GetPath())
          : CreateStreamFileDescriptor(file.content);
  mojo::edk::ScopedInternalPlatformHandle platform_handle(
      mojo::edk::InternalPlatformHandle(fd.release()));
  MojoHandle wrapped_handle;
  MojoResult result = mojo::edk::CreateInternalPlatformHandleWrapper(
      std::move(platform_handle), &wrapped_handle);
  DCHECK_EQ(MOJO_RESULT_OK, result);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback),
                     mojo::ScopedHandle(mojo::Handle(wrapped_handle))));
}

void FakeFileSystemInstance::GetDocument(const std::string& authority,
                                         const std::string& document_id,
                                         GetDocumentCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto iter = documents_.find(DocumentKey(authority, document_id));
  if (iter == documents_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), mojom::DocumentPtr()));
    return;
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), MakeDocument(iter->second)));
}

void FakeFileSystemInstance::GetChildDocuments(
    const std::string& authority,
    const std::string& parent_document_id,
    GetChildDocumentsCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  ++get_child_documents_count_;
  auto child_iter =
      child_documents_.find(DocumentKey(authority, parent_document_id));
  if (child_iter == child_documents_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), base::nullopt));
    return;
  }
  std::vector<mojom::DocumentPtr> children;
  for (const auto& child_key : child_iter->second) {
    auto doc_iter = documents_.find(child_key);
    DCHECK(doc_iter != documents_.end());
    children.emplace_back(MakeDocument(doc_iter->second));
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback),
                                base::make_optional(std::move(children))));
}

void FakeFileSystemInstance::GetRecentDocuments(
    const std::string& authority,
    const std::string& root_id,
    GetRecentDocumentsCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto recent_iter = recent_documents_.find(RootKey(authority, root_id));
  if (recent_iter == recent_documents_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), base::nullopt));
    return;
  }
  std::vector<mojom::DocumentPtr> recents;
  for (const Document& document : recent_iter->second)
    recents.emplace_back(MakeDocument(document));
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback),
                                base::make_optional(std::move(recents))));
}

void FakeFileSystemInstance::InitDeprecated(mojom::FileSystemHostPtr host) {
  Init(std::move(host), base::DoNothing());
}

void FakeFileSystemInstance::Init(mojom::FileSystemHostPtr host,
                                  InitCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(host);
  DCHECK(!host_);
  host_ = std::move(host);
  std::move(callback).Run();
}

void FakeFileSystemInstance::RemoveWatcher(int64_t watcher_id,
                                           RemoveWatcherCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto iter = watcher_to_document_.find(watcher_id);
  if (iter == watcher_to_document_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), false));
    return;
  }
  document_to_watchers_[iter->second].erase(watcher_id);
  watcher_to_document_.erase(iter);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), true));
}

void FakeFileSystemInstance::RequestMediaScan(
    const std::vector<std::string>& paths) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Do nothing and pretend we scaned them.
}

}  // namespace arc
