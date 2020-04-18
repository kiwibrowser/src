// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

/**
 * Interface for Observable state.
 *
 * Observables can have some data associated with them, which is provided to observers when the
 * Observable activates. The <T> parameter determines the type of this data.
 *
 * Only this class has access to addObserver(). Clients should use the `watch()` method to track
 * the life cycle of Observables.
 *
 * @param <T> The type of the state data.
 */
public abstract class Observable<T> {
    /**
     * Tracks this Observable with the given scope factory.
     *
     * Returns a Scope that, when closed, will unregister the scope factory so that it will no
     * longer be notified of updates.
     *
     * When this Observable is activated, the factory will be invoked with the activation data
     * to produce a scope. When this Observable is deactivated, that scope will have its close()
     * method invoked. In this way, one can define state transitions from the ScopeFactory and
     * its return value's close() method.
     */
    public abstract Scope watch(ScopeFactory<? super T> factory);

    /**
     * Tracks this Observable with the given scope factory, ignoring activation data.
     *
     * Returns a Scope that, when closed, will unregister the scope factory so that it will no
     * longer be notifies of updates.
     *
     * A VoidScopeFactory does not care about the activation data, as its create() function
     * takes no arguments.
     */
    public final Scope watch(VoidScopeFactory factory) {
        return watch((T value) -> factory.create());
    }

    /**
     * Creates an Observable that activates observers only if both `this` and `other` are activated,
     * and deactivates observers if either of `this` or `other` are deactivated.
     *
     * This is useful for creating an event handler that should only activate when two events
     * have occurred, but those events may occur in any order.
     *
     * This is composable (returns an Observable), so one can use this to observe the intersection
     * of arbitrarily many Observables.
     */
    public final <U> Observable<Both<T, U>> and(Observable<U> other) {
        Controller<Both<T, U>> controller = new Controller<>();
        watch(t -> other.watch(u -> {
            controller.set(Both.both(t, u));
            return controller::reset;
        }));
        return controller;
    }

    /**
     * Returns an Observable that is activated when `this` and `other` are activated in order.
     *
     * This is similar to `and()`, but does not activate if `other` is activated before `this`.
     *
     * @param <U> The activation data type of the other Observable.
     */
    public final <U> Observable<Both<T, U>> andThen(Observable<U> other) {
        return new SequenceStateObserver<>(this, other).asObservable();
    }

    /**
     * Returns an Observable that applies the given Function to this Observable's activation
     * values.
     *
     * @param <R> The return type of the transform function.
     */
    public final <R> Observable<R> map(Function<? super T, ? extends R> transform) {
        Controller<R> controller = new Controller<>();
        watch((T value) -> {
            controller.set(transform.apply(value));
            return controller::reset;
        });
        return controller;
    }

    /**
     * Returns an Observable that is only activated when `this` is activated with a value such that
     * the given `predicate` returns true.
     */
    public final Observable<T> filter(Predicate<? super T> predicate) {
        Controller<T> controller = new Controller<>();
        watch((T value) -> {
            if (predicate.test(value)) {
                controller.set(value);
            }
            return controller::reset;
        });
        return controller;
    }

    /**
     * Returns an Observable that is activated only when `this` is first activated, and is not
     * activated an subsequent activations of `this`.
     *
     * This is useful for ensuring that a callback registered with watch() is only run once.
     */
    public final Observable<T> first() {
        return new FirstActivationStateObserver<>(this).asObservable();
    }

    /**
     * Returns an Observable that is activated when `this` is activated any time besides the first,
     * and provides as activation data a `Both` object containing the previous and new activation
     * data of `this`.
     *
     * This is useful if registered callbacks need to know the data of the previous activation.
     */
    public final Observable<Both<T, T>> changes() {
        return new ChangeStateObserver<>(this).asObservable();
    }

    /**
     * Returns an Observable that does not activate if `this` is set with a value such that the
     * given predicate returns true for the previous value and the current value.
     *
     * Can be used to ignore repeat activations that contain the same data. Beware that even though
     * a repeat activation that passes the given predicate will not re-activate the new Observable,
     * it will deactivate it.
     */
    public final Observable<T> unique(BiPredicate<? super T, ? super T> predicate) {
        Controller<T> controller = new Controller<>();
        ScopeFactory<T> pipeToController = (T value) -> {
            controller.set(value);
            return controller::reset;
        };
        first().watch(pipeToController);
        changes()
                .filter(Both.adapt((T a, T b) -> !predicate.test(a, b)))
                .map(Both::getSecond)
                .watch(pipeToController);
        return controller;
    }

    /**
     * Returns an Observable that does not activate if `this` is activated with a value that is
     * equal to the data of a previous activation, according to that data's `equals()` method.
     *
     * Can be used to ignore repeat activations that contain the same data. Beware that even though
     * a repeat activation that passes the given predicate will not re-activate the new Observable,
     * it will deactivate it.
     */
    public final Observable<T> unique() {
        return unique(Object::equals);
    }

    /**
     * Returns an Observable that is activated only when the given Observable is not activated.
     */
    public static Observable<Unit> not(Observable<?> observable) {
        Controller<Unit> opposite = new Controller<>();
        opposite.set(Unit.unit());
        observable.watch(() -> {
            opposite.reset();
            return () -> opposite.set(Unit.unit());
        });
        return opposite;
    }

    // Owns a Controller that is activated only when the Observables are activated in order.
    private static class SequenceStateObserver<A, B> {
        private final Controller<Both<A, B>> mController = new Controller<>();
        private A mA = null;

        private SequenceStateObserver(Observable<A> stateA, Observable<B> stateB) {
            stateA.watch((A a) -> {
                mA = a;
                return () -> {
                    mA = null;
                    mController.reset();
                };
            });
            stateB.watch((B b) -> {
                if (mA != null) {
                    mController.set(Both.both(mA, b));
                }
                return () -> {
                    mController.reset();
                };
            });
        }

        private Observable<Both<A, B>> asObservable() {
            return mController;
        }
    }

    // Owns a Controller that is activated only on the Observable's first activation.
    private static class FirstActivationStateObserver<T> {
        private final Controller<T> mController = new Controller<>();
        private boolean mIsActivated = false;

        private FirstActivationStateObserver(Observable<T> state) {
            state.watch((T value) -> {
                if (!mIsActivated) {
                    mController.set(value);
                    mIsActivated = true;
                }
                return mController::reset;
            });
        }

        private Observable<T> asObservable() {
            return mController;
        }
    }

    // Owns a Controller that is activated on non-first activations with the previous and new
    // activation data.
    private static class ChangeStateObserver<T> {
        private final Controller<Both<T, T>> mController = new Controller<>();
        private T mCurrent = null;

        private ChangeStateObserver(Observable<T> state) {
            state.watch((T value) -> {
                if (mCurrent != null) {
                    mController.set(Both.both(mCurrent, value));
                }
                mCurrent = value;
                return mController::reset;
            });
        }

        private Observable<Both<T, T>> asObservable() {
            return mController;
        }
    }
}
