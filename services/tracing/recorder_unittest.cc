// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/recorder.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tracing {

class RecorderTest : public testing::Test {
 public:
  void SetUp() override { message_loop_.reset(new base::MessageLoop()); }

  void TearDown() override {
    recorder_.reset();
    message_loop_.reset();
  }

  void CreateRecorder(mojom::RecorderRequest request,
                      mojom::TraceDataType data_type,
                      const base::Closure& callback) {
    recorder_.reset(new Recorder(std::move(request), data_type, callback));
  }

  void CreateRecorder(mojom::TraceDataType data_type,
                      const base::Closure& callback) {
    CreateRecorder(nullptr, data_type, callback);
  }

  void AddChunk(const std::string& chunk) { recorder_->AddChunk(chunk); }

  void AddMetadata(base::Value metadata) {
    recorder_->AddMetadata(std::move(metadata));
  }

  std::unique_ptr<Recorder> recorder_;

 private:
  std::unique_ptr<base::MessageLoop> message_loop_;
};

TEST_F(RecorderTest, AddChunkArray) {
  size_t num_calls = 0;
  CreateRecorder(mojom::TraceDataType::ARRAY,
                 base::BindRepeating([](size_t* num_calls) { (*num_calls)++; },
                                     base::Unretained(&num_calls)));
  AddChunk("chunk1");
  AddChunk("chunk2");
  AddChunk("chunk3");
  EXPECT_EQ("chunk1,chunk2,chunk3", recorder_->data());

  // Verify that the recorder has called the callback every time it received a
  // chunk.
  EXPECT_EQ(3u, num_calls);
}

TEST_F(RecorderTest, AddChunkObject) {
  size_t num_calls = 0;
  CreateRecorder(mojom::TraceDataType::OBJECT,
                 base::BindRepeating([](size_t* num_calls) { (*num_calls)++; },
                                     base::Unretained(&num_calls)));
  AddChunk("chunk1");
  AddChunk("chunk2");
  AddChunk("chunk3");

  // Objects are similar to arrays. Their chunks are separated by commas.
  EXPECT_EQ("chunk1,chunk2,chunk3", recorder_->data());

  // Verify that the recorder has called the callback every time it received a
  // chunk.
  EXPECT_EQ(3u, num_calls);
}

TEST_F(RecorderTest, AddChunkString) {
  size_t num_calls = 0;
  CreateRecorder(mojom::TraceDataType::STRING,
                 base::BindRepeating([](size_t* num_calls) { (*num_calls)++; },
                                     base::Unretained(&num_calls)));
  AddChunk("chunk1");
  AddChunk("chunk2");
  AddChunk("chunk3");
  EXPECT_EQ("chunk1chunk2chunk3", recorder_->data());
  EXPECT_EQ(3u, num_calls);
}

TEST_F(RecorderTest, AddMetadata) {
  CreateRecorder(mojom::TraceDataType::ARRAY, base::BindRepeating([] {}));

  base::DictionaryValue dict1;
  dict1.SetKey("network-type", base::Value("Ethernet"));
  AddMetadata(std::move(dict1));

  base::DictionaryValue dict2;
  dict2.SetKey("os-name", base::Value("CrOS"));
  AddMetadata(std::move(dict2));

  EXPECT_EQ(2u, recorder_->metadata().size());
  std::string net;
  EXPECT_TRUE(recorder_->metadata().GetString("network-type", &net));
  EXPECT_EQ("Ethernet", net);
  std::string os;
  EXPECT_TRUE(recorder_->metadata().GetString("os-name", &os));
  EXPECT_EQ("CrOS", os);
}

TEST_F(RecorderTest, OnConnectionError) {
  base::RunLoop run_loop;
  size_t num_calls = 0;
  {
    mojom::RecorderPtr ptr;
    auto request = MakeRequest(&ptr);
    CreateRecorder(std::move(request), mojom::TraceDataType::STRING,
                   base::BindRepeating(
                       [](size_t* num_calls, base::Closure quit_closure) {
                         (*num_calls)++;
                         quit_closure.Run();
                       },
                       base::Unretained(&num_calls), run_loop.QuitClosure()));
  }
  // |ptr| is deleted at this point and so the recorder should notify us that
  // the client is not going to send any more data by running the callback.
  run_loop.Run();
  EXPECT_EQ(1u, num_calls);
}

}  // namespace tracing
