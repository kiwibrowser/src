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

package com.google.android.libraries.feed.common.time;

import android.text.TextUtils;
import android.util.LongSparseArray;
import com.google.android.libraries.feed.api.common.ThreadUtils;
import com.google.android.libraries.feed.common.logging.Dumpable;
import com.google.android.libraries.feed.common.logging.Dumper;
import com.google.android.libraries.feed.common.logging.Logger;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Queue;
import java.util.Stack;
import javax.annotation.concurrent.GuardedBy;

/**
 * Utility class providing timing related utilities. The primary feature is to created {@link
 * ElapsedTimeTracker} instances, which are used to track elapsed time for tasks. The timing
 * information is output to the state dump.
 */
public class TimingUtils implements Dumpable {
  private static final String TAG = "TimingUtils";
  private static final String BACKGROUND_THREAD = "background-";
  private static final String UI_THREAD = "ui";
  private static final int MAX_TO_DUMP = 10;

  private static int bgThreadId = 1;

  private final ThreadUtils threadUtils = new ThreadUtils();
  private final Object lock = new Object();

  @GuardedBy("lock")
  private final Queue<ThreadState> threadDumps = new ArrayDeque<>(MAX_TO_DUMP);

  @GuardedBy("lock")
  private final LongSparseArray<ThreadStack> threadStacks = new LongSparseArray<>();

  /**
   * ElapsedTimeTracker works similar to Stopwatch. This is used to track elapsed time for some
   * task. The start time is tracked when the instance is created. {@code stop} is used to capture
   * the end time and other statistics about the task. The ElapsedTimeTrackers dumped through the
   * Dumper. A stack is maintained to log sub-tasks with proper indentation in the Dumper.
   *
   * <p>The class will dump only the {@code MAX_TO_DUMP} most recent dumps, discarding older dumps
   * when new dumps are created.
   *
   * <p>ElapsedTimeTracker is designed as a one use class, {@code IllegalStateException}s are thrown
   * if the class isn't used correctly.
   */
  public static class ElapsedTimeTracker {
    private final ThreadStack threadStack;
    private final String source;

    private final long startTime;
    private long endTime = 0;

    private ElapsedTimeTracker(ThreadStack threadStack, String source) {
      this.threadStack = threadStack;
      this.source = source;
      startTime = System.nanoTime();
    }

    /**
     * Capture the end time for the elapsed time. {@code IllegalStateException} is thrown if stop is
     * called more than once. Arguments are treated as pairs within the Dumper output.
     *
     * <p>For example: dumper.forKey(arg[0]).value(arg[1])
     */
    public void stop(Object... args) {
      if (endTime > 0) {
        throw new IllegalStateException("ElapsedTimeTracker has already been stopped.");
      }
      endTime = System.nanoTime();
      TrackerState trackerState =
          new TrackerState(endTime - startTime, source, args, threadStack.stack.size());
      threadStack.addTrackerState(trackerState);
      threadStack.popElapsedTimeTracker(this);
    }
  }

  /**
   * Return a new {@link ElapsedTimeTracker} which is added to the Thread scoped stack. When we dump
   * the tracker, we will indent the source to indicate sub-tasks within a larger task.
   */
  public ElapsedTimeTracker getElapsedTimeTracker(String source) {
    long threadId = Thread.currentThread().getId();
    synchronized (lock) {
      ThreadStack timerStack = threadStacks.get(threadId);
      if (timerStack == null) {
        timerStack =
            new ThreadStack(
                threadUtils.isMainThread() ? UI_THREAD : BACKGROUND_THREAD + bgThreadId++, false);
        threadStacks.put(threadId, timerStack);
      }
      ElapsedTimeTracker timeTracker = new ElapsedTimeTracker(timerStack, source);
      timerStack.stack.push(timeTracker);
      return timeTracker;
    }
  }

  /**
   * This is called to pin the stack structure for a thread. This should only be done for threads
   * which are long lived. Non-pinned thread will have their stack structures clean up when the
   * stack is empty.
   */
  public void pinThread(Thread thread, String name) {
    ThreadStack timerStack = new ThreadStack(name, true);
    synchronized (lock) {
      threadStacks.put(thread.getId(), timerStack);
    }
  }

