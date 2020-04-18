// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/leveldb_wrapper_test_util.h"

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"

namespace content {
namespace test {
namespace {
void SuccessCallback(base::OnceClosure callback,
                     bool* success_out,
                     bool success) {
  *success_out = success;
  std::move(callback).Run();
}
}  // namespace

base::OnceCallback<void(bool)> MakeSuccessCallback(base::OnceClosure callback,
                                                   bool* success_out) {
  return base::BindOnce(&SuccessCallback, std::move(callback), success_out);
}

bool PutSync(mojom::LevelDBWrapper* wrapper,
             const std::vector<uint8_t>& key,
             const std::vector<uint8_t>& value,
             const base::Optional<std::vector<uint8_t>>& old_value,
             const std::string& source) {
  bool success = false;
  base::RunLoop loop;
  wrapper->Put(key, value, old_value, source,
               base::BindLambdaForTesting([&](bool success_in) {
                 success = success_in;
                 loop.Quit();
               }));
  loop.Run();
  return success;
}

bool GetSync(mojom::LevelDBWrapper* wrapper,
             const std::vector<uint8_t>& key,
             std::vector<uint8_t>* data_out) {
  bool success = false;
  base::RunLoop loop;
  wrapper->Get(key,
               base::BindLambdaForTesting(
                   [&](bool success_in, const std::vector<uint8_t>& value) {
                     success = success_in;
                     *data_out = std::move(value);
                     loop.Quit();
                   }));
  loop.Run();
  return success;
}

leveldb::mojom::DatabaseError GetAllSync(
    mojom::LevelDBWrapper* wrapper,
    std::vector<mojom::KeyValuePtr>* data_out) {
  DCHECK(data_out);
  base::RunLoop loop;
  bool complete = false;
  leveldb::mojom::DatabaseError status =
      leveldb::mojom::DatabaseError::INVALID_ARGUMENT;
  wrapper->GetAll(
      GetAllCallback::CreateAndBind(&complete, loop.QuitClosure()),
      base::BindLambdaForTesting([&](leveldb::mojom::DatabaseError status_in,
                                     std::vector<mojom::KeyValuePtr> data_in) {
        status = status_in;
        *data_out = std::move(data_in);
      }));
  loop.Run();
  DCHECK(complete);
  return status;
}

leveldb::mojom::DatabaseError GetAllSyncOnDedicatedPipe(
    mojom::LevelDBWrapper* wrapper,
    std::vector<mojom::KeyValuePtr>* data_out) {
  DCHECK(data_out);
  base::RunLoop loop;
  bool complete = false;
  leveldb::mojom::DatabaseError status =
      leveldb::mojom::DatabaseError::INVALID_ARGUMENT;
  wrapper->GetAll(
      GetAllCallback::CreateAndBindOnDedicatedPipe(&complete,
                                                   loop.QuitClosure()),
      base::BindLambdaForTesting([&](leveldb::mojom::DatabaseError status_in,
                                     std::vector<mojom::KeyValuePtr> data_in) {
        status = status_in;
        *data_out = std::move(data_in);
      }));
  loop.Run();
  DCHECK(complete);
  return status;
}

bool DeleteSync(mojom::LevelDBWrapper* wrapper,
                const std::vector<uint8_t>& key,
                const base::Optional<std::vector<uint8_t>>& client_old_value,
                const std::string& source) {
  bool success = false;
  base::RunLoop loop;
  wrapper->Delete(key, client_old_value, source,
                  base::BindLambdaForTesting([&](bool success_in) {
                    success = success_in;
                    loop.Quit();
                  }));
  loop.Run();
  return success;
}

bool DeleteAllSync(mojom::LevelDBWrapper* wrapper, const std::string& source) {
  bool success = false;
  base::RunLoop loop;
  wrapper->DeleteAll(source, base::BindLambdaForTesting([&](bool success_in) {
                       success = success_in;
                       loop.Quit();
                     }));
  loop.Run();
  return success;
}

base::OnceCallback<void(leveldb::mojom::DatabaseError,
                        std::vector<mojom::KeyValuePtr>)>
MakeGetAllCallback(leveldb::mojom::DatabaseError* status_out,
                   std::vector<mojom::KeyValuePtr>* data_out) {
  DCHECK(status_out);
  DCHECK(data_out);
  return base::BindLambdaForTesting(
      [status_out, data_out](leveldb::mojom::DatabaseError status_in,
                             std::vector<mojom::KeyValuePtr> data_in) {
        *status_out = status_in;
        *data_out = std::move(data_in);
      });
}

// static
mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo
GetAllCallback::CreateAndBind(bool* result, base::OnceClosure callback) {
  mojom::LevelDBWrapperGetAllCallbackAssociatedPtr ptr;
  auto request = mojo::MakeRequest(&ptr);
  mojo::MakeStrongAssociatedBinding(
      base::WrapUnique(new GetAllCallback(result, std::move(callback))),
      std::move(request));
  return ptr.PassInterface();
}

// static
mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo
GetAllCallback::CreateAndBindOnDedicatedPipe(bool* result,
                                             base::OnceClosure callback) {
  mojom::LevelDBWrapperGetAllCallbackAssociatedPtr ptr;
  auto request = mojo::MakeRequestAssociatedWithDedicatedPipe(&ptr);
  mojo::MakeStrongAssociatedBinding(
      base::WrapUnique(new GetAllCallback(result, std::move(callback))),
      std::move(request));
  return ptr.PassInterface();
}

GetAllCallback::GetAllCallback(bool* result, base::OnceClosure callback)
    : result_(result), callback_(std::move(callback)) {}

GetAllCallback::~GetAllCallback() = default;

void GetAllCallback::Complete(bool success) {
  *result_ = success;
  if (callback_)
    std::move(callback_).Run();
}

MockLevelDBObserver::MockLevelDBObserver() : binding_(this) {}
MockLevelDBObserver::~MockLevelDBObserver() = default;

mojom::LevelDBObserverAssociatedPtrInfo MockLevelDBObserver::Bind() {
  mojom::LevelDBObserverAssociatedPtrInfo ptr_info;
  binding_.Bind(mojo::MakeRequest(&ptr_info));
  return ptr_info;
}

}  // namespace test
}  // namespace content
