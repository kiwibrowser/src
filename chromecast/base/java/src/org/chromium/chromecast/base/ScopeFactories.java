// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

/**
 * Helper functions for creating ScopeFactories, used by Observable.watch() to handle state changes.
 */
public final class ScopeFactories {
    // Uninstantiable.
    private ScopeFactories() {}

    /**
     * Shorthand for making a ScopeFactory that only has side effects on activation.
     *
     * @param <T> The type of the activation data.
     */
    public static <T> ScopeFactory<T> onEnter(Consumer<T> consumer) {
        return (T value) -> {
            consumer.accept(value);
            return () -> {};
        };
    }

    /**
     * Shorthand for making a ScopeFactory that only has side effects on activation, and is
     * independent of the activation value.
     *
     * @param <T> The type of the activation data.
     */
    public static <T> ScopeFactory<T> onEnter(Runnable runnable) {
        return onEnter((T value) -> runnable.run());
    }

    /**
     * Shorthand for making a ScopeFactory that only has side effects on activation, and has a Both
     * object for activation data.
     *
     * For example, one can refactor the following:
     *
     *     observableA.and(observableB).watch((Both<A, B> data) -> {
     *         A a = data.first;
     *         B b = data.second;
     *         ... // side effects on activation
     *         return () -> {}; // no side effects on deactivation.
     *     });
     *
     * ... into this:
     *
     *    observableA.and(observableB).watch(ScopeFactories.onEnter((A a, B b) -> ...));
     *
     * @param <A> The first argument of the consumer (and the first item in the Both).
     * @param <B> The second argument of the consumer (and the second item in the Both).
     */
    public static <A, B> ScopeFactory<Both<A, B>> onEnter(BiConsumer<A, B> consumer) {
        return onEnter(Both.adapt(consumer));
    }

    /**
     * Shorthand for making a ScopeFactory that only has side effects on deactivation.
     *
     * @param <T> The type of the activation data.
     */
    public static <T> ScopeFactory<T> onExit(Consumer<T> consumer) {
        return (T value) -> () -> consumer.accept(value);
    }

    /**
     * Shorthand for making a ScopeFactory that only has side effects on deactivation, and is
     * independent of the activation data.
     *
     * @param <T> The type of the activation data.
     */
    public static <T> ScopeFactory<T> onExit(Runnable runnable) {
        return onExit((T value) -> runnable.run());
    }

    /**
     * Shorthand for making a ScopeFactory that only has side effects on deactivation, and has a
     * Both object for activation data.
     *
     * For example, one can refactor the following:
     *
     *     observableA.and(observableB).watch((Both<A, B> data) -> {
     *         A a = data.first;
     *         B b = data.second;
     *         return () -> {
     *             // side effects on deactivation.
     *         };
     *     });
     *
     * ... into this:
     *
     *    observableA.and(observableB).watch(ScopeFactories.onExit((A a, B b) -> ...));
     *
     * @param <A> The first argument of the consumer (and the first item in the Both).
     * @param <B> The second argument of the consumer (and the second item in the Both).
     */
    public static <A, B> ScopeFactory<Both<A, B>> onExit(BiConsumer<A, B> consumer) {
        return onExit(Both.adapt(consumer));
    }

    /**
     * Adapts a ScopeFactory-like function that takes two arguments into a true ScopeFactory that
     * takes a Both object.
     *
     * @param <A> The type of the first argument (and the first item in the Both).
     * @param <B> The type of the second argument (and the second item in the Both).
     *
     * For example, one can refactor the following:
     *
     *     observableA.and(observableB).watch((Both<A, B> data) -> {
     *         A a = data.first;
     *         B b = data.second;
     *         ...
     *     });
     *
     * ... into this:
     *
     *     observableA.and(observableB).watch(ScopeFactories.both((A a, B b) -> ...));
     */
    public static <A, B> ScopeFactory<Both<A, B>> both(
            BiFunction<? super A, ? super B, Scope> function) {
        return (Both<A, B> data) -> function.apply(data.first, data.second);
    }
}
