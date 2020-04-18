// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.emptyIterable;
import static org.junit.Assert.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import org.chromium.chromecast.base.Inheritance.Base;
import org.chromium.chromecast.base.Inheritance.Derived;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for Observable and Controller.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class ObservableAndControllerTest {
    // Convenience method to create a scope that mutates a list of strings on state transitions.
    // When entering the state, it will append "enter ${id} ${data}" to the result list, where
    // `data` is the String that is associated with the state activation. When exiting the state,
    // it will append "exit ${id}" to the result list. This provides a readable way to track and
    // verify the behavior of observers in response to the Observables they are linked to.
    public static <T> ScopeFactory<T> report(List<String> result, String id) {
        // Did you know that lambdas are awesome.
        return (T data) -> {
            result.add("enter " + id + ": " + data);
            return () -> result.add("exit " + id);
        };
    }

    @Test
    public void testNoStateTransitionAfterRegisteringWithInactiveController() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        assertThat(result, emptyIterable());
    }

    @Test
    public void testStateIsEnteredWhenControllerIsSet() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        // Activate the state by setting the controller.
        controller.set("cool");
        assertThat(result, contains("enter a: cool"));
    }

    @Test
    public void testBasicStateFromController() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        controller.set("fun");
        // Deactivate the state by resetting the controller.
        controller.reset();
        assertThat(result, contains("enter a: fun", "exit a"));
    }

    @Test
    public void testSetStateTwicePerformsImplicitReset() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        // Activate the state for the first time.
        controller.set("first");
        // Activate the state for the second time.
        controller.set("second");
        // If set() is called without a reset() in-between, the tracking state exits, then re-enters
        // with the new data. So we expect to find an "exit" call between the two enter calls.
        assertThat(result, contains("enter a: first", "exit a", "enter a: second"));
    }

    @Test
    public void testResetWhileStateIsNotEnteredIsNoOp() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        controller.reset();
        assertThat(result, emptyIterable());
    }

    @Test
    public void testMultipleStatesObservingSingleController() {
        // Construct two states that watch the same Controller. Verify both observers' events are
        // triggered.
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        controller.watch(report(result, "b"));
        // Activate the controller, which should propagate a state transition to both states.
        // Both states should be updated, so we should get two enter events.
        controller.set("neat");
        controller.reset();
        assertThat(result, contains("enter a: neat", "enter b: neat", "exit b", "exit a"));
    }

    @Test
    public void testNewStateIsActivatedImmediatelyIfObservingAlreadyActiveObservable() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.set("surprise");
        controller.watch(report(result, "a"));
        assertThat(result, contains("enter a: surprise"));
    }

    @Test
    public void testNewStateIsNotActivatedIfObservingObservableThatHasBeenDeactivated() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.set("surprise");
        controller.reset();
        controller.watch(report(result, "a"));
        assertThat(result, emptyIterable());
    }

    @Test
    public void testResetWhileAlreadyDeactivatedIsANoOp() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        controller.set("radical");
        controller.reset();
        // Resetting again after already resetting should not notify the observer.
        controller.reset();
        assertThat(result, contains("enter a: radical", "exit a"));
    }

    @Test
    public void testClosedWatchScopeDoesNotGetNotifiedOfFutureActivations() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        Scope watching = a.watch(report(result, "temp"));
        a.set("during temp");
        a.reset();
        watching.close();
        a.set("after temp");
        assertThat(result, contains("enter temp: during temp", "exit temp"));
    }

    @Test
    public void testClosedWatchScopeIsImplicitlyDeactivated() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        Scope watching = a.watch(report(result, "temp"));
        a.set("implicitly reset this");
        watching.close();
        assertThat(result, contains("enter temp: implicitly reset this", "exit temp"));
    }

    @Test
    public void testCloseWatchScopeAfterDeactivatingSourceStateDoesNotCallExitHAndlerAgain() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        Scope watching = a.watch(report(result, "temp"));
        a.set("and a one");
        a.reset();
        watching.close();
        assertThat(result, contains("enter temp: and a one", "exit temp"));
    }

    @Test
    public void testBothState_activateFirstDoesNotTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testBothState_activateSecondDoesNotTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        b.set("B");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testBothState_activateBothTriggers() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        b.set("B");
        assertThat(result, contains("enter both: A, B"));
    }

    @Test
    public void testBothState_deactivateFirstAfterTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        b.set("B");
        a.reset();
        assertThat(result, contains("enter both: A, B", "exit both"));
    }

    @Test
    public void testBothState_deactivateSecondAfterTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        b.set("B");
        b.reset();
        assertThat(result, contains("enter both: A, B", "exit both"));
    }

    @Test
    public void testBothState_resetFirstBeforeSettingSecond_doesNotTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        a.reset();
        b.set("B");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testBothState_resetSecondBeforeSettingFirst_doesNotTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        b.set("B");
        b.reset();
        a.set("A");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testBothState_setOneControllerAfterTrigger_implicitlyResetsAndSets() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A1");
        b.set("B1");
        a.set("A2");
        b.set("B2");
        assertThat(result,
                contains("enter both: A1, B1", "exit both", "enter both: A2, B1", "exit both",
                        "enter both: A2, B2"));
    }

    @Test
    public void testComposeBoth() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        Controller<String> c = new Controller<>();
        Controller<String> d = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).and(c).and(d).watch(report(result, "all four"));
        a.set("A");
        b.set("B");
        c.set("C");
        d.set("D");
        a.reset();
        assertThat(result, contains("enter all four: A, B, C, D", "exit all four"));
    }

    @Test
    public void testFirst() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.first().watch(report(result, "first"));
        a.set("first");
        a.set("second");
        assertThat(result, contains("enter first: first", "exit first"));
    }

    @Test
    public void testChanges() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.changes().watch(report(result, "changes"));
        a.set("first");
        a.set("second");
        a.set("third");
        assertThat(result,
                contains("enter changes: first, second", "exit changes",
                        "enter changes: second, third"));
    }

    @Test
    public void testChangesIsResetWhenSourceIsReset() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.changes().watch(report(result, "changes"));
        a.set("first");
        a.set("second");
        a.reset();
        assertThat(result, contains("enter changes: first, second", "exit changes"));
    }

    @Test
    public void testUniqueDefault() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.unique().watch(report(result, "unique"));
        a.set("hi");
        a.set("ho");
        a.set("hey");
        a.set("hey");
        a.set("hey");
        a.set("hi");
        assertThat(result,
                contains("enter unique: hi", "exit unique", "enter unique: ho", "exit unique",
                        "enter unique: hey", "exit unique", "enter unique: hi"));
    }

    @Test
    public void testUniqueWithCustomPredicate() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.unique((p, s) -> p.equalsIgnoreCase(s)).watch(report(result, "unique ignore case"));
        a.set("kale");
        a.set("KALE");
        a.set("steamed kale");
        a.set("STEAMED");
        a.set("sTeAmEd");
        assertThat(result,
                contains("enter unique ignore case: kale", "exit unique ignore case",
                        "enter unique ignore case: steamed kale", "exit unique ignore case",
                        "enter unique ignore case: STEAMED", "exit unique ignore case"));
    }

    @Test
    public void testMap() {
        Controller<String> original = new Controller<>();
        Observable<String> lowerCase = original.map(String::toLowerCase);
        Observable<String> upperCase = lowerCase.map(String::toUpperCase);
        List<String> result = new ArrayList<>();
        original.watch(report(result, "unchanged"));
        lowerCase.watch(report(result, "lower"));
        upperCase.watch(report(result, "upper"));
        original.set("sImPlY sTeAmEd KaLe");
        original.reset();
        // Note: order of activation doesn't really matter, but deactivation should be in reverse
        // order or activation.
        assertThat(result,
                contains("enter upper: SIMPLY STEAMED KALE", "enter lower: simply steamed kale",
                        "enter unchanged: sImPlY sTeAmEd KaLe", "exit unchanged", "exit lower",
                        "exit upper"));
    }

    @Test
    public void testFilter() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.filter(String::isEmpty).watch(report(result, "empty"));
        a.filter(s -> s.startsWith("a")).watch(report(result, "starts with a"));
        a.filter(s -> s.endsWith("a")).watch(report(result, "ends with a"));
        a.set("");
        a.set("none");
        a.set("add");
        a.set("doa");
        a.set("ada");
        assertThat(result,
                contains("enter empty: ", "exit empty", "enter starts with a: add",
                        "exit starts with a", "enter ends with a: doa", "exit ends with a",
                        "enter starts with a: ada", "enter ends with a: ada"));
    }

    @Test
    public void testSetControllerWithNullImplicitlyResets() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "controller"));
        a.set("not null");
        a.set(null);
        assertThat(result, contains("enter controller: not null", "exit controller"));
    }

    @Test
    public void testResetControllerInActivationHandler() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch((String s) -> {
            result.add("enter " + s);
            a.reset();
            result.add("after reset");
            return () -> {
                result.add("exit");
            };
        });
        a.set("immediately retracted");
        assertThat(result, contains("enter immediately retracted", "after reset", "exit"));
    }

    @Test
    public void testSetControllerInActivationHandler() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "weirdness"));
        a.watch((String s) -> {
            // If the activation handler always calls set() on the source controller, you will have
            // an infinite loop, which is not cool. However, if the activation handler only
            // conditionally calls set() on its source controller, then the case where set() is not
            // called will break the loop. It is the responsibility of the programmer to solve the
            // halting problem for activation handlers.
            if (s.equals("first")) {
                a.set("second");
            }
            return () -> {
                result.add("haha");
            };
        });
        a.set("first");
        assertThat(result,
                contains("enter weirdness: first", "haha", "exit weirdness",
                        "enter weirdness: second"));
    }

    @Test
    public void testResetControllerInDeactivationHandler() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "bizzareness"));
        a.watch((String s) -> () -> a.reset());
        a.set("yo");
        a.reset();
        // The reset() called by the deactivation handler should be a no-op.
        assertThat(result, contains("enter bizzareness: yo", "exit bizzareness"));
    }

    @Test
    public void testSetControllerInDeactivationHandler() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "astoundingness"));
        a.watch((String s) -> () -> a.set("never mind"));
        a.set("retract this");
        a.reset();
        // The set() called by the deactivation handler should immediately set the controller back.
        assertThat(result,
                contains("enter astoundingness: retract this", "exit astoundingness",
                        "enter astoundingness: never mind"));
    }

    @Test
    public void testBeingTooCleverWithScopeFactoriesAndInheritance() {
        Controller<Base> baseController = new Controller<>();
        Controller<Derived> derivedController = new Controller<>();
        List<String> result = new ArrayList<>();
        // Test that the same ScopeFactory object can observe Observables of different types, as
        // long as the ScopeFactory type is a superclass of both Observable types.
        ScopeFactory<Base> scopeFactory = (Base value) -> {
            result.add("enter: " + value.toString());
            return () -> result.add("exit: " + value.toString());
        };
        baseController.watch(scopeFactory);
        // Compile error if generics are wrong.
        derivedController.watch(scopeFactory);
        baseController.set(new Base());
        // The scope from the previous set() call will not be overridden because this is activating
        // a different Controller.
        derivedController.set(new Derived());
        // The Controller<Base> can be activated with an object that extends Base.
        baseController.set(new Derived());
        assertThat(
                result, contains("enter: Base", "enter: Derived", "exit: Base", "enter: Derived"));
    }

    @Test
    public void testNotIsActivatedAtTheStart() {
        Controller<String> invertThis = new Controller<>();
        List<String> result = new ArrayList<>();
        Observable.not(invertThis).watch(() -> {
            result.add("enter inverted");
            return () -> result.add("exit inverted");
        });
        assertThat(result, contains("enter inverted"));
    }

    @Test
    public void testNotIsDeactivatedAtTheStartIfSourceIsAlreadyActivated() {
        Controller<String> invertThis = new Controller<>();
        List<String> result = new ArrayList<>();
        invertThis.set("way ahead of you");
        Observable.not(invertThis).watch(() -> {
            result.add("enter inverted");
            return () -> result.add("exit inverted");
        });
        assertThat(result, emptyIterable());
    }

    @Test
    public void testNotExitsWhenSourceIsActivated() {
        Controller<String> invertThis = new Controller<>();
        List<String> result = new ArrayList<>();
        Observable.not(invertThis).watch(() -> {
            result.add("enter inverted");
            return () -> result.add("exit inverted");
        });
        invertThis.set("first");
        assertThat(result, contains("enter inverted", "exit inverted"));
    }

    @Test
    public void testNotReentersWhenSourceIsReset() {
        Controller<String> invertThis = new Controller<>();
        List<String> result = new ArrayList<>();
        Observable.not(invertThis).watch(() -> {
            result.add("enter inverted");
            return () -> result.add("exit inverted");
        });
        invertThis.set("first");
        invertThis.reset();
        assertThat(result, contains("enter inverted", "exit inverted", "enter inverted"));
    }

    @Test
    public void testAndThenNotActivatedInitially() {
        Controller<String> aState = new Controller<>();
        Controller<String> bState = new Controller<>();
        List<String> result = new ArrayList<>();
        aState.andThen(bState).watch(ScopeFactories.onEnter(
                (String a, String b) -> { result.add("a=" + a + ", b=" + b); }));
        assertThat(result, emptyIterable());
    }

    @Test
    public void testAndThenNotActivatedIfSecondBeforeFirst() {
        Controller<String> aState = new Controller<>();
        Controller<String> bState = new Controller<>();
        List<String> result = new ArrayList<>();
        aState.andThen(bState).watch(ScopeFactories.onEnter(
                (String a, String b) -> { result.add("a=" + a + ", b=" + b); }));
        bState.set("b");
        aState.set("a");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testAndThenActivatedIfFirstThenSecond() {
        Controller<String> aState = new Controller<>();
        Controller<String> bState = new Controller<>();
        List<String> result = new ArrayList<>();
        aState.andThen(bState).watch(ScopeFactories.onEnter(
                (String a, String b) -> { result.add("a=" + a + ", b=" + b); }));
        aState.set("a");
        bState.set("b");
        assertThat(result, contains("a=a, b=b"));
    }

    @Test
    public void testAndThenActivated_plusBplusAminusBplusB() {
        Controller<String> aState = new Controller<>();
        Controller<String> bState = new Controller<>();
        List<String> result = new ArrayList<>();
        aState.andThen(bState).watch(ScopeFactories.onEnter(
                (String a, String b) -> { result.add("a=" + a + ", b=" + b); }));
        bState.set("b");
        aState.set("a");
        bState.reset();
        bState.set("B");
        assertThat(result, contains("a=a, b=B"));
    }

    @Test
    public void testAndThenDeactivated_plusAplusBminusA() {
        Controller<String> aState = new Controller<>();
        Controller<String> bState = new Controller<>();
        List<String> result = new ArrayList<>();
        aState.andThen(bState).watch(ScopeFactories.onExit(
                (String a, String b) -> { result.add("a=" + a + ", b=" + b); }));
        aState.set("A");
        bState.set("B");
        aState.reset();
        assertThat(result, contains("a=A, b=B"));
    }

    @Test
    public void testAndThenDeactivated_plusAplusBminusB() {
        Controller<String> aState = new Controller<>();
        Controller<String> bState = new Controller<>();
        List<String> result = new ArrayList<>();
        aState.andThen(bState).watch(ScopeFactories.onExit(
                (String a, String b) -> { result.add("a=" + a + ", b=" + b); }));
        aState.set("A");
        bState.set("B");
        bState.reset();
        assertThat(result, contains("a=A, b=B"));
    }

    @Test
    public void testComposeAndThen() {
        Controller<Unit> aState = new Controller<>();
        Controller<Unit> bState = new Controller<>();
        Controller<Unit> cState = new Controller<>();
        Controller<Unit> dState = new Controller<>();
        Observable<Both<Unit, Unit>> aThenB = aState.andThen(bState);
        Observable<Both<Both<Unit, Unit>, Unit>> aThenBThenC = aThenB.andThen(cState);
        Observable<Both<Both<Both<Unit, Unit>, Unit>, Unit>> aThenBThenCThenD =
                aThenBThenC.andThen(dState);
        List<String> result = new ArrayList<>();
        aState.watch(ScopeFactories.onEnter(() -> result.add("A")));
        aThenB.watch(ScopeFactories.onEnter(() -> result.add("B")));
        aThenBThenC.watch(ScopeFactories.onEnter(() -> result.add("C")));
        aThenBThenCThenD.watch(ScopeFactories.onEnter(() -> result.add("D")));
        aState.set(Unit.unit());
        bState.set(Unit.unit());
        cState.set(Unit.unit());
        dState.set(Unit.unit());
        aState.reset();
        assertThat(result, contains("A", "B", "C", "D"));
    }

    @Test
    public void testUseWatchScopeAsScopeFactory() {
        Controller<String> aState = new Controller<>();
        Controller<String> bState = new Controller<>();
        List<String> result = new ArrayList<>();
        aState.watch(report(result, "a"));
        bState.watch(report(result, "b"));
        // I guess this makes .and() obsolete?
        aState.watch(a -> bState.watch(b -> {
            result.add("enter both: " + a + ", " + b);
            return () -> result.add("exit both");
        }));
        aState.set("A");
        bState.set("B");
        assertThat(result, contains("enter a: A", "enter b: B", "enter both: A, B"));
        result.clear();
        aState.reset();
        assertThat(result, contains("exit both", "exit a"));
        result.clear();
        aState.set("AA");
        assertThat(result, contains("enter a: AA", "enter both: AA, B"));
        result.clear();
        bState.reset();
        assertThat(result, contains("exit both", "exit b"));
    }

    @Test
    public void testPowerUnlimitedPower() {
        Controller<Unit> aState = new Controller<>();
        Controller<Unit> bState = new Controller<>();
        Controller<Unit> cState = new Controller<>();
        Controller<Unit> dState = new Controller<>();
        List<String> result = new ArrayList<>();
        // Praise be to Haskell Curry.
        aState.watch(a -> bState.watch(b -> cState.watch(c -> dState.watch(d -> {
            result.add("it worked!");
            return () -> result.add("exit");
        }))));
        aState.set(Unit.unit());
        bState.set(Unit.unit());
        cState.set(Unit.unit());
        dState.set(Unit.unit());
        assertThat(result, contains("it worked!"));
        result.clear();
        aState.reset();
        assertThat(result, contains("exit"));
        result.clear();
        aState.set(Unit.unit());
        assertThat(result, contains("it worked!"));
        result.clear();
        bState.reset();
        assertThat(result, contains("exit"));
        result.clear();
        bState.set(Unit.unit());
        assertThat(result, contains("it worked!"));
        result.clear();
        cState.reset();
        assertThat(result, contains("exit"));
        result.clear();
        cState.set(Unit.unit());
        assertThat(result, contains("it worked!"));
        result.clear();
        dState.reset();
        assertThat(result, contains("exit"));
        result.clear();
        dState.set(Unit.unit());
        assertThat(result, contains("it worked!"));
    }

    // Any Scope's constructor whose parameters match the scope can be used as a method reference.
    private static class TransitionLogger implements Scope {
        public static final List<String> sResult = new ArrayList<>();
        private final String mData;

        public TransitionLogger(String data) {
            mData = data;
            sResult.add("enter: " + mData);
        }

        @Override
        public void close() {
            sResult.add("exit: " + mData);
        }
    }

    @Test
    public void testScopeFactoryWithAutoCloseableConstructor() {
        Controller<String> controller = new Controller<>();
        // You can use a constructor method reference in a watch() call.
        controller.watch(TransitionLogger::new);
        controller.set("a");
        controller.reset();
        assertThat(TransitionLogger.sResult, contains("enter: a", "exit: a"));
    }
}
