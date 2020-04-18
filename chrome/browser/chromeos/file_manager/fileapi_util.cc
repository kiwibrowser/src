// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_manager/fileapi_util.h"

#include <stddef.h>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/drive/file_system_util.h"
#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/chromeos/file_manager/filesystem_api_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/drive/file_system_core_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/file_chooser_file_info.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/extension.h"
#include "google_apis/drive/task_util.h"
#include "net/base/escape.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/browser/fileapi/open_file_system_mode.h"
#include "storage/common/fileapi/file_system_util.h"
#include "ui/shell_dialogs/selected_file_info.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace file_manager {
namespace util {

namespace {

GURL ConvertRelativeFilePathToFileSystemUrl(const base::FilePath& relative_path,
                                            const std::string& extension_id) {
  GURL base_url = storage::GetFileSystemRootURI(
      extensions::Extension::GetBaseURLFromExtensionId(extension_id),
      storage::kFileSystemTypeExternal);
  return GURL(base_url.spec() +
              net::EscapeUrlEncodedData(relative_path.AsUTF8Unsafe(),
                                        false));  // Space to %20 instead of +.
}

// Creates an ErrorDefinition with an error set to |error|.
EntryDefinition CreateEntryDefinitionWithError(base::File::Error error) {
  EntryDefinition result;
  result.error = error;
  return result;
}

// Helper class for performing conversions from file definitions to entry
// definitions. It is possible to do it without a class, but the code would be
// crazy and super tricky.
//
// This class copies the input |file_definition_list|,
// so there is no need to worry about validity of passed |file_definition_list|
// reference. Also, it automatically deletes itself after converting finished,
// or if shutdown is invoked during ResolveURL(). Must be called on UI thread.
class FileDefinitionListConverter {
 public:
  FileDefinitionListConverter(Profile* profile,
                              const std::string& extension_id,
                              const FileDefinitionList& file_definition_list,
                              const EntryDefinitionListCallback& callback);
  ~FileDefinitionListConverter() = default;

 private:
  // Converts the element under the iterator to an entry. First, converts
  // the virtual path to an URL, and calls OnResolvedURL(). In case of error
  // calls OnIteratorConverted with an error entry definition.
  void ConvertNextIterator(
      std::unique_ptr<FileDefinitionListConverter> self_deleter,
      FileDefinitionList::const_iterator iterator);

  // Creates an entry definition from the URL as well as the file definition.
  // Then, calls OnIteratorConverted with the created entry definition.
  void OnResolvedURL(std::unique_ptr<FileDefinitionListConverter> self_deleter,
                     FileDefinitionList::const_iterator iterator,
                     base::File::Error error,
                     const storage::FileSystemInfo& info,
                     const base::FilePath& file_path,
                     storage::FileSystemContext::ResolvedEntryType type);

  // Called when the iterator is converted. Adds the |entry_definition| to
  // |results_| and calls ConvertNextIterator() for the next element.
  void OnIteratorConverted(
      std::unique_ptr<FileDefinitionListConverter> self_deleter,
      FileDefinitionList::const_iterator iterator,
      const EntryDefinition& entry_definition);

  scoped_refptr<storage::FileSystemContext> file_system_context_;
  const std::string extension_id_;
  const FileDefinitionList file_definition_list_;
  const EntryDefinitionListCallback callback_;
  std::unique_ptr<EntryDefinitionList> result_;
};

FileDefinitionListConverter::FileDefinitionListConverter(
    Profile* profile,
    const std::string& extension_id,
    const FileDefinitionList& file_definition_list,
    const EntryDefinitionListCallback& callback)
    : extension_id_(extension_id),
      file_definition_list_(file_definition_list),
      callback_(callback),
      result_(new EntryDefinitionList) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // File browser APIs are meant to be used only from extension context, so
  // the extension's site is the one in whose file system context the virtual
  // path should be found.
  GURL site = extensions::util::GetSiteForExtensionId(extension_id_, profile);
  file_system_context_ =
      content::BrowserContext::GetStoragePartitionForSite(
          profile, site)->GetFileSystemContext();

