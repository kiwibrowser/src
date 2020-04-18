// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/in_progress_cache_impl.h"

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/task_runner_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/download/downloader/in_progress/in_progress_conversions.h"

namespace download {

const base::FilePath::CharType kDownloadMetadataStoreFilename[] =
    FILE_PATH_LITERAL("in_progress_download_metadata_store");

namespace {

// Helper functions for |entries_| related operations.
int GetIndexFromEntries(const metadata_pb::DownloadEntries& entries,
                        const std::string& guid) {
  int size_of_entries = entries.entries_size();
  for (int i = 0; i < size_of_entries; i++) {
    if (entries.entries(i).guid() == guid)
      return i;
  }
  return -1;
}

void AddOrReplaceEntryInEntries(metadata_pb::DownloadEntries& entries,
                                const DownloadEntry& entry) {
  metadata_pb::DownloadEntry metadata_entry =
      InProgressConversions::DownloadEntryToProto(entry);
  int entry_index = GetIndexFromEntries(entries, metadata_entry.guid());
  metadata_pb::DownloadEntry* entry_ptr =
      (entry_index < 0) ? entries.add_entries()
                        : entries.mutable_entries(entry_index);
  *entry_ptr = metadata_entry;
}

base::Optional<DownloadEntry> GetEntryFromEntries(
    const metadata_pb::DownloadEntries& entries,
    const std::string& guid) {
  int entry_index = GetIndexFromEntries(entries, guid);
  if (entry_index < 0)
    return base::nullopt;
  return InProgressConversions::DownloadEntryFromProto(
      entries.entries(entry_index));
}

void RemoveEntryFromEntries(metadata_pb::DownloadEntries& entries,
                            const std::string& guid) {
  int entry_index = GetIndexFromEntries(entries, guid);
  if (entry_index >= 0)
    entries.mutable_entries()->DeleteSubrange(entry_index, 1);
}

// Helper functions for file read/write operations.
std::vector<char> ReadEntriesFromFile(base::FilePath file_path) {
  if (file_path.empty())
    return std::vector<char>();

  // Check validity of file.
  base::File entries_file(file_path,
                          base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!entries_file.IsValid()) {
    // The file just doesn't exist yet (potentially because no entries have been
    // written yet) so we can't read from it.
    return std::vector<char>();
  }

  // Get file info.
  base::File::Info info;
  if (!entries_file.GetInfo(&info)) {
    LOG(ERROR) << "Could not read download entries from file "
               << "because get info failed.";
    return std::vector<char>();
  }

  if (info.is_directory) {
    LOG(ERROR) << "Could not read download entries from file "
               << "because file is a directory.";
    return std::vector<char>();
  }

  // Read and parse file.
  if (info.size < 0) {
    LOG(ERROR) << "Could not read download entries from file "
               << "because the file size was unexpected.";
    return std::vector<char>();
  }

  auto file_data = std::vector<char>(info.size);
  if (entries_file.Read(0, file_data.data(), info.size) < 0) {
    LOG(ERROR) << "Could not read download entries from file "
               << "because there was a read failure.";
    return std::vector<char>();
  }

  return file_data;
}

std::string EntriesToString(const metadata_pb::DownloadEntries& entries) {
  std::string entries_string;
  if (!entries.SerializeToString(&entries_string)) {
    // TODO(crbug.com/778425): Have more robust error-handling for serialization
    // error.
    LOG(ERROR) << "Could not write download entries to file "
               << "because of a serialization issue.";
    return std::string();
  }
  return entries_string;
}

void WriteEntriesToFile(const std::string& entries, base::FilePath file_path) {
  if (file_path.empty())
    return;

  if (!base::ImportantFileWriter::WriteFileAtomically(file_path, entries)) {
    LOG(ERROR) << "Could not write download entries to file: "
               << file_path.value();
  }
}
}  // namespace

InProgressCacheImpl::InProgressCacheImpl(
    const base::FilePath& cache_file_path,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : file_path_(cache_file_path),
      initialization_status_(CACHE_UNINITIALIZED),
      task_runner_(task_runner),
      weak_ptr_factory_(this) {}

InProgressCacheImpl::~InProgressCacheImpl() = default;

void InProgressCacheImpl::Initialize(base::OnceClosure callback) {
  // If it's already initialized, just run the callback.
  if (initialization_status_ == CACHE_INITIALIZED) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  std::move(callback));
    return;
  }

  // If uninitialized, initialize |entries_| by reading from file.
  if (initialization_status_ == CACHE_UNINITIALIZED) {
    base::PostTaskAndReplyWithResult(
        task_runner_.get(), FROM_HERE,
        base::BindOnce(&ReadEntriesFromFile, file_path_),
        base::BindOnce(&InProgressCacheImpl::OnInitialized,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }
}

void InProgressCacheImpl::OnInitialized(base::OnceClosure callback,
                                        const std::vector<char>& entries) {
  if (!entries.empty()) {
    if (!entries_.ParseFromArray(entries.data(), entries.size())) {
      // TODO(crbug.com/778425): Get UMA for errors.
      LOG(ERROR) << "Could not read download entries from file "
                 << "because there was a parse failure.";
      return;
    }
  }

  initialization_status_ = CACHE_INITIALIZED;

  std::move(callback).Run();
}

void InProgressCacheImpl::AddOrReplaceEntry(const DownloadEntry& entry) {
  if (initialization_status_ != CACHE_INITIALIZED) {
    LOG(ERROR) << "Cache is not initialized, cannot AddOrReplaceEntry.";
    return;
  }

  // Update |entries_|.
  AddOrReplaceEntryInEntries(entries_, entry);

  // Serialize |entries_| and write to file.
  std::string entries_string = EntriesToString(entries_);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&WriteEntriesToFile,
                                                   entries_string, file_path_));
}

base::Optional<DownloadEntry> InProgressCacheImpl::RetrieveEntry(
    const std::string& guid) {
  if (initialization_status_ != CACHE_INITIALIZED) {
    LOG(ERROR) << "Cache is not initialized, cannot RetrieveEntry.";
    return base::nullopt;
  }

  return GetEntryFromEntries(entries_, guid);
}

void InProgressCacheImpl::RemoveEntry(const std::string& guid) {
  if (initialization_status_ != CACHE_INITIALIZED) {
    LOG(ERROR) << "Cache is not initialized, cannot RemoveEntry.";
    return;
  }

  // Update |entries_|.
  RemoveEntryFromEntries(entries_, guid);

  // Serialize |entries_| and write to file.
  std::string entries_string = EntriesToString(entries_);
  task_runner_->PostTask(FROM_HERE, base::BindOnce(&WriteEntriesToFile,
                                                   entries_string, file_path_));
}

}  // namespace download
