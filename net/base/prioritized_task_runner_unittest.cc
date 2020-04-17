// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/prioritized_task_runner.h"

#include <limits>
#include <vector>

#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/scoped_task_environment.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

class PrioritizedTaskRunnerTest : public testing::Test {
 public:
  PrioritizedTaskRunnerTest() {}

  void PushName(const std::string& task_name) {
    base::AutoLock auto_lock(callback_names_lock_);
    callback_names_.push_back(task_name);
  }

  std::string PushNameWithResult(const std::string& task_name) {
    PushName(task_name);
    std::string reply_name = task_name;
    base::ReplaceSubstringsAfterOffset(&reply_name, 0, "Task", "Reply");
    return reply_name;
  }

  std::vector<std::string> TaskOrder() {
    std::vector<std::string> out;
    for (const std::string& name : callback_names_) {
      if (base::StartsWith(name, "Task", base::CompareCase::SENSITIVE))
        out.push_back(name);
    }
    return out;
  }

  std::vector<std::string> ReplyOrder() {
    std::vector<std::string> out;
    for (const std::string& name : callback_names_) {
      if (base::StartsWith(name, "Reply", base::CompareCase::SENSITIVE))
        out.push_back(name);
    }
    return out;
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::vector<std::string> callback_names_;
  base::Lock callback_names_lock_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PrioritizedTaskRunnerTest);
};

TEST_F(PrioritizedTaskRunnerTest, PostTaskAndReplyThreadCheck) {
  auto task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  auto prioritized_task_runner =
      base::MakeRefCounted<PrioritizedTaskRunner>(task_runner);

  base::RunLoop run_loop;

  auto thread_check = [](scoped_refptr<base::TaskRunner> expected_task_runner,
                         base::OnceClosure callback) {
    EXPECT_TRUE(expected_task_runner->RunsTasksInCurrentSequence());
    std::move(callback).Run();
  };

  prioritized_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(thread_check, task_runner, base::DoNothing::Once()),
      base::BindOnce(thread_check,
                     scoped_task_environment_.GetMainThreadTaskRunner(),
                     run_loop.QuitClosure()),
      0);

  run_loop.Run();
}

TEST_F(PrioritizedTaskRunnerTest, PostTaskAndReplyRunsBothTasks) {
  auto task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  auto prioritized_task_runner =
      base::MakeRefCounted<PrioritizedTaskRunner>(task_runner);

  prioritized_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "Task"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "Reply"),
      0);

  // Run the TaskRunner and both the Task and Reply should run.
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ((std::vector<std::string>{"Task", "Reply"}), callback_names_);
}

TEST_F(PrioritizedTaskRunnerTest, PostTaskAndReplyTestPriority) {
  auto task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  auto prioritized_task_runner =
      base::MakeRefCounted<PrioritizedTaskRunner>(task_runner);

  prioritized_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "Task5"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "Reply5"),
      5);

  prioritized_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "Task0"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "Reply0"),
      0);

  prioritized_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "Task7"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "Reply7"),
      7);

  // Run the TaskRunner and all of the tasks and replies should have run, in
  // priority order.
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ((std::vector<std::string>{"Task0", "Task5", "Task7"}), TaskOrder());
  EXPECT_EQ((std::vector<std::string>{"Reply0", "Reply5", "Reply7"}),
            ReplyOrder());
}

TEST_F(PrioritizedTaskRunnerTest, PriorityOverflow) {
  auto task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  auto prioritized_task_runner =
      base::MakeRefCounted<PrioritizedTaskRunner>(task_runner);

  const uint32_t kMaxPriority = std::numeric_limits<uint32_t>::max();

  prioritized_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "TaskMinus1"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "ReplyMinus1"),
      kMaxPriority - 1);

  prioritized_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "TaskMax"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "ReplyMax"),
      kMaxPriority);

  prioritized_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "TaskMaxPlus1"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this), "ReplyMaxPlus1"),
      kMaxPriority + 1);

  // Run the TaskRunner and all of the tasks and replies should have run, in
  // priority order.
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ((std::vector<std::string>{"TaskMaxPlus1", "TaskMinus1", "TaskMax"}),
            TaskOrder());
  EXPECT_EQ(
      (std::vector<std::string>{"ReplyMaxPlus1", "ReplyMinus1", "ReplyMax"}),
      ReplyOrder());
}

TEST_F(PrioritizedTaskRunnerTest, PostTaskAndReplyWithResultRunsBothTasks) {
  auto task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  auto prioritized_task_runner =
      base::MakeRefCounted<PrioritizedTaskRunner>(task_runner);

  prioritized_task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushNameWithResult,
                     base::Unretained(this), "Task"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this)),
      0);

  // Run the TaskRunner and both the Task and Reply should run.
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ((std::vector<std::string>{"Task", "Reply"}), callback_names_);
}

TEST_F(PrioritizedTaskRunnerTest, PostTaskAndReplyWithResultTestPriority) {
  auto task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::TaskTraits());
  auto prioritized_task_runner =
      base::MakeRefCounted<PrioritizedTaskRunner>(task_runner);

  prioritized_task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushNameWithResult,
                     base::Unretained(this), "Task0"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this)),
      0);

  prioritized_task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushNameWithResult,
                     base::Unretained(this), "Task7"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this)),
      7);

  prioritized_task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&PrioritizedTaskRunnerTest::PushNameWithResult,
                     base::Unretained(this), "Task3"),
      base::BindOnce(&PrioritizedTaskRunnerTest::PushName,
                     base::Unretained(this)),
      3);

  // Run the TaskRunner and both the Task and Reply should run.
  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ((std::vector<std::string>{"Task0", "Task3", "Task7"}), TaskOrder());
  EXPECT_EQ((std::vector<std::string>{"Reply0", "Reply3", "Reply7"}),
            ReplyOrder());
}

}  // namespace
}  // namespace net
