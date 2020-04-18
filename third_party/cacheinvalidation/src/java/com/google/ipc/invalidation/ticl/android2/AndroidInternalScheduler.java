/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ipc.invalidation.ticl.android2;

import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.external.client.SystemResources.Logger;
import com.google.ipc.invalidation.external.client.SystemResources.Scheduler;
import com.google.ipc.invalidation.external.client.android.service.AndroidLogger;
import com.google.ipc.invalidation.ticl.RecurringTask;
import com.google.ipc.invalidation.ticl.proto.AndroidService.AndroidSchedulerEvent;
import com.google.ipc.invalidation.ticl.proto.AndroidService.ScheduledTask;
import com.google.ipc.invalidation.util.NamedRunnable;
import com.google.ipc.invalidation.util.Preconditions;
import com.google.ipc.invalidation.util.TypedUtil;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;


/**
 * Scheduler for controlling access to internal Ticl state in Android.
 * <p>
 * This class maintains a map from recurring task names to the recurring task instances in the
 * associated Ticl. To schedule a recurring task, it uses the {@link AlarmManager} to schedule
 * an intent to itself at the appropriate time in the future. This intent contains the name of
 * the task to run; when it is received, this class looks up the appropriate recurring task
 * instance and runs it.
 * <p>
 * Note that this class only supports scheduling recurring tasks, not ordinary runnables. In
 * order for it to be used, the application must declare the AlarmReceiver of the scheduler
 * in the application's manifest file; see the implementation comment in AlarmReceiver for
 * details.
 *
 */
public final class AndroidInternalScheduler implements Scheduler {
  /** Class that receives AlarmManager broadcasts and reissues them as intents for this service. */
  public static final class AlarmReceiver extends BroadcastReceiver {
    private static final Logger logger = AndroidLogger.forTag("AlarmReceiver");

    /*
     * This class needs to be public so that it can be instantiated by the Android runtime.
     * Additionally, it should be declared as a broadcast receiver in the application manifest:
     * <receiver android:name="com.google.ipc.invalidation.ticl.android2.\
     *  AndroidInternalScheduler$AlarmReceiver" android:enabled="true"/>
     */

    @Override
    public void onReceive(Context context, Intent intent) {
      // Resend the intent to the service so that it's processed on the handler thread and with
      // the automatic shutdown logic provided by IntentService.
      intent.setClassName(context, new AndroidTiclManifest(context).getTiclServiceClass());
      try {
        context.startService(intent);
      } catch (IllegalStateException exception) {
        logger.warning("Unable to handle alarm: %s", exception);
      }
    }
  }

  /**
   * If {@code true}, {@link #isRunningOnThread} will verify that calls are being made from either
   * the {@link TiclService} or the {@link TestableTiclService.TestableClient}.
   */
  public static boolean checkStackForTest = false;

  /** Class name of the testable client class, for checking call stacks in tests. */
  private static final String TESTABLE_CLIENT_CLASSNAME_FOR_TEST =
      "com.google.ipc.invalidation.ticl.android2.TestableTiclService$TestableClient";

  /**
   * {@link RecurringTask}-created runnables that can be executed by this instance, by their names.
   */
  private final Map<String, Runnable> registeredTasks = new HashMap<>();

  /** Scheduled tasks (also stored as persistent state). */
  private final TreeMap<Long, String> scheduledTasks = new TreeMap<>();

  /** Android system context. */
  private final Context context;

  /** Source of time for computing scheduling delays. */
  private final AndroidClock clock;

  private Logger logger;

  /** Id of the Ticl for which this scheduler will process events. */
  private long ticlId = -1;

  AndroidInternalScheduler(Context context, AndroidClock clock) {
    this.context = Preconditions.checkNotNull(context);
    this.clock = Preconditions.checkNotNull(clock);
  }

  @Override
  public void setSystemResources(SystemResources resources) {
    this.logger = Preconditions.checkNotNull(resources.getLogger());
  }

  @Override
  public void schedule(int delayMs, Runnable runnable) {
    if (!(runnable instanceof NamedRunnable)) {
      throw new RuntimeException("Unsupported: can only schedule named runnables, not " + runnable);
    }
    // Create an intent that will cause the service to run the right recurring task. We explicitly
    // target it to our AlarmReceiver so that no other process in the system can receive it and so
    // that our AlarmReceiver will not be able to receive events from any other broadcaster (which
    // it would be if we used action-based targeting).
    String taskName = ((NamedRunnable) runnable).getName();
    long executeMs = clock.nowMs() + delayMs;
    while (scheduledTasks.containsKey(executeMs)) {
      ++executeMs;
    }
    scheduledTasks.put(executeMs, taskName);
    ensureIntentScheduledForSoonestTask();
  }