  // Deletes the converter, once the scoped pointer gets out of scope. It is
  // either, if the conversion is finished, or ResolveURL() is terminated, and
  // the callback not called because of shutdown.
  std::unique_ptr<FileDefinitionListConverter> self_deleter(this);
  ConvertNextIterator(std::move(self_deleter), file_definition_list_.begin());
}

void FileDefinitionListConverter::ConvertNextIterator(
    std::unique_ptr<FileDefinitionListConverter> self_deleter,
    FileDefinitionList::const_iterator iterator) {
  if (iterator == file_definition_list_.end()) {
    // The converter object will be destroyed since |self_deleter| gets out of
    // scope.
    callback_.Run(std::move(result_));
    return;
  }

  if (!file_system_context_.get()) {
    OnIteratorConverted(std::move(self_deleter), iterator,
                        CreateEntryDefinitionWithError(
                            base::File::FILE_ERROR_INVALID_OPERATION));
    return;
  }

  storage::FileSystemURL url = file_system_context_->CreateCrackedFileSystemURL(
      extensions::Extension::GetBaseURLFromExtensionId(extension_id_),
      storage::kFileSystemTypeExternal,
      iterator->virtual_path);
  DCHECK(url.is_valid());

  // The converter object will be deleted if the callback is not called because
  // of shutdown during ResolveURL().
  file_system_context_->ResolveURL(
      url,
      base::Bind(&FileDefinitionListConverter::OnResolvedURL,
                 base::Unretained(this),
                 base::Passed(&self_deleter),
                 iterator));
}

void FileDefinitionListConverter::OnResolvedURL(
    std::unique_ptr<FileDefinitionListConverter> self_deleter,
    FileDefinitionList::const_iterator iterator,
    base::File::Error error,
    const storage::FileSystemInfo& info,
    const base::FilePath& file_path,
    storage::FileSystemContext::ResolvedEntryType type) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (error != base::File::FILE_OK) {
    OnIteratorConverted(std::move(self_deleter), iterator,
                        CreateEntryDefinitionWithError(error));
    return;
  }

  EntryDefinition entry_definition;
  entry_definition.file_system_root_url = info.root_url.spec();
  entry_definition.file_system_name = info.name;
  switch (type) {
    case storage::FileSystemContext::RESOLVED_ENTRY_FILE:
      entry_definition.is_directory = false;
      break;
    case storage::FileSystemContext::RESOLVED_ENTRY_DIRECTORY:
      entry_definition.is_directory = true;
      break;
    case storage::FileSystemContext::RESOLVED_ENTRY_NOT_FOUND:
      entry_definition.is_directory = iterator->is_directory;
      break;
  }
  entry_definition.error = base::File::FILE_OK;

  // Construct a target Entry.fullPath value from the virtual path and the
  // root URL. Eg. Downloads/A/b.txt -> A/b.txt.
  const base::FilePath root_virtual_path =
      file_system_context_->CrackURL(info.root_url).virtual_path();
  DCHECK(root_virtual_path == iterator->virtual_path ||
         root_virtual_path.IsParent(iterator->virtual_path));
  base::FilePath full_path;
  root_virtual_path.AppendRelativePath(iterator->virtual_path, &full_path);
  entry_definition.full_path = full_path;

  OnIteratorConverted(std::move(self_deleter), iterator, entry_definition);
}

void FileDefinitionListConverter::OnIteratorConverted(
    std::unique_ptr<FileDefinitionListConverter> self_deleter,
    FileDefinitionList::const_iterator iterator,
    const EntryDefinition& entry_definition) {
  result_->push_back(entry_definition);
  ConvertNextIterator(std::move(self_deleter), ++iterator);
}

// Helper function to return the converted definition entry directly, without
// the redundant container.
void OnConvertFileDefinitionDone(
    const EntryDefinitionCallback& callback,
    std::unique_ptr<EntryDefinitionList> entry_definition_list) {
  DCHECK_EQ(1u, entry_definition_list->size());
  callback.Run(entry_definition_list->at(0));
}

// Checks if the |file_path| points non-native location or not.
bool IsUnderNonNativeLocalPath(const storage::FileSystemContext& context,
                               const base::FilePath& file_path) {
  base::FilePath virtual_path;
  if (!context.external_backend()->GetVirtualPath(file_path, &virtual_path))
    return false;

  const storage::FileSystemURL url = context.CreateCrackedFileSystemURL(
      GURL(), storage::kFileSystemTypeExternal, virtual_path);
  if (!url.is_valid())
    return false;

  return IsNonNativeFileSystemType(url.type());
}

