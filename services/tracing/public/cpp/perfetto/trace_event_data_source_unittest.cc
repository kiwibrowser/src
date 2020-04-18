// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/public/cpp/perfetto/trace_event_data_source.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/trace_event/trace_event.h"
#include "services/tracing/public/mojom/perfetto_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/perfetto/include/perfetto/protozero/scattered_stream_null_delegate.h"
#include "third_party/perfetto/include/perfetto/tracing/core/trace_writer.h"
#include "third_party/perfetto/protos/perfetto/trace/trace_packet.pb.h"
#include "third_party/perfetto/protos/perfetto/trace/trace_packet.pbzero.h"

namespace tracing {

namespace {

const char kCategoryGroup[] = "foo";

class MockProducerClient : public ProducerClient {
 public:
  explicit MockProducerClient(
      scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner)
      : delegate_(perfetto::base::kPageSize),
        stream_(&delegate_),
        main_thread_task_runner_(std::move(main_thread_task_runner)) {
    trace_packet_.Reset(&stream_);
  }

  std::unique_ptr<perfetto::TraceWriter> CreateTraceWriter(
      perfetto::BufferID target_buffer) override;

  void FlushPacketIfPossible() {
    // GetNewBuffer() in ScatteredStreamWriterNullDelegate doesn't
    // actually return a new buffer, but rather lets us access the buffer
    // buffer already used by protozero to write the TracePacket into.
    protozero::ContiguousMemoryRange buffer = delegate_.GetNewBuffer();

    uint32_t message_size = trace_packet_.Finalize();
    if (message_size) {
      EXPECT_GE(buffer.size(), message_size);

      auto proto = std::make_unique<perfetto::protos::TracePacket>();
      EXPECT_TRUE(proto->ParseFromArray(buffer.begin, message_size));
      if (proto->has_chrome_events() &&
          proto->chrome_events().trace_events().size() > 0 &&
          proto->chrome_events().trace_events()[0].category_group_name() ==
              kCategoryGroup) {
        finalized_packets_.push_back(std::move(proto));
      }
    }

    stream_.Reset(buffer);
    trace_packet_.Reset(&stream_);
  }

  perfetto::protos::pbzero::TracePacket* NewTracePacket() {
    FlushPacketIfPossible();

    return &trace_packet_;
  }

  size_t GetFinalizedPacketCount() {
    FlushPacketIfPossible();
    return finalized_packets_.size();
  }

  const google::protobuf::RepeatedPtrField<perfetto::protos::ChromeTraceEvent>
  GetChromeTraceEvents(size_t packet_index = 0) {
    FlushPacketIfPossible();
    EXPECT_GT(finalized_packets_.size(), packet_index);

    auto event_bundle = finalized_packets_[packet_index]->chrome_events();
    return event_bundle.trace_events();
  }

 private:
  std::vector<std::unique_ptr<perfetto::protos::TracePacket>>
      finalized_packets_;
  perfetto::protos::pbzero::TracePacket trace_packet_;
  protozero::ScatteredStreamWriterNullDelegate delegate_;
  protozero::ScatteredStreamWriter stream_;
  scoped_refptr<base::SequencedTaskRunner> main_thread_task_runner_;
};

// For sequences/threads other than our own, we just want to ignore
// any events coming in.
class DummyTraceWriter : public perfetto::TraceWriter {
 public:
  DummyTraceWriter()
      : delegate_(perfetto::base::kPageSize), stream_(&delegate_) {}

  perfetto::TraceWriter::TracePacketHandle NewTracePacket() override {
    stream_.Reset(delegate_.GetNewBuffer());
    trace_packet_.Reset(&stream_);

    return perfetto::TraceWriter::TracePacketHandle(&trace_packet_);
  }

  void Flush(std::function<void()> callback = {}) override {}

  perfetto::WriterID writer_id() const override {
    return perfetto::WriterID(0);
  }

 private:
  perfetto::protos::pbzero::TracePacket trace_packet_;
  protozero::ScatteredStreamWriterNullDelegate delegate_;
  protozero::ScatteredStreamWriter stream_;
};

class MockTraceWriter : public perfetto::TraceWriter {
 public:
  explicit MockTraceWriter(MockProducerClient* producer_client)
      : producer_client_(producer_client) {}

  perfetto::TraceWriter::TracePacketHandle NewTracePacket() override {
    return perfetto::TraceWriter::TracePacketHandle(
        producer_client_->NewTracePacket());
  }

  void Flush(std::function<void()> callback = {}) override {}

  perfetto::WriterID writer_id() const override {
    return perfetto::WriterID(0);
  }

