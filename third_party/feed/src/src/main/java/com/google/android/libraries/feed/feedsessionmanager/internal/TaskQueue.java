// Copyright 2018 The Feed Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.android.libraries.feed.feedsessionmanager.internal;

import android.os.Handler;
import android.os.Looper;
import android.support.annotation.IntDef;
import android.util.Pair;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import com.google.android.libraries.feed.common.time.TimingUtils;
import com.google.android.libraries.feed.common.time.TimingUtils.ElapsedTimeTracker;
import java.util.ArrayDeque;
import java.util.Queue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.annotation.concurrent.GuardedBy;

/**
 * This class is responsible for running tasks on the Feed single-threaded Executor. The primary job
 * of this class is to run high priority tasks and to delay certain task until other complete. When
 * we are delaying tasks, they will be added to a set of queues and will run within order within the
 * task priority. There are three priorities of tasks defined:
 *
 * <ol>
 *   <li>Initialization, HEAD_INVALIDATE, HEAD_RESET - These tasks will be placed on the Executor
 *       when they are received.
 *   <li>USER_FACING - These tasks are high priority, running after the immediate tasks.
 *   <li>BACKGROUND - These are low priority tasks which run after all other tasks finish.
 * </ol>
 *
 * <p>The {@code TaskQueue} start in initialization mode. All tasks will be delayed until we
 * initialization is completed. The {@link #executeInitialization} method is run to initialize the
 * FeedSessionManager. We also enter delayed mode when we either reset the $HEAD or invalidate the
 * $HEAD. For HEAD_RESET, we are making a request which will complete. Once it's complete, we will
 * process any delayed tasks. HEAD_INVALIDATE simply clears the contents of $HEAD. The expectation
 * is a future HEAD_RESET will populate $HEAD. Once the delay is cleared, we will run the
 * USER_FACING tasks followed by the BACKGROUND tasks. Once all of these tasks have run, we will run
 * tasks immediately until we either have a task which is of type HEAD_INVALIDATE or HEAD_RESET.
 */
public class TaskQueue implements Dumpable {
  private static final String TAG = "TaskQueue";

  /** TaskType identifies the type of task being run and implicitly the priority of the task */
  @IntDef({
    TaskType.BACKGROUND,
    TaskType.HEAD_RESET,
    TaskType.HEAD_INVALIDATE,
    TaskType.USER_FACING
  })
  public @interface TaskType {
    // Background tasks run at the lowest priority
    int BACKGROUND = 0;
    // Runs immediately, $HEAD is indicates the task will create a new $HEAD instance.
    // Once finished, start running other tasks until the delayed tasks are all run.
    int HEAD_RESET = 1;
    // Runs immediately, $HEAD is invalidated (cleared) and delay tasks until HEAD_RESET
    int HEAD_INVALIDATE = 2;
    // User facing task which should run at a higher priority than Background.
    int USER_FACING = 3;
  }

  private final Object lock = new Object();

  @GuardedBy("lock")
  private final Queue<Pair<String, Runnable>> userTasks = new ArrayDeque<>();

  @GuardedBy("lock")
  private final Queue<Pair<String, Runnable>> backgroundTasks = new ArrayDeque<>();

  @GuardedBy("lock")
  private boolean waitForHeadReset = false;

  @GuardedBy("lock")
  private boolean initialized = false;

  private final ExecutorService executor;
  private final TimingUtils timingUtils;
  private final Handler mainThreadHandler;

  // counters used for dump
  private int taskCount = 0;
  private int delayedTaskCount = 0;
  private int backgroundTaskCount = 0;
  private int maxBackgroundTasks = 0;
  private int userFacingTaskCount = 0;
  private int maxUserFacingTasks = 0;
  private int immediateTasks = 0;

  public TaskQueue(ExecutorService executor, TimingUtils timingUtils) {
    this.executor = executor;
    this.timingUtils = timingUtils;
    mainThreadHandler = new Handler(Looper.getMainLooper());
  }

  /** Execute a Task on the Executor. */
  public void execute(String task, @TaskType int taskType, Runnable runnable) {
    execute(task, taskType, runnable, null, 0);
  }

  public void execute(
      String task,
      @TaskType int taskType,
      Runnable runnable,
      /*@Nullable*/ Runnable timeOut,
      long timeoutMillis) {
    taskCount++;
    if (taskType == TaskType.USER_FACING) {
      userFacingTaskCount++;
    } else if (taskType == TaskType.BACKGROUND) {
      backgroundTaskCount++;
    }
    if (!isDelayed()) {
      immediateTasks++;
      if (taskType == TaskType.HEAD_RESET || taskType == TaskType.HEAD_INVALIDATE) {
        if (taskType == TaskType.HEAD_RESET) {
          executor.execute(() -> executeHeadReset(task, runnable));
        } else {
          executor.execute(() -> executeHeadInvalidate(task, runnable));
        }
      } else {
        executor.execute(() -> executeImmediate(task, runnable));
      }
      return;
    }
    if (timeOut != null) {
      TaskWithTimeout taskWithTimeout = new TaskWithTimeout(runnable, timeOut);
      mainThreadHandler.postDelayed(taskWithTimeout::timeoutCallback, timeoutMillis);
      scheduleTask(task, taskType, taskWithTimeout::taskCallback);
    } else {
      scheduleTask(task, taskType, runnable);
    }
  }