// Helper class to convert SelectedFileInfoList into ChooserFileInfoList.
class ConvertSelectedFileInfoListToFileChooserFileInfoListImpl {
 public:
  // The scoped pointer to control lifetime of the instance itself. The pointer
  // is passed to callback functions and binds the lifetime of the instance to
  // the callback's lifetime.
  typedef std::unique_ptr<
      ConvertSelectedFileInfoListToFileChooserFileInfoListImpl>
      Lifetime;

  ConvertSelectedFileInfoListToFileChooserFileInfoListImpl(
      storage::FileSystemContext* context,
      const GURL& origin,
      const SelectedFileInfoList& selected_info_list,
      const FileChooserFileInfoListCallback& callback)
      : context_(context),
        chooser_info_list_(new FileChooserFileInfoList),
        callback_(callback) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    Lifetime lifetime(this);
    bool need_fill_metadata = false;

    for (size_t i = 0; i < selected_info_list.size(); ++i) {
      content::FileChooserFileInfo chooser_info;

      // Native file.
      if (!IsUnderNonNativeLocalPath(*context,
                                     selected_info_list[i].file_path)) {
        chooser_info.file_path = selected_info_list[i].file_path;
        chooser_info.display_name = selected_info_list[i].display_name;
        chooser_info_list_->push_back(chooser_info);
        continue;
      }

      // Non-native file, but it has a native snapshot file.
      if (!selected_info_list[i].local_path.empty()) {
        chooser_info.file_path = selected_info_list[i].local_path;
        chooser_info.display_name = selected_info_list[i].display_name;
        chooser_info_list_->push_back(chooser_info);
        continue;
      }

      // Non-native file without a snapshot file.
      base::FilePath virtual_path;
      if (!context->external_backend()->GetVirtualPath(
              selected_info_list[i].file_path, &virtual_path)) {
        NotifyError(std::move(lifetime));
        return;
      }

      const GURL url = CreateIsolatedURLFromVirtualPath(
                           *context_, origin, virtual_path).ToGURL();
      if (!url.is_valid()) {
        NotifyError(std::move(lifetime));
        return;
      }

      chooser_info.file_path = selected_info_list[i].file_path;
      chooser_info.file_system_url = url;
      chooser_info_list_->push_back(chooser_info);
      need_fill_metadata = true;
    }

