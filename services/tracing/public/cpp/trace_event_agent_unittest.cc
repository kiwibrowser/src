// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/public/cpp/trace_event_agent.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/trace_event_analyzer.h"
#include "base/time/time.h"
#include "base/trace_event/trace_config.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_log.h"
#include "base/values.h"
#include "services/tracing/public/mojom/tracing.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tracing {

namespace {
const char kTestCategory[] = "TraceEventAgentTestCategory";
const char kTestMetadataKey[] = "TraceEventAgentTestMetadata";
}  // namespace

class MockRecorder : public mojom::Recorder {
 public:
  explicit MockRecorder(mojom::RecorderRequest request)
      : binding_(this, std::move(request)) {
    binding_.set_connection_error_handler(base::BindRepeating(
        &MockRecorder::OnConnectionError, base::Unretained(this)));
  }

  std::string events() const { return events_; }
  std::string metadata() const { return metadata_; }
  void set_quit_closure(base::Closure quit_closure) {
    quit_closure_ = quit_closure;
  }

 private:
  void Append(std::string* dest, const std::string& chunk) {
    if (chunk.empty())
      return;
    if (!dest->empty())
      dest->push_back(',');
    dest->append(chunk);
  }

  void AddChunk(const std::string& chunk) override {
    std::unique_ptr<trace_analyzer::TraceAnalyzer> analyzer(
        trace_analyzer::TraceAnalyzer::Create("[" + chunk + "]"));
    trace_analyzer::TraceEventVector events;
    analyzer->FindEvents(trace_analyzer::Query::EventCategoryIs(kTestCategory),
                         &events);
    for (const auto* event : events) {
      Append(&events_, event->name);
    }
  }

  void AddMetadata(base::Value metadata) override {
    base::DictionaryValue* dict = nullptr;
    EXPECT_TRUE(metadata.GetAsDictionary(&dict));
    std::string value;
    if (dict->GetString(kTestMetadataKey, &value))
      Append(&metadata_, value);
  }

  void OnConnectionError() {
    if (quit_closure_)
      quit_closure_.Run();
  }

  mojo::Binding<mojom::Recorder> binding_;
  std::string events_;
  std::string metadata_;
  base::Closure quit_closure_;
};

class TraceEventAgentTest : public testing::Test {
 public:
  void SetUp() override {
    message_loop_.reset(new base::MessageLoop());
    agent_.reset(new TraceEventAgentImpl(nullptr, false));
  }

  void TearDown() override {
    base::trace_event::TraceLog::GetInstance()->SetDisabled();
    recorder_.reset();
    agent_.reset();
    message_loop_.reset();
  }

  void StartTracing(const std::string& categories) {
    agent_->StartTracing(
        base::trace_event::TraceConfig(categories, "").ToString(),
        base::TimeTicks::Now(),
        base::BindRepeating([](bool success) { EXPECT_TRUE(success); }));
  }

  void StopAndFlush(base::Closure quit_closure) {
    mojom::RecorderPtr recorder_ptr;
    recorder_.reset(new MockRecorder(MakeRequest(&recorder_ptr)));
    recorder_->set_quit_closure(quit_closure);
    agent_->StopAndFlush(std::move(recorder_ptr));
  }

  void AddMetadataGeneratorFunction(
      TraceEventAgent::MetadataGeneratorFunction generator) {
    agent_->AddMetadataGeneratorFunction(generator);
  }

  void GetCategories(const std::string& expected_category,
                     base::Closure quit_closure) {
    agent_->GetCategories(base::BindRepeating(
        &TraceEventAgentTest::OnGetCategoriesReply, base::Unretained(this),
        expected_category, quit_closure));
  }

  void OnGetCategoriesReply(const std::string& expected_category,
                            base::Closure quit_closure,
                            const std::string& categories) {
    EXPECT_FALSE(categories.rfind(expected_category) == std::string::npos);
    quit_closure.Run();
  }

  MockRecorder* recorder() const { return recorder_.get(); }

 private:
  std::unique_ptr<base::MessageLoop> message_loop_;
  std::unique_ptr<TraceEventAgentImpl> agent_;
  std::unique_ptr<MockRecorder> recorder_;
};

TEST_F(TraceEventAgentTest, StartTracing) {
  EXPECT_FALSE(base::trace_event::TraceLog::GetInstance()->IsEnabled());
  base::RunLoop run_loop;
  StartTracing("*");
  EXPECT_TRUE(base::trace_event::TraceLog::GetInstance()->IsEnabled());
  StopAndFlush(run_loop.QuitClosure());
  run_loop.Run();
}

TEST_F(TraceEventAgentTest, StopAndFlushEvents) {
  EXPECT_FALSE(base::trace_event::TraceLog::GetInstance()->IsEnabled());
  base::RunLoop run_loop;
  StartTracing(kTestCategory);
  TRACE_EVENT_INSTANT0(kTestCategory, "event1", TRACE_EVENT_SCOPE_THREAD);
  TRACE_EVENT_INSTANT0(kTestCategory, "event2", TRACE_EVENT_SCOPE_THREAD);
  StopAndFlush(run_loop.QuitClosure());
  run_loop.Run();

  auto* mock_recorder = recorder();
  EXPECT_EQ("event1,event2", mock_recorder->events());
  EXPECT_EQ("", mock_recorder->metadata());
  EXPECT_FALSE(base::trace_event::TraceLog::GetInstance()->IsEnabled());
}

TEST_F(TraceEventAgentTest, GetCategories) {
  base::RunLoop run_loop;
  TRACE_EVENT_INSTANT0(kTestCategory, "event1", TRACE_EVENT_SCOPE_THREAD);
  GetCategories(kTestCategory, run_loop.QuitClosure());
  run_loop.Run();
}

TEST_F(TraceEventAgentTest, StopAndFlushMetadata) {
  EXPECT_FALSE(base::trace_event::TraceLog::GetInstance()->IsEnabled());
  base::RunLoop run_loop;
  AddMetadataGeneratorFunction(base::BindRepeating([] {
    std::unique_ptr<base::DictionaryValue> metadata_dict(
        new base::DictionaryValue());
    metadata_dict->SetString(kTestMetadataKey, "test metadata");
    return metadata_dict;
  }));
  StartTracing(kTestCategory);
  StopAndFlush(run_loop.QuitClosure());
  run_loop.Run();

  auto* mock_recorder = recorder();
  EXPECT_EQ("", mock_recorder->events());
  EXPECT_EQ("test metadata", mock_recorder->metadata());
  EXPECT_FALSE(base::trace_event::TraceLog::GetInstance()->IsEnabled());
}
}  // namespace tracing