  /**
   * Schedules an intent (or updates an existing one) to ensure that we'll wake up and run once the
   * next pending task is due.
   */
  private void ensureIntentScheduledForSoonestTask() {
    Preconditions.checkState(!scheduledTasks.isEmpty());
    Map.Entry<Long, String> soonestTask = scheduledTasks.firstEntry();
    Intent eventIntent = ProtocolIntents.newImplicitSchedulerIntent();
    eventIntent.setClass(context, AlarmReceiver.class);

    // Create a pending intent that will cause the AlarmManager to fire the above intent.
    PendingIntent sender = PendingIntent.getBroadcast(context, 0, eventIntent,
        PendingIntent.FLAG_UPDATE_CURRENT);

    // Schedule the pending intent after the appropriate delay.
    AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
    try {
      alarmManager.set(AlarmManager.RTC, soonestTask.getKey(), sender);
    } catch (SecurityException exception) {
      logger.warning("Unable to schedule delayed registration: %s", exception);
    }
  }

  /**
   * Handles an event intent created in {@link #schedule} by running that event, along with any
   * other events whose time has come.
   */
  void handleSchedulerEvent(AndroidSchedulerEvent event) {
    Runnable recurringTaskRunnable = TypedUtil.mapGet(registeredTasks, event.getEventName());
    if (recurringTaskRunnable == null) {
      throw new NullPointerException("No task registered for " + event.getEventName());
    }
    if (ticlId != event.getTiclId()) {
      logger.warning("Ignoring event with wrong ticl id (not %s): %s", ticlId, event);
      return;
    }
    recurringTaskRunnable.run();
    handleImplicitSchedulerEvent();
  }

  /** Runs all tasks that are ready to run. */
  void handleImplicitSchedulerEvent() {
    try {
      while (!scheduledTasks.isEmpty() && (scheduledTasks.firstKey() <= clock.nowMs())) {
        Map.Entry<Long, String> scheduledTask = scheduledTasks.pollFirstEntry();
        Runnable runnable = TypedUtil.mapGet(registeredTasks, scheduledTask.getValue());
        if (runnable == null) {
          logger.severe("No task registered for %s", scheduledTask.getValue());
          continue;
        }
        runnable.run();
      }
    } finally {
      if (!scheduledTasks.isEmpty()) {
        ensureIntentScheduledForSoonestTask();
      }
    }
  }

  /**
   * Registers {@code task} so that it can be subsequently run by the scheduler.
   * <p>
   * REQUIRES: no recurring task with the same name be already present in {@link #registeredTasks}.
   */
  void registerTask(String name, Runnable runnable) {
    Runnable previous = registeredTasks.put(name, runnable);
    if (previous != null) {
      String message = new StringBuilder()
          .append("Cannot overwrite task registered on ")
          .append(name)
          .append(", ")
          .append(this)
          .append("; tasks = ")
          .append(registeredTasks.keySet())
          .toString();
      throw new IllegalStateException(message);
    }
  }

  @Override
  public boolean isRunningOnThread() {
    if (!checkStackForTest) {
      return true;
    }
    // If requested, check that the current stack looks legitimate.
    for (StackTraceElement stackElement : Thread.currentThread().getStackTrace()) {
      if (stackElement.getMethodName().equals("onHandleIntent") &&
          stackElement.getClassName().contains("TiclService")) {
        // Called from the TiclService.
        return true;
      }
      if (stackElement.getClassName().equals(TESTABLE_CLIENT_CLASSNAME_FOR_TEST)) {
        // Called from the TestableClient.
        return true;
      }
    }
    return false;
  }

  @Override
  public long getCurrentTimeMs() {
    return clock.nowMs();
  }

  /** Removes the registered tasks. */
  void reset() {
    logger.fine("Clearing registered tasks on %s", this);
    registeredTasks.clear();
    scheduledTasks.clear();
    ticlId = -1;
  }

  /**
   * Sets the id of the Ticl for which this scheduler will process events and populates the
   * in-memory pending task queue with whatever was written to storage. We do not know the Ticl id
   * until done constructing the Ticl, and we need the scheduler to construct a Ticl. This method
   * breaks what would otherwise be a dependency cycle on getting the Ticl id.
   */
  void init(long ticlId, Collection<ScheduledTask> tasks) {
    this.ticlId = ticlId;

    // Clear out any scheduled tasks from the old ticl id (for tests only?).
    scheduledTasks.clear();

    // Add tasks from persistent storage.
    for (ScheduledTask task : tasks) {
      long executeTimeMs = task.getExecuteTimeMs();
      scheduledTasks.put(executeTimeMs, task.getEventName());
    }
  }

  /** Marshals the state of the scheduler, which consists of all tasks that are still pending. */
  Collection<ScheduledTask> marshal() {
    ArrayList<ScheduledTask> taskList = new ArrayList<>(scheduledTasks.size());
    for (Map.Entry<Long, String> entry : scheduledTasks.entrySet()) {
      taskList.add(ScheduledTask.create(entry.getValue(),  entry.getKey()));
    }
    return taskList;
  }
}
