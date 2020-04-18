// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_BLOB_REGISTRY_IMPL_H_
#define STORAGE_BROWSER_BLOB_BLOB_REGISTRY_IMPL_H_

#include <memory>
#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/storage_browser_export.h"
#include "third_party/blink/public/mojom/blob/blob_registry.mojom.h"

namespace storage {

class BlobBuilderFromStream;
class BlobDataHandle;
class BlobStorageContext;
class FileSystemURL;

class STORAGE_EXPORT BlobRegistryImpl : public blink::mojom::BlobRegistry {
 public:
  // Per binding delegate, used for security checks for requests coming in on
  // specific bindings/from specific processes.
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual bool CanReadFile(const base::FilePath& file) = 0;
    virtual bool CanReadFileSystemFile(const FileSystemURL& url) = 0;
    virtual bool CanCommitURL(const GURL& url) = 0;
  };

  BlobRegistryImpl(base::WeakPtr<BlobStorageContext> context,
                   scoped_refptr<FileSystemContext> file_system_context);
  ~BlobRegistryImpl() override;

  void Bind(blink::mojom::BlobRegistryRequest request,
            std::unique_ptr<Delegate> delegate);

  void Register(blink::mojom::BlobRequest blob,
                const std::string& uuid,
                const std::string& content_type,
                const std::string& content_disposition,
                std::vector<blink::mojom::DataElementPtr> elements,
                RegisterCallback callback) override;
  void RegisterFromStream(
      const std::string& content_type,
      const std::string& content_disposition,
      uint64_t expected_length,
      mojo::ScopedDataPipeConsumerHandle data,
      blink::mojom::ProgressClientAssociatedPtrInfo progress_client,
      RegisterFromStreamCallback callback) override;
  void GetBlobFromUUID(blink::mojom::BlobRequest blob,
                       const std::string& uuid,
                       GetBlobFromUUIDCallback callback) override;

  void URLStoreForOrigin(
      const url::Origin& origin,
      blink::mojom::BlobURLStoreAssociatedRequest url_store) override;

  size_t BlobsUnderConstructionForTesting() const {
    return blobs_under_construction_.size();
  }

  using URLStoreCreationHook = base::RepeatingCallback<void(
      mojo::StrongAssociatedBindingPtr<blink::mojom::BlobURLStore>)>;
  static void SetURLStoreCreationHookForTesting(URLStoreCreationHook* hook);

 private:
  class BlobUnderConstruction;

  void BlobBuildAborted(const std::string& uuid);

  void StreamingBlobDone(RegisterFromStreamCallback callback,
                         BlobBuilderFromStream* builder,
                         std::unique_ptr<BlobDataHandle> result);

  base::WeakPtr<BlobStorageContext> context_;
  scoped_refptr<FileSystemContext> file_system_context_;

  mojo::BindingSet<blink::mojom::BlobRegistry, std::unique_ptr<Delegate>>
      bindings_;

  std::map<std::string, std::unique_ptr<BlobUnderConstruction>>
      blobs_under_construction_;
  base::flat_set<std::unique_ptr<BlobBuilderFromStream>,
                 base::UniquePtrComparator>
      blobs_being_streamed_;

  base::WeakPtrFactory<BlobRegistryImpl> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BlobRegistryImpl);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_BLOB_BLOB_REGISTRY_IMPL_H_