 private:
  MockProducerClient* producer_client_;
};

std::unique_ptr<perfetto::TraceWriter> MockProducerClient::CreateTraceWriter(
    perfetto::BufferID target_buffer) {
  if (main_thread_task_runner_->RunsTasksInCurrentSequence()) {
    return std::make_unique<MockTraceWriter>(this);
  } else {
    return std::make_unique<DummyTraceWriter>();
  }
}

class TraceEventDataSourceTest : public testing::Test {
 public:
  void SetUp() override {
    ProducerClient::ResetTaskRunnerForTesting();
    producer_client_ = std::make_unique<MockProducerClient>(
        scoped_task_environment_.GetMainThreadTaskRunner());
  }

  void TearDown() override {
    base::RunLoop wait_for_tracelog_flush;

    TraceEventDataSource::GetInstance()->StopTracing(base::BindRepeating(
        [](const base::RepeatingClosure& quit_closure) { quit_closure.Run(); },
        wait_for_tracelog_flush.QuitClosure()));

    wait_for_tracelog_flush.Run();

    // As MockTraceWriter keeps a pointer to our MockProducerClient,
    // we need to make sure to clean it up from TLS. The other sequences
    // get DummyTraceWriters that we don't care about.
    TraceEventDataSource::GetInstance()->ResetCurrentThreadForTesting();
    producer_client_.reset();
  }

  void CreateTraceEventDataSource() {
    auto data_source_config = mojom::DataSourceConfig::New();
    TraceEventDataSource::GetInstance()->StartTracing(producer_client(),
                                                      *data_source_config);
  }

  MockProducerClient* producer_client() { return producer_client_.get(); }

