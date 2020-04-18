// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/performance_monitor.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/location.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

#include <memory>

namespace blink {

class PerformanceMonitorTest : public testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;
  LocalFrame* GetFrame() const {
    return page_holder_->GetDocument().GetFrame();
  }
  ExecutionContext* GetExecutionContext() const {
    return &page_holder_->GetDocument();
  }
  LocalFrame* AnotherFrame() const {
    return another_page_holder_->GetDocument().GetFrame();
  }
  ExecutionContext* AnotherExecutionContext() const {
    return &another_page_holder_->GetDocument();
  }

  void WillExecuteScript(ExecutionContext* execution_context) {
    monitor_->WillExecuteScript(execution_context);
  }

  // scheduler::TaskTimeObserver implementation
  void WillProcessTask(double start_time) {
    monitor_->WillProcessTask(start_time);
  }

  void DidProcessTask(double start_time, double end_time) {
    monitor_->DidProcessTask(start_time, end_time);
  }
  void UpdateTaskAttribution(ExecutionContext* execution_context) {
    monitor_->UpdateTaskAttribution(execution_context);
  }
  void RecalculateStyle(Document* document) {
    probe::RecalculateStyle probe(document);
    monitor_->Will(probe);
    monitor_->Did(probe);
  }
  void UpdateLayout(Document* document) {
    probe::UpdateLayout probe(document);
    monitor_->Will(probe);
    monitor_->Did(probe);
  }
  bool TaskShouldBeReported() { return monitor_->task_should_be_reported_; }

  String FrameContextURL();
  int NumUniqueFrameContextsSeen();

  Persistent<PerformanceMonitor> monitor_;
  std::unique_ptr<DummyPageHolder> page_holder_;
  std::unique_ptr<DummyPageHolder> another_page_holder_;
};

void PerformanceMonitorTest::SetUp() {
  page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
  page_holder_->GetDocument().SetURL(KURL("https://example.com/foo"));
  monitor_ = new PerformanceMonitor(GetFrame());

  // Create another dummy page holder and pretend this is the iframe.
  another_page_holder_ = DummyPageHolder::Create(IntSize(400, 300));
  another_page_holder_->GetDocument().SetURL(KURL("https://iframed.com/bar"));
}

void PerformanceMonitorTest::TearDown() {
  monitor_->Shutdown();
}

String PerformanceMonitorTest::FrameContextURL() {
  // This is reported only if there is a single frameContext URL.
  if (monitor_->task_has_multiple_contexts_)
    return "";
  Frame* frame = ToDocument(monitor_->task_execution_context_)->GetFrame();
  return ToLocalFrame(frame)->GetDocument()->location()->toString();
}

int PerformanceMonitorTest::NumUniqueFrameContextsSeen() {
  if (!monitor_->task_execution_context_)
    return 0;
  if (!monitor_->task_has_multiple_contexts_)
    return 1;
  return 2;
}

TEST_F(PerformanceMonitorTest, SingleScriptInTask) {
  WillProcessTask(3719349.445172);
  EXPECT_EQ(0, NumUniqueFrameContextsSeen());
  WillExecuteScript(GetExecutionContext());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  DidProcessTask(3719349.445172, 3719349.5561923);  // Long task
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  EXPECT_EQ("https://example.com/foo", FrameContextURL());
}

TEST_F(PerformanceMonitorTest, MultipleScriptsInTask_SingleContext) {
  WillProcessTask(3719349.445172);
  EXPECT_EQ(0, NumUniqueFrameContextsSeen());
  WillExecuteScript(GetExecutionContext());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  EXPECT_EQ("https://example.com/foo", FrameContextURL());

  WillExecuteScript(GetExecutionContext());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  DidProcessTask(3719349.445172, 3719349.5561923);  // Long task
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  EXPECT_EQ("https://example.com/foo", FrameContextURL());
}

TEST_F(PerformanceMonitorTest, MultipleScriptsInTask_MultipleContexts) {
  WillProcessTask(3719349.445172);
  EXPECT_EQ(0, NumUniqueFrameContextsSeen());
  WillExecuteScript(GetExecutionContext());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  EXPECT_EQ("https://example.com/foo", FrameContextURL());

  WillExecuteScript(AnotherExecutionContext());
  EXPECT_EQ(2, NumUniqueFrameContextsSeen());
  DidProcessTask(3719349.445172, 3719349.5561923);  // Long task
  EXPECT_EQ(2, NumUniqueFrameContextsSeen());
  EXPECT_EQ("", FrameContextURL());
}

TEST_F(PerformanceMonitorTest, NoScriptInLongTask) {
  WillProcessTask(3719349.445172);
  WillExecuteScript(GetExecutionContext());
  DidProcessTask(3719349.445172, 3719349.445182);

  WillProcessTask(3719349.445172);
  DidProcessTask(3719349.445172, 3719349.5561923);  // Long task
  // Without presence of Script, FrameContext URL is not available
  EXPECT_EQ(0, NumUniqueFrameContextsSeen());
}

TEST_F(PerformanceMonitorTest, TaskWithoutLocalRoot) {
  WillProcessTask(1234.5678);
  UpdateTaskAttribution(AnotherExecutionContext());
  DidProcessTask(1234.5678, 2345.6789);
  EXPECT_FALSE(TaskShouldBeReported());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
}

TEST_F(PerformanceMonitorTest, TaskWithLocalRoot) {
  WillProcessTask(1234.5678);
  UpdateTaskAttribution(GetExecutionContext());
  EXPECT_TRUE(TaskShouldBeReported());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  UpdateTaskAttribution(AnotherExecutionContext());
  DidProcessTask(1234.5678, 2345.6789);
  EXPECT_TRUE(TaskShouldBeReported());
  EXPECT_EQ(2, NumUniqueFrameContextsSeen());
}

TEST_F(PerformanceMonitorTest, RecalculateStyleWithDocument) {
  WillProcessTask(1234.5678);
  RecalculateStyle(&another_page_holder_->GetDocument());
  DidProcessTask(1234.5678, 2345.6789);
  // Task from unrelated context should not be reported.
  EXPECT_FALSE(TaskShouldBeReported());

  WillProcessTask(3234.5678);
  RecalculateStyle(&page_holder_->GetDocument());
  DidProcessTask(3234.5678, 4345.6789);
  EXPECT_TRUE(TaskShouldBeReported());

  WillProcessTask(3234.5678);
  RecalculateStyle(&another_page_holder_->GetDocument());
  RecalculateStyle(&page_holder_->GetDocument());
  DidProcessTask(3234.5678, 4345.6789);
  // This task involves the current context, so it should be reported.
  EXPECT_TRUE(TaskShouldBeReported());
}

TEST_F(PerformanceMonitorTest, UpdateLayoutWithDocument) {
  WillProcessTask(1234.5678);
  UpdateLayout(&another_page_holder_->GetDocument());
  DidProcessTask(1234.5678, 2345.6789);
  // Task from unrelated context should not be reported.
  EXPECT_FALSE(TaskShouldBeReported());

  WillProcessTask(3234.5678);
  UpdateLayout(&page_holder_->GetDocument());
  DidProcessTask(3234.5678, 4345.6789);
  EXPECT_TRUE(TaskShouldBeReported());

  WillProcessTask(3234.5678);
  UpdateLayout(&another_page_holder_->GetDocument());
  UpdateLayout(&page_holder_->GetDocument());
  DidProcessTask(3234.5678, 4345.6789);
  // This task involves the current context, so it should be reported.
  EXPECT_TRUE(TaskShouldBeReported());
}

}  // namespace blink