    // If the list includes at least one non-native file (wihtout a snapshot
    // file), move to IO thread to obtian metadata for the non-native file.
    if (need_fill_metadata) {
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::BindOnce(
              &ConvertSelectedFileInfoListToFileChooserFileInfoListImpl::
                  FillMetadataOnIOThread,
              base::Unretained(this), base::Passed(&lifetime),
              chooser_info_list_->begin()));
      return;
    }

    NotifyComplete(std::move(lifetime));
  }

  ~ConvertSelectedFileInfoListToFileChooserFileInfoListImpl() {
    if (chooser_info_list_) {
      for (size_t i = 0; i < chooser_info_list_->size(); ++i) {
        if (chooser_info_list_->at(i).file_system_url.is_valid()) {
          storage::IsolatedContext::GetInstance()->RevokeFileSystem(
              context_->CrackURL(chooser_info_list_->at(i).file_system_url)
                  .mount_filesystem_id());
        }
      }
    }
  }

 private:
  // Obtains metadata for the non-native file |it|.
  void FillMetadataOnIOThread(Lifetime lifetime,
                              const FileChooserFileInfoList::iterator& it) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);

    if (it == chooser_info_list_->end()) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::BindOnce(
              &ConvertSelectedFileInfoListToFileChooserFileInfoListImpl::
                  NotifyComplete,
              base::Unretained(this), base::Passed(&lifetime)));
      return;
    }

    if (!it->file_system_url.is_valid()) {
      FillMetadataOnIOThread(std::move(lifetime), it + 1);
      return;
    }

    context_->operation_runner()->GetMetadata(
        context_->CrackURL(it->file_system_url),
        storage::FileSystemOperation::GET_METADATA_FIELD_IS_DIRECTORY |
            storage::FileSystemOperation::GET_METADATA_FIELD_SIZE |
            storage::FileSystemOperation::GET_METADATA_FIELD_LAST_MODIFIED,
        base::Bind(&ConvertSelectedFileInfoListToFileChooserFileInfoListImpl::
                       OnGotMetadataOnIOThread,
                   base::Unretained(this), base::Passed(&lifetime), it));
  }

  // Callback invoked after GetMetadata.
  void OnGotMetadataOnIOThread(Lifetime lifetime,
                               const FileChooserFileInfoList::iterator& it,
                               base::File::Error result,
                               const base::File::Info& file_info) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);

    if (result != base::File::FILE_OK) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::BindOnce(
              &ConvertSelectedFileInfoListToFileChooserFileInfoListImpl::
                  NotifyError,
              base::Unretained(this), base::Passed(&lifetime)));
      return;
    }

    it->length = file_info.size;
    it->modification_time = file_info.last_modified;
    it->is_directory = file_info.is_directory;
    FillMetadataOnIOThread(std::move(lifetime), it + 1);
  }

  // Returns a result to the |callback_|.
  void NotifyComplete(Lifetime /* lifetime */) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    callback_.Run(*chooser_info_list_);
    // Reset the list so that the file systems are not revoked at the
    // destructor.
    chooser_info_list_.reset();
  }

  // Returns an empty list to the |callback_|.
  void NotifyError(Lifetime /* lifetime */) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    callback_.Run(FileChooserFileInfoList());
  }

  scoped_refptr<storage::FileSystemContext> context_;
  std::unique_ptr<FileChooserFileInfoList> chooser_info_list_;
  const FileChooserFileInfoListCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(
      ConvertSelectedFileInfoListToFileChooserFileInfoListImpl);
};

}  // namespace

EntryDefinition::EntryDefinition() = default;

EntryDefinition::EntryDefinition(const EntryDefinition& other) = default;

EntryDefinition::~EntryDefinition() = default;

storage::FileSystemContext* GetFileSystemContextForExtensionId(
    Profile* profile,
    const std::string& extension_id) {
  GURL site = extensions::util::GetSiteForExtensionId(extension_id, profile);
  return content::BrowserContext::GetStoragePartitionForSite(profile, site)->
      GetFileSystemContext();
}

storage::FileSystemContext* GetFileSystemContextForRenderFrameHost(
    Profile* profile,
    content::RenderFrameHost* render_frame_host) {
  content::SiteInstance* site_instance = render_frame_host->GetSiteInstance();
  return content::BrowserContext::GetStoragePartition(profile, site_instance)->
      GetFileSystemContext();
}

base::FilePath ConvertDrivePathToRelativeFileSystemPath(
    Profile* profile,
    const std::string& extension_id,
    const base::FilePath& drive_path) {
  // "/special/drive-xxx"
  base::FilePath path = drive::util::GetDriveMountPointPath(profile);
  // appended with (|drive_path| - "drive").
  drive::util::GetDriveGrandRootPath().AppendRelativePath(drive_path, &path);

  base::FilePath relative_path;
  ConvertAbsoluteFilePathToRelativeFileSystemPath(profile,
                                                  extension_id,
                                                  path,
                                                  &relative_path);
  return relative_path;
}

GURL ConvertDrivePathToFileSystemUrl(Profile* profile,
                                     const base::FilePath& drive_path,
                                     const std::string& extension_id) {
  const base::FilePath relative_path =
      ConvertDrivePathToRelativeFileSystemPath(profile, extension_id,
                                               drive_path);
  if (relative_path.empty())
    return GURL();
  return ConvertRelativeFilePathToFileSystemUrl(relative_path, extension_id);
}

bool ConvertAbsoluteFilePathToFileSystemUrl(Profile* profile,
                                            const base::FilePath& absolute_path,
                                            const std::string& extension_id,
                                            GURL* url) {
  base::FilePath relative_path;
  if (!ConvertAbsoluteFilePathToRelativeFileSystemPath(profile,
                                                       extension_id,
                                                       absolute_path,
                                                       &relative_path)) {
    return false;
  }
  *url = ConvertRelativeFilePathToFileSystemUrl(relative_path, extension_id);
  return true;
}

