/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package java.lang;

import android.system.Os;
import android.system.OsConstants;
import dalvik.system.VMRuntime;
import java.lang.ref.FinalizerReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.TimeoutException;
import libcore.util.EmptyArray;

/**
 * Calls Object.finalize() on objects in the finalizer reference queue. The VM
 * will abort if any finalize() call takes more than the maximum finalize time
 * to complete.
 *
 * @hide
 */
public final class Daemons {
    private static final int NANOS_PER_MILLI = 1000 * 1000;
    private static final int NANOS_PER_SECOND = NANOS_PER_MILLI * 1000;
    private static final long MAX_FINALIZE_NANOS = 10L * NANOS_PER_SECOND;

    public static void start() {
        ReferenceQueueDaemon.INSTANCE.start();
        FinalizerDaemon.INSTANCE.start();
        FinalizerWatchdogDaemon.INSTANCE.start();
        HeapTaskDaemon.INSTANCE.start();
    }

    public static void stop() {
        HeapTaskDaemon.INSTANCE.stop();
        ReferenceQueueDaemon.INSTANCE.stop();
        FinalizerDaemon.INSTANCE.stop();
        FinalizerWatchdogDaemon.INSTANCE.stop();
    }

    /**
     * A background task that provides runtime support to the application.
     * Daemons can be stopped and started, but only so that the zygote can be a
     * single-threaded process when it forks.
     */
    private static abstract class Daemon implements Runnable {
        private Thread thread;
        private String name;

        protected Daemon(String name) {
            this.name = name;
        }

        public synchronized void start() {
            if (thread != null) {
                throw new IllegalStateException("already running");
            }
            thread = new Thread(ThreadGroup.systemThreadGroup, this, name);
            thread.setDaemon(true);
            thread.start();
        }

        public abstract void run();

        /**
         * Returns true while the current thread should continue to run; false
         * when it should return.
         */
        protected synchronized boolean isRunning() {
            return thread != null;
        }

        public synchronized void interrupt() {
            interrupt(thread);
        }

        public synchronized void interrupt(Thread thread) {
            if (thread == null) {
                throw new IllegalStateException("not running");
            }
            thread.interrupt();
        }

        /**
         * Waits for the runtime thread to stop. This interrupts the thread
         * currently running the runnable and then waits for it to exit.
         */
        public void stop() {
            Thread threadToStop;
            synchronized (this) {
                threadToStop = thread;
                thread = null;
            }
            if (threadToStop == null) {
                throw new IllegalStateException("not running");
            }
            interrupt(threadToStop);
            while (true) {
                try {
                    threadToStop.join();
                    return;
                } catch (InterruptedException ignored) {
                } catch (OutOfMemoryError ignored) {
                    // An OOME may be thrown if allocating the InterruptedException failed.
                }
            }
        }

        /**
         * Returns the current stack trace of the thread, or an empty stack trace
         * if the thread is not currently running.
         */
        public synchronized StackTraceElement[] getStackTrace() {
            return thread != null ? thread.getStackTrace() : EmptyArray.STACK_TRACE_ELEMENT;
        }
    }

    /**
     * This heap management thread moves elements from the garbage collector's
     * pending list to the managed reference queue.
     */
    private static class ReferenceQueueDaemon extends Daemon {
        private static final ReferenceQueueDaemon INSTANCE = new ReferenceQueueDaemon();

        ReferenceQueueDaemon() {
            super("ReferenceQueueDaemon");
        }

        @Override public void run() {
            while (isRunning()) {
                Reference<?> list;
                try {
                    synchronized (ReferenceQueue.class) {
                        while (ReferenceQueue.unenqueued == null) {
                            ReferenceQueue.class.wait();
                        }
                        list = ReferenceQueue.unenqueued;
                        ReferenceQueue.unenqueued = null;
                    }
                } catch (InterruptedException e) {
                    continue;
                } catch (OutOfMemoryError e) {
                    continue;
                }
                ReferenceQueue.enqueuePending(list);
            }
        }
    }

    private static class FinalizerDaemon extends Daemon {
        private static final FinalizerDaemon INSTANCE = new FinalizerDaemon();
        private final ReferenceQueue<Object> queue = FinalizerReference.queue;
        private final AtomicInteger progressCounter = new AtomicInteger(0);
        // Object (not reference!) being finalized. Accesses may race!
        private Object finalizingObject = null;

        FinalizerDaemon() {
            super("FinalizerDaemon");
        }

