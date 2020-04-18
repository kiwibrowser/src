// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/task_graph_work_queue.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class FakeTaskImpl : public Task {
 public:
  FakeTaskImpl() = default;

  // Overridden from Task:
  void RunOnWorkerThread() override {}

 private:
  ~FakeTaskImpl() override = default;
  DISALLOW_COPY_AND_ASSIGN(FakeTaskImpl);
};

TEST(TaskGraphWorkQueueTest, TestChangingDependency) {
  TaskGraphWorkQueue work_queue;
  NamespaceToken token = work_queue.GenerateNamespaceToken();

  // Create a graph with one task
  TaskGraph graph1;
  scoped_refptr<FakeTaskImpl> task(new FakeTaskImpl());
  graph1.nodes.push_back(TaskGraph::Node(task.get(), 0u, 0u, 0u));

  // Schedule the graph
  work_queue.ScheduleTasks(token, &graph1);

  // Run the task.
  TaskGraphWorkQueue::PrioritizedTask prioritized_task =
      work_queue.GetNextTaskToRun(0u);
  work_queue.CompleteTask(std::move(prioritized_task));

  // Create a graph where task1 has a dependency
  TaskGraph graph2;
  scoped_refptr<FakeTaskImpl> dependency_task(new FakeTaskImpl());
  graph2.nodes.push_back(TaskGraph::Node(task.get(), 0u, 0u, 1u));
  graph2.nodes.push_back(TaskGraph::Node(dependency_task.get(), 0u, 0u, 0u));
  graph2.edges.push_back(TaskGraph::Edge(dependency_task.get(), task.get()));

  // Schedule the second graph.
  work_queue.ScheduleTasks(token, &graph2);

  // Run the |dependency_task|
  TaskGraphWorkQueue::PrioritizedTask prioritized_dependency_task =
      work_queue.GetNextTaskToRun(0u);
  EXPECT_EQ(prioritized_dependency_task.task.get(), dependency_task.get());
  work_queue.CompleteTask(std::move(prioritized_dependency_task));

  // We should have no tasks to run, as the dependent task already completed.
  EXPECT_FALSE(work_queue.HasReadyToRunTasks());
}

}  // namespace
}  // namespace cc
