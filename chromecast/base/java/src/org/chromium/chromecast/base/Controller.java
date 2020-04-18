// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import org.chromium.base.ObserverList;

import java.util.ArrayDeque;
import java.util.HashMap;
import java.util.Map;

/**
 * An Observable with public mutators that can control the state that it represents.
 *
 * The two mutators are set() and reset(). The set() method sets the state, and can inject
 * arbitrary data of the parameterized type, which will be forwarded to any observers of this
 * Controller. The reset() method deactivates the state.
 *
 * @param <T> The type of the state data.
 */
public class Controller<T> extends Observable<T> {
    private final Sequencer mSequencer = new Sequencer();
    private final ObserverList<ScopeFactory<? super T>> mEnterObservers = new ObserverList<>();
    private final Map<ScopeFactory<? super T>, Scope> mScopeMap = new HashMap<>();
    private T mData = null;

    @Override
    public Scope watch(ScopeFactory<? super T> observer) {
        mSequencer.sequence(() -> {
            mEnterObservers.addObserver(observer);
            if (mData != null) notifyEnter(observer);
        });
        return () -> mSequencer.sequence(() -> {
            if (mData != null) notifyExit(observer);
            mEnterObservers.removeObserver(observer);
        });
    }

    /**
     * Activates all observers of this Controller.
     *
     * If this controller is already set(), an implicit reset() is called. This allows observing
     * scopes to properly clean themselves up before the scope for the new activation is
     * created.
     *
     * This can be called inside a scope that is triggered by this very controller. If set() is
     * called while handling another set() or reset() call on the same Controller, it will be
     * queued and handled synchronously after the current set() or reset() is resolved.
     */
    public void set(T data) {
        mSequencer.sequence(() -> {
            // set(null) is equivalent to reset().
            if (data == null) {
                resetInternal();
                return;
            }
            // If this Controller was already set(), call reset() so observing Scopes can clean up.
            if (mData != null) {
                resetInternal();
            }
            mData = data;
            for (ScopeFactory<? super T> observer : mEnterObservers) {
                notifyEnter(observer);
            }
        });
    }

    /**
     * Deactivates all observers of this Controller.
     *
     * If this Controller is already reset(), this is a no-op.
     *
     * This can be called inside a scope that is triggered by this very controller. If reset()
     * is called while handling another set() or reset() call on the same Controller, it will be
     * queued and handled synchronously after the current set() or reset() is resolved.
     */
    public void reset() {
        mSequencer.sequence(() -> resetInternal());
    }

    private void resetInternal() {
        assert mSequencer.inSequence();
        if (mData == null) return;
        mData = null;
        for (ScopeFactory<? super T> observer : Itertools.reverse(mEnterObservers)) {
            notifyExit(observer);
        }
    }

    private void notifyEnter(ScopeFactory<? super T> factory) {
        assert mSequencer.inSequence();
        Scope scope = factory.create(mData);
        assert scope != null;
        mScopeMap.put(factory, scope);
    }

    private void notifyExit(ScopeFactory<? super T> factory) {
        assert mSequencer.inSequence();
        Scope scope = mScopeMap.get(factory);
        assert scope != null;
        mScopeMap.remove(factory);
        scope.close();
    }

    // TODO(sanfin): make this its own public class and add tests.
    private static class Sequencer {
        private boolean mIsRunning = false;
        private final ArrayDeque<Runnable> mMessageQueue = new ArrayDeque<>();

        /**
         * Runs the task synchronously, or, if a sequence()d task is already running, posts the task
         * to a queue, whose items will be run synchronously when the current task is finished.
         */
        public void sequence(Runnable impl) {
            if (mIsRunning) {
                mMessageQueue.add(() -> sequence(impl));
                return;
            }
            mIsRunning = true;
            impl.run();
            mIsRunning = false;
            while (!mMessageQueue.isEmpty()) {
                mMessageQueue.removeFirst().run();
            }
        }

        public boolean inSequence() {
            return mIsRunning;
        }
    }
}