        @Override public void run() {
            // This loop may be performance critical, since we need to keep up with mutator
            // generation of finalizable objects.
            // We minimize the amount of work we do per finalizable object. For example, we avoid
            // reading the current time here, since that involves a kernel call per object.  We
            // limit fast path communication with FinalizerWatchDogDaemon to what's unavoidable: A
            // non-volatile store to communicate the current finalizable object, e.g. for
            // reporting, and a release store (lazySet) to a counter.
            // We do stop the  FinalizerWatchDogDaemon if we have nothing to do for a
            // potentially extended period.  This prevents the device from waking up regularly
            // during idle times.

            // Local copy of progressCounter; saves a fence per increment on ARM and MIPS.
            int localProgressCounter = progressCounter.get();

            while (isRunning()) {
                try {
                    // Use non-blocking poll to avoid FinalizerWatchdogDaemon communication
                    // when busy.
                    FinalizerReference<?> finalizingReference = (FinalizerReference<?>)queue.poll();
                    if (finalizingReference != null) {
                        finalizingObject = finalizingReference.get();
                        progressCounter.lazySet(++localProgressCounter);
                    } else {
                        finalizingObject = null;
                        progressCounter.lazySet(++localProgressCounter);
                        // Slow path; block.
                        FinalizerWatchdogDaemon.INSTANCE.goToSleep();
                        finalizingReference = (FinalizerReference<?>)queue.remove();
                        finalizingObject = finalizingReference.get();
                        progressCounter.set(++localProgressCounter);
                        FinalizerWatchdogDaemon.INSTANCE.wakeUp();
                    }
                    doFinalize(finalizingReference);
                } catch (InterruptedException ignored) {
                } catch (OutOfMemoryError ignored) {
                }
            }
        }

        @FindBugsSuppressWarnings("FI_EXPLICIT_INVOCATION")
        private void doFinalize(FinalizerReference<?> reference) {
            FinalizerReference.remove(reference);
            Object object = reference.get();
            reference.clear();
            try {
                object.finalize();
            } catch (Throwable ex) {
                // The RI silently swallows these, but Android has always logged.
                System.logE("Uncaught exception thrown by finalizer", ex);
            } finally {
                // Done finalizing, stop holding the object as live.
                finalizingObject = null;
            }
        }
    }

    /**
     * The watchdog exits the VM if the finalizer ever gets stuck. We consider
     * the finalizer to be stuck if it spends more than MAX_FINALIZATION_MILLIS
     * on one instance.
     */
    private static class FinalizerWatchdogDaemon extends Daemon {
        private static final FinalizerWatchdogDaemon INSTANCE = new FinalizerWatchdogDaemon();

        private boolean needToWork = true;  // Only accessed in synchronized methods.

        FinalizerWatchdogDaemon() {
            super("FinalizerWatchdogDaemon");
        }

        @Override public void run() {
            while (isRunning()) {
                if (!sleepUntilNeeded()) {
                    // We have been interrupted, need to see if this daemon has been stopped.
                    continue;
                }
                final Object finalizing = waitForFinalization();
                if (finalizing != null && !VMRuntime.getRuntime().isDebuggerActive()) {
                    finalizerTimedOut(finalizing);
                    break;
                }
            }
        }

        /**
         * Wait until something is ready to be finalized.
         * Return false if we have been interrupted
         * See also http://code.google.com/p/android/issues/detail?id=22778.
         */
        private synchronized boolean sleepUntilNeeded() {
            while (!needToWork) {
                try {
                    wait();
                } catch (InterruptedException e) {
                    // Daemon.stop may have interrupted us.
                    return false;
                } catch (OutOfMemoryError e) {
                    return false;
                }
            }
            return true;
        }

        /**
         * Notify daemon that it's OK to sleep until notified that something is ready to be
         * finalized.
         */
        private synchronized void goToSleep() {
            needToWork = false;
        }

        /**
         * Notify daemon that there is something ready to be finalized.
         */
        private synchronized void wakeUp() {
            needToWork = true;
            notify();
        }

        private synchronized boolean getNeedToWork() {
            return needToWork;
        }

        /**
         * Sleep for the given number of nanoseconds.
         * @return false if we were interrupted.
         */
        private boolean sleepFor(long durationNanos) {
            long startNanos = System.nanoTime();
            while (true) {
                long elapsedNanos = System.nanoTime() - startNanos;
                long sleepNanos = durationNanos - elapsedNanos;
                long sleepMills = sleepNanos / NANOS_PER_MILLI;
                if (sleepMills <= 0) {
                    return true;
                }
                try {
                    Thread.sleep(sleepMills);
                } catch (InterruptedException e) {
                    if (!isRunning()) {
                        return false;
                    }
                } catch (OutOfMemoryError ignored) {
                    if (!isRunning()) {
                        return false;
                    }
                }
            }
        }


