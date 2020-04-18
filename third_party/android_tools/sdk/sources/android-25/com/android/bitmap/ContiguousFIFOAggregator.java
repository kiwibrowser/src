/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.bitmap;

import android.util.Log;
import android.util.SparseArray;

import com.android.bitmap.util.Trace;

import java.util.ArrayDeque;
import java.util.Iterator;
import java.util.Queue;

/**
 * An implementation of a task aggregator that executes tasks in the order that they are expected
 * . All tasks that is given to {@link #execute(Object, Runnable)} must correspond to a key. This
 * key must have been previously declared with {@link #expect(Object, Callback)}.
 * The task will be scheduled to run when its corresponding key becomes the first expected key
 * amongst the other keys in this aggregator.
 * <p/>
 * If on {@link #execute(Object, Runnable)} the key is not expected, the task will be executed
 * immediately as an edge case.
 * <p/>
 * A characteristic scenario is as follows:
 * <ol>
 * <li>Expect keys <b>A</b>, <b>B</b>, <b>C</b>, and <b>Z</b>, in that order. Key <b>A</b> is now
 * the first expected key.</li>
 * <li>Execute task <b>2</b> for key <b>B</b>. The first expected key is <b>A</b>,
 * which has no task associated with it, so we store task <b>2</b>. </li>
 * <li>Execute task <b>4</b> for key <b>Z</b>. We store task <b>4</b>. </li>
 * <li>Execute task <b>1</b> for key <b>A</b>. We run task <b>1</b> because its key <b>A</b> is
 * the first expected, then we remove key <b>A</b> from the list of keys that we expect.</li>
 * <li>This causes key <b>B</b> to be the first expected key, and we see that we have previously
 * stored task <b>2</b> for that key. We run task <b>2</b> and remove key <b>B</b>. </li>
 * <li>Key <b>C</b> is now the first expected key, but it has no task,
 * so nothing happens. Task <b>4</b>, which was previously stored,
 * cannot run until its corresponding key <b>Z</b> becomes the first expected key. This can
 * happen if a task comes in for key <b>C</b> or if forget is called on key <b>C</b>.</li>
 * </ol>
 * <p/>
 * ContiguousFIFOAggregator is not thread safe.
 */
public class ContiguousFIFOAggregator<T> {
    private final Queue<T> mExpected;
    private final SparseArray<Value> mTasks;

    private static final String TAG = ContiguousFIFOAggregator.class.getSimpleName();
    private static final boolean DEBUG = false;

    /**
     * Create a new ContiguousFIFOAggregator.
     * <p/>
     * The nature of the prioritization means that the number of stored keys and tasks may grow
     * unbounded. This may happen, for instance, if the top priority key is never given a task to
     * {@link #execute(Object, Runnable)}. However, in practice, if you are generating tasks in
     * response to UI elements appearing on the screen, you will only have a bounded set of keys.
     * UI elements that scroll off the screen will call {@link #forget(Object)} while new elements
     * will call {@link #expect(Object, Callback)}. This means that the expected
     * number of keys and tasks is
     * the maximum number of UI elements that you expect to show on the screen at any time.
     */
    public ContiguousFIFOAggregator() {
        mExpected = new ArrayDeque<T>();
        mTasks = new SparseArray<Value>();
    }

    /**
     * Declare a key to be expected in the future. The order in which you expect keys is very
     * important. Keys that are declared first are guaranteed to have their tasks run first. You
     * must call either {@link #forget(Object)} or {@link #execute(Object, Runnable)}
     * with this key in the future, or you will leak the key.
     *
     * If you call expect with a previously expected key, the key will be placed at the back of
     * the expected queue and its callback will be replaced with this one.
     *
     * @param key      the key to expect a task for. Use the same key when setting the task later
     *                 with {@link #execute (Object, Runnable)}.
     * @param callback the callback to notify when the key becomes the first expected key, or null.
     */
    public void expect(final T key, final Callback<T> callback) {
        if (key == null) {
            throw new IllegalArgumentException("Do not use null keys.");
        }

        Trace.beginSection("pool expect");
        final int hash = key.hashCode();
        if (contains(key)) {
            mExpected.remove(key);
            mTasks.remove(hash);
        }
        final boolean isFirst = mExpected.isEmpty();
        mExpected.offer(key);
        mTasks.put(hash, new Value(callback, null));
        if (DEBUG) {
            Log.d(TAG, String.format("ContiguousFIFOAggregator >> tasks: %s", prettyPrint()));
        }

        if (isFirst) {
            onFirstExpectedChanged(key);
        }
        Trace.endSection();
    }

    /**
     * Remove a previously declared key that we no longer expect to execute a task for. This
     * potentially allows another key to now become the first expected key,
     * and so this may trigger one or more tasks to be executed.
     *
     * @param key the key previously declared in {@link #expect(Object, Callback)}.
     *
     */
    public void forget(final T key) {
        if (key == null) {
            throw new IllegalArgumentException("Do not use null keys.");
        }

        if (!contains(key)) {
            return;
        }

        Trace.beginSection("pool forget");
        final boolean removedFirst = key.equals(mExpected.peek());
        mExpected.remove(key);
        mTasks.delete(key.hashCode());
        if (DEBUG) {
            Log.d(TAG, String.format("ContiguousFIFOAggregator  < tasks: %s", prettyPrint()));
        }

        final T second;
        if (removedFirst && (second = mExpected.peek()) != null) {
            // We removed the first key. The second key is now first.
            onFirstExpectedChanged(second);
        }

        maybeExecuteNow();
        Trace.endSection();
    }