bool ConvertAbsoluteFilePathToRelativeFileSystemPath(
    Profile* profile,
    const std::string& extension_id,
    const base::FilePath& absolute_path,
    base::FilePath* virtual_path) {
  // File browser APIs are meant to be used only from extension context, so the
  // extension's site is the one in whose file system context the virtual path
  // should be found.
  GURL site = extensions::util::GetSiteForExtensionId(extension_id, profile);
  storage::ExternalFileSystemBackend* backend =
      content::BrowserContext::GetStoragePartitionForSite(profile, site)
          ->GetFileSystemContext()
          ->external_backend();
  if (!backend)
    return false;

  // Find if this file path is managed by the external backend.
  if (!backend->GetVirtualPath(absolute_path, virtual_path))
    return false;

  return true;
}

void ConvertFileDefinitionListToEntryDefinitionList(
    Profile* profile,
    const std::string& extension_id,
    const FileDefinitionList& file_definition_list,
    const EntryDefinitionListCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // The converter object destroys itself.
  new FileDefinitionListConverter(
      profile, extension_id, file_definition_list, callback);
}

void ConvertFileDefinitionToEntryDefinition(
    Profile* profile,
    const std::string& extension_id,
    const FileDefinition& file_definition,
    const EntryDefinitionCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  FileDefinitionList file_definition_list;
  file_definition_list.push_back(file_definition);
  ConvertFileDefinitionListToEntryDefinitionList(
      profile,
      extension_id,
      file_definition_list,
      base::Bind(&OnConvertFileDefinitionDone, callback));
}

void ConvertSelectedFileInfoListToFileChooserFileInfoList(
    storage::FileSystemContext* context,
    const GURL& origin,
    const SelectedFileInfoList& selected_info_list,
    const FileChooserFileInfoListCallback& callback) {
  // The object deletes itself.
  new ConvertSelectedFileInfoListToFileChooserFileInfoListImpl(
      context, origin, selected_info_list, callback);
}

void CheckIfDirectoryExists(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const base::FilePath& directory_path,
    const storage::FileSystemOperationRunner::StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  storage::ExternalFileSystemBackend* const backend =
      file_system_context->external_backend();
  DCHECK(backend);
  const storage::FileSystemURL internal_url =
      backend->CreateInternalURL(file_system_context.get(), directory_path);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(base::IgnoreResult(
                         &storage::FileSystemOperationRunner::DirectoryExists),
                     file_system_context->operation_runner()->AsWeakPtr(),
                     internal_url, google_apis::CreateRelayCallback(callback)));
}

void GetMetadataForPath(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const base::FilePath& entry_path,
    int fields,
    const storage::FileSystemOperationRunner::GetMetadataCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  storage::ExternalFileSystemBackend* const backend =
      file_system_context->external_backend();
  DCHECK(backend);
  const storage::FileSystemURL internal_url =
      backend->CreateInternalURL(file_system_context.get(), entry_path);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          base::IgnoreResult(&storage::FileSystemOperationRunner::GetMetadata),
          file_system_context->operation_runner()->AsWeakPtr(), internal_url,
          fields, google_apis::CreateRelayCallback(callback)));
}

storage::FileSystemURL CreateIsolatedURLFromVirtualPath(
    const storage::FileSystemContext& context,
    const GURL& origin,
    const base::FilePath& virtual_path) {
  const storage::FileSystemURL original_url =
      context.CreateCrackedFileSystemURL(
          origin, storage::kFileSystemTypeExternal, virtual_path);

  std::string register_name;
  const std::string isolated_file_system_id =
      storage::IsolatedContext::GetInstance()->RegisterFileSystemForPath(
          original_url.type(),
          original_url.filesystem_id(),
          original_url.path(),
          &register_name);
  const storage::FileSystemURL isolated_url =
      context.CreateCrackedFileSystemURL(
          origin,
          storage::kFileSystemTypeIsolated,
          base::FilePath(isolated_file_system_id).Append(register_name));
  return isolated_url;
}

}  // namespace util
}  // namespace file_manager
