# VR Instrumentation Rules

## Introduction

This directory contains all the Java files related to VR-specific JUnit4 rules.
For the most part, the rules are identical to the standard activity-specific
rules (e.g. `ChromeTabbedActivityTestRule` for `ChromeTabbedActivity`), but with
additional code to work with JUnit4 test parameterization to support running
a test case multiple times in different activities.

Usage of this feature is already covered in `../README.md`, so this
documentation concerns implementation details.

## How It Works

When a test is set up to use test parameterization (determined by whether the
test class is annotated with `@RunWith(ParameterizedRunner.class)` or not), the
test runner automatically runs each test case in the class once with every
`ParameterSet` in the list of `ParameterSet`s that is annotated with
`@ClassParameter`, passing each `ParameterSet`'s contents to the class constructor
each time.

In the case of VR tests, the `ParameterSet` list contains `Callable`s that
construct VR test rules for the three supported activity types
(`ChromeTabbedActivity`, `CustomTabActivity`, and `WebappActivity`). The
constructor for parameterized VR tests runs the provided `Callable`, effectively
running every test case once in each activity type.

However, if the class were to assign the `Callable` return value to its member
variable annotated with `@Rule`, then every test case in the class would always
run in all three activity types, which isn't desirable since some test cases
test scenarios that are only valid in some activities (e.g. tests that involve
the VR browser are currently only supported in ChromeTabbedActivity). To avoid
this, the `Rule` annotated with `@Rule` is actually a `RuleChain` that wraps the
generated VR test rule in a `VrActivityRestrictionRule`.

`VrActivityRestrictionRule` interacts with the `@VrActivityRestriction`
annotation and the VR test rule for the current test case run. If the activity
type of the current rule is contained in the list provided by
`@VrActivityRestriction` (or there is no restriction annotation and the activity
type is `ChromeTabbedActivity`), then the `VrActivityRestrictionRule` becomes a
no-op and the test case runs normally. Otherwise, `VrActivityRestrictionRule`
causes an assumption failure, which is interpreted by the test runner as a
signal to skip that particular test case/parameter combination.

Thus, the end result is that the same test case can be run multiple times in
different activities without having to duplicate any code for use in different
activity types, while still retaining the ability to not run every test in every
activity type if necessary.