 private:
  std::unique_ptr<MockProducerClient> producer_client_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(TraceEventDataSourceTest, BasicTraceEvent) {
  CreateTraceEventDataSource();

  TRACE_EVENT_BEGIN0(kCategoryGroup, "bar");

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_event = trace_events[0];
  EXPECT_EQ("bar", trace_event.name());
  EXPECT_EQ(kCategoryGroup, trace_event.category_group_name());
  EXPECT_EQ(TRACE_EVENT_PHASE_BEGIN, trace_event.phase());
}

TEST_F(TraceEventDataSourceTest, TimestampedTraceEvent) {
  CreateTraceEventDataSource();

  TRACE_EVENT_BEGIN_WITH_ID_TID_AND_TIMESTAMP0(
      kCategoryGroup, "bar", 42, 4242,
      base::TimeTicks() + base::TimeDelta::FromMicroseconds(424242));

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_event = trace_events[0];
  EXPECT_EQ("bar", trace_event.name());
  EXPECT_EQ(kCategoryGroup, trace_event.category_group_name());
  EXPECT_EQ(42u, trace_event.id());
  EXPECT_EQ(4242, trace_event.thread_id());
  EXPECT_EQ(424242, trace_event.timestamp());
  EXPECT_EQ(TRACE_EVENT_PHASE_ASYNC_BEGIN, trace_event.phase());
}

TEST_F(TraceEventDataSourceTest, InstantTraceEvent) {
  CreateTraceEventDataSource();

  TRACE_EVENT_INSTANT0(kCategoryGroup, "bar", TRACE_EVENT_SCOPE_THREAD);

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_event = trace_events[0];
  EXPECT_EQ("bar", trace_event.name());
  EXPECT_EQ(kCategoryGroup, trace_event.category_group_name());
  EXPECT_EQ(TRACE_EVENT_SCOPE_THREAD, trace_event.flags());
  EXPECT_EQ(TRACE_EVENT_PHASE_INSTANT, trace_event.phase());
}

TEST_F(TraceEventDataSourceTest, EventWithStringArgs) {
  CreateTraceEventDataSource();

  TRACE_EVENT_INSTANT2(kCategoryGroup, "bar", TRACE_EVENT_SCOPE_THREAD,
                       "arg1_name", "arg1_val", "arg2_name", "arg2_val");

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_args = trace_events[0].args();
  EXPECT_EQ(trace_args.size(), 2);

  EXPECT_EQ("arg1_name", trace_args[0].name());
  EXPECT_EQ("arg1_val", trace_args[0].string_value());
  EXPECT_EQ("arg2_name", trace_args[1].name());
  EXPECT_EQ("arg2_val", trace_args[1].string_value());
}

TEST_F(TraceEventDataSourceTest, EventWithUIntArgs) {
  CreateTraceEventDataSource();

  TRACE_EVENT_INSTANT2(kCategoryGroup, "bar", TRACE_EVENT_SCOPE_THREAD, "foo",
                       42u, "bar", 4242u);

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_args = trace_events[0].args();
  EXPECT_EQ(trace_args.size(), 2);

  EXPECT_EQ(42u, trace_args[0].uint_value());
  EXPECT_EQ(4242u, trace_args[1].uint_value());
}

TEST_F(TraceEventDataSourceTest, EventWithIntArgs) {
  CreateTraceEventDataSource();

  TRACE_EVENT_INSTANT2(kCategoryGroup, "bar", TRACE_EVENT_SCOPE_THREAD, "foo",
                       42, "bar", 4242);

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_args = trace_events[0].args();
  EXPECT_EQ(trace_args.size(), 2);

  EXPECT_EQ(42, trace_args[0].int_value());
  EXPECT_EQ(4242, trace_args[1].int_value());
}

TEST_F(TraceEventDataSourceTest, EventWithBoolArgs) {
  CreateTraceEventDataSource();

  TRACE_EVENT_INSTANT2(kCategoryGroup, "bar", TRACE_EVENT_SCOPE_THREAD, "foo",
                       true, "bar", false);

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_args = trace_events[0].args();
  EXPECT_EQ(trace_args.size(), 2);

  EXPECT_TRUE(trace_args[0].has_bool_value());
  EXPECT_EQ(true, trace_args[0].bool_value());
  EXPECT_TRUE(trace_args[1].has_bool_value());
  EXPECT_EQ(false, trace_args[1].bool_value());
}

TEST_F(TraceEventDataSourceTest, EventWithDoubleArgs) {
  CreateTraceEventDataSource();

  TRACE_EVENT_INSTANT2(kCategoryGroup, "bar", TRACE_EVENT_SCOPE_THREAD, "foo",
                       42.42, "bar", 4242.42);

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_args = trace_events[0].args();
  EXPECT_EQ(trace_args.size(), 2);

  EXPECT_EQ(42.42, trace_args[0].double_value());
  EXPECT_EQ(4242.42, trace_args[1].double_value());
}

TEST_F(TraceEventDataSourceTest, EventWithPointerArgs) {
  CreateTraceEventDataSource();

  TRACE_EVENT_INSTANT2(kCategoryGroup, "bar", TRACE_EVENT_SCOPE_THREAD, "foo",
                       reinterpret_cast<void*>(0xBEEF), "bar",
                       reinterpret_cast<void*>(0xF00D));

  auto trace_events = producer_client()->GetChromeTraceEvents();
  EXPECT_EQ(trace_events.size(), 1);

  auto trace_args = trace_events[0].args();
  EXPECT_EQ(trace_args.size(), 2);

  EXPECT_EQ(static_cast<uintptr_t>(0xBEEF), trace_args[0].pointer_value());
  EXPECT_EQ(static_cast<uintptr_t>(0xF00D), trace_args[1].pointer_value());
}

TEST_F(TraceEventDataSourceTest, CompleteTraceEventsIntoSeparateBeginAndEnd) {
  static const char kEventName[] = "bar";

  CreateTraceEventDataSource();

  auto* category_group_enabled =
      TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(kCategoryGroup);

  trace_event_internal::TraceID trace_event_trace_id =
      trace_event_internal::kNoId;

  auto handle = trace_event_internal::AddTraceEventWithThreadIdAndTimestamp(
      TRACE_EVENT_PHASE_COMPLETE, category_group_enabled, kEventName,
      trace_event_trace_id.scope(), trace_event_trace_id.raw_id(),
      1 /* thread_id */,
      base::TimeTicks() + base::TimeDelta::FromMicroseconds(10),
      trace_event_trace_id.id_flags() | TRACE_EVENT_FLAG_EXPLICIT_TIMESTAMP,
      trace_event_internal::kNoId);

  base::trace_event::TraceLog::GetInstance()->UpdateTraceEventDurationExplicit(
      category_group_enabled, kEventName, handle,
      base::TimeTicks() + base::TimeDelta::FromMicroseconds(20),
      base::ThreadTicks() + base::TimeDelta::FromMicroseconds(50));

  // TRACE_EVENT_PHASE_COMPLETE events should internally emit a
  // TRACE_EVENT_PHASE_BEGIN event first, and then a TRACE_EVENT_PHASE_END event
  // when the duration is attempted set on the first event.
  EXPECT_EQ(2u, producer_client()->GetFinalizedPacketCount());

  auto events_from_first_packet = producer_client()->GetChromeTraceEvents(0);
  EXPECT_EQ(events_from_first_packet.size(), 1);

  auto begin_trace_event = events_from_first_packet[0];
  EXPECT_EQ(TRACE_EVENT_PHASE_BEGIN, begin_trace_event.phase());
  EXPECT_EQ(10, begin_trace_event.timestamp());

  auto events_from_second_packet = producer_client()->GetChromeTraceEvents(1);
  EXPECT_EQ(events_from_second_packet.size(), 1);

  auto end_trace_event = events_from_second_packet[0];
  EXPECT_EQ(TRACE_EVENT_PHASE_END, end_trace_event.phase());
  EXPECT_EQ(20, end_trace_event.timestamp());
  EXPECT_EQ(50, end_trace_event.thread_timestamp());
}

}  // namespace

}  // namespace tracing