        /**
         * Return an object that took too long to finalize or return null.
         * Wait MAX_FINALIZE_NANOS.  If the FinalizerDaemon took essentially the whole time
         * processing a single reference, return that reference.  Otherwise return null.
         */
        private Object waitForFinalization() {
            long startCount = FinalizerDaemon.INSTANCE.progressCounter.get();
            // Avoid remembering object being finalized, so as not to keep it alive.
            if (!sleepFor(MAX_FINALIZE_NANOS)) {
                // Don't report possibly spurious timeout if we are interrupted.
                return null;
            }
            if (getNeedToWork() && FinalizerDaemon.INSTANCE.progressCounter.get() == startCount) {
                // We assume that only remove() and doFinalize() may take time comparable to
                // MAX_FINALIZE_NANOS.
                // We observed neither the effect of the gotoSleep() nor the increment preceding a
                // later wakeUp. Any remove() call by the FinalizerDaemon during our sleep
                // interval must have been followed by a wakeUp call before we checked needToWork.
                // But then we would have seen the counter increment.  Thus there cannot have
                // been such a remove() call.
                // The FinalizerDaemon must not have progressed (from either the beginning or the
                // last progressCounter increment) to either the next increment or gotoSleep()
                // call.  Thus we must have taken essentially the whole MAX_FINALIZE_NANOS in a
                // single doFinalize() call.  Thus it's OK to time out.  finalizingObject was set
                // just before the counter increment, which preceded the doFinalize call.  Thus we
                // are guaranteed to get the correct finalizing value below, unless doFinalize()
                // just finished as we were timing out, in which case we may get null or a later
                // one.  In this last case, we are very likely to discard it below.
                Object finalizing = FinalizerDaemon.INSTANCE.finalizingObject;
                sleepFor(NANOS_PER_SECOND / 2);
                // Recheck to make it even less likely we report the wrong finalizing object in
                // the case which a very slow finalization just finished as we were timing out.
                if (getNeedToWork()
                        && FinalizerDaemon.INSTANCE.progressCounter.get() == startCount) {
                    return finalizing;
                }
            }
            return null;
        }

        private static void finalizerTimedOut(Object object) {
            // The current object has exceeded the finalization deadline; abort!
            String message = object.getClass().getName() + ".finalize() timed out after "
                    + (MAX_FINALIZE_NANOS / NANOS_PER_SECOND) + " seconds";
            Exception syntheticException = new TimeoutException(message);
            // We use the stack from where finalize() was running to show where it was stuck.
            syntheticException.setStackTrace(FinalizerDaemon.INSTANCE.getStackTrace());
            Thread.UncaughtExceptionHandler h = Thread.getDefaultUncaughtExceptionHandler();
            // Send SIGQUIT to get native stack traces.
            try {
                Os.kill(Os.getpid(), OsConstants.SIGQUIT);
                // Sleep a few seconds to let the stack traces print.
                Thread.sleep(5000);
            } catch (Exception e) {
                System.logE("failed to send SIGQUIT", e);
            } catch (OutOfMemoryError ignored) {
                // May occur while trying to allocate the exception.
            }
            if (h == null) {
                // If we have no handler, log and exit.
                System.logE(message, syntheticException);
                System.exit(2);
            }
            // Otherwise call the handler to do crash reporting.
            // We don't just throw because we're not the thread that
            // timed out; we're the thread that detected it.
            h.uncaughtException(Thread.currentThread(), syntheticException);
        }
    }

    // Adds a heap trim task to the heap event processor, not called from java. Left for
    // compatibility purposes due to reflection.
    public static void requestHeapTrim() {
        VMRuntime.getRuntime().requestHeapTrim();
    }

    // Adds a concurrent GC request task ot the heap event processor, not called from java. Left
    // for compatibility purposes due to reflection.
    public static void requestGC() {
        VMRuntime.getRuntime().requestConcurrentGC();
    }

    private static class HeapTaskDaemon extends Daemon {
        private static final HeapTaskDaemon INSTANCE = new HeapTaskDaemon();

        HeapTaskDaemon() {
            super("HeapTaskDaemon");
        }

        // Overrides the Daemon.interupt method which is called from Daemons.stop.
        public synchronized void interrupt(Thread thread) {
            VMRuntime.getRuntime().stopHeapTaskProcessor();
        }

        @Override public void run() {
            synchronized (this) {
                if (isRunning()) {
                  // Needs to be synchronized or else we there is a race condition where we start
                  // the thread, call stopHeapTaskProcessor before we start the heap task
                  // processor, resulting in a deadlock since startHeapTaskProcessor restarts it
                  // while the other thread is waiting in Daemons.stop().
                  VMRuntime.getRuntime().startHeapTaskProcessor();
                }
            }
            // This runs tasks until we are stopped and there is no more pending task.
            VMRuntime.getRuntime().runHeapTasks();
        }
    }
}
