// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_RESOURCE_METADATA_H_
#define COMPONENTS_DRIVE_CHROMEOS_RESOURCE_METADATA_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"
#include "components/drive/resource_metadata_storage.h"

namespace base {
class SequencedTaskRunner;
}

namespace drive {

typedef std::vector<ResourceEntry> ResourceEntryVector;

namespace internal {

class FileCache;

// Storage for Drive Metadata.
// All methods except the constructor and Destroy() function must be run with
// |blocking_task_runner| unless otherwise noted.
class ResourceMetadata {
 public:
  typedef ResourceMetadataStorage::Iterator Iterator;

  ResourceMetadata(
      ResourceMetadataStorage* storage,
      FileCache* cache,
      scoped_refptr<base::SequencedTaskRunner> blocking_task_runner);

  // Initializes this object.
  // This method should be called before any other methods.
  FileError Initialize() WARN_UNUSED_RESULT;

  // Destroys this object.  This method posts a task to |blocking_task_runner_|
  // to safely delete this object.
  // Must be called on the UI thread.
  void Destroy();

  // Resets this object.
  FileError Reset();

  // Returns the start page token for the users default corpus.
  FileError GetStartPageToken(std::string* out_value);

  // Sets the start page token for the users default corpus.
  FileError SetStartPageToken(const std::string& value);

  // Adds |entry| to the metadata tree based on its parent_local_id.
  FileError AddEntry(const ResourceEntry& entry, std::string* out_id);

  // Removes entry with |id| from its parent.
  FileError RemoveEntry(const std::string& id);

  // Finds an entry (a file or a directory) by |id|.
  FileError GetResourceEntryById(const std::string& id,
                                 ResourceEntry* out_entry);

  // Synchronous version of GetResourceEntryByPathOnUIThread().
  FileError GetResourceEntryByPath(const base::FilePath& file_path,
                                   ResourceEntry* out_entry);

  // Finds and reads a directory by |file_path|.
  FileError ReadDirectoryByPath(const base::FilePath& file_path,
                                ResourceEntryVector* out_entries);

  // Finds and reads a directory by |id|.
  FileError ReadDirectoryById(const std::string& id,
                              ResourceEntryVector* out_entries);

  // Replaces an existing entry with the same local ID as |entry|.
  FileError RefreshEntry(const ResourceEntry& entry);

  // Recursively gets directories under the entry pointed to by |id|.
  FileError GetSubDirectoriesRecursively(
      const std::string& id,
      std::set<base::FilePath>* sub_directories);

  // Returns the id of the resource named |base_name| directly under
  // the directory with |parent_local_id|.
  // If not found, empty string will be returned.
  FileError GetChildId(const std::string& parent_local_id,
                       const std::string& base_name,
                       std::string* out_child_id);

  // Returns an object to iterate over entries.
  std::unique_ptr<Iterator> GetIterator();

  // Returns virtual file path of the entry.
  FileError GetFilePath(const std::string& id, base::FilePath* out_file_path);

  // Returns ID of the entry at the given path.
  FileError GetIdByPath(const base::FilePath& file_path, std::string* out_id);

  // Returns the local ID associated with the given resource ID.
  FileError GetIdByResourceId(const std::string& resource_id,
                              std::string* out_local_id);

 private:
  // Note: Use Destroy() to delete this object.
  ~ResourceMetadata();

  // Sets up entries which should be present by default.
  FileError SetUpDefaultEntries();

  // Used to implement Destroy().
  void DestroyOnBlockingPool();

  // Puts an entry under its parent directory. Removes the child from the old
  // parent if there is. This method will also do name de-duplication to ensure
  // that the exposed presentation path does not have naming conflicts. Two
  // files with the same name "Foo" will be renamed to "Foo (1)" and "Foo (2)".
  FileError PutEntryUnderDirectory(const ResourceEntry& entry);

  // Returns an unused base name for |entry|.
  FileError GetDeduplicatedBaseName(const ResourceEntry& entry,
                                    std::string* base_name);

  // Removes the entry and its descendants.
  FileError RemoveEntryRecursively(const std::string& id);

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  ResourceMetadataStorage* storage_;
  FileCache* cache_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(ResourceMetadata);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_RESOURCE_METADATA_H_
