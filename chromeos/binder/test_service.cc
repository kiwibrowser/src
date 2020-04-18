// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/test_service.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "chromeos/binder/local_object.h"
#include "chromeos/binder/service_manager_proxy.h"
#include "chromeos/binder/transaction_data.h"
#include "chromeos/binder/transaction_data_reader.h"
#include "chromeos/binder/writable_transaction_data.h"

namespace binder {

class TestService::TestObject : public LocalObject::TransactionHandler {
 public:
  TestObject()
      : event_(false /* manual_reset */, false /* initially_signaled */) {
    VLOG(1) << "Object created: " << this;
  }

  ~TestObject() override { VLOG(1) << "Object destroyed: " << this; }

  std::unique_ptr<binder::TransactionData> OnTransact(
      binder::CommandBroker* command_broker,
      const binder::TransactionData& data) {
    VLOG(1) << "Transact code = " << data.GetCode();
    binder::TransactionDataReader reader(data);
    switch (data.GetCode()) {
      case INCREMENT_INT_TRANSACTION: {
        int32_t arg = 0;
        reader.ReadInt32(&arg);
        std::unique_ptr<binder::WritableTransactionData> reply(
            new binder::WritableTransactionData());
        reply->WriteInt32(arg + 1);
        return std::move(reply);
      }
      case GET_FD_TRANSACTION: {
        // Prepare a file.
        std::string data = GetFileContents();
        base::ScopedTempDir temp_dir;
        base::FilePath path;
        if (!temp_dir.CreateUniqueTempDir() ||
            !base::CreateTemporaryFileInDir(temp_dir.GetPath(), &path) ||
            !base::WriteFile(path, data.data(), data.size())) {
          LOG(ERROR) << "Failed to create a file";
          return std::unique_ptr<TransactionData>();
        }
        // Open the file.
        base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
        if (!file.IsValid()) {
          LOG(ERROR) << "Failed to open the file.";
          return std::unique_ptr<TransactionData>();
        }
        // Return the FD.
        // The file will be deleted by |temp_dir|, but the FD remains valid
        // until the receiving process closes it.
        std::unique_ptr<binder::WritableTransactionData> reply(
            new binder::WritableTransactionData());
        reply->WriteFileDescriptor(base::ScopedFD(file.TakePlatformFile()));
        return std::move(reply);
      }
      case WAIT_TRANSACTION: {
        event_.Wait();
        std::unique_ptr<binder::WritableTransactionData> reply(
            new binder::WritableTransactionData());
        reply->WriteUint32(WAIT_TRANSACTION);
        return std::move(reply);
      }
      case SIGNAL_TRANSACTION: {
        event_.Signal();
        std::unique_ptr<binder::WritableTransactionData> reply(
            new binder::WritableTransactionData());
        reply->WriteUint32(SIGNAL_TRANSACTION);
        return std::move(reply);
      }
    }
    return std::unique_ptr<TransactionData>();
  }

 private:
  base::WaitableEvent event_;

  DISALLOW_COPY_AND_ASSIGN(TestObject);
};

TestService::TestService()
    : service_name_(base::ASCIIToUTF16("org.chromium.TestService-" +
                                       base::GenerateGUID())),
      sub_thread_(&main_thread_) {}

TestService::~TestService() {}

bool TestService::StartAndWait() {
  if (!main_thread_.Start() || !sub_thread_.Start() ||
      !main_thread_.WaitUntilThreadStarted() || !main_thread_.initialized() ||
      !sub_thread_.WaitUntilThreadStarted() || !sub_thread_.initialized()) {
    LOG(ERROR) << "Failed to start the threads.";
    return false;
  }
  bool result = false;
  base::RunLoop run_loop;
  main_thread_.task_runner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&TestService::Initialize, base::Unretained(this), &result),
      run_loop.QuitClosure());
  run_loop.Run();
  return result;
}

void TestService::Stop() {
  sub_thread_.Stop();
  main_thread_.Stop();
}

// static
std::string TestService::GetFileContents() {
  return "Test data";
}

void TestService::Initialize(bool* result) {
  // Add service.
  scoped_refptr<LocalObject> object(
      new LocalObject(base::WrapUnique(new TestObject)));
  *result = ServiceManagerProxy::AddService(main_thread_.command_broker(),
                                            service_name_, object, 0);
}

}  // namespace binder