  class TaskWithTimeout {
    private final Runnable taskRunnable;
    private final Runnable timeOutRunnable;
    private final AtomicBoolean started = new AtomicBoolean(false);

    TaskWithTimeout(Runnable taskRunnable, Runnable timeOutRunnable) {
      this.taskRunnable = taskRunnable;
      this.timeOutRunnable = timeOutRunnable;
    }

    void taskCallback() {
      started.set(true);
      taskRunnable.run();
    }

    void timeoutCallback() {
      if (started.get()) {
        return;
      }
      executor.execute(timeOutRunnable);
    }
  }

  /**
   * Called to initialize the {@link
   * com.google.android.libraries.feed.feedsessionmanager.FeedSessionManager}. This needs to be the
   * first task run, all other tasks are delayed until initialization finishes.
   */
  public void executeInitialization(Runnable runnable) {
    Logger.i(TAG, "executeInitialization: task initialization");
    executor.execute(
        () -> {
          runnable.run();
          synchronized (lock) {
            // We are not initialized
            initialized = true;
            if (!waitForHeadReset) {
              executeNextTask();
            }
          }
        });
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    dumper.forKey("tasks").value(taskCount);
    dumper.forKey("backgroundTasks").value(backgroundTaskCount).compactPrevious();
    dumper.forKey("userFacingTasks").value(userFacingTaskCount).compactPrevious();
    dumper.forKey("delayedTasks").value(delayedTaskCount);
    dumper.forKey("immediateTasks").value(immediateTasks).compactPrevious();
    dumper.forKey("maxUserFacingDelay").value(maxUserFacingTasks).compactPrevious();
    dumper.forKey("maxBackgroundDelay").value(maxBackgroundTasks).compactPrevious();
  }

  private void scheduleTask(String task, @TaskType int taskType, Runnable runnable) {
    if (taskType == TaskType.HEAD_INVALIDATE) {
      synchronized (lock) {
        waitForHeadReset = true;
      }
      immediateTasks++;
      executor.execute(() -> executeHeadInvalidate(task, runnable));
      return;
    }
    if (taskType == TaskType.HEAD_RESET) {
      immediateTasks++;
      executor.execute(() -> executeHeadReset(task, runnable));
      return;
    }
    Logger.i(TAG, "Delayed task %s", task);
    delayedTaskCount++;
    synchronized (lock) {
      if (taskType == TaskType.USER_FACING) {
        userTasks.add(Pair.create(task, runnable));
        maxUserFacingTasks = Math.max(maxUserFacingTasks, userTasks.size());
      } else if (taskType == TaskType.BACKGROUND) {
        backgroundTasks.add(Pair.create(task, runnable));
        maxBackgroundTasks = Math.max(maxBackgroundTasks, backgroundTasks.size());
      }
    }
  }

  private void executeNextTask() {
    synchronized (lock) {
      if (!isDelayed()) {
        // Nothing is delayed, so we are done.
        return;
      }
      // The priority is userTasks before backgroundTasks.
      if (!userTasks.isEmpty()) {
        Pair<String, Runnable> task = userTasks.remove();
        executor.execute(() -> executeTask(task.first, task.second));
      } else if (!backgroundTasks.isEmpty()) {
        Pair<String, Runnable> task = backgroundTasks.remove();
        executor.execute(() -> executeTask(task.first, task.second));
      } else {
        Logger.e(TAG, "Delayed without any Tasks found to execute");
      }
    }
  }

  private void executeHeadReset(String task, Runnable runnable) {
    Logger.i(TAG, "executeHeadReset %s", task);
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    runnable.run();
    synchronized (lock) {
      waitForHeadReset = false;
      executeNextTask();
    }
    timeTracker.stop("task", task, "", "executeHeadReset");
  }

  private void executeHeadInvalidate(String task, Runnable runnable) {
    Logger.i(TAG, "executeHeadInvalidate %s", task);
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    runnable.run();
    synchronized (lock) {
      waitForHeadReset = true;
    }
    timeTracker.stop("task", task, "", "executeHeadInvalidate");
  }

  private void executeTask(String task, Runnable runnable) {
    Logger.i(TAG, "executeTask %s", task);
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    runnable.run();
    timeTracker.stop("task", task, "", "executeTask");
    executeNextTask();
  }

  private void executeImmediate(String task, Runnable runnable) {
    Logger.i(TAG, "executeImmediate %s", task);
    ElapsedTimeTracker timeTracker = timingUtils.getElapsedTimeTracker(TAG);
    runnable.run();
    timeTracker.stop("task", task, "", "executeImmediate");
  }

  private boolean isDelayed() {
    synchronized (lock) {
      return !initialized || waitForHeadReset || !userTasks.isEmpty() || !backgroundTasks.isEmpty();
    }
  }
}
