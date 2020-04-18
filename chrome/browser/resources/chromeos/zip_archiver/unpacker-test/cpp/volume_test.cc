// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "volume.h"

#include "native_client_sdk/src/libraries/ppapi_simple/ps_main.h"
#include "request.h"
#include "testing/gtest/gtest.h"

namespace {

// A file system id used at the creation of Volume.
const char kFileSystemId[] = "fileSystemId";

// A fake implementation of JavaScriptMessageSender used for testing purposes.
class FakeJavaScriptMessageSender : public JavaScriptMessageSenderInterface {
 public:
  virtual void SendFileSystemError(const std::string& file_system_id,
                                   const std::string& request_id,
                                   const std::string& message) {}

  virtual void SendFileChunkRequest(const std::string& file_system_id,
                                    const std::string& request_id,
                                    int64_t offset,
                                    int64_t bytes_to_read) {}

  virtual void SendPassphraseRequest(const std::string& file_system_id,
                                     const std::string& request_id) {}

  virtual void SendReadMetadataDone(const std::string& file_system_id,
                                    const std::string& request_id,
                                    const pp::VarDictionary& metadata) {}

  virtual void SendOpenFileDone(const std::string& file_system_id,
                                const std::string& request_id) {}

  virtual void SendCloseFileDone(const std::string& file_system_id,
                                 const std::string& request_id,
                                 const std::string& open_request_id) {}

  virtual void SendReadFileDone(const std::string& file_system_id,
                                const std::string& request_id,
                                const pp::VarArrayBuffer& array_buffer,
                                bool has_more_data) {}
};

}  // namespace

// Class used by TEST_F macro to initialize the environment for testing
// Volume methods.
class VolumeTest : public testing::Test {
 protected:
  VolumeTest() : message_sender(NULL), volume(NULL) {}

  virtual void SetUp() {
    message_sender = new FakeJavaScriptMessageSender();
    // TODO(cmihail): Use the constructor with custom factories for
    // VolumeArchive and VolumeReader.
    volume = new Volume(pp::InstanceHandle(PSGetInstanceId()), kFileSystemId,
                        message_sender);
  }

  virtual void TearDown() {
    delete message_sender;
    message_sender = NULL;
    delete volume;
    volume = NULL;
  }

  FakeJavaScriptMessageSender* message_sender;
  Volume* volume;
};

TEST_F(VolumeTest, Init) {
  EXPECT_TRUE(volume->Init());
}

// TODO(cmihail): Write the actual tests (see crbug.com/417973).