    /**
     * Attempt to execute a task corresponding to a previously declared key. If the key is the
     * first expected key, the task will be executed immediately. Otherwise, the task is stored
     * until its key becomes the first expected key. Execution of a task results in the removal
     * of that key, which potentially allows another key to now become the first expected key,
     * and may cause one or more other tasks to be executed.
     * <p/>
     * If the key is not expected, the task will be executed immediately as an edge case.
     *
     * @param key  the key previously declared in {@link #expect(Object, Callback)}.
     * @param task the task to execute or store, depending on its corresponding key.
     */
    public void execute(final T key, final Runnable task) {
        Trace.beginSection("pool execute");
        final int hash = key.hashCode();
        final Value value = mTasks.get(hash);
        if (value == null || task == null) {
            if (task != null) {
                task.run();
            }
            Trace.endSection();
            return;
        }
        value.task = task;
        if (DEBUG) {
            Log.d(TAG, String.format("ContiguousFIFOAggregator ++ tasks: %s", prettyPrint()));
        }
        maybeExecuteNow();
        Trace.endSection();
    }

    /**
     * Triggered by {@link #execute(Object, Runnable)} and {@link #forget(Object)}. The keys or
     * tasks have changed, which may cause one or more tasks to be executed. This method will
     * continue to execute tasks associated with the first expected key to the last expected key,
     * stopping when it finds that the first expected key has not yet been assigned a task.
     */
    private void maybeExecuteNow() {
        T first;
        int count = 0;
        while (!mExpected.isEmpty()) {
            Trace.beginSection("pool maybeExecuteNow loop");
            first = mExpected.peek();
            if (count > 0) {
                // When count == 0, the key is already first.
                onFirstExpectedChanged(first);
            }

            final int hash = first.hashCode();
            final Value value = mTasks.get(hash);
            if (value.task == null) {
                Trace.endSection();
                break;
            }

            mExpected.poll();
            mTasks.delete(hash);
            if (DEBUG) {
                Log.d(TAG, String.format("ContiguousFIFOAggregator  - tasks: %s", prettyPrint()));
            }
            value.task.run();
            count++;
            Trace.endSection();
        }
    }

    /**
     * This method should only be called once for any key.
     * @param key The key that has become the new first expected key.
     */
    private void onFirstExpectedChanged(final T key) {
        final int hash = key.hashCode();
        final Value value = mTasks.get(hash);
        if (value == null) {
            return;
        }
        final Callback<T> callback = value.callback;
        if (callback == null) {
            return;
        }
        if (DEBUG) {
            Log.d(TAG, String.format("ContiguousFIFOAggregator    first: %d", hash));
        }
        callback.onBecomeFirstExpected(key);
    }

    private boolean contains(final T key) {
        return mTasks.get(key.hashCode()) != null;
    }

    /**
     * Get a pretty string representing the internal data.
     * @return  a String for the internal data.
     */
    private String prettyPrint() {
        if (mExpected.isEmpty()) {
            return "{}";
        }

        StringBuilder buffer = new StringBuilder(mExpected.size() * 28);
        buffer.append('{');
        Iterator<T> it = mExpected.iterator();
        while (it.hasNext()) {
            final T key = it.next();
            final int hash = key.hashCode();
            buffer.append(hash);
            buffer.append('=');
            final Value value = mTasks.get(hash);
            buffer.append(value);
            if (it.hasNext()) {
                buffer.append(", ");
            }
        }
        buffer.append('}');
        if (mExpected.size() != mTasks.size()) {
            buffer.append(" error");
        }
        return buffer.toString();
    }

    /**
     * Implement this interface if you want to be notified when the key becomes the first
     * expected key.
     * @param <T> The type of the key used for the aggregator.
     */
    public interface Callback<T> {

        /**
         * The key you declared as expected has become the first expected key in this aggregator.
         *
         * We don't need a noLongerFirstExpected() method because this aggregator strictly adds
         * additional to the end of the queue. For a first expected key to no longer be the first
         * expected, it would either have to be forgotten with {@link #forget(Object)} or a task
         * assigned and executed with {@link #execute(Object, Runnable)}.
         *
         * @param key The key that became first. We provide the key so the callback can either not
         *            keep state, or it can keep state which may have changed so the callback can do
         *            a comparison.
         */
        void onBecomeFirstExpected(final T key);
    }

    /**
     * Holds the callback and task for when a key later becomes the first expected key.
     */
    private class Value {

        final Callback<T> callback;
        Runnable task;

        Value(final Callback<T> callback, final Runnable task) {
            this.callback = callback;
            this.task = task;
        }

        @Override
        public String toString() {
            return String.valueOf(task);
        }
    }
}
