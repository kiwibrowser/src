// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <list>
#include <memory>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

// Test helper. Reads a consumer handle, accumulating data into a string. Reads
// until encountering an error (e.g. peer closure), at which point it invokes an
// async callback.
class DataPipeReader {
 public:
  explicit DataPipeReader(ScopedDataPipeConsumerHandle consumer_handle,
                          base::OnceClosure on_read_done)
      : consumer_handle_(std::move(consumer_handle)),
        on_read_done_(std::move(on_read_done)),
        watcher_(FROM_HERE,
                 SimpleWatcher::ArmingPolicy::AUTOMATIC,
                 base::SequencedTaskRunnerHandle::Get()) {
    watcher_.Watch(
        consumer_handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
        MOJO_WATCH_CONDITION_SATISFIED,
        base::Bind(&DataPipeReader::OnDataAvailable, base::Unretained(this)));
  }
  ~DataPipeReader() = default;

  const std::string& data() const { return data_; }

 private:
  void OnDataAvailable(MojoResult result, const HandleSignalsState& state) {
    if (result == MOJO_RESULT_OK) {
      uint32_t size = 64;
      std::vector<char> buffer(size, 0);
      MojoResult read_result;
      do {
        read_result = consumer_handle_->ReadData(buffer.data(), &size,
                                                 MOJO_READ_DATA_FLAG_NONE);
        if (read_result == MOJO_RESULT_OK) {
          std::copy(buffer.begin(), buffer.begin() + size,
                    std::back_inserter(data_));
        }
      } while (read_result == MOJO_RESULT_OK);

      if (read_result == MOJO_RESULT_SHOULD_WAIT)
        return;
    }

    if (result != MOJO_RESULT_CANCELLED)
      watcher_.Cancel();

    std::move(on_read_done_).Run();
  }

  ScopedDataPipeConsumerHandle consumer_handle_;
  base::OnceClosure on_read_done_;
  SimpleWatcher watcher_;
  std::string data_;

  DISALLOW_COPY_AND_ASSIGN(DataPipeReader);
};

class StringDataPipeProducerTest : public testing::Test {
 public:
  StringDataPipeProducerTest() = default;
  ~StringDataPipeProducerTest() override = default;

 protected:
  static void WriteStringThenCloseProducer(
      std::unique_ptr<StringDataPipeProducer> producer,
      const base::StringPiece& str,
      StringDataPipeProducer::AsyncWritingMode mode) {
    StringDataPipeProducer* raw_producer = producer.get();
    raw_producer->Write(
        str, mode,
        base::BindOnce([](std::unique_ptr<StringDataPipeProducer> producer,
                          MojoResult result) {},
                       std::move(producer)));
  }

  static void WriteStringsThenCloseProducer(
      std::unique_ptr<StringDataPipeProducer> producer,
      std::list<base::StringPiece> strings,
      StringDataPipeProducer::AsyncWritingMode mode) {
    StringDataPipeProducer* raw_producer = producer.get();
    base::StringPiece str = strings.front();
    strings.pop_front();
    raw_producer->Write(str, mode,
                        base::BindOnce(
                            [](std::unique_ptr<StringDataPipeProducer> producer,
                               std::list<base::StringPiece> strings,
                               StringDataPipeProducer::AsyncWritingMode mode,
                               MojoResult result) {
                              if (!strings.empty())
                                WriteStringsThenCloseProducer(
                                    std::move(producer), std::move(strings),
                                    mode);
                            },
                            std::move(producer), std::move(strings), mode));
  }

 private:
  base::test::ScopedTaskEnvironment task_environment_;

  DISALLOW_COPY_AND_ASSIGN(StringDataPipeProducerTest);
};

TEST_F(StringDataPipeProducerTest, EqualCapacity) {
  const std::string kTestString = "Hello, world!";

  base::RunLoop loop;
  mojo::DataPipe pipe(static_cast<uint32_t>(kTestString.size()));
  DataPipeReader reader(std::move(pipe.consumer_handle), loop.QuitClosure());
  WriteStringThenCloseProducer(
      std::make_unique<StringDataPipeProducer>(std::move(pipe.producer_handle)),
      kTestString,
      StringDataPipeProducer::AsyncWritingMode::
          STRING_MAY_BE_INVALIDATED_BEFORE_COMPLETION);
  loop.Run();

  EXPECT_EQ(kTestString, reader.data());
}

TEST_F(StringDataPipeProducerTest, UnderCapacity) {
  const std::string kTestString = "Hello, world!";

  base::RunLoop loop;
  mojo::DataPipe pipe(static_cast<uint32_t>(kTestString.size() * 2));
  DataPipeReader reader(std::move(pipe.consumer_handle), loop.QuitClosure());
  WriteStringThenCloseProducer(
      std::make_unique<StringDataPipeProducer>(std::move(pipe.producer_handle)),
      kTestString,
      StringDataPipeProducer::AsyncWritingMode::
          STRING_MAY_BE_INVALIDATED_BEFORE_COMPLETION);
  loop.Run();

  EXPECT_EQ(kTestString, reader.data());
}

TEST_F(StringDataPipeProducerTest, OverCapacity) {
  const std::string kTestString = "Hello, world!";

  base::RunLoop loop;
  mojo::DataPipe pipe(static_cast<uint32_t>(kTestString.size() / 2));
  DataPipeReader reader(std::move(pipe.consumer_handle), loop.QuitClosure());
  WriteStringThenCloseProducer(
      std::make_unique<StringDataPipeProducer>(std::move(pipe.producer_handle)),
      kTestString,
      StringDataPipeProducer::AsyncWritingMode::
          STRING_STAYS_VALID_UNTIL_COMPLETION);
  loop.Run();

  EXPECT_EQ(kTestString, reader.data());
}

TEST_F(StringDataPipeProducerTest, TinyPipe) {
  const std::string kTestString = "Hello, world!";

  base::RunLoop loop;
  mojo::DataPipe pipe(1);
  DataPipeReader reader(std::move(pipe.consumer_handle), loop.QuitClosure());
  WriteStringThenCloseProducer(
      std::make_unique<StringDataPipeProducer>(std::move(pipe.producer_handle)),
      kTestString,
      StringDataPipeProducer::AsyncWritingMode::
          STRING_MAY_BE_INVALIDATED_BEFORE_COMPLETION);
  loop.Run();

  EXPECT_EQ(kTestString, reader.data());
}

TEST_F(StringDataPipeProducerTest, MultipleWrites) {
  const std::string kTestString1 = "Hello, world!";
  const std::string kTestString2 = "There is a lot of data coming your way!";
  const std::string kTestString3 = "So many strings!";
  const std::string kTestString4 = "Your cup runneth over!";

  base::RunLoop loop;
  mojo::DataPipe pipe(4);
  DataPipeReader reader(std::move(pipe.consumer_handle), loop.QuitClosure());
  WriteStringsThenCloseProducer(
      std::make_unique<StringDataPipeProducer>(std::move(pipe.producer_handle)),
      {kTestString1, kTestString2, kTestString3, kTestString4},
      StringDataPipeProducer::AsyncWritingMode::
          STRING_MAY_BE_INVALIDATED_BEFORE_COMPLETION);
  loop.Run();

  EXPECT_EQ(kTestString1 + kTestString2 + kTestString3 + kTestString4,
            reader.data());
}

}  // namespace
}  // namespace mojo
