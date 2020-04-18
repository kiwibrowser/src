// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_BLOB_IMPL_H_
#define STORAGE_BROWSER_BLOB_BLOB_IMPL_H_

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/mojom/data_pipe_getter.mojom.h"
#include "storage/browser/storage_browser_export.h"
#include "third_party/blink/public/mojom/blob/blob.mojom.h"

namespace storage {

class BlobDataHandle;

// Self destroys when no more bindings exist.
class STORAGE_EXPORT BlobImpl : public blink::mojom::Blob,
                                public network::mojom::DataPipeGetter {
 public:
  static base::WeakPtr<BlobImpl> Create(std::unique_ptr<BlobDataHandle> handle,
                                        blink::mojom::BlobRequest request);

  // blink::mojom::Blob:
  void Clone(blink::mojom::BlobRequest request) override;
  void AsDataPipeGetter(network::mojom::DataPipeGetterRequest request) override;
  void ReadRange(uint64_t offset,
                 uint64_t length,
                 mojo::ScopedDataPipeProducerHandle handle,
                 blink::mojom::BlobReaderClientPtr client) override;
  void ReadAll(mojo::ScopedDataPipeProducerHandle handle,
               blink::mojom::BlobReaderClientPtr client) override;
  void ReadSideData(ReadSideDataCallback callback) override;
  void GetInternalUUID(GetInternalUUIDCallback callback) override;

  // network::mojom::DataPipeGetter:
  void Clone(network::mojom::DataPipeGetterRequest request) override;
  void Read(mojo::ScopedDataPipeProducerHandle pipe,
            ReadCallback callback) override;

  void FlushForTesting();

 private:
  BlobImpl(std::unique_ptr<BlobDataHandle> handle,
           blink::mojom::BlobRequest request);
  ~BlobImpl() override;
  void OnConnectionError();

  std::unique_ptr<BlobDataHandle> handle_;

  mojo::BindingSet<blink::mojom::Blob> bindings_;
  mojo::BindingSet<network::mojom::DataPipeGetter> data_pipe_getter_bindings_;

  base::WeakPtrFactory<BlobImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BlobImpl);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_BLOB_BLOB_IMPL_H_
