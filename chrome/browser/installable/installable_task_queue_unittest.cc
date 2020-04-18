// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/installable/installable_manager.h"

#include "testing/gtest/include/gtest/gtest.h"

using IconPurpose = blink::Manifest::Icon::IconPurpose;

class InstallableTaskQueueUnitTest : public testing::Test {};

// Constructs an InstallableTask, with the supplied bools stored in it.
InstallableTask CreateTask(bool valid_manifest,
                           bool has_worker,
                           bool valid_primary_icon,
                           bool valid_badge_icon) {
  InstallableTask task;
  task.params.valid_manifest = valid_manifest;
  task.params.has_worker = has_worker;
  task.params.valid_primary_icon = valid_primary_icon;
  task.params.valid_badge_icon = valid_badge_icon;
  return task;
}

bool IsEqual(const InstallableTask& task1, const InstallableTask& task2) {
  return task1.params.valid_manifest == task2.params.valid_manifest &&
         task1.params.has_worker == task2.params.has_worker &&
         task1.params.valid_primary_icon == task2.params.valid_primary_icon &&
         task1.params.valid_badge_icon == task2.params.valid_badge_icon;
}

TEST_F(InstallableTaskQueueUnitTest, PausingMakesNextTaskAvailable) {
  InstallableTaskQueue task_queue;
  InstallableTask task1 = CreateTask(false, false, false, false);
  InstallableTask task2 = CreateTask(true, true, true, true);

  task_queue.Add(task1);
  task_queue.Add(task2);

  EXPECT_TRUE(IsEqual(task1, task_queue.Current()));
  // There is another task in the main queue, so it becomes current.
  task_queue.PauseCurrent();
  EXPECT_TRUE(IsEqual(task2, task_queue.Current()));
}

TEST_F(InstallableTaskQueueUnitTest, PausedTaskCanBeRetrieved) {
  InstallableTaskQueue task_queue;
  InstallableTask task1 = CreateTask(false, false, false, false);
  InstallableTask task2 = CreateTask(true, true, true, true);

  task_queue.Add(task1);
  task_queue.Add(task2);

  EXPECT_TRUE(IsEqual(task1, task_queue.Current()));
  task_queue.PauseCurrent();
  EXPECT_TRUE(IsEqual(task2, task_queue.Current()));
  task_queue.UnpauseAll();
  // We've unpaused "1", but "2" is still current.
  EXPECT_TRUE(IsEqual(task2, task_queue.Current()));
  task_queue.Next();
  EXPECT_TRUE(IsEqual(task1, task_queue.Current()));
}

TEST_F(InstallableTaskQueueUnitTest, NextDiscardsTask) {
  InstallableTaskQueue task_queue;
  InstallableTask task1 = CreateTask(false, false, false, false);
  InstallableTask task2 = CreateTask(true, true, true, true);

  task_queue.Add(task1);
  task_queue.Add(task2);

  EXPECT_TRUE(IsEqual(task1, task_queue.Current()));
  task_queue.Next();
  EXPECT_TRUE(IsEqual(task2, task_queue.Current()));
  // Next() does not pause "1"; it just drops it, so there is nothing to
  // unpause.
  task_queue.UnpauseAll();
  // "2" is still current.
  EXPECT_TRUE(IsEqual(task2, task_queue.Current()));
  // Unpausing does not retrieve "1"; it's gone forever.
  task_queue.Next();
  EXPECT_FALSE(task_queue.HasCurrent());
}