  @Override
  public void dump(Dumper dumper) {
    dumper.title(TAG);
    synchronized (lock) {
      for (ThreadState threadState : threadDumps) {
        dumpThreadState(dumper, threadState);
      }
    }
  }

  private void dumpThreadState(Dumper dumper, ThreadState threadState) {
    if (threadState.trackerStates.isEmpty()) {
      Logger.w(TAG, "Found Empty TrackerState List");
      return;
    }
    dumper.forKey("thread").value(threadState.threadName);
    dumper.forKey("timeStamp").value(threadState.date).compactPrevious();
    for (int i = threadState.trackerStates.size() - 1; i >= 0; i--) {
      TrackerState trackerState = threadState.trackerStates.get(i);
      Dumper child = dumper.getChildDumper();
      child.forKey("time", trackerState.indent - 1).value(trackerState.duration / 1000000 + "ms");
      child.forKey("source").value(trackerState.source).compactPrevious();
      if (trackerState.args != null && trackerState.args.length > 0) {
        for (int j = 0; j < trackerState.args.length; j++) {
          String key = trackerState.args[j++].toString();
          Object value = (j < trackerState.args.length) ? trackerState.args[j] : "";
          child.forKey(key, trackerState.indent - 1).valueObject(value).compactPrevious();
        }
      }
    }
  }

  /** Definition of a Stack of {@link ElapsedTimeTracker} instances. */
  private class ThreadStack {
    final String name;
    final Stack<ElapsedTimeTracker> stack = new Stack<>();
    private List<TrackerState> trackerStates = new ArrayList<>();
    final boolean pin;

    ThreadStack(String name, boolean pin) {
      this.name = name;
      this.pin = pin;
    }

    void addTrackerState(TrackerState trackerState) {
      trackerStates.add(trackerState);
    }

    void popElapsedTimeTracker(ElapsedTimeTracker tracker) {
      ElapsedTimeTracker top = stack.peek();
      if (top != tracker) {
        // TODO: What do we do about errors?
        Logger.w(TAG, "Trying to Pop non-top of stack timer");
      }
      stack.pop();
      if (stack.isEmpty()) {
        StringBuilder sb = new StringBuilder();
        TrackerState ts = trackerStates.get(trackerStates.size() - 1);
        for (int i = 0; i < ts.args.length; i++) {
          String key = ts.args[i++].toString();
          Object value = (i < ts.args.length) ? ts.args[i] : "";
          if (!TextUtils.isEmpty(key)) {
            sb.append(key).append(" : ").append(value);
          } else {
            sb.append(value);
          }
          if ((i + 1) < ts.args.length) {
            sb.append(" | ");
          }
        }
        Logger.i(
            TAG,
            "Task Timing %3sms, thread %s | %s",
            ((tracker.endTime - tracker.startTime) / 1000000),
            tracker.threadStack.name,
            sb);
        synchronized (lock) {
          if (threadDumps.size() == MAX_TO_DUMP) {
            // Before adding a new tracker state, remove the oldest one.
            threadDumps.remove();
          }
          threadDumps.add(new ThreadState(trackerStates, name));
          trackerStates = new ArrayList<>();
          if (!pin) {
            threadStacks.remove(Thread.currentThread().getId());
          }
        }
      }
    }
  }

  /** State associated with a thread */
  private static class ThreadState {
    final List<TrackerState> trackerStates;
    final String threadName;
    final Date date;

    ThreadState(List<TrackerState> trackerStates, String threadName) {
      this.trackerStates = trackerStates;
      this.threadName = threadName;
      date = new Date();
    }
  }

  /** State associated with a completed ElapsedTimeTracker */
  private static class TrackerState {
    final long duration;
    final String source;
    // TODO: Should we convert this to Pair?
    final Object[] args;
    final int indent;

    TrackerState(long duration, String source, Object[] args, int indent) {
      this.duration = duration;
      this.source = source;
      this.args = args;
      this.indent = indent;
    }
  }
}